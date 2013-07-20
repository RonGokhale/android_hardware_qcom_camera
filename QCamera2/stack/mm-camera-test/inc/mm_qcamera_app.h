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

#ifndef __MM_QCAMERA_APP_H__
#define __MM_QCAMERA_APP_H__

#include <pthread.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>

#include "mm_camera_interface.h"
#include "mm_jpeg_interface.h"

#define MM_QCAMERA_APP_INTERATION 1

#define PREIVEW_FRAMEDUMP_INTERVAL 50

#define PREVIEW_BUF_NUM 7
#define VIDEO_BUF_NUM 7
#define ISP_PIX_BUF_NUM 9
#define STATS_BUF_NUM 4
#define RDI_BUF_NUM 8

#define CAMERA_MIN_STREAMING_BUFFERS     5
#define CAMERA_MIN_JPEG_ENCODING_BUFFERS 2
#define CAMERA_MIN_VIDEO_BUFFERS         9
#define CAMERA_FRAME_ID_OFFSET           2

 #define CAMERA_MAX_JPEG_SESSIONS (CAMERA_MIN_JPEG_ENCODING_BUFFERS + CAMERA_MIN_STREAMING_BUFFERS + 1)

#define DEFAULT_PREVIEW_FORMAT    CAM_FORMAT_YUV_420_NV21
#define DEFAULT_PREVIEW_WIDTH     800
#define DEFAULT_PREVIEW_HEIGHT    480
#define DEFAULT_PREVIEW_PADDING   CAM_PAD_TO_WORD
#define DEFAULT_VIDEO_FORMAT      CAM_FORMAT_YUV_420_NV21
#define DEFAULT_VIDEO_WIDTH       800
#define DEFAULT_VIDEO_HEIGHT      480
#define DEFAULT_VIDEO_PADDING     CAM_PAD_TO_2K
#define DEFAULT_SNAPSHOT_FORMAT   CAM_FORMAT_YUV_420_NV21
#define DEFAULT_SNAPSHOT_WIDTH    1280
#define DEFAULT_SNAPSHOT_HEIGHT   960
#define DEFAULT_SNAPSHOT_PADDING  CAM_PAD_TO_WORD
#define DEFAULT_FLIP_MODE         FLIP_MODE_NONE

#define MM_QCAMERA_APP_MAX_DISPLAY_WIDTH 2048

#define MM_QCAMERA_APP_UTEST_MAX_MAIN_LOOP 1
#define MM_QCAMERA_APP_UTEST_OUTER_LOOP 1
#define MM_QCAMERA_APP_UTEST_INNER_LOOP 1
#define MM_QCAM_APP_TEST_NUM 128
#define MM_QCAM_APP_TEST_PREVIEW_TIME  5 //in seconds

#define MM_QCAMERA_APP_SINGLE_MODE 0
#define MM_QCAMERA_APP_DUAL_MODE   1

#define MM_QCAMERA_APP_EXPOSURE_BRACKET_1 -12
#define MM_QCAMERA_APP_EXPOSURE_BRACKET_2 0
#define MM_QCAMERA_APP_EXPOSURE_BRACKET_3 12

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef enum {
    MM_CAMERA_OK,
    MM_CAMERA_E_GENERAL,
    MM_CAMERA_E_NO_MEMORY,
    MM_CAMERA_E_NOT_SUPPORTED,
    MM_CAMERA_E_INVALID_INPUT,
    MM_CAMERA_E_INVALID_OPERATION, /* 5 */
    MM_CAMERA_E_ENCODE,
    MM_CAMERA_E_BUFFER_REG,
    MM_CAMERA_E_PMEM_ALLOC,
    MM_CAMERA_E_CAPTURE_FAILED,
    MM_CAMERA_E_CAPTURE_TIMEOUT, /* 10 */
} mm_camera_status_type_t;

typedef enum {
    MM_CHANNEL_TYPE_ZSL,      /* preview, and snapshot main */
    MM_CHANNEL_TYPE_CAPTURE,  /* snapshot main, and postview */
    MM_CHANNEL_TYPE_PREVIEW,  /* preview only */
    MM_CHANNEL_TYPE_SNAPSHOT, /* snapshot main only */
    MM_CHANNEL_TYPE_VIDEO,    /* video only */
    MM_CHANNEL_TYPE_RDI,      /* rdi only */
    MM_CHANNEL_TYPE_MAX
} mm_camera_channel_type_t;

