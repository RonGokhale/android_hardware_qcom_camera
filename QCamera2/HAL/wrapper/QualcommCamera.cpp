/* Copyright (c) 2011-2013, The Linux Foundation. All rights reserved.
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

#define LOG_TAG "QCameraWrapper"

#include <fcntl.h>
#include <sys/mman.h>
#include <utils/RefBase.h>

#include "QualcommCamera.h"
#include "QCamera2Factory.h"
#include "QCamera2HWI.h"

extern "C" {
#include <sys/time.h>
}


#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__ ((dllexport))
    #else
      #define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #else
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__ ((dllimport))
    #else
      #define DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
  #define DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define DLL_PUBLIC __attribute__ ((visibility ("default")))
    #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define DLL_PUBLIC
    #define DLL_LOCAL
  #endif
#endif


using namespace qcamera;
namespace android
{

QCamera2HardwareInterface *util_get_Hal_obj( struct camera_device *device)
{
    QCamera2HardwareInterface *qcam2HWI = NULL;
    if (device && device->priv) {
        qcam2HWI = (QCamera2HardwareInterface *)device->priv;
    }
    return qcam2HWI;
}

extern "C" DLL_PUBLIC int get_number_of_cameras()
{
    /* try to query every time we get the call!*/

    ALOGE("Q%s: E", __func__);
    return QCamera2Factory::get_number_of_cameras();
}

extern "C" DLL_PUBLIC int get_camera_info(int camera_id, struct camera_info *info)
{
    int rc = -1;
    ALOGE("Q%s: E", __func__);

    if (info) {
        QCamera2Factory::get_camera_info(camera_id, info);
    }
    ALOGV("Q%s: X", __func__);
    return rc;
}


/* HAL should return NULL if it fails to open camera qcam2HWI. */
extern "C" DLL_PUBLIC int  camera_device_open(
    const struct hw_module_t *module, int camera_id,
    struct hw_device_t **hw_device)
{
    ALOGV("Enter");

    *hw_device = NULL;

    if (module && hw_device) {

        ALOGV("Opening camera: %d", camera_id);

        QCamera2HardwareInterface *qcam2HWI = new QCamera2HardwareInterface(camera_id);

        if (NULL != qcam2HWI) {
            qcam2HWI->openCamera(hw_device);
        }

        if(*hw_device == NULL) {
            ALOGE("Camera open failed for camera: %d", camera_id);
            return -1;
        }

    }

    ALOGV("Exit");
    return 0;
}

extern "C" DLL_PUBLIC int release_resources(struct camera_device *device)
{
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);

    if (qcam2HWI != NULL) {
        qcam2HWI->release(device);
    }

    return 0;
}


extern "C" DLL_PUBLIC int close_camera_device(struct camera_device *device)
{
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);

    if (qcam2HWI != NULL) {
        qcam2HWI->close_camera_device(reinterpret_cast<hw_device_t *>(device));
    }

    return 0;
}

extern "C" DLL_PUBLIC void set_CallBacks(struct camera_device *device,
                   camera_notify_callback notify_cb,
                   camera_data_callback data_cb,
                   camera_data_timestamp_callback data_cb_timestamp,
                   camera_request_memory get_memory,
                   void *user)
{
    ALOGV("Q%s: E", __func__);
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        qcam2HWI->set_CallBacks(device, notify_cb, data_cb, data_cb_timestamp, get_memory, user);
    }
}

extern "C" DLL_PUBLIC void enable_msg_type(struct camera_device *device, int32_t msg_type)
{
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        qcam2HWI->enable_msg_type(device, msg_type);
    }
}

extern "C" DLL_PUBLIC void disable_msg_type(struct camera_device *device, int32_t msg_type)
{
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    ALOGE("Q%s: E", __func__);
    if (qcam2HWI != NULL) {
        qcam2HWI->disable_msg_type(device, msg_type);
    }
}

extern "C" DLL_PUBLIC int msg_type_enabled(struct camera_device *device, int32_t msg_type)
{
    ALOGE("Q%s: E", __func__);
    int rc = -1;
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        rc = qcam2HWI->msg_type_enabled(device, msg_type);
    }
    return rc;
}

