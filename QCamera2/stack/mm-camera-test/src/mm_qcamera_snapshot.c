/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

#include "mm_qcamera_dbg.h"
#include "mm_qcamera_app.h"
#include <assert.h>

static mm_jpeg_color_format get_jpeg_color_format(cam_format_t img_fmt)
{
    switch (img_fmt) {
    case CAM_FORMAT_YUV_420_NV21:
        return MM_JPEG_COLOR_FORMAT_YCRCBLP_H2V2;
    case CAM_FORMAT_YUV_420_NV21_ADRENO:
        return MM_JPEG_COLOR_FORMAT_YCRCBLP_H2V2;
    case CAM_FORMAT_YUV_420_NV12:
        return MM_JPEG_COLOR_FORMAT_YCBCRLP_H2V2;
    case CAM_FORMAT_YUV_420_YV12:
        return MM_JPEG_COLOR_FORMAT_YCBCRLP_H2V2;
    case CAM_FORMAT_YUV_422_NV61:
        return MM_JPEG_COLOR_FORMAT_YCRCBLP_H2V1;
    case CAM_FORMAT_YUV_422_NV16:
        return MM_JPEG_COLOR_FORMAT_YCBCRLP_H2V1;
    default:
        return MM_JPEG_COLOR_FORMAT_YCRCBLP_H2V2;
    }
}

static mm_camera_app_frame_stack * find_free_frame_slot(mm_camera_app_frame_stack *queue, int size)
{
    int i;
    for (i = 0; i < size; i++) {
        if (queue[i].job_id == 0) {
            CDBG("%s: Index: %d", __func__, i);
            return &queue[i];
        }
    }

    return NULL;
}

static mm_camera_app_frame_stack * find_jobid_frame_slot(mm_camera_app_frame_stack *queue, int size, uint32_t job_id)
{
    int i;
    for (i = 0; i < size; i++) {
        if (queue[i].job_id == job_id) {
            CDBG("%s: Index: %d", __func__, i);
            return &queue[i];
        }
    }

    return NULL;
}

/* This callback is received once the complete JPEG encoding is done */
static void jpeg_encode_cb(jpeg_job_status_t status,
                           uint32_t client_hdl,
                           uint32_t job_id,
                           mm_jpeg_output_t *p_buf,
                           void *user_data)
{
    int i = 0;
    mm_camera_app_frame_stack *frame_slot;
    mm_camera_test_obj_t *pme = (mm_camera_test_obj_t *)user_data;

    CDBG("%s: Enter\n", __func__);

    assert(NULL != pme);
    assert(NULL != p_buf);
    assert(status == JPEG_JOB_STATUS_DONE);

    frame_slot = find_jobid_frame_slot(pme->jpeg_frame_queue, CAMERA_MAX_JPEG_SESSIONS, job_id);
    assert(NULL != frame_slot);

    CDBG("%s: Job-frame-jobid: %d, Job-frame-camera-handle: %x, Job-frame-buff[0]: %x, job-frame-numbuffs: %d",
    __func__, frame_slot->job_id, frame_slot->job_frame.camera_handle, frame_slot->job_frame.bufs[0], frame_slot->job_frame.num_bufs);

    mm_app_dump_jpeg_frame(p_buf->buf_vaddr, p_buf->buf_filled_len, "jpeg_dump", "jpg", job_id + (uint32_t)pme->cam->camera_handle);

    //invalidate cache
    mm_app_cache_ops(&pme->jpeg_buf.mem_info, ION_IOC_CLEAN_INV_CACHES);

    /* signal snapshot is done */
    mm_camera_app_done();

    //free up the slot
    frame_slot->job_id = 0;

    /* enqueue buffers back if more jpegs needs to be captured */
    if (pme->app_handle->num_rcvd_snapshot < pme->app_handle->num_snapshot)
    {
        for (i = 0; i < frame_slot->job_frame.num_bufs; i++)
        {
            int rc = pme->cam->ops->qbuf(
                frame_slot->job_frame.camera_handle,
                frame_slot->job_frame.ch_id,
                frame_slot->job_frame.bufs[i]);

            assert(rc == MM_CAMERA_OK);
        }
    }

    CDBG("%s: Exit\n", __func__);
}

