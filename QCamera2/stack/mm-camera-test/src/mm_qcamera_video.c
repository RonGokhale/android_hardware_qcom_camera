/*
Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "mm_qcamera_dbg.h"
#include "mm_qcamera_app.h"
#include <assert.h>

static void mm_app_video_notify_cb(mm_camera_super_buf_t *bufs,
                                   void *user_data)
{
    char file_name[64];
    int rc;

    mm_camera_buf_def_t *frame = bufs->bufs[0];
    mm_camera_test_obj_t *pme = (mm_camera_test_obj_t *)user_data;

    CDBG("%s: BEGIN - length=%d, frame idx = %d\n",
         __func__, frame->frame_len, frame->frame_idx);

    if (0  == frame->frame_idx % PREIVEW_FRAMEDUMP_INTERVAL) {
        snprintf(file_name, sizeof(file_name), "V_C%d", pme->cam->camera_handle);
        mm_app_dump_frame(frame, file_name, "yuv", frame->frame_idx);
    }

    rc = pme->cam->ops->qbuf(bufs->camera_handle, bufs->ch_id, frame);
    assert(rc == MM_CAMERA_OK);

    mm_app_cache_ops((mm_camera_app_meminfo_t *)frame->mem_info,
                     ION_IOC_INV_CACHES);

    CDBG("%s: END\n", __func__);
}

mm_camera_stream_t * mm_app_add_video_stream(mm_camera_test_obj_t *test_obj,
                                             mm_camera_channel_t *channel,
                                             mm_camera_buf_notify_t stream_cb,
                                             void *userdata,
                                             uint8_t num_bufs)
{
    int rc = MM_CAMERA_OK;
    mm_camera_stream_t *stream = NULL;
    cam_capability_t *cam_cap = (cam_capability_t *)(test_obj->cap_buf.buf.buffer);

    stream = mm_app_add_stream(test_obj, channel);
    assert(NULL != stream);

    stream->s_config.mem_vtbl.get_bufs = mm_app_stream_initbuf;
    stream->s_config.mem_vtbl.put_bufs = mm_app_stream_deinitbuf;
    stream->s_config.mem_vtbl.clean_invalidate_buf =
      mm_app_stream_clean_invalidate_buf;
    stream->s_config.mem_vtbl.invalidate_buf = mm_app_stream_invalidate_buf;
    stream->s_config.mem_vtbl.user_data = (void *)stream;
    stream->s_config.stream_cb = stream_cb;
    stream->s_config.userdata = userdata;

    uint8_t min_bufs = CAMERA_MIN_STREAMING_BUFFERS + CAMERA_MIN_VIDEO_BUFFERS + 1;

    if(num_bufs < min_bufs)
        stream->num_of_bufs = min_bufs;
    else
        stream->num_of_bufs = num_bufs;

    stream->s_config.stream_info = (cam_stream_info_t *)stream->s_info_buf.buf.buffer;
    memset(stream->s_config.stream_info, 0, sizeof(cam_stream_info_t));
    stream->s_config.stream_info->stream_type = CAM_STREAM_TYPE_VIDEO;
    stream->s_config.stream_info->streaming_mode = CAM_STREAMING_MODE_CONTINUOUS;
    stream->s_config.stream_info->fmt = test_obj->app_handle->video_format;  //DEFAULT_VIDEO_FORMAT;
    stream->s_config.stream_info->dim.width = (int32_t)test_obj->app_handle->video_width; //DEFAULT_VIDEO_WIDTH;
    stream->s_config.stream_info->dim.height = (int32_t)test_obj->app_handle->video_height; //DEFAULT_VIDEO_HEIGHT;
    stream->s_config.padding_info = cam_cap->padding_info;

    stream->s_config.stream_info->pp_config.feature_mask = CAM_QCOM_FEATURE_FLIP;
    stream->s_config.stream_info->pp_config.flip = test_obj->app_handle->flip_mode;

    rc = mm_app_config_stream(test_obj, channel, stream, &stream->s_config);
    assert(MM_CAMERA_OK == rc);

    return stream;
}

mm_camera_channel_t * mm_app_add_video_channel(mm_camera_test_obj_t *test_obj)
{
    mm_camera_channel_t *channel = NULL;
    mm_camera_stream_t *stream = NULL;

    channel = mm_app_add_channel(test_obj,
                                 MM_CHANNEL_TYPE_VIDEO,
                                 NULL,
                                 NULL,
                                 NULL);
    assert(NULL != channel);

    stream = mm_app_add_video_stream(test_obj,
                                     channel,
                                     mm_app_video_notify_cb,
                                     (void *)test_obj,
                                     1);
    assert(NULL != stream);

    return channel;
}

int mm_app_start_record_preview(mm_camera_test_obj_t *test_obj)
{
    int rc = MM_CAMERA_OK;
    mm_camera_channel_t *p_ch = NULL;
    mm_camera_channel_t *v_ch = NULL;
    mm_camera_channel_t *s_ch = NULL;

    p_ch = mm_app_add_preview_channel(test_obj);
    assert(NULL != p_ch);

    v_ch = mm_app_add_video_channel(test_obj);
    assert(NULL != v_ch);

    s_ch = mm_app_add_snapshot_channel(test_obj);
    assert(NULL != s_ch);

    rc = mm_app_start_channel(test_obj, p_ch);
    assert(MM_CAMERA_OK == rc);

    //Launch display thread.
    launch_camframe_fb_thread();

    return rc;
}

int mm_app_stop_record_preview(mm_camera_test_obj_t *test_obj)
{
    int rc = MM_CAMERA_OK;
    mm_camera_channel_t *p_ch = NULL;
    mm_camera_channel_t *v_ch = NULL;
    mm_camera_channel_t *s_ch = NULL;

    p_ch = mm_app_get_channel_by_type(test_obj, MM_CHANNEL_TYPE_PREVIEW);
    v_ch = mm_app_get_channel_by_type(test_obj, MM_CHANNEL_TYPE_VIDEO);
    s_ch = mm_app_get_channel_by_type(test_obj, MM_CHANNEL_TYPE_SNAPSHOT);

    rc = mm_app_stop_and_del_channel(test_obj, p_ch);
    assert(MM_CAMERA_OK == rc);

    rc = mm_app_stop_and_del_channel(test_obj, v_ch);
    assert(MM_CAMERA_OK == rc);

    rc = mm_app_stop_and_del_channel(test_obj, s_ch);
    assert(MM_CAMERA_OK == rc);

    //Release display thread.
    release_camframe_fb_thread();

    return rc;
}

int mm_app_start_record(mm_camera_test_obj_t *test_obj)
{
    int rc = MM_CAMERA_OK;
    mm_camera_channel_t *v_ch = NULL;

    v_ch = mm_app_get_channel_by_type(test_obj, MM_CHANNEL_TYPE_VIDEO);

    rc = mm_app_start_channel(test_obj, v_ch);
    assert(MM_CAMERA_OK == rc);

    return rc;
}

int mm_app_stop_record(mm_camera_test_obj_t *test_obj)
{
    int rc = MM_CAMERA_OK;
    mm_camera_channel_t *v_ch = NULL;

    v_ch = mm_app_get_channel_by_type(test_obj, MM_CHANNEL_TYPE_VIDEO);

    rc = mm_app_stop_channel(test_obj, v_ch);
    assert(MM_CAMERA_OK == rc);

    return rc;
}

int mm_app_start_live_snapshot(mm_camera_test_obj_t *test_obj)
{
    int rc = MM_CAMERA_OK;
    mm_camera_channel_t *s_ch = NULL;

    s_ch = mm_app_get_channel_by_type(test_obj, MM_CHANNEL_TYPE_SNAPSHOT);

    rc = mm_app_start_channel(test_obj, s_ch);
    assert(MM_CAMERA_OK == rc);

    return rc;
}

int mm_app_stop_live_snapshot(mm_camera_test_obj_t *test_obj)
{
    int rc = MM_CAMERA_OK;
    mm_camera_channel_t *s_ch = NULL;

    s_ch = mm_app_get_channel_by_type(test_obj, MM_CHANNEL_TYPE_SNAPSHOT);

    rc = mm_app_stop_channel(test_obj, s_ch);
    assert(MM_CAMERA_OK == rc);

    return rc;
}
