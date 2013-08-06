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

static void mm_app_metadata_notify_cb(mm_camera_super_buf_t *bufs, void *user_data);
mm_camera_stream_t * mm_app_add_metadata_stream(mm_camera_test_obj_t *test_obj,
                                             mm_camera_channel_t *channel,
                                             mm_camera_buf_notify_t stream_cb,
                                             void *userdata,
                                             uint8_t num_bufs);

static void mm_app_preview_notify_cb(mm_camera_super_buf_t *bufs,
                                     void *user_data)
{
    char file_name[64];
    int rc, i;

    mm_camera_buf_def_t *frame = bufs->bufs[0];
    mm_camera_test_obj_t *pme = (mm_camera_test_obj_t *)user_data;
    mm_camera_channel_t *channel = NULL;
    mm_camera_stream_t *p_stream = NULL;
    cam_stream_buf_plane_info_t *buf_planes = NULL;

    CDBG("%s: BEGIN - frame length=%d, frame idx = %d\n", __func__, frame->frame_len, frame->frame_idx);

    /* find channel */
    for (i = 0; i < MM_CHANNEL_TYPE_MAX; i++) {
        if (pme->channels[i].ch_id == bufs->ch_id) {
            channel = &pme->channels[i];
            break;
        }
    }
    assert(NULL != channel);

    /* find video stream */
    for (i = 0; i < channel->num_streams; i++) {
        if (channel->streams[i].s_config.stream_info->stream_type == CAM_STREAM_TYPE_PREVIEW) {
            p_stream = &channel->streams[i];
            break;
        }
    }
    assert(NULL != p_stream);

    //extract buf planes to get stride and scanlines
    buf_planes = &(p_stream->s_config.stream_info->buf_planes);

    //CDBG_HIGH("%s: Y offset: %d, CbCr offset: %d", __func__, buf_planes->plane_info.sp.y_offset, buf_planes->plane_info.sp.cbcr_offset);
    CDBG("%s: Frame length: %d, Num planes: %d", __func__, buf_planes->plane_info.frame_len, buf_planes->plane_info.num_planes);
    CDBG("%s: Y stride: %d  Y scanlines: %d", __func__, buf_planes->plane_info.mp[0].stride, buf_planes->plane_info.mp[0].scanline);
    CDBG("%s: UV stride: %d  UV scanlines: %d", __func__, buf_planes->plane_info.mp[1].stride, buf_planes->plane_info.mp[1].scanline);

    if (0  == frame->frame_idx % PREIVEW_FRAMEDUMP_INTERVAL) {
        snprintf(file_name, sizeof(file_name), "p_c%d_%d_%d", pme->cam->camera_handle, buf_planes->plane_info.mp[0].stride, buf_planes->plane_info.mp[0].scanline);
        mm_app_dump_frame(frame, file_name, "yuv", frame->frame_idx);
    }

    /* render camera preview buffers to display thread if 'no display mode' is not set, and
       callback is not from front camera in dual camera mode
    */
    if (pme->app_handle->no_display == false) {
        if ((pme->cam_id == 0) || (((pme->app_handle->test_mode & 0xFF00) >> 8) == MM_QCAMERA_APP_SINGLE_MODE)) {
            rc = mm_app_dl_render(frame->fd, pme);
            assert(rc == MM_CAMERA_OK);
        }
    }

    rc = pme->cam->ops->qbuf(bufs->camera_handle, bufs->ch_id, frame);
    assert(rc == MM_CAMERA_OK);

    mm_app_cache_ops((mm_camera_app_meminfo_t *)frame->mem_info,
                     ION_IOC_INV_CACHES);

    CDBG("%s: END\n", __func__);
}