typedef struct {
    int                     fd;
    int                     main_ion_fd;
    struct ion_handle *     handle;
    uint32_t                size;
    void *                  data;
} mm_camera_app_meminfo_t;

typedef struct {
    mm_camera_buf_def_t buf;
    mm_camera_app_meminfo_t mem_info;
} mm_camera_app_buf_t;

typedef struct {
    uint32_t s_id;
    mm_camera_stream_config_t s_config;
    cam_frame_len_offset_t offset;
    uint8_t num_of_bufs;
    mm_camera_app_buf_t s_bufs[MM_CAMERA_MAX_NUM_FRAMES];
    mm_camera_app_buf_t s_info_buf;
} mm_camera_stream_t;

typedef struct {
    uint32_t ch_id;
    uint8_t num_streams;
    mm_camera_stream_t streams[MAX_STREAM_NUM_IN_BUNDLE];
} mm_camera_channel_t;

typedef struct {
    uint16_t user_input_display_width;
    uint16_t user_input_display_height;
} USER_INPUT_DISPLAY_T;

typedef enum {
  FLIP_MODE_NONE = 0,
  FLIP_MODE_H = FLIP_H,
  FLIP_MODE_V = FLIP_V,
  FLIP_MODE_V_H = FLIP_H | FLIP_V,
} flip_modes;

typedef struct {
  void *ptr;
  void* ptr_jpeg;

  uint8_t (*get_num_of_cameras) ();
  mm_camera_vtbl_t *(*mm_camera_open) (uint8_t camera_idx);
  uint32_t (*jpeg_open) (mm_jpeg_ops_t *ops);
} hal_interface_lib_t;

typedef struct {
    uint16_t num_cameras;
    int32_t preview_width;
    int32_t preview_height;
    cam_format_t preview_format;
    int32_t snapshot_width;
    int32_t snapshot_height;
    cam_format_t snapshot_format;
    int32_t video_width;
    int32_t video_height;
    cam_format_t video_format;
    uint32_t test_mode;
    uint32_t test_idx;
    hal_interface_lib_t hal_lib;
    uint32_t num_rcvd_snapshot;
    uint32_t num_snapshot;
    uint8_t flip_mode;
    bool no_display;
} mm_camera_app_t;

typedef struct {
    mm_camera_super_buf_t job_frame;
    uint32_t job_id;

} mm_camera_app_frame_stack;

typedef struct {
    mm_camera_vtbl_t *cam;
    uint8_t num_channels;
    mm_camera_channel_t channels[MM_CHANNEL_TYPE_MAX];
    mm_jpeg_ops_t jpeg_ops;
    uint32_t jpeg_hdl;
    mm_camera_app_buf_t cap_buf;
    mm_camera_app_buf_t parm_buf;

    uint32_t current_jpeg_sess_id;
    mm_camera_app_buf_t jpeg_buf;
    parm_buffer_t *params_buffer;
    mm_camera_app_t *app_handle;
    mm_camera_app_frame_stack jpeg_frame_queue[CAMERA_MAX_JPEG_SESSIONS];
    uint8_t cam_id;
} mm_camera_test_obj_t;


typedef int (*mm_app_test_t) (mm_camera_app_t *cam_apps);
typedef struct {
    mm_app_test_t f;
    int r;
} mm_app_tc_t;

extern int mm_app_unit_test_entry(mm_camera_app_t *cam_app);
extern int mm_app_dual_test_entry(mm_camera_app_t *cam_app);
extern void mm_app_dump_frame(mm_camera_buf_def_t *frame,
                              char *name,
                              char *ext,
                              int frame_idx);
extern void mm_app_dump_jpeg_frame(const void * data,
                                   uint32_t size,
                                   char* name,
                                   char* ext,
                                   int index);
extern int mm_camera_app_timedwait(uint8_t seconds);
extern int mm_camera_app_wait();
extern void mm_camera_app_done();
extern int mm_app_alloc_bufs(mm_camera_app_buf_t* app_bufs,
                             cam_frame_len_offset_t *frame_offset_info,
                             uint8_t num_bufs,
                             uint8_t is_streambuf);
extern int mm_app_release_bufs(uint8_t num_bufs,
                               mm_camera_app_buf_t* app_bufs);
