/* Copyright (c) 2015, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "mm_qcamera_app.h"
#include "qcam_log.h"
#include "qcamlib.h"
#include <stdbool.h>
#include <media/msm_media_info.h>
#include <sys/time.h>

typedef struct _qcamlib_obj {
    qcamlib_frame_cb pcb;
    qcamlib_frame_cb vcb;
    mm_camera_test_obj_t test_obj;
    mm_camera_app_t app;
    bool preview_on;
    bool video_on;
    void *cb_userdata;
    bool video_preview;
    cam_focus_mode_type af_mode;
    qcamlib_video_buf_info_cb v_buf_info_cb;
};

qcamlib_t g_handle;

static void qcamlib_buf_info_cb(mm_camera_stream_t *stream);

qcamlib_t qcamlib_create()
{
    int rc;
    if (g_handle != NULL) {
        QCAM_ERR("only single qcamlib instance is supported");
        return NULL;
    }
    qcamlib_t h = (qcamlib_t)malloc(sizeof(struct _qcamlib_obj));
    if (h == NULL) {
        QCAM_ERR("malloc() failed");
        return h;
    }
    memset(h, 0x00, sizeof(struct _qcamlib_obj));

    rc = mm_app_load_hal(&h->app);
    if (rc != MM_CAMERA_OK) {
        QCAM_ERR("mm_app_load_hal() failed");
        goto cleanup;
    }
    rc = mm_app_open(&h->app, 0, &h->test_obj);
    if (rc != MM_CAMERA_OK) {
        QCAM_ERR("mm_app_open() rc=%d", rc);
        goto cleanup;
    }
    h->video_preview = false;
    g_handle = h;

    h->test_obj.video_width = DEFAULT_VIDEO_WIDTH;
    h->test_obj.video_height = DEFAULT_VIDEO_HEIGHT;
    h->test_obj.preview_width = DEFAULT_PREVIEW_WIDTH;
    h->test_obj.preview_height = DEFAULT_PREVIEW_HEIGHT;
    /* set focus mode to fixed by default */
    h->af_mode = CAM_FOCUS_MODE_FIXED;
#if 0
    h->test_obj.snapshot_width = DEFAULT_SNAPSHOT_WIDTH;
    h->test_obj.snapshot_height = DEFAULT_SNAPSHOT_HEIGHT;
#endif

    h->test_obj.buf_info_cb = qcamlib_buf_info_cb;

    return h;
cleanup:
    free(h);
    return NULL;
}

void qcamlib_destroy(qcamlib_t h)
{
  int rc;
  if (h->preview_on) {
    QCAM_ERR("preview needs to be stopped before exit");
    // TODO: stop preview
  }
  rc = mm_app_close(&h->test_obj);
  if (rc != MM_CAMERA_OK) {
    QCAM_ERR("mm_app_close() failed");
  }
  free(h);
  g_handle = NULL;
}

qcamlib_frame_info_t qcamlib_get_frame_info(qcamlib_t h,
                                            qcamlib_stream_type_t stream_type)
{
  qcamlib_frame_info_t frame_info;

  switch (stream_type) {
    case QCAMLIB_STREAM_PREVIEW:
      frame_info.fmt = QCAMLIB_FMT_NV21;
      frame_info.width = h->test_obj.preview_width;
      frame_info.height = h->test_obj.preview_height;
      /* Y plane */
      frame_info.planes[0].width = h->test_obj.preview_width;
      frame_info.planes[0].height = h->test_obj.preview_height;
      frame_info.planes[0].stride = h->test_obj.preview_width;
      frame_info.planes[0].scanlines = h->test_obj.preview_height;
      /* CbCr Plane */
      frame_info.planes[1].width = h->test_obj.preview_width;
      frame_info.planes[1].height = h->test_obj.preview_height/2;
      frame_info.planes[1].stride = h->test_obj.preview_width;
      frame_info.planes[1].scanlines = h->test_obj.preview_height/2;

      frame_info.buf_size =
          frame_info.planes[0].stride * frame_info.planes[0].scanlines +
          frame_info.planes[1].stride * frame_info.planes[1].scanlines;

      break;
    case QCAMLIB_STREAM_VIDEO:
      /* Video buffers are VENUS encoder compatible */
      frame_info.fmt = QCAMLIB_FMT_NV12;
      frame_info.width = h->test_obj.video_width;
      frame_info.height = h->test_obj.video_height;
      /* Y plane */
      frame_info.planes[0].width = h->test_obj.video_width;
      frame_info.planes[0].height = h->test_obj.video_height;
      frame_info.planes[0].stride =
          VENUS_Y_STRIDE(COLOR_FMT_NV12, h->test_obj.video_width);
      frame_info.planes[0].scanlines =
          VENUS_Y_SCANLINES(COLOR_FMT_NV12, h->test_obj.video_height);
      /* CbCr Plane */
      frame_info.planes[1].width = h->test_obj.video_width;
      frame_info.planes[1].height = h->test_obj.video_height/2;
      frame_info.planes[1].stride =
          VENUS_UV_STRIDE(COLOR_FMT_NV12, h->test_obj.video_width);
      frame_info.planes[1].scanlines =
          VENUS_UV_SCANLINES(COLOR_FMT_NV12, h->test_obj.video_height);
      frame_info.buf_size = VENUS_BUFFER_SIZE(COLOR_FMT_NV12,
                                              h->test_obj.video_width,
                                              h->test_obj.video_height);
  }
  frame_info.planes[0].offset = 0;
  frame_info.planes[0].len =
    frame_info.planes[0].stride * frame_info.planes[0].scanlines;
  frame_info.planes[1].offset = frame_info.planes[0].len;
  frame_info.planes[1].len =
    frame_info.planes[1].stride * frame_info.planes[1].scanlines;

  return frame_info;
}