static void mm_app_zsl_notify_cb(mm_camera_super_buf_t *bufs,
                                 void *user_data)
{
    int i = 0, rc;
    mm_camera_test_obj_t *pme = (mm_camera_test_obj_t *)user_data;
    mm_camera_channel_t *channel = NULL;
    mm_camera_stream_t *p_stream = NULL;
    mm_camera_stream_t *m_stream = NULL;
    mm_camera_buf_def_t *p_frame = NULL;
    mm_camera_buf_def_t *m_frame = NULL;

    CDBG("%s: BEGIN\n", __func__);

    /* find channel */
    for (i = 0; i < MM_CHANNEL_TYPE_MAX; i++) {
        if (pme->channels[i].ch_id == bufs->ch_id) {
            channel = &pme->channels[i];
            break;
        }
    }
    assert(NULL != channel);

    /* find preview stream */
    for (i = 0; i < channel->num_streams; i++) {
        if (channel->streams[i].s_config.stream_info->stream_type == CAM_STREAM_TYPE_PREVIEW) {
            p_stream = &channel->streams[i];
            break;
        }
    }
    assert(NULL != p_stream);

    /* find snapshot stream */
    for (i = 0; i < channel->num_streams; i++) {
        if (channel->streams[i].s_config.stream_info->stream_type == CAM_STREAM_TYPE_SNAPSHOT) {
            m_stream = &channel->streams[i];
            break;
        }
    }
    assert(NULL != m_stream);

    /* find preview frame */
    for (i = 0; i < bufs->num_bufs; i++) {
        if (bufs->bufs[i]->stream_id == p_stream->s_id) {
            p_frame = bufs->bufs[i];
            break;
        }
    }
    assert(NULL != p_frame);

    /* find snapshot frame */
    for (i = 0; i < bufs->num_bufs; i++) {
        if (bufs->bufs[i]->stream_id == m_stream->s_id) {
            m_frame = bufs->bufs[i];
            break;
        }
    }
    assert(NULL != m_frame);

    //render preview frame
    rc = mm_app_dl_render(p_frame->fd, pme);
    assert(MM_CAMERA_OK == rc);

    if (0  == p_frame->frame_idx % PREIVEW_FRAMEDUMP_INTERVAL) {
        mm_app_dump_frame(p_frame, "zsl_preview", "yuv", p_frame->frame_idx);
        mm_app_dump_frame(m_frame, "zsl_main", "yuv", m_frame->frame_idx);
    }

    rc = pme->cam->ops->qbuf(bufs->camera_handle,
                                            bufs->ch_id,
                                            p_frame);
    assert(MM_CAMERA_OK == rc);

    mm_app_cache_ops((mm_camera_app_meminfo_t *)p_frame->mem_info,
                     ION_IOC_INV_CACHES);

    rc = pme->cam->ops->qbuf(bufs->camera_handle,
                                            bufs->ch_id,
                                            m_frame);
    assert(MM_CAMERA_OK == rc);

    mm_app_cache_ops((mm_camera_app_meminfo_t *)m_frame->mem_info,
                     ION_IOC_INV_CACHES);

    CDBG("%s: END\n", __func__);
}

mm_camera_stream_t * mm_app_add_preview_stream(mm_camera_test_obj_t *test_obj,
                                               mm_camera_channel_t *channel,
                                               mm_camera_buf_notify_t stream_cb,
                                               void *userdata,
                                               uint8_t num_bufs)
{
    int rc = MM_CAMERA_OK;
    mm_camera_stream_t *stream = NULL;
    cam_capability_t *cam_cap = (cam_capability_t *)(test_obj->cap_buf.buf.buffer);

    stream = mm_app_add_stream(test_obj, channel);
    if (NULL == stream) {
        CDBG_ERROR("%s: add stream failed\n", __func__);
        return NULL;
    }

    stream->s_config.mem_vtbl.get_bufs = mm_app_stream_initbuf;
    stream->s_config.mem_vtbl.put_bufs = mm_app_stream_deinitbuf;
    stream->s_config.mem_vtbl.clean_invalidate_buf =
      mm_app_stream_clean_invalidate_buf;
    stream->s_config.mem_vtbl.invalidate_buf = mm_app_stream_invalidate_buf;
    stream->s_config.mem_vtbl.user_data = (void *)stream;
    stream->s_config.stream_cb = stream_cb;
    stream->s_config.userdata = userdata;

    uint8_t min_bufs = CAMERA_MIN_STREAMING_BUFFERS + CAMERA_MIN_JPEG_ENCODING_BUFFERS + 1;

    if(num_bufs < min_bufs)
        stream->num_of_bufs = min_bufs;
    else
        stream->num_of_bufs = num_bufs;

    stream->s_config.stream_info = (cam_stream_info_t *)stream->s_info_buf.buf.buffer;
    memset(stream->s_config.stream_info, 0, sizeof(cam_stream_info_t));
    stream->s_config.stream_info->stream_type = CAM_STREAM_TYPE_PREVIEW;
    stream->s_config.stream_info->streaming_mode = CAM_STREAMING_MODE_CONTINUOUS;
    stream->s_config.stream_info->fmt = test_obj->app_handle->preview_format; //DEFAULT_PREVIEW_FORMAT;
    stream->s_config.stream_info->dim.width = test_obj->app_handle->preview_width; //DEFAULT_PREVIEW_WIDTH;
    stream->s_config.stream_info->dim.height = test_obj->app_handle->preview_height; //DEFAULT_PREVIEW_HEIGHT;
    stream->s_config.padding_info = cam_cap->padding_info;

    stream->s_config.stream_info->pp_config.feature_mask = CAM_QCOM_FEATURE_FLIP;
    stream->s_config.stream_info->pp_config.flip = test_obj->app_handle->flip_mode;

    CDBG("%s: Preview W=%d & H=%d & Flip mode =%d", __func__, test_obj->app_handle->preview_width,
        test_obj->app_handle->preview_height, test_obj->app_handle->flip_mode);

    rc = mm_app_config_stream(test_obj, channel, stream, &stream->s_config);
    assert(MM_CAMERA_OK == rc);

    return stream;
}