int start_jpeg_encode(mm_camera_test_obj_t *test_obj,
                          mm_camera_stream_t *m_stream,
                          mm_camera_buf_def_t *m_frame,
                          mm_camera_super_buf_t* recvd_frame)
{
    mm_jpeg_encode_params_t encode_param;
    mm_camera_app_frame_stack *frame_slot;
    int rc = 0;

    CDBG("%s: Start", __func__);

    memset(&encode_param, 0, sizeof(mm_jpeg_encode_params_t));
    encode_param.jpeg_cb = jpeg_encode_cb;

    encode_param.encode_thumbnail = 0;
    encode_param.quality = test_obj->app_handle->jpeg_quality;
    encode_param.color_format = get_jpeg_color_format(DEFAULT_SNAPSHOT_FORMAT);
    encode_param.thumb_color_format = get_jpeg_color_format(DEFAULT_PREVIEW_FORMAT);

    encode_param.userdata = (void*)test_obj;

    /* fill in main src img encode param */
    encode_param.num_src_bufs = 1;
    encode_param.src_main_buf[0].index = 0;
    encode_param.src_main_buf[0].buf_size = m_frame->frame_len;
    encode_param.src_main_buf[0].buf_vaddr = (uint8_t *)m_frame->buffer;
    encode_param.src_main_buf[0].fd = m_frame->fd;
    encode_param.src_main_buf[0].format = MM_JPEG_FMT_YUV;
    encode_param.src_main_buf[0].offset = m_stream->offset;

    CDBG("%s: JPEG input buffer - frame len: %d, frame_offset: %d", __func__, m_frame->frame_len, m_stream->offset);

    /* fill in sink img param */
    encode_param.num_dst_bufs = 1;
    encode_param.dest_buf[0].index = 0;
    encode_param.dest_buf[0].buf_size = test_obj->jpeg_buf.buf.frame_len;
    encode_param.dest_buf[0].buf_vaddr = (uint8_t *)test_obj->jpeg_buf.buf.buffer;
    encode_param.dest_buf[0].fd = test_obj->jpeg_buf.buf.fd;
    encode_param.dest_buf[0].format = MM_JPEG_FMT_YUV;

    assert(NULL != encode_param.dest_buf[0].buf_vaddr);
    assert(NULL != encode_param.src_main_buf[0].buf_vaddr);
    assert(0 != encode_param.src_main_buf[0].buf_size);
    assert(0 != encode_param.dest_buf[0].buf_size);

    rc = test_obj->jpeg_ops.create_session(test_obj->jpeg_hdl,
                                             &encode_param,
                                             &test_obj->current_jpeg_sess_id);
    assert(0 == rc);

    /* Start encode process */
    mm_jpeg_job_t job;
    memset(&job, 0, sizeof(job));
    job.job_type = JPEG_JOB_TYPE_ENCODE;
    job.encode_job.session_id = test_obj->current_jpeg_sess_id;
    job.encode_job.rotation = 0;

    /* fill in main src img encode param */
    job.encode_job.main_dim.src_dim = m_stream->s_config.stream_info->dim;
    job.encode_job.main_dim.dst_dim = m_stream->s_config.stream_info->dim;
    job.encode_job.src_index = 0;

    job.encode_job.thumb_dim.src_dim = m_stream->s_config.stream_info->dim;
    job.encode_job.thumb_dim.dst_dim.width = test_obj->app_handle->preview_width;//DEFAULT_PREVIEW_WIDTH;
    job.encode_job.thumb_dim.dst_dim.height = test_obj->app_handle->preview_height; //DEFAULT_PREVIEW_HEIGHT;

    /* fill in sink img param */
    job.encode_job.dst_index = 0;

    /*  TODO: Enable rotation through reprocess
    //back camera, rotate 90
    if (cam_cap->position == CAM_POSITION_BACK)
        job.encode_job.rotation = 90;
    */

    //TODO: Enable exif based on meta data

    //Get a free slot from test object frame queue and
    //copy received frame structure into it
    frame_slot = find_free_frame_slot(test_obj->jpeg_frame_queue, CAMERA_MAX_JPEG_SESSIONS);
    assert(NULL != frame_slot);
    frame_slot->job_frame = *recvd_frame;

    //call async encode routine
    rc = test_obj->jpeg_ops.start_job(&job, &frame_slot->job_id);
    assert(0 == rc);

    CDBG("%s: Job-frame-jobid: %d, Job-frame-camera-handle: %x, Job-frame-buff[0]: %x, job-frame-numbuffs: %d",
        __func__, frame_slot->job_id, recvd_frame->camera_handle, recvd_frame->bufs[0], recvd_frame->num_bufs);

end:
    CDBG("%s: End", __func__);
    return rc;
}