int qcamlib_start_preview(qcamlib_t h)
{
  int rc;
  QCAM_INFO("starting preview");

  rc = mm_app_start_record_preview(&h->test_obj);
  if (rc != MM_CAMERA_OK) {
      QCAM_ERR("mm_app_start_record_preview() failed");
      return -1;
  }
  h->preview_on = true;
  return 0;
}

int qcamlib_start_video(qcamlib_t h)
{
    int rc;
    QCAM_INFO("starting video");

    /* set fps mode to fixed for video recording */
    cam_fps_range_t fr;
    fr.min_fps = 29.8;
    fr.max_fps = 30.0;
    fr.video_min_fps = 29.8;
    fr.video_max_fps = 30.0;

    rc = setFPSRange(&h->test_obj, fr);
    if (rc != MM_CAMERA_OK) {
        QCAM_ERR("setFPSRange() rc=%d", rc);
        return -1;
    }

    if (h->preview_on == false) {
        rc = qcamlib_start_preview(h);
        if (rc != 0) {
            QCAM_ERR("qcamlib_start_preview() failed");
            return -1;
        }
        /* this indicates the preview is started by qcamlib
           internally for video recording, not by user */
        h->video_preview = true;
    }
    rc = mm_app_start_record(&h->test_obj);
    if (rc != MM_CAMERA_OK) {
        QCAM_ERR("mm_app_start_record() failed");
        return -1;
    }
    h->video_on = true;
    return 0;
}

int qcamlib_stop_video(qcamlib_t h)
{
  int rc;
  QCAM_INFO("stopping video");
  if (h->video_on == false) {
      QCAM_ERR("video not started");
      return 0;
  }
  rc = mm_app_stop_record(&h->test_obj);
  if (rc != MM_CAMERA_OK) {
      QCAM_ERR("mm_app_stop_record() failed");
      return -1;
  }
  h->video_on = false;

  if (h->video_preview == true) {
      rc = qcamlib_stop_preview(h);
      if (rc != 0) {
          QCAM_ERR("qcamlib_stop_preview() failed");
          return -1;
      }
      h->video_preview = false;
  }
  return 0;
}

int qcamlib_stop_preview(qcamlib_t h)
{
  int rc;
  QCAM_INFO("stopping preview");

  if (h->video_on) {
    QCAM_ERR("cannot stop preview before video");
    return -1;
  }
  rc = mm_app_stop_record_preview(&h->test_obj);
  if (rc != MM_CAMERA_OK) {
      QCAM_ERR("mm_app_stop_record_preview() failed");
      return -1;
  }
  h->preview_on = false;
  return 0;
}

static void qcamlib_buf_info_cb(mm_camera_stream_t *stream)
{
    QCAM_INFO("E");
    qcamlib_t h = g_handle;
    qcamlib_video_buf_cb_data_t cb_data;
    int i;
    switch (stream->channel_type) {
    case MM_CHANNEL_TYPE_VIDEO: {
        cb_data.buf_info = (qcamlib_video_buf_info_t *)
                malloc (stream->num_of_bufs * sizeof(qcamlib_video_buf_info_t));

        for (i=0; i < stream->num_of_bufs; i++) {
            cb_data.buf_info[i].fd = stream->s_bufs[i].mem_info.fd;
            cb_data.buf_info[i].vaddr = stream->s_bufs[i].mem_info.data;
            cb_data.buf_info[i].size = stream->s_bufs[i].mem_info.size;
        }
        cb_data.userdata = h->cb_userdata;
        cb_data.num_bufs = stream->num_of_bufs;
        if (h->v_buf_info_cb) {
            h->v_buf_info_cb(cb_data);
        }
        free(cb_data.buf_info);
        break;
    }
    default:
        break;
    }
}

static void preview_cb_internal(mm_camera_buf_def_t *buf_def)
{
    qcamlib_cb_data_t frame;
    qcamlib_t h = g_handle;
    if (h->pcb == NULL) {
        return;
    }
    frame.buffer = buf_def->buffer;
    frame.ts = buf_def->ts;
    frame.userdata = h->cb_userdata;
    h->pcb(frame);
}