mm_camera_stream_t * mm_app_add_snapshot_stream(mm_camera_test_obj_t *test_obj,
                                                mm_camera_channel_t *channel,
                                                mm_camera_buf_notify_t stream_cb,
                                                void *userdata,
                                                uint8_t num_bufs,
                                                uint8_t num_burst)
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

	uint8_t min_bufs = num_burst + CAMERA_MIN_STREAMING_BUFFERS +
                                CAMERA_MIN_JPEG_ENCODING_BUFFERS + 1;

    if(num_bufs < min_bufs)
        stream->num_of_bufs = min_bufs;
    else
        stream->num_of_bufs = num_bufs;

    stream->s_config.stream_info = (cam_stream_info_t *)stream->s_info_buf.buf.buffer;
    memset(stream->s_config.stream_info, 0, sizeof(cam_stream_info_t));
    stream->s_config.stream_info->stream_type = CAM_STREAM_TYPE_SNAPSHOT;

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

    stream->s_config.stream_info->fmt = test_obj->app_handle->snapshot_format;  //DEFAULT_SNAPSHOT_FORMAT;
    stream->s_config.stream_info->dim.width = test_obj->app_handle->snapshot_width; //DEFAULT_SNAPSHOT_WIDTH;
    stream->s_config.stream_info->dim.height = test_obj->app_handle->snapshot_height; //DEFAULT_SNAPSHOT_HEIGHT;
    stream->s_config.padding_info = cam_cap->padding_info;

    stream->s_config.stream_info->pp_config.feature_mask = CAM_QCOM_FEATURE_FLIP;
    stream->s_config.stream_info->pp_config.flip = test_obj->app_handle->flip_mode;

    rc = mm_app_config_stream(test_obj, channel, stream, &stream->s_config);
    assert(MM_CAMERA_OK == rc);

    return stream;
}

mm_camera_channel_t * mm_app_add_preview_channel(mm_camera_test_obj_t *test_obj)
{
    mm_camera_channel_t *channel = NULL;
    mm_camera_stream_t *p_stream = NULL;
    mm_camera_stream_t *m_stream = NULL;

    channel = mm_app_add_channel(
        test_obj,
        MM_CHANNEL_TYPE_PREVIEW,
        NULL,
        NULL,
        NULL);
    assert (NULL != channel);

    p_stream = mm_app_add_preview_stream(
        test_obj,
        channel,
        mm_app_preview_notify_cb,
        (void *)test_obj,
        PREVIEW_BUF_NUM);
    assert(NULL != p_stream);

    m_stream = mm_app_add_metadata_stream(
        test_obj,
        channel,
        mm_app_metadata_notify_cb,
        (void *)test_obj,
        META_BUF_NUM);
    assert(NULL != m_stream);

    return channel;
}