static void mm_app_snapshot_notify_cb(mm_camera_super_buf_t *bufs,
                                      void *user_data)
{
    int i = 0;
    mm_camera_test_obj_t *pme = (mm_camera_test_obj_t *)user_data;
    mm_camera_channel_t *channel = NULL;
    mm_camera_stream_t *p_stream = NULL;
    mm_camera_stream_t *m_stream = NULL;
    mm_camera_buf_def_t *p_frame = NULL;
    mm_camera_buf_def_t *m_frame = NULL;

    CDBG("%s: Enter\n", __func__);

    //skip jpeg encoding if required number of jpegs are already captured
    if(pme->app_handle->num_rcvd_snapshot >= pme->app_handle->num_snapshot) {
        CDBG("%s: JPEG encoding skipped. Received snapshot: %d, Required snapshots: %d",
            __func__, pme->app_handle->num_rcvd_snapshot, pme->app_handle->num_snapshot);
        goto end;
    }

    /* find channel */
    for (i = 0; i < MM_CHANNEL_TYPE_MAX; i++) {
        if (pme->channels[i].ch_id == bufs->ch_id) {
            channel = &pme->channels[i];
            break;
        }
    }
    assert(NULL != channel);

    /* find snapshot stream */
    for (i = 0; i < channel->num_streams; i++) {
        if (channel->streams[i].s_config.stream_info->stream_type == CAM_STREAM_TYPE_SNAPSHOT) {
            m_stream = &channel->streams[i];
            break;
        }
    }
    assert(NULL != m_stream);

    /* find snapshot frame */
    for (i = 0; i < bufs->num_bufs; i++) {
        if (bufs->bufs[i]->stream_id == m_stream->s_id) {
            m_frame = bufs->bufs[i];
            break;
        }
    }
    assert(NULL != m_frame);

    mm_app_dump_frame(m_frame, "main", "yuv", m_frame->frame_idx + (uint32_t)pme->cam->camera_handle);

    /* find postview stream */
    for (i = 0; i < channel->num_streams; i++) {
        if (channel->streams[i].s_config.stream_info->stream_type == CAM_STREAM_TYPE_POSTVIEW) {
            p_stream = &channel->streams[i];
            break;
        }
    }
    if (NULL != p_stream) {
        /* find preview frame */
        for (i = 0; i < bufs->num_bufs; i++) {
            if (bufs->bufs[i]->stream_id == p_stream->s_id) {
                p_frame = bufs->bufs[i];
                break;
            }
        }
        if (NULL != p_frame) {
            mm_app_dump_frame(p_frame, "postview", "yuv", p_frame->frame_idx + (uint32_t)pme->cam->camera_handle);
            mm_app_cache_ops((mm_camera_app_meminfo_t *)p_frame->mem_info,
                     ION_IOC_CLEAN_INV_CACHES);
        }
    }

    mm_app_cache_ops((mm_camera_app_meminfo_t *)m_frame->mem_info,
                     ION_IOC_CLEAN_INV_CACHES);

    //increment the captured jpeg counter
    pme->app_handle->num_rcvd_snapshot++;
    start_jpeg_encode(pme, m_stream, m_frame, bufs);

end:
    CDBG("%s: Exit\n", __func__);
}