extern int mm_app_stream_initbuf(cam_frame_len_offset_t *frame_offset_info,
                                 uint8_t *num_bufs,
                                 uint8_t **initial_reg_flag,
                                 mm_camera_buf_def_t **bufs,
                                 mm_camera_map_unmap_ops_tbl_t *ops_tbl,
                                 void *user_data);
extern int32_t mm_app_stream_deinitbuf(mm_camera_map_unmap_ops_tbl_t *ops_tbl,
                                       void *user_data);
extern int mm_app_cache_ops(mm_camera_app_meminfo_t *mem_info,
                            unsigned int cmd);
extern int32_t mm_app_stream_clean_invalidate_buf(int index, void *user_data);
extern int32_t mm_app_stream_invalidate_buf(int index, void *user_data);
extern int mm_app_open(mm_camera_app_t *cam_app,
                       uint8_t cam_id,
                       mm_camera_test_obj_t *test_obj);
extern int mm_app_close(mm_camera_test_obj_t *test_obj);
extern mm_camera_channel_t * mm_app_add_channel(
                                         mm_camera_test_obj_t *test_obj,
                                         mm_camera_channel_type_t ch_type,
                                         mm_camera_channel_attr_t *attr,
                                         mm_camera_buf_notify_t channel_cb,
                                         void *userdata);
extern int mm_app_del_channel(mm_camera_test_obj_t *test_obj,
                              mm_camera_channel_t *channel);
extern mm_camera_stream_t * mm_app_add_stream(mm_camera_test_obj_t *test_obj,
                                              mm_camera_channel_t *channel);
extern int mm_app_del_stream(mm_camera_test_obj_t *test_obj,
                             mm_camera_channel_t *channel,
                             mm_camera_stream_t *stream);
extern int mm_app_config_stream(mm_camera_test_obj_t *test_obj,
                                mm_camera_channel_t *channel,
                                mm_camera_stream_t *stream,
                                mm_camera_stream_config_t *config);
extern int mm_app_start_channel(mm_camera_test_obj_t *test_obj,
                                mm_camera_channel_t *channel);
extern int mm_app_stop_channel(mm_camera_test_obj_t *test_obj,
                               mm_camera_channel_t *channel);
extern mm_camera_channel_t *mm_app_get_channel_by_type(
                                    mm_camera_test_obj_t *test_obj,
                                    mm_camera_channel_type_t ch_type);

extern int mm_app_start_preview(mm_camera_test_obj_t *test_obj);
extern int mm_app_stop_preview(mm_camera_test_obj_t *test_obj);
extern int mm_app_start_preview_zsl(mm_camera_test_obj_t *test_obj);
extern int mm_app_stop_preview_zsl(mm_camera_test_obj_t *test_obj);
extern mm_camera_channel_t * mm_app_add_preview_channel(
                                mm_camera_test_obj_t *test_obj);
extern int mm_app_stop_and_del_channel(mm_camera_test_obj_t *test_obj,
                                       mm_camera_channel_t *channel);
extern mm_camera_channel_t * mm_app_add_snapshot_channel(
                                               mm_camera_test_obj_t *test_obj);
extern mm_camera_stream_t * mm_app_add_snapshot_stream(
                                                mm_camera_test_obj_t *test_obj,
                                                mm_camera_channel_t *channel,
                                                mm_camera_buf_notify_t stream_cb,
                                                void *userdata,
                                                uint8_t num_bufs,
                                                uint8_t num_burst);
extern int mm_app_start_record_preview(mm_camera_test_obj_t *test_obj);
extern int mm_app_stop_record_preview(mm_camera_test_obj_t *test_obj);
extern int mm_app_start_record(mm_camera_test_obj_t *test_obj);
extern int mm_app_stop_record(mm_camera_test_obj_t *test_obj);
extern int mm_app_start_live_snapshot(mm_camera_test_obj_t *test_obj);
extern int mm_app_stop_live_snapshot(mm_camera_test_obj_t *test_obj);
extern int mm_app_start_capture(mm_camera_test_obj_t *test_obj,
                                uint8_t num_snapshots);
extern int mm_app_stop_capture(mm_camera_test_obj_t *test_obj);
extern int mm_app_start_rdi(mm_camera_test_obj_t *test_obj, uint8_t num_burst);
extern int mm_app_stop_rdi(mm_camera_test_obj_t *test_obj);
extern int mm_app_take_picture(mm_camera_test_obj_t *test_obj, uint8_t);

#endif /* __MM_QCAMERA_APP_H__ */