extern "C" DLL_PUBLIC int start_preview(struct camera_device *device)
{
    ALOGE("Q%s: E", __func__);
    int rc = -1;
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        rc = qcam2HWI->start_preview(device);
    }
    ALOGV("Exit");
    return rc;
}

extern "C" DLL_PUBLIC void stop_preview(struct camera_device *device)
{
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        qcam2HWI->stop_preview(device);

        //also release preview resources
        qcam2HWI->release(device);
    }
    ALOGV("Exit");
}

extern "C" DLL_PUBLIC int preview_enabled(struct camera_device *device)
{
    ALOGE("Q%s: E", __func__);
    int rc = -1;
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        rc = qcam2HWI->preview_enabled(device);
    }
    return rc;
}

extern "C" DLL_PUBLIC int store_meta_data_in_buffers(struct camera_device *device, int enable)
{
    ALOGE("Q%s: E", __func__);
    int rc = -1;
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        rc = qcam2HWI->store_meta_data_in_buffers(device, enable);
    }
    return rc;
}

extern "C" DLL_PUBLIC int start_recording(struct camera_device *device)
{
    ALOGE("Q%s: E", __func__);
    int rc = -1;
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        rc = qcam2HWI->start_recording(device);
    }
    return rc;
}

extern "C" DLL_PUBLIC void stop_recording(struct camera_device *device)
{
    ALOGE("Q%s: E", __func__);
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        qcam2HWI->stop_recording(device);
    }
}

extern "C" DLL_PUBLIC int recording_enabled(struct camera_device *device)
{
    ALOGE("Q%s: E", __func__);
    int rc = -1;
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        rc = qcam2HWI->recording_enabled(device);
    }
    return rc;
}

extern "C" DLL_PUBLIC void release_recording_frame(struct camera_device *device,
                             const void *opaque)
{
    ALOGV("Q%s: E", __func__);
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        qcam2HWI->release_recording_frame(device, opaque);
    }
}

extern "C" DLL_PUBLIC int auto_focus(struct camera_device *device)
{
    ALOGE("Q%s: E", __func__);
    int rc = -1;
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        rc = qcam2HWI->auto_focus(device);
    }
    return rc;
}

extern "C" DLL_PUBLIC int cancel_auto_focus(struct camera_device *device)
{
    ALOGE("Q%s: E", __func__);
    int rc = -1;
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        rc = qcam2HWI->cancel_auto_focus(device);
    }
    return rc;
}

extern "C" DLL_PUBLIC int take_picture(struct camera_device *device)
{
    ALOGE("Q%s: E", __func__);
    int rc = -1;
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        rc = qcam2HWI->take_picture(device);
    }
    return rc;
}

extern "C" DLL_PUBLIC int cancel_picture(struct camera_device *device)

{
    ALOGE("Q%s: E", __func__);
    int rc = -1;
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        rc = qcam2HWI->cancel_picture(device);
    }
    return rc;
}

extern "C" DLL_PUBLIC int set_parameters(struct camera_device *device, const char *parms)

{
    ALOGE("Q%s: E", __func__);
    int rc = -1;
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL && parms) {
        rc = qcam2HWI->set_parameters(device, parms);
    }
    return rc;
}

extern "C" DLL_PUBLIC char *get_parameters(struct camera_device *device)
{
    ALOGE("Q%s: E", __func__);
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        char *parms = NULL;
        parms = qcam2HWI->get_parameters(device);
        return parms;
    }
    return NULL;
}

extern "C" DLL_PUBLIC void put_parameters(struct camera_device *device, char *parm)

{
    ALOGE("Q%s: E", __func__);
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        qcam2HWI->put_parameters(device, parm);
    }
}

extern "C" DLL_PUBLIC int send_command(struct camera_device *device,
                 int32_t cmd, int32_t arg1, int32_t arg2)
{
    ALOGE("Q%s: E", __func__);
    int rc = -1;
    QCamera2HardwareInterface *qcam2HWI = util_get_Hal_obj(device);
    if (qcam2HWI != NULL) {
        rc = qcam2HWI->send_command(device, cmd, arg1, arg2);
    }
    return rc;
}

}; // namespace android