mm_camera_channel_t * mm_app_add_snapshot_channel(mm_camera_test_obj_t *test_obj)
{
    mm_camera_channel_t *channel = NULL;
    mm_camera_stream_t *stream = NULL;

    CDBG("%s: Enter\n", __func__);

    channel = mm_app_add_channel(test_obj,
                                 MM_CHANNEL_TYPE_SNAPSHOT,
                                 NULL,
                                 NULL,
                                 NULL);
    assert(NULL != channel);

    stream = mm_app_add_snapshot_stream(test_obj,
                                        channel,
                                        mm_app_snapshot_notify_cb,
                                        (void *)test_obj,
                                        1,
                                        1);
    assert(NULL != stream);

    CDBG("%s: Exit\n", __func__);

    return channel;
}

mm_camera_stream_t * mm_app_add_postview_stream(mm_camera_test_obj_t *test_obj,
                                                mm_camera_channel_t *channel,
                                                mm_camera_buf_notify_t stream_cb,
                                                void *userdata,
                                                uint8_t num_bufs,
                                                uint8_t num_burst)
{
    int rc = MM_CAMERA_OK;
    mm_camera_stream_t *stream = NULL;
    cam_capability_t *cam_cap = (cam_capability_t *)(test_obj->cap_buf.buf.buffer);

    CDBG("%s: Enter\n", __func__);

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

    uint8_t min_bufs = num_burst + CAMERA_MIN_STREAMING_BUFFERS + CAMERA_MIN_JPEG_ENCODING_BUFFERS + 1;

    if(num_bufs < min_bufs)
        stream->num_of_bufs = min_bufs;
    else
        stream->num_of_bufs = num_bufs;

    stream->s_config.stream_info = (cam_stream_info_t *)stream->s_info_buf.buf.buffer;
    memset(stream->s_config.stream_info, 0, sizeof(cam_stream_info_t));
    stream->s_config.stream_info->stream_type = CAM_STREAM_TYPE_POSTVIEW;

    if (num_burst > 0) {
        stream->s_config.stream_info->streaming_mode = CAM_STREAMING_MODE_BURST;
        /* Info: There could be some frame mismatch in the frameIDs of postview
           and snapshot images. This may result in not getting callback as the
           mm-camera interface tries to match the two IDs before giving callback
           for super buff. To compensate it, configure num_bursts to be slightly
           higher than what is requested. There is a logic in test app to discard
           additional callbacks
        */
        stream->s_config.stream_info->num_of_burst = num_burst + CAMERA_FRAME_ID_OFFSET;
    } else {
        stream->s_config.stream_info->streaming_mode = CAM_STREAMING_MODE_CONTINUOUS;
    }

    stream->s_config.stream_info->fmt = test_obj->app_handle->preview_format; //DEFAULT_PREVIEW_FORMAT;
    stream->s_config.stream_info->dim.width = test_obj->app_handle->preview_width; //DEFAULT_PREVIEW_WIDTH;
    stream->s_config.stream_info->dim.height = test_obj->app_handle->preview_height; //DEFAULT_PREVIEW_HEIGHT;
    stream->s_config.padding_info = cam_cap->padding_info;

    rc = mm_app_config_stream(test_obj, channel, stream, &stream->s_config);
    assert(MM_CAMERA_OK == rc);

    CDBG("%s: Exit\n", __func__);

    return stream;
}