int32_t dump_face_detection_result(cam_face_detection_data_t *fd_data, mm_camera_test_obj_t *test_obj)
{
    int i =0;
    cam_dimension_t display_dim;
    display_dim.width = test_obj->app_handle->preview_width;
    display_dim.height = test_obj->app_handle->preview_height;
    CDBG_HIGH("Number of Faces detected fd_data->num_faces_detected=%d\n", fd_data->num_faces_detected);
    if (fd_data->num_faces_detected > 0) {
        for (i = 0; i < fd_data->num_faces_detected; i++) {
        CDBG("############## DUMP Facedetect Properties ############\n");
        CDBG("fd_data->faces[i].score=%d\n", fd_data->faces[i].score);
        CDBG("left coordinate=%d\n", MAP_TO_DRIVER_COORDINATE(fd_data->faces[i].face_boundary.left, display_dim.width, 2000, -1000));
        CDBG("top coordinate=%d\n", MAP_TO_DRIVER_COORDINATE(fd_data->faces[i].face_boundary.top, display_dim.height, 2000, -1000));
        CDBG("right coordinate=%d\n", MAP_TO_DRIVER_COORDINATE(fd_data->faces[i].face_boundary.width, display_dim.width, 2000, 0));
        CDBG("bottom coordinate=%d\n", MAP_TO_DRIVER_COORDINATE(fd_data->faces[i].face_boundary.height, display_dim.height, 2000, 0));
        CDBG("Center of left eye coordinate[0]=%d\n", MAP_TO_DRIVER_COORDINATE(fd_data->faces[i].left_eye_center.x, display_dim.width, 2000, -1000));
        CDBG("Center of left eye coordinate[1]=%d\n", MAP_TO_DRIVER_COORDINATE(fd_data->faces[i].left_eye_center.y, display_dim.height, 2000, -1000));
        CDBG("Center of right eye coordinate[0]=%d\n", MAP_TO_DRIVER_COORDINATE(fd_data->faces[i].right_eye_center.x, display_dim.width, 2000, -1000));
        CDBG("Center of right eye coordinate[1]=%d\n", MAP_TO_DRIVER_COORDINATE(fd_data->faces[i].right_eye_center.y, display_dim.height, 2000, -1000));
        CDBG("Center of mouth coordinate[0]=%d\n", MAP_TO_DRIVER_COORDINATE(fd_data->faces[i].mouth_center.x, display_dim.width, 2000, -1000));
        CDBG("Center of mouth coordinate[1]=%d\n", MAP_TO_DRIVER_COORDINATE(fd_data->faces[i].mouth_center.y, display_dim.height, 2000, -1000));
        CDBG("smile_degree=%d\n", fd_data->faces[i].smile_degree);
        CDBG("smile_confidence=%d\n", fd_data->faces[i].smile_confidence);
        CDBG("blink_detected=%d\n", fd_data->faces[i].blink_detected);
        CDBG("face_recognised=%d\n", fd_data->faces[i].face_recognised);
        CDBG("gaze_angle=%d\n", fd_data->faces[i].gaze_angle);
        // upscale by 2 to recover from demaen downscaling
        CDBG("updown_dir=%d\n", fd_data->faces[i].updown_dir * 2 );
        CDBG("leftright_dir =%d\n", fd_data->faces[i].leftright_dir * 2 );
        CDBG("roll_dir=%d\n", fd_data->faces[i].roll_dir * 2 );
        CDBG("uleft_blink=%d\n", fd_data->faces[i].left_blink);
        CDBG("right_blink=%d\n", fd_data->faces[i].right_blink);
        CDBG("left_right_gaze =%d\n", fd_data->faces[i].left_right_gaze);
        CDBG("top_bottom_gaze =%d\n", fd_data->faces[i].top_bottom_gaze);
       }
    }
    return MM_CAMERA_OK;
}

static void mm_app_metadata_notify_cb(mm_camera_super_buf_t *bufs,
                                   void *user_data)
{
    char file_name[64];
    int rc;

    mm_camera_buf_def_t *frame = bufs->bufs[0];
    cam_metadata_info_t *pMetaData = (cam_metadata_info_t *)frame->buffer;
    mm_camera_test_obj_t *pme = (mm_camera_test_obj_t *)user_data;

    CDBG("%s: BEGIN - length=%d, frame idx = %d frame->buffer =%x\n",
         __func__, frame->frame_len, frame->frame_idx, frame->buffer);

    CDBG("pMetaData->is_faces_valid =%d \n", pMetaData->is_faces_valid);

    if (pMetaData->is_faces_valid) {
        if (pMetaData->faces_data.num_faces_detected > MAX_ROI) {
            CDBG_ERROR("%s: Invalid number of faces %d\n",
                __func__, pMetaData->faces_data.num_faces_detected);
        } else {
            // process face detection result
            CDBG("%s: Number of faces detected %d\n",__func__,pMetaData->faces_data.num_faces_detected);
#if(FACEPROC_DUMP)
                int32_t rc = dump_face_detection_result(&(pMetaData->faces_data), pme);
                if (rc != MM_CAMERA_OK) {
                    CDBG_ERROR("%s: face detection dump failed\n", __func__);
                }
#endif
         }
    }

    rc = pme->cam->ops->qbuf(bufs->camera_handle, bufs->ch_id, frame);
    assert(rc == MM_CAMERA_OK);

    mm_app_cache_ops((mm_camera_app_meminfo_t *)frame->mem_info,
                     ION_IOC_INV_CACHES);

    CDBG("%s: END\n", __func__);
}


