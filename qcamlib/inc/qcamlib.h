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

/**
 * @file qcamlib.h
 * @brief Camera API definition header file
 *
 */

#ifndef __QCAMLIB_H__
#define __QCAMLIB_H__

#include <stdint.h>
/**
 *  @brief frame buffer plane information
 */
typedef struct _qcamlib_plane_info_t {
  uint32_t len;
  uint32_t stride;
  uint32_t scanlines;
  uint32_t width;
  uint32_t height;
  uint32_t offset;
} qcamlib_plane_info_t;

/**
 *  @brief Enumeration for image format types
 */
typedef enum _qcamlib_img_format_t {
  QCAMLIB_FMT_NV12,
  QCAMLIB_FMT_NV21,
  QCAMLIB_FMT_MAX,
} qcamlib_img_format_t;

/**
 *  @brief frame buffer information about size, format etc
 */
typedef struct _qcamlib_frame_info_t {
  qcamlib_img_format_t fmt;
  uint32_t buf_size;
  uint32_t width;
  uint32_t height;
  qcamlib_plane_info_t planes[2];
} qcamlib_frame_info_t;

/**
 *  @brief data structure returned with callbacks
 */
typedef struct _qcamlib_cb_data_t {
    uint8_t *buffer;
    /* frame timestamp */
    struct timespec ts;
    /* client private userdata */
    void *userdata;
    /* pointer to framebuffer data maintained by camera framework */
    void *sbuf;
} qcamlib_cb_data_t;

/**
 *  @brief enum type for streams
 */
typedef enum _qcamlib_stream_type_t {
  QCAMLIB_STREAM_PREVIEW,
  QCAMLIB_STREAM_VIDEO,
  QCAMLIB_STREAM_MAX,
} qcamlib_stream_type_t;

/**
 *  @brief stream dimensions
 */
typedef struct _qcamlib_stream_dim_t {
    uint32_t width;
    uint32_t height;
} qcamlib_stream_dim_t;

/**
 * @brief ion memory information single video buffer
 */
typedef struct _qcamlib_video_buf_info_t {
    int fd;
    uint32_t size;
    void *vaddr;
} qcamlib_video_buf_info_t;

/**
 * @brief data structure delivered as part of video buf_info
 *        callback.
 */
typedef struct _qcamlib_video_buf_cb_data_t {
    int32_t num_bufs;
    qcamlib_video_buf_info_t *buf_info;
    void *userdata;
} qcamlib_video_buf_cb_data_t;

/**
 * @brief buffer information callback to get ion memory
 *        information for video stream
 */
typedef void (*qcamlib_video_buf_info_cb) (qcamlib_video_buf_cb_data_t cb_data);


typedef enum {
    QCAM_FOCUS_FIXED,
    QCAM_FOCUS_AUTO,
} qcamlib_focus_mode_t;

/**
 *  @brief qcamlib configuration
 */
typedef struct _qcamlib_config_t {
    qcamlib_stream_dim_t preview_dim;
    qcamlib_stream_dim_t video_dim;
    qcamlib_stream_dim_t snapshot_dim;
    qcamlib_focus_mode_t af_mode;
} qcamlib_config_t;

/**
 *  @brief opaque datatype for qcamilb handle
 */
typedef struct _qcamlib_obj* qcamlib_t;

/**
 * @brief Function pointer type for frame callbacks
 */
typedef void (*qcamlib_frame_cb) (qcamlib_cb_data_t frame);

/**
 * @brief Create qcamlib instance and initialize camera
 * @return
 *    returns qcamlib handle, NULL if failed
 */
qcamlib_t qcamlib_create();

/**
 * @brief Clean up library resources
 *
 */
void qcamlib_destroy(qcamlib_t h);

/**
 * @brief Start the Preview stream
 * @param h qcamlib handle
 * @return int
 *    returns 0 on success, negative value on failure
 */
int qcamlib_start_preview(qcamlib_t h);

/**
 * @brief Stop the Preview stream
 * @param h qcamlib handle
 * @return int
 *    returns 0 on success, negative value on failure
 */
int qcamlib_stop_preview(qcamlib_t h);

/**
 * @brief Start the Video stream
 * @param h qcamlib handle
 * @return int
 *    returns 0 on success, negative value on failure
 */
int qcamlib_start_video(qcamlib_t h);

/**
 * @brief Stop the Video stream
 * @param h qcamlib handle
 * @return int
 *    returns 0 on success, negative value on failure
 */
int qcamlib_stop_video(qcamlib_t h);

/**
 * @brief Register preview frame callback
 * @param h qcamlib handle
 * @param upcb pointer to the callback function
 * @return int
 *    returns 0 on success, negative value on failure
 */
int qcamlib_register_preview_cb(qcamlib_t h, qcamlib_frame_cb upcb);

/**
 * @brief Register video frame callback
 * @param h qcamlib handle
 * @param uvcb pointer to the callback function
 * @return int
 *    returns 0 on success, negative value on failure
 */
int qcamlib_register_video_cb(qcamlib_t h, qcamlib_frame_cb uvcb);

/**
 * @brief Deregister preview callback
 * @param h qcamlib handle
 */
void qcamlib_deregister_preview_cb(qcamlib_t h);

/**
 * @brief Deregister video callback
 * @param h qcamlib handle
 */
void qcamlib_deregister_video_cb(qcamlib_t h);

/**
 * @brief get frame buffer info for a stream
 * @param h qcamlib handle
 * @param stream_type type of the stream
 * @return qcamlib_frame_info_t
 *    returns structure with frame buffer information
 */
qcamlib_frame_info_t qcamlib_get_frame_info(qcamlib_t h,
                                            qcamlib_stream_type_t stream_type);

/**
 * @brief set userdata that is going to be returned with
 *        callbacks
 * @param h qcamlib handle
 * @param void* userdata pointer
 */
void qcamlib_set_cb_userdata(qcamlib_t h, void *userdata);

/**
 * @brief configure the camera parameters
 * @param h qcamlib handle
 * @param config configuration data structure
 *
 * @return int 0:success, non-zero: failure
 */
int qcamlib_configure(qcamlib_t h, qcamlib_config_t config);

/**
 * @brief copy the frame to a user buffer
 *
 * @param frame frame data received in callback
 * @param frame_info frame information data
 * @param buf user allocated buffer
 *
 * @return int 0: success, non-zero: failure
 */
int qcamlib_copy_frame_to_buf(qcamlib_cb_data_t frame,
                              qcamlib_frame_info_t frame_info,
                              uint8_t *buf);

/**
 * @brief register video buf info callback
 * @param h
 *     qcamlib handle
 * @param cb
 *     user callback funtion pointer
 * @return int
 *     0: sucess
 *     non-zero: failure
 */
int qcamlib_register_video_buf_info_cb(qcamlib_t h,
                                       qcamlib_video_buf_info_cb cb);

/**
 * @brief function to indicate that client is done with video
 *        frame
 * @param h
 *     qcamlib handle
 * @param frame
 *     video frame data
 * @return int
 *     0: sucess
 *     non-zero: failure
 */
int qcamlib_release_video_frame(qcamlib_t h, qcamlib_cb_data_t frame);

#endif