int mm_app_start_capture(mm_camera_test_obj_t *test_obj,
                         uint8_t num_snapshots)
{
    int32_t rc = MM_CAMERA_OK;
    mm_camera_channel_t *channel = NULL;
    mm_camera_stream_t *s_main = NULL;
    mm_camera_stream_t *s_postview = NULL;
    mm_camera_channel_attr_t attr;

    CDBG("%s: Enter\n", __func__);

    memset(&attr, 0, sizeof(mm_camera_channel_attr_t));
    attr.notify_mode = MM_CAMERA_SUPER_BUF_NOTIFY_CONTINUOUS;
    attr.max_unmatched_frames = 3;
    channel = mm_app_add_channel(test_obj,
                                 MM_CHANNEL_TYPE_CAPTURE,
                                 &attr,
                                 mm_app_snapshot_notify_cb,
                                 test_obj);
    assert(NULL != channel);

    s_postview = mm_app_add_postview_stream(test_obj,
                                            channel,
                                            NULL,
                                            NULL,
                                            num_snapshots,
                                            num_snapshots);
    assert(NULL != s_postview);

    s_main = mm_app_add_snapshot_stream(test_obj,
                                        channel,
                                        NULL,
                                        NULL,
                                        num_snapshots,
                                        num_snapshots);
    assert(NULL != s_main);

    rc = mm_app_start_channel(test_obj, channel);
    assert(MM_CAMERA_OK == rc);

    CDBG("%s: Exit\n", __func__);
    //rc = mm_app_request_super_buf(test_obj, channel, num_snapshots);
    //assert(MM_CAMERA_OK == rc);

    return rc;
}

int mm_app_stop_capture(mm_camera_test_obj_t *test_obj)
{
    int rc = MM_CAMERA_OK;
    CDBG("%s: Enter\n", __func__);

    mm_camera_channel_t *channel =
    mm_app_get_channel_by_type(test_obj, MM_CHANNEL_TYPE_CAPTURE);

    rc = mm_app_stop_and_del_channel(test_obj, channel);
    assert(MM_CAMERA_OK == rc);

    CDBG("%s: Exit\n", __func__);

    return rc;
}

int mm_app_take_picture(mm_camera_test_obj_t *test_obj, uint8_t is_burst_mode)
{
    CDBG_HIGH("\nEnter %s!!\n", __func__);
    int rc = MM_CAMERA_OK;

    test_obj->app_handle->num_rcvd_snapshot = 0;
    test_obj->app_handle->num_snapshot = 1;

    if (is_burst_mode)
        test_obj->app_handle->num_snapshot = 4;


    test_obj->cam->ops->prepare_snapshot(test_obj->cam->camera_handle, false);
    CDBG("%s:Wait for prepare snapshot!!\n",__func__);
    //wait for prepare snapshot done.
    mm_camera_app_wait();

    //stop preview before starting capture.
    rc = mm_app_stop_preview(test_obj);
    assert(rc == MM_CAMERA_OK);

    rc = mm_app_start_capture(test_obj, test_obj->app_handle->num_snapshot);
    assert(rc == MM_CAMERA_OK);

    //wait till required snapshots are captured
    do {
        mm_camera_app_wait();
        CDBG("%s: current snapshot count:%d, required count: %d", __func__, test_obj->app_handle->num_rcvd_snapshot, test_obj->app_handle->num_snapshot);
    }
    while (test_obj->app_handle->num_rcvd_snapshot < test_obj->app_handle->num_snapshot);

    rc = mm_app_stop_capture(test_obj);
    assert(rc == MM_CAMERA_OK);

    //start preview after capture.
    rc = mm_app_start_preview(test_obj);
    assert(rc == MM_CAMERA_OK);

    CDBG_HIGH("\nExit %s!!\n", __func__);
    return rc;
}