mm_camera_stream_t * mm_app_add_metadata_stream(mm_camera_test_obj_t *test_obj,
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
    stream->s_config.stream_info->stream_type = CAM_STREAM_TYPE_METADATA;
    stream->s_config.stream_info->streaming_mode = CAM_STREAMING_MODE_CONTINUOUS;
    stream->s_config.stream_info->fmt = test_obj->app_handle->preview_format;
    stream->s_config.stream_info->dim.width = test_obj->app_handle->preview_width;
    stream->s_config.stream_info->dim.height = test_obj->app_handle->preview_height;
    stream->s_config.padding_info = cam_cap->padding_info;

    stream->s_config.stream_info->pp_config.feature_mask = CAM_QCOM_FEATURE_FLIP;
    stream->s_config.stream_info->pp_config.flip = test_obj->app_handle->flip_mode;
    rc = mm_app_config_stream(test_obj, channel, stream, &stream->s_config);
    assert(MM_CAMERA_OK == rc);
    return stream;
}

int mm_app_start_preview(mm_camera_test_obj_t *test_obj)
{
    int rc = MM_CAMERA_OK;
    mm_camera_channel_t *channel = NULL;
    mm_camera_stream_t *stream = NULL;

    channel =  mm_app_add_preview_channel(test_obj);
    assert(NULL != channel);

    rc = mm_app_start_channel(test_obj, channel);
    assert(MM_CAMERA_OK == rc);

    launch_camframe_fb_thread();

    return rc;
}

int mm_app_stop_preview(mm_camera_test_obj_t *test_obj)
{
    int rc = MM_CAMERA_OK;

    mm_camera_channel_t *channel =
    mm_app_get_channel_by_type(test_obj, MM_CHANNEL_TYPE_PREVIEW);

    rc = mm_app_stop_and_del_channel(test_obj, channel);
    assert(MM_CAMERA_OK == rc);

    release_camframe_fb_thread();

    return rc;
}

int mm_app_start_preview_zsl(mm_camera_test_obj_t *test_obj)
{
    int32_t rc = MM_CAMERA_OK;
    mm_camera_channel_t *channel = NULL;
    mm_camera_stream_t *s_preview = NULL;
    mm_camera_stream_t *s_main = NULL;
    mm_camera_channel_attr_t attr;

    memset(&attr, 0, sizeof(mm_camera_channel_attr_t));
    attr.notify_mode = MM_CAMERA_SUPER_BUF_NOTIFY_BURST;
    attr.look_back = 2;
    attr.post_frame_skip = 0;
    attr.water_mark = 2;
    attr.max_unmatched_frames = 3;
    channel = mm_app_add_channel(test_obj,
                                 MM_CHANNEL_TYPE_ZSL,
                                 &attr,
                                 mm_app_zsl_notify_cb,
                                 test_obj);
    assert(NULL != channel);

    s_preview = mm_app_add_preview_stream(test_obj,
                                          channel,
                                          mm_app_preview_notify_cb,
                                          (void *)test_obj,
                                          PREVIEW_BUF_NUM);
    assert(NULL != s_preview);

    s_main = mm_app_add_snapshot_stream(test_obj,
                                        channel,
                                        NULL,
                                        NULL,
                                        PREVIEW_BUF_NUM,
                                        0);
    assert(NULL != s_main);

    rc = mm_app_start_channel(test_obj, channel);
    assert(MM_CAMERA_OK == rc);

    //Launch display thread.
    launch_camframe_fb_thread();

    return rc;
}

int mm_app_stop_preview_zsl(mm_camera_test_obj_t *test_obj)
{
    int rc = MM_CAMERA_OK;

    mm_camera_channel_t *channel =
        mm_app_get_channel_by_type(test_obj, MM_CHANNEL_TYPE_ZSL);

    rc = mm_app_stop_and_del_channel(test_obj, channel);
    assert(MM_CAMERA_OK == rc);

    release_camframe_fb_thread();

    return rc;
}
