/* Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
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
 */

#ifndef ANDROID_HARDWARE_QUALCOMM_CAMERA_USBCAM_H
#define ANDROID_HARDWARE_QUALCOMM_CAMERA_USBCAM_H


#include "QCameraHWI.h"

extern "C" {
/*#include <hardware/camera.h>*/

  int usbcam_get_number_of_cameras();
  int usbcam_get_camera_info(int camera_id, struct camera_info *info);

  int usbcam_camera_device_open(const struct hw_module_t* module, const char* id,
          struct hw_device_t** device);

  hw_device_t * usbcam_open_camera_device(int cameraId);

  int usbcam_close_camera_device( hw_device_t *);

namespace android {
  int usbcam_set_preview_window(struct camera_device *,
          struct preview_stream_ops *window);
  void usbcam_set_CallBacks(struct camera_device *,
          camera_notify_callback notify_cb,
          camera_data_callback data_cb,
          camera_data_timestamp_callback data_cb_timestamp,
          camera_request_memory get_memory,
          void *user);

  void usbcam_enable_msg_type(struct camera_device *, int32_t msg_type);

  void usbcam_disable_msg_type(struct camera_device *, int32_t msg_type);
  int usbcam_msg_type_enabled(struct camera_device *, int32_t msg_type);

  int usbcam_start_preview(struct camera_device *);

  void usbcam_stop_preview(struct camera_device *);

  int usbcam_preview_enabled(struct camera_device *);
  int usbcam_store_meta_data_in_buffers(struct camera_device *, int enable);

  int usbcam_start_recording(struct camera_device *);

  void usbcam_stop_recording(struct camera_device *);

  int usbcam_recording_enabled(struct camera_device *);

  void usbcam_release_recording_frame(struct camera_device *,
                  const void *opaque);

  int usbcam_auto_focus(struct camera_device *);

  int usbcam_cancel_auto_focus(struct camera_device *);

  int usbcam_take_picture(struct camera_device *);

  int usbcam_cancel_picture(struct camera_device *);

  int usbcam_set_parameters(struct camera_device *, const char *parms);

  char* usbcam_get_parameters(struct camera_device *);

  void usbcam_put_parameters(struct camera_device *, char *);

  int usbcam_send_command(struct camera_device *,
              int32_t cmd, int32_t arg1, int32_t arg2);

  void usbcam_release(struct camera_device *);

  int usbcam_dump(struct camera_device *, int fd);

#define PRVW_WD     640
#define PRVW_HT     480

}; // namespace android

} //extern "C"

#endif