static void video_cb_internal(mm_camera_super_buf_t *super_buf)
{
    qcamlib_cb_data_t frame;
    qcamlib_t h = g_handle;
    if (h->vcb == NULL) {
        return;
    }
    mm_camera_buf_def_t *buf_def = super_buf->bufs[0];
    frame.buffer = buf_def->buffer;
    frame.ts = buf_def->ts;
    frame.userdata = h->cb_userdata;
    frame.sbuf = super_buf;

    struct timeval tv0, tv1;

    gettimeofday(&tv0, NULL);
    h->vcb(frame);
    gettimeofday(&tv1, NULL);

    uint64_t us;
    us = (tv1.tv_sec - tv0.tv_sec) * 1000000 + (tv1.tv_usec - tv0.tv_usec);
    QCAM_DBG("user vcb processing time = %lld us", us);
}

int qcamlib_register_preview_cb(qcamlib_t h, qcamlib_frame_cb upcb)
{
  int rc;
  if (h->preview_on) {
    QCAM_ERR("preview callback needs to be set before starting the preview");
    return -1;
  }
  h->pcb = upcb;
  h->test_obj.user_preview_cb = (upcb) ? preview_cb_internal : NULL;
  return 0;
}

int qcamlib_register_video_cb(qcamlib_t h, qcamlib_frame_cb uvcb)
{
  int rc;
  if (h->video_on) {
    QCAM_ERR("video callback needs to be set before starting the video");
    return -1;
  }
  h->vcb = uvcb;
  h->test_obj.user_video_cb = (uvcb) ? video_cb_internal : NULL;
  return 0;
}

int qcamlib_register_video_buf_info_cb(qcamlib_t h, qcamlib_video_buf_info_cb cb)
{
  h->v_buf_info_cb = cb;
  return 0;
}

void qcamlib_deregister_preview_cb(qcamlib_t h)
{
    h->pcb = NULL;
    h->test_obj.user_preview_cb = NULL;
}

void qcamlib_deregister_video_cb(qcamlib_t h)
{
    h->vcb = NULL;
    h->test_obj.user_video_cb = NULL;
}

void qcamlib_set_cb_userdata(qcamlib_t h, void *userdata)
{
    h->cb_userdata = userdata;
}

int qcamlib_configure(qcamlib_t h, qcamlib_config_t config)
{
    // TODO: add input validation
    h->test_obj.preview_width = config.preview_dim.width;
    h->test_obj.preview_height = config.preview_dim.height;
    h->test_obj.video_width = config.video_dim.width;
    h->test_obj.video_height = config.video_dim.height;

    QCAM_DBG("preview %dx%d, video %dx%d",
			h->test_obj.preview_width, h->test_obj.preview_height,
			h->test_obj.video_width, h->test_obj.video_height);
    /* set focus mode */
    switch (config.af_mode) {
    case QCAM_FOCUS_FIXED:
        h->af_mode = CAM_FOCUS_MODE_FIXED;
        break;
    case QCAM_FOCUS_AUTO:
        h->af_mode = CAM_FOCUS_MODE_CONTINOUS_VIDEO;
        break;
    default:
        QCAM_ERR("Invalid focus mode");
        return -1;
    }
    int rc = setFocusMode(&h->test_obj, h->af_mode);
    if (rc != MM_CAMERA_OK) {
        QCAM_ERR("setFocusMode() rc=%d", rc);
        return -1;
    }
    return 0;
}

int qcamlib_copy_frame_to_buf(qcamlib_cb_data_t frame,
                              qcamlib_frame_info_t frame_info,
                              uint8_t *buf)
{
    int i;
    uint32_t src_offset, dest_offset;
    /* copy one line at a time */
    /* Y plane */
    src_offset = frame_info.planes[0].offset;
    dest_offset = 0;
    for (i=0; i<frame_info.height; i++) {
        memcpy(buf + dest_offset, frame.buffer + src_offset,
               frame_info.width);
        src_offset += frame_info.planes[0].stride;
        dest_offset += frame_info.width;
    }
    /* UV plane */
    src_offset = frame_info.planes[1].offset;
    dest_offset = frame_info.width * frame_info.height;
    for (i=0; i<frame_info.height/2; i++) {
        memcpy(buf + dest_offset, frame.buffer + src_offset,
               frame_info.width);
        src_offset += frame_info.planes[1].stride;
        dest_offset += frame_info.width;
    }
    return 0;
}

int qcamlib_release_video_frame(qcamlib_t h, qcamlib_cb_data_t frame)
{
    if (!frame.sbuf) {
        QCAM_ERR("invalid argument, frame.sbuf=NULL");
        return -1;
    }
    return mm_app_relese_video_frame((mm_camera_super_buf_t *)frame.sbuf,
                                     &h->test_obj);
}
