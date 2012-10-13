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
 /*#error uncomment this for compiler test!*/
#define ALOG_NDEBUG 1
#define ALOG_NIDEBUG 1
#define LOG_TAG "QualcommUsbCamera"
#include <utils/Log.h>
#include <utils/threads.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <pthread.h>

#include "QCameraHAL.h"
#include "QualcommUsbCamera.h"
#include <gralloc_priv.h>
#include <genlock.h>

extern "C" {
#include <sys/time.h>
}

/* HAL function implementation goes here*/

/**
 * The functions need to be provided by the camera HAL.
 *
 * If getNumberOfCameras() returns N, the valid cameraId for getCameraInfo()
 * and openCameraHardware() is 0 to N-1.
 */

camera_device_ops_t usbcam_camera_ops = {
  set_preview_window:         android::usbcam_set_preview_window,
  set_callbacks:              android::usbcam_set_CallBacks,
  enable_msg_type:            android::usbcam_enable_msg_type,
  disable_msg_type:           android::usbcam_disable_msg_type,
  msg_type_enabled:           android::usbcam_msg_type_enabled,

  start_preview:              android::usbcam_start_preview,
  stop_preview:               android::usbcam_stop_preview,
  preview_enabled:            android::usbcam_preview_enabled,
  store_meta_data_in_buffers: android::usbcam_store_meta_data_in_buffers,

  start_recording:            android::usbcam_start_recording,
  stop_recording:             android::usbcam_stop_recording,
  recording_enabled:          android::usbcam_recording_enabled,
  release_recording_frame:    android::usbcam_release_recording_frame,

  auto_focus:                 android::usbcam_auto_focus,
  cancel_auto_focus:          android::usbcam_cancel_auto_focus,

  take_picture:               android::usbcam_take_picture,
  cancel_picture:             android::usbcam_cancel_picture,

  set_parameters:             android::usbcam_set_parameters,
  get_parameters:             android::usbcam_get_parameters,
  put_parameters:             android::usbcam_put_parameters,
  send_command:               android::usbcam_send_command,

  release:                    android::usbcam_release,
  dump:                       android::usbcam_dump,
};

#define DISPLAY 1
#define MEMSET  0
#define CAPTURE 1
#define FILE_DUMP_CAMERA 0
#define FILE_DUMP_B4_DISP 0
#define CALL_BACK   1

/* TBR: Number of display buffers. On what basis should this number be chosen */
#define PRVW_DISP_BUF_CNT   5

/* TBR: Number of V4L2 capture  buffers. On what basis should this number be */
/* chosen */
#define PRVW_CAP_BUF_CNT    4

struct bufObj {
    void    *data;
    int     len;
};

namespace android {

typedef struct {
    camera_device                       hw_dev;
    Mutex                               lock;
    int                                 previewEnabledFlag;
    int                                 msgEnabledFlag;
    pthread_t                           previewThread;

    camera_notify_callback              notify_cb;
    camera_data_callback                data_cb;
    camera_data_timestamp_callback      data_cb_timestamp;
    camera_request_memory               get_memory;
    void*                               cb_ctxt;

    /* capture related members */
    int                                 prevWidth;
    int                                 prevHeight;
    int                                 fd;
    unsigned int                        n_buffers;
    struct v4l2_buffer                  curCaptureBuf;
    struct bufObj                *buffers;

    /* Display related members */
    preview_stream_ops*                 window;
    QCameraHalMemory_t                  previewMem;
} camera_hardware_t;

typedef struct {
  camera_memory_t mem;
  int32_t msgType;
  sp<IMemory> dataPtr;
  void* user;
  unsigned int index;
} q_cam_memory_t;

static int initUsbCamera(camera_hardware_t *camHal, int width, int height);
static int startUsbCamCapture(camera_hardware_t *camHal);
static int launch_preview_thread(       camera_hardware_t *camHal);
static int initDisplayBuffers(          camera_hardware_t *camHal);
static int deInitDisplayBuffers(          camera_hardware_t *camHal);
static int get_buf_from_display( camera_hardware_t *camHal, int *buffer_id);
static int put_buf_to_display(   camera_hardware_t *camHal, int buffer_id);
static int copy_data_frm_cam_to_disp(camera_hardware_t *camHal, int buffer_id);
static void * previewloop(void *);

static int covert_YUYV_to_420_NV12(char *in_buf, char *out_buf, int wd, int ht);

/******************************************************************************/
/* No in place conversion supported. Output buffer and input MUST should be   */
/* different input buffer for a 4x4 pixel video                             ***/
/******                  YUYVYUYV          00 01 02 03 04 05 06 07 ************/
/******                  YUYVYUYV          08 09 10 11 12 13 14 15 ************/
/******                  YUYVYUYV          16 17 18 19 20 21 22 23 ************/
/******                  YUYVYUYV          24 25 26 27 28 29 30 31 ************/
/******************************************************************************/
/* output generated by this function ******************************************/
/************************** YYYY            00 02 04 06            ************/
/************************** YYYY            08 10 12 14            ************/
/************************** YYYY            16 18 20 22            ************/
/************************** YYYY            24 26 28 30            ************/
/************************** VUVU            03 01 07 05            ************/
/************************** VUVU            19 17 23 21            ************/
/******************************************************************************/

static int covert_YUYV_to_420_NV12(char *in_buf, char *out_buf, int wd, int ht)
{
    int rc =0;

    int row, col, uv_row;
    printf("inside conversion function \n");

    /* Arrange Y */
    for(row = 0; row < ht; row++)
        for(col = 0; col < wd * 2; col += 2)
        {
            out_buf[row * wd + col / 2] = in_buf[row * wd * 2 + col];
        }

    /* Arrange UV */
    for(row = 0, uv_row = ht; row < ht; row += 2, uv_row++)
        for(col = 1; col < wd * 2; col += 4)
        {
            out_buf[uv_row * wd + col / 2]= in_buf[row * wd * 2 + col + 2];
            out_buf[uv_row * wd + col / 2 + 1]  = in_buf[row * wd * 2 + col];
        }

    return rc;
}

static int initDisplayBuffers(camera_hardware_t *camHal)
{
  preview_stream_ops    *mPreviewWindow;
  int                   numMinUndequeuedBufs = 0;
  int                   rc = 0;
  int                   gralloc_usage = 0;
  int                   err;
  int                   color=30;

  ALOGE("%s: E", __func__);

#if DISPLAY
  if(camHal == NULL) {
      ALOGE("%s: camHal = NULL", __func__);
      return -1;
  }

  mPreviewWindow = camHal->window;
  if(!mPreviewWindow) {
      ALOGE("%s: mPreviewWindow = NULL", __func__);
      return -1;
  }

  ALOGE("%s: mPreviewWindow->get_min_undequeued_buffer_count: %p",
       __func__, mPreviewWindow->get_min_undequeued_buffer_count);

  if(mPreviewWindow->get_min_undequeued_buffer_count) {
      rc = mPreviewWindow->get_min_undequeued_buffer_count(
          mPreviewWindow, &numMinUndequeuedBufs);
      if (0 != rc) {
          ALOGE("%s: get_min_undequeued_buffer_count returned error", __func__);
      }
      else
          ALOGE("%s: get_min_undequeued_buffer_count returned: %d ",
               __func__, numMinUndequeuedBufs);
  }
  else
      ALOGE("%s: get_min_undequeued_buffer_count is NULL pointer", __func__);

  if(mPreviewWindow->set_buffer_count) {
      camHal->previewMem.buffer_count = PRVW_DISP_BUF_CNT;
      rc = mPreviewWindow->set_buffer_count(
          mPreviewWindow,
          camHal->previewMem.buffer_count + numMinUndequeuedBufs);
      if (rc != 0) {
          ALOGE("%s: set_buffer_count returned error", __func__);
      }
      else
          ALOGE("%s: set_buffer_count returned success", __func__);
  }
  else
      ALOGE("%s: set_buffer_count is NULL pointer", __func__);

  if(mPreviewWindow->set_buffers_geometry) {
      /* TBD: Preview color format and conversion */
      rc = mPreviewWindow->set_buffers_geometry(mPreviewWindow,
                                                PRVW_WD, PRVW_HT,
                                                HAL_PIXEL_FORMAT_YCrCb_420_SP);
      if (rc != 0) {
          ALOGE("%s: set_buffers_geometry returned error. %s (%d)",
               __func__, strerror(-rc), -rc);
      }
      else
          ALOGE("%s: set_buffers_geometry returned success", __func__);
  }
  else
      ALOGE("%s: set_buffers_geometry is NULL pointer", __func__);

    gralloc_usage = CAMERA_GRALLOC_HEAP_ID | CAMERA_GRALLOC_FALLBACK_HEAP_ID |
                    CAMERA_GRALLOC_CACHING_ID;

    if(mPreviewWindow->set_usage) {
        rc = mPreviewWindow->set_usage(mPreviewWindow, gralloc_usage);
        if (rc != 0) {
            ALOGE("%s: set_usage returned error", __func__);
        }
        else
            ALOGE("%s: set_usage returned success", __func__);
    }
    else
        ALOGE("%s: set_usage is NULL pointer", __func__);

    /* Loop here for DQ and En-Q*/
   {
        for (int cnt = 0;
             cnt < camHal->previewMem.buffer_count + numMinUndequeuedBufs;
             cnt++) {
            int stride;
            err = mPreviewWindow->dequeue_buffer(
                mPreviewWindow,
                &camHal->previewMem.buffer_handle[cnt],
                &camHal->previewMem.stride[cnt]);
            if(!err) {
                ALOGE("%s: dequeue buf: %p\n",
                     __func__, camHal->previewMem.buffer_handle[cnt]);
                    {
                        ALOGE("%s: mPreviewWindow->lock_buffer: %p",
                             __func__, mPreviewWindow->lock_buffer);
                        if(mPreviewWindow->lock_buffer) {
                            err = mPreviewWindow->lock_buffer(
                                mPreviewWindow,
                                camHal->previewMem.buffer_handle[cnt]);
                            ALOGE("%s: mPreviewWindow->lock_buffer success",
                                 __func__);
                        }
                        // lock the buffer using genlock
                        ALOGE("%s: camera call genlock_lock, hdl=%p",
                            __func__, (*camHal->previewMem.buffer_handle[cnt]));

                        if (GENLOCK_NO_ERROR !=
                            genlock_lock_buffer(
                                (native_handle_t *)
                                (*camHal->previewMem.buffer_handle[cnt]),
                                GENLOCK_WRITE_LOCK, GENLOCK_MAX_TIMEOUT)) {
                           ALOGE("%s: genlock_lock_buffer(WRITE) failed",
                                __func__);
                           camHal->previewMem.local_flag[cnt] = BUFFER_UNLOCKED;
                            //mHalCamCtrl->mPreviewMemoryLock.unlock();
                           //return -EINVAL;
                       } else {
                         ALOGE("%s: genlock_lock_buffer hdl =%p",
                              __func__, *camHal->previewMem.buffer_handle[cnt]);
                       }
                    }
                camHal->previewMem.private_buffer_handle[cnt] =
                    (struct private_handle_t *)
                    (*camHal->previewMem.buffer_handle[cnt]);

                ALOGE("%s: idx = %d, fd = %d, size = %d, offset = %d", __func__,
                    cnt, camHal->previewMem.private_buffer_handle[cnt]->fd,
                camHal->previewMem.private_buffer_handle[cnt]->size,
                camHal->previewMem.private_buffer_handle[cnt]->offset);

                camHal->previewMem.camera_memory[cnt] =
                    camHal->get_memory(
                        camHal->previewMem.private_buffer_handle[cnt]->fd,
                        camHal->previewMem.private_buffer_handle[cnt]->size,
                        1, camHal->cb_ctxt);

                ALOGE("%s: data = %p, size = %d, handle = %p", __func__,
                    camHal->previewMem.camera_memory[cnt]->data,
                    camHal->previewMem.camera_memory[cnt]->size,
                    camHal->previewMem.camera_memory[cnt]->handle);
            }
            else
                ALOGE("%s: dequeue buf %d failed \n", __func__, cnt);
        }
   }
   {
       for (int cnt = 0;
            cnt < camHal->previewMem.buffer_count + numMinUndequeuedBufs;
            cnt++) {
        if (GENLOCK_FAILURE == genlock_unlock_buffer(
                (native_handle_t *)(*(camHal->previewMem.buffer_handle[cnt])))){
           ALOGE("%s: genlock_unlock_buffer failed: hdl =%p", __func__,
                (*(camHal->previewMem.buffer_handle[cnt])) );
            //mHalCamCtrl->mPreviewMemoryLock.unlock();
           //return -EINVAL;
        } else {
          camHal->previewMem.local_flag[cnt] = BUFFER_UNLOCKED;
          ALOGE("%s: genlock_unlock_buffer success: hdl = %p",
               __func__, (*(camHal->previewMem.buffer_handle[cnt])));
        }

        err = mPreviewWindow->cancel_buffer(mPreviewWindow,
          (buffer_handle_t *)camHal->previewMem.buffer_handle[cnt]);
        if(!err) {
            ALOGE("%s: cancel_buffer successful: %p\n",
                 __func__, camHal->previewMem.buffer_handle[cnt]);
        }else
            ALOGE("%s: cancel_buffer failed: %p\n", __func__,
                 camHal->previewMem.buffer_handle[cnt]);
       }
   }
    ALOGE("%s: X", __func__);
#else
    rc = 0;
#endif /* #if DISPLAY */
    return rc;
}

/**
* This function de-initializes all the display buffers allocated
* in initDisplayBuffers
*/
static int deInitDisplayBuffers(          camera_hardware_t *camHal)
{
    int rc = 0;

#if DISPLAY
  if(camHal == NULL) {
      ALOGE("%s: camHal = NULL", __func__);
      return -1;
  }

#else
    rc = 0;
#endif /* #if DISPLAY */

    return rc;
}

extern "C" int usbcam_get_number_of_cameras()
{
    /* TBR: This is hardcoded currently to 1 USB camera */
    int numCameras = 1;
    /* try to query every time we get the call!*/
    ALOGE("%s: E and X", __func__);

    return numCameras;
}


extern "C" int usbcam_get_camera_info(int camera_id, struct camera_info *info)
{
    int rc = -1;
    ALOGE("%s: E", __func__);

    /* TBR: This info is hardcoded currently irrespective of camera_id */
    if(info) {
        struct CameraInfo camInfo;
        memset(&camInfo, -1, sizeof (struct CameraInfo));

        info->facing = CAMERA_FACING_FRONT;//CAMERA_FACING_BACK;
        info->orientation = 0;
        rc = 0;
    }
    ALOGE("%s: X", __func__);
    return rc;
}

/* HAL should return NULL if it fails to open camera hardware. */
extern "C" int  usbcam_camera_device_open(
  const struct hw_module_t* module, const char* id,
          struct hw_device_t** hw_device)
{
    int rc = -1;
    camera_device *device = NULL;
    camera_hardware_t *camHal;
    /* TBD: detect the video node corresponding to USB. Currently hardcoded */
    char            *dev_name;


    ALOGE("%s: E", __func__);

    camHal = (camera_hardware_t *) malloc(sizeof (camera_hardware_t));
    if(!camHal) {
        *hw_device = NULL;
            ALOGE("%s:  end in no mem", __func__);
            return rc;
    }
    memset(camHal, 0, sizeof (camera_hardware_t));

#if CAPTURE

    /* Check if the USB camera device is available and is in character mode */
    {
        struct          stat st;

        /* Free dev_name memory before every return in this function */
        dev_name = (char *) malloc(128);
        strncpy(dev_name, "/dev/video1", 128);

        if (-1 == stat(dev_name, &st)) {
            ALOGE("%s: Cannot identify '%s': %d, %s\n",
                 __func__, dev_name, errno, strerror(errno));
        }

        if (!S_ISCHR(st.st_mode)) {
            ALOGE("%s: %s is no device\n", __func__, dev_name);
            rc = -1;
        }
    }

    camHal->fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (camHal->fd <  0) {
        ALOGE("%s: Cannot open '%s'", __func__, dev_name);
        free(camHal);
        rc = -1;
    }else{
        rc = 0;
    }
#else /* CAPTURE */
    rc = 0;
#endif /* CAPTURE */
    /* Default initializations */
    /* TBD: These values should get updated when set properties are recvd */
    camHal->prevWidth = PRVW_WD;
    camHal->prevHeight = PRVW_HT;
    camHal->previewEnabledFlag = 0;

    /* TBD: Create the lock here */
    /* TBD: REMOVE THIS GLOBAL */
    //fd = camHal->fd;

    device                  = &camHal->hw_dev;
    device->common.close    = usbcam_close_camera_device;
    device->ops             = &usbcam_camera_ops;
    device->priv            = (void *)camHal;
    *hw_device              = &(device->common);

    ALOGE("%s: camHal: %p", __func__, camHal);
    ALOGE("%s: X rc %d", __func__, rc);

    free(dev_name);
    return rc;
}

extern "C"  int usbcam_close_camera_device( hw_device_t *hw_dev)
{
    ALOGE("%s: device =%p E", __func__, hw_dev);
    int rc =  -1;
    camera_device_t *device     = (camera_device_t *)hw_dev;

    if(device) {
        camera_hardware_t *camHal   = (camera_hardware_t *)device->priv;
        if(camHal) {
            Mutex::Autolock autoLock(camHal->lock);
            rc = close(camHal->fd);
            if(rc < 0) {
                ALOGE("%s: close failed ", __func__);
            }
            camHal->fd = 0;
            /* TBR: What happens to the auto lock 'unlocking' if memory is freed.*/
            free(camHal);
        }else{
                ALOGE("%s: camHal is NULL pointer ", __func__);
        }
    }
    ALOGE("%s: device =%p, rc = %d X", __func__, hw_dev, rc);
    return rc;
}

int usbcam_set_preview_window(struct camera_device * device,
        struct preview_stream_ops *window)
{
    ALOGE("%s: E", __func__);
    int rc = 0;
    camera_hardware_t *camHal;

    if(device && device->priv){
        camHal = (camera_hardware_t *)device->priv;
    }else{
        ALOGE("%s: Null device or device->priv", __func__);
        return -1;
    }

    Mutex::Autolock autoLock(camHal->lock);

    if(window != NULL) {
        if(camHal->window){
            /* if window is already set, de-init */
            rc = deInitDisplayBuffers(camHal);
            if(rc < 0)
            {
                ALOGE("%s: deInitDisplayBuffers returned error", __func__);
            }
        }
        camHal->window = window;
        initDisplayBuffers(camHal);
    }

    ALOGE("%s: X", __func__);
    return rc;
}

void usbcam_set_CallBacks(struct camera_device * device,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void *user)
{
    ALOGE("%s: E", __func__);
    camera_hardware_t *camHal;

    if(device && device->priv){
        camHal = (camera_hardware_t *)device->priv;
    }else{
        ALOGE("%s: Null device or device->priv", __func__);
        return;
    }

    Mutex::Autolock autoLock(camHal->lock);

    camHal->notify_cb           = notify_cb;
    camHal->data_cb             = data_cb;
    camHal->data_cb_timestamp   = data_cb_timestamp;
    camHal->get_memory          = get_memory;
    camHal->cb_ctxt             = user;

    ALOGE("%s: X", __func__);
}

void usbcam_enable_msg_type(struct camera_device * device, int32_t msg_type)
{
    ALOGE("%s: E", __func__);

    camera_hardware_t *camHal;

    if(device && device->priv){
        camHal = (camera_hardware_t *)device->priv;
    }else{
        ALOGE("%s: Null device or device->priv", __func__);
        return;
    }

    Mutex::Autolock autoLock(camHal->lock);
    /* TBD?? */
    camHal->msgEnabledFlag |= msg_type;

    ALOGE("%s: X", __func__);
}

void usbcam_disable_msg_type(struct camera_device * device, int32_t msg_type)
{
    ALOGE("%s: E", __func__);

    camera_hardware_t *camHal;
    if(device && device->priv){
        camHal = (camera_hardware_t *)device->priv;
    }else{
        ALOGE("%s: Null device or device->priv", __func__);
        return;
    }

    Mutex::Autolock autoLock(camHal->lock);
    /* TBD?? */
    camHal->msgEnabledFlag &= ~msg_type;

    ALOGE("%s: X", __func__);
}

int usbcam_msg_type_enabled(struct camera_device * device, int32_t msg_type)
{
    ALOGE("%s: E", __func__);

    camera_hardware_t *camHal;
    if(device && device->priv){
        camHal = (camera_hardware_t *)device->priv;
    }else{
        ALOGE("%s: Null device or device->priv", __func__);
        return -1;
    }

    Mutex::Autolock autoLock(camHal->lock);
    /* TBD?? */
    ALOGE("%s: X", __func__);
    return (camHal->msgEnabledFlag & msg_type);
}

int usbcam_start_preview(struct camera_device * device)
{
    ALOGE("%s: E", __func__);

    int rc = -1;
    camera_hardware_t *camHal = NULL;

    if(device && device->priv){
        camHal = (camera_hardware_t *)device->priv;
    }else{
        ALOGE("%s: Null device or device->priv", __func__);
        return -1;
    }
    Mutex::Autolock autoLock(camHal->lock);

    ALOGE("%s: camHal: %p", __func__, camHal);
#if CAPTURE
    rc = initUsbCamera(camHal, camHal->prevWidth, camHal->prevHeight);
    if(rc < 0) {
        ALOGE("%s: Failed to intialize the device", __func__);
    }else{
        rc = startUsbCamCapture(camHal);
        if(rc < 0) {
            ALOGE("%s: Failed to startUsbCamCapture", __func__);
        }else{
            rc = launch_preview_thread(camHal);
            if(rc < 0) {
                ALOGE("%s: Failed to launch_preview_thread", __func__);
            }
        }
    }
#else /* CAPTURE */
    rc = launch_preview_thread(camHal);
    if(rc < 0) {
        ALOGE("%s: Failed to launch_preview_thread", __func__);
    }
#endif /* CAPTURE */
    camHal->previewEnabledFlag = 1;

    ALOGE("%s: X", __func__);
    return rc;
}

/* TBD */
void usbcam_stop_preview(struct camera_device * device)
{
    ALOGE("%s: E", __func__);

    int rc = 0;
    camera_hardware_t *camHal;

    if(device && device->priv){
        camHal = (camera_hardware_t *)device->priv;
    }else{
        ALOGE("%s: Null device or device->priv", __func__);
        return;
    }
    Mutex::Autolock autoLock(camHal->lock);

    /* TBD: Implement thread stop */
    camHal->previewEnabledFlag = 0;

    ALOGE("%s: X", __func__);
}

/* This function is equivalent to is_preview_enabled */
int usbcam_preview_enabled(struct camera_device * device)
{
    ALOGE("%s: E", __func__);
    camera_hardware_t *camHal;

    if(device && device->priv){
        camHal = (camera_hardware_t *)device->priv;
    }else{
        ALOGE("%s: Null device or device->priv", __func__);
        return -1;
    }
    Mutex::Autolock autoLock(camHal->lock);

    ALOGE("%s: X", __func__);
    return camHal->previewEnabledFlag;
}

/* TBD */
int usbcam_store_meta_data_in_buffers(struct camera_device * device, int enable)
{
    ALOGE("%s: E", __func__);
    int rc = 0;

    ALOGE("%s: X", __func__);
    return rc;
}

/* TBD */
int usbcam_start_recording(struct camera_device * device)
{
    int rc = 0;
    ALOGE("%s: E", __func__);

    ALOGE("%s: X", __func__);

    return rc;
}

/* TBD */
void usbcam_stop_recording(struct camera_device * device)
{
    ALOGE("%s: E", __func__);

    ALOGE("%s: X", __func__);
}

/* TBD */
int usbcam_recording_enabled(struct camera_device * device)
{
    int rc = 0;
    ALOGE("%s: E", __func__);

    ALOGE("%s: X", __func__);
    return rc;
}

/* TBD */
void usbcam_release_recording_frame(struct camera_device * device,
                const void *opaque)
{
    ALOGV("%s: E", __func__);

    ALOGE("%s: X", __func__);
}

/* TBD */
int usbcam_auto_focus(struct camera_device * device)
{
    ALOGE("%s: E", __func__);
    int rc = 0;

    ALOGE("%s: X", __func__);
    return rc;
}

/* TBD */
int usbcam_cancel_auto_focus(struct camera_device * device)
{
    int rc = 0;
    ALOGE("%s: E", __func__);

    ALOGE("%s: X", __func__);
    return rc;
}

/* TBD */
int usbcam_take_picture(struct camera_device * device)
{
    ALOGE("%s: E", __func__);
    int rc = 0;

    ALOGE("%s: X", __func__);
    return rc;
}

/* TBD */
int usbcam_cancel_picture(struct camera_device * device)

{
    ALOGE("%s: E", __func__);
    int rc = 0;

    ALOGE("%s: X", __func__);
    return rc;
}

/* TBD */
int usbcam_set_parameters(struct camera_device * device, const char *parms)

{
    ALOGE("%s: E", __func__);
    int rc = 0;
    char temp[1001] = {0};
    int n=0;

    while(1) {
        strncpy(temp,parms+n,1000);
        ALOGE("parms = %s", temp);
        if (strlen(temp) < 1000) break;
        n += 1000;
    }
    /* TBD: detect preview width and height */

    ALOGE("%s: X", __func__);
  return rc;
}

/* TBD */
char* usbcam_get_parameters(struct camera_device * device)
{
    char *parms;
    char temp[1001] = {0};
    int n=0;
    ALOGE("%s: E", __func__);
    /* TBD ??*/
    /* TBD: Free this memory in put_parameters */
    parms = (char *) malloc(4096);
    strncpy(parms,"picture-size-values=1600x1200;power-mode=Normal_Power;power-mode-supported=true;preferred-preview-size-for-video=1920x1088;preview-format=yuv420sp;preview-format-values=yuv420sp,yuv420sp-adreno,yuv420p,yuv420p,nv12;preview-fps-range=5000,121000;preview-fps-range-values=(5000,121000);preview-frame-rate=121;preview-frame-rate-mode=frame-rate-auto;preview-frame-rate-modes=frame-rate-auto,frame-rate-fixed;preview-frame-rate-values=5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121;preview-size=1920x1088;preview-size-values=640x480;redeye-reduction=disable;redeye-reduction-values=enable,disable;saturation=5;scene-detect=off;scene-detect,landscape,night,night-portrait,theatre,beach,snow,sunset,steadyphoto,fireworks,sports,party,candlelight,backlight,flowers,AR;selectable-zone-af=auto;selectable-zone-af-values=auto,spot-metering,center-weighted,frame-average;sharpness=10;skinToneEnhancement=0;skinToneEnhancement-values=enable,disable;strtextures=OFF;touch-af-aec=touch-off;touch-af-aec-values=touch-off,touch-on;touchAfAec-dx=100;touchAfAec-dy=100;vertical-view-angle=42.5;video-frame-format=yuv420sp;video-hfr=off;video-hfr-values=off,60,90,120;video-size=1920x1088;video-size-values=1920x1088,1280x720,800x480,720x480,640x480,480x320,352x288,320x240,176x144;video-snapshot-supported=true;video-zoom-support=true;whitebalance=incandescent;whitebalance-values=auto,incandescent,fluorescent,daylight,cloudy-daylight;zoom=0;zoom-ratios=100,102,104,107,109,112,114,117,120,123,125,128,131,135,138,141,144,148,151,155,158,162,166,170,174,178,182,186,190,195,200,204,209,214,219,224,229,235,240,246,251,257,263,270,276,282,289,296,303,310,317,324,332,340,348,356,364,ff;ae-bracket-hdr-values=Off,HDR,AE-Bracket;antibanding=off;antibanding-values=off,50hz,60hz,auto;auto-exposure=frame-average;auto-exposure-lock=false;auto-exposure-lock-supported=true;auto-exposure-values=frame-average,center-weighted,spot-metering;auto-whitebalance-lock=false;auto-whitebalance-lock-supported=true;camera-mode=0;camera-mode-values=0,1;capture-burst-captures-values=2;capture-burst-exposures=;capture-burst-exposures-values=-12,-11,-10,-9,-8,-7,-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6,7,8,9,10,11,12;capture-burst-interval=1;capture-burst-interval-max=10;capture-burst-interval-min=1;capture-burst-interval-supported=true;capture-burst-retroactive=0;capture-burst-retroactive-max=2;contrast=5;denoise=denoise-off;denoise-values=denoise-off,denoise-on;effect=none;effect-values=none,mono,negative,solarize,sepia,posterize,whiteboard,blackboard,aqua,emboss,sketch,neon;exposure-compensation=0;exposure-compensation-step=0.166667;face-detection=off;flash-mode=off;flash-mode-values=off,auto,on,torch;focal-length=4.6;focus-areas=(0, 0, 0, 0, 0);focus-distances=1.231085,2.162743,8.892049;focus-mode=auto;focus-mode-values=auto,infinity,normal,macro,continuous-picture,continuous-video;hfr-size-values=800x480,640x480;histogram=disable;histogram-values=enable,disable;horizontal-view-angle=54.8;iso=auto;iso-values=auto,ISO_HJR,ISO100,ISO200,ISO400,ISO800,ISO1600;jpeg-quality=85;jpeg-thumbnail-height=384;jpeg-thumbnail-quality=90;jpeg-thumbnail-size-values=512x288,480x288,432x288,512x384,352x288,0x0;jpeg-thumbnail-width=512;lensshade=enable;", 4096);

    while(1) {
        strncpy(temp,parms+n,1000);
        ALOGE("parms = %s", temp);
        if (strlen(temp) < 1000) break;
        n += 1000;
    }

    ALOGE("%s: X", __func__);
    return parms;
}

void usbcam_put_parameters(struct camera_device * device, char *parm)

{
    ALOGE("%s: E", __func__);
    //ALOGE("params = %s", parm);
    parm = NULL;
    ALOGE("%s: X", __func__);

}

int usbcam_send_command(struct camera_device * device,
            int32_t cmd, int32_t arg1, int32_t arg2)
{
    int rc = 0;
    ALOGE("%s: E", __func__);
    ALOGE("%d", cmd);

    ALOGE("%s: X", __func__);
    return rc;
}

void usbcam_release(struct camera_device * device)
{
    ALOGE("%s: E", __func__);

    ALOGE("%s: X", __func__);
}

/* TBD */
int usbcam_dump(struct camera_device * device, int fd)
{
    ALOGE("%s: E", __func__);
    int rc = 0;

    ALOGE("%s: X", __func__);
    return rc;
}

/*
This function is a blocking call around ioctl
*/
static int ioctlLoop(int fd, int ioctlCmd, void *args)
{
    int rc = -1;

    while(1)
    {
        rc = ioctl(fd, ioctlCmd, args);
        if(!((-1 == rc) && (EINTR == errno)))
            break;
    }
    return rc;
}

/*
This function requests for V4L2 driver allocated buffers
*/
static int initV4L2mmap(camera_hardware_t *camHal)
{
    int rc = -1;
    struct v4l2_requestbuffers  reqBufs;
    struct v4l2_buffer          tempBuf;

    ALOGE("%s: E", __func__);
    memset(&reqBufs, 0, sizeof(v4l2_requestbuffers));
    reqBufs.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqBufs.memory  = V4L2_MEMORY_MMAP;
    reqBufs.count   = PRVW_CAP_BUF_CNT;

    if (-1 == ioctlLoop(camHal->fd, VIDIOC_REQBUFS, &reqBufs)) {
		if (EINVAL == errno) {
			ALOGE("%s: does not support memory mapping\n", __func__);
		} else {
			ALOGE("%s: VIDIOC_REQBUFS failed", __func__);
		}
	}
    ALOGE("%s: VIDIOC_REQBUFS success", __func__);

    if (reqBufs.count < PRVW_CAP_BUF_CNT) {
		ALOGE("%s: Insufficient buffer memory on\n", __func__);
	}

	camHal->buffers =
	    ( bufObj* ) calloc(reqBufs.count, sizeof(bufObj));

	if (!camHal->buffers) {
        ALOGE("%s: Out of memory\n", __func__);
	}

	for (camHal->n_buffers = 0;
	     camHal->n_buffers < reqBufs.count;
	     camHal->n_buffers++) {

        memset(&tempBuf, 0, sizeof(tempBuf));

        tempBuf.index       = camHal->n_buffers;
		tempBuf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		tempBuf.memory      = V4L2_MEMORY_MMAP;

		if (-1 == ioctlLoop(camHal->fd, VIDIOC_QUERYBUF, &tempBuf))
			ALOGE("%s: VIDIOC_QUERYBUF failed", __func__);

        ALOGE("%s: VIDIOC_QUERYBUF success", __func__);

		camHal->buffers[camHal->n_buffers].len = tempBuf.length;
		camHal->buffers[camHal->n_buffers].data =
			mmap(NULL /* start anywhere */,
			      tempBuf.length,
			      PROT_READ | PROT_WRITE,
			      MAP_SHARED,
			      camHal->fd, tempBuf.m.offset);

		if (MAP_FAILED == camHal->buffers[camHal->n_buffers].data)
			ALOGE("%s: mmap failed", __func__);
	}
    ALOGE("%s: X", __func__);
    return 0;
}

/*
This function sets the resolution and pixel format of the USB camera
*/
static int initUsbCamera(camera_hardware_t *camHal, int wd, int ht)
{
    int     rc = -1;
	struct  v4l2_capability     cap;
	struct  v4l2_cropcap        cropcap;
	struct  v4l2_crop           crop;
	struct  v4l2_format         v4l2format;
	unsigned int                min;

    ALOGE("%s: E", __func__);

	if (-1 == ioctlLoop(camHal->fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			ALOGE( "%s: This is not V4L2 device\n", __func__);
            return -1;
		} else {
            /* TBD */
            ALOGE("%s: VIDIOC_QUERYCAP errno: %d", __func__, errno);
        }
    }
    ALOGE("%s: VIDIOC_QUERYCAP success", __func__);

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        ALOGE("%s: This is not video capture device\n", __func__);
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        ALOGE("%s: This does not support streaming i/o\n", __func__);
        return -1;
    }

    /* Select video input, video standard and tune here. */
	memset(&cropcap, 0, sizeof(cropcap));

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == ioctlLoop(camHal->fd, VIDIOC_CROPCAP, &cropcap)) {

        /* reset to default */
		crop.c = cropcap.defrect;
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        ALOGE("%s: VIDIOC_CROPCAP success", __func__);
		if (-1 == ioctlLoop(camHal->fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
                ALOGE("%s: VIDIOC_S_CROP success", __func__);

	} else {
		/* Errors ignored. */
               ALOGE("%s: VIDIOC_S_CROP failed", __func__);
	}


    memset(&v4l2format, 0, sizeof(v4l2format));

	v4l2format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	{
        v4l2format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        v4l2format.fmt.pix.field       = V4L2_FIELD_INTERLACED;
		v4l2format.fmt.pix.width       = wd;
		v4l2format.fmt.pix.height      = ht;

        if (-1 == ioctlLoop(camHal->fd, VIDIOC_S_FMT, &v4l2format))
        {
			ALOGE("%s: VIDIOC_S_FMT failed", __func__);
            return -1;
        }
        ALOGE("%s: VIDIOC_S_FMT success", __func__);

		/* Note VIDIOC_S_FMT may change width and height. */
	}

	/* Recommended due to driver bug */
    ALOGE("%s: pix.bytesperline: %d, pix.width: %d, pix.height: %d",
         __func__, v4l2format.fmt.pix.bytesperline,
         v4l2format.fmt.pix.width, v4l2format.fmt.pix.height);

    min = v4l2format.fmt.pix.width * 2;
    if (v4l2format.fmt.pix.bytesperline < min)
        v4l2format.fmt.pix.bytesperline = min;
    min = v4l2format.fmt.pix.bytesperline * v4l2format.fmt.pix.height;
    if (v4l2format.fmt.pix.sizeimage < min)
        v4l2format.fmt.pix.sizeimage = min;

    ALOGE("%s: pix.sizeimage: %d", __func__, v4l2format.fmt.pix.sizeimage);
	rc = initV4L2mmap(camHal);
    ALOGE("%s: X", __func__);
    return rc;
}

/*
This function sends STREAM ON command to the USB camera driver
*/
static int startUsbCamCapture(camera_hardware_t *camHal)
{
    int         rc = -1;
	unsigned    int i;
	enum        v4l2_buf_type   v4l2BufType;
    ALOGE("%s: E", __func__);

    for (i = 0; i < camHal->n_buffers; ++i) {
        struct v4l2_buffer tempBuf;

        memset(&tempBuf, 0, sizeof(tempBuf));
        tempBuf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        tempBuf.memory  = V4L2_MEMORY_MMAP;
        tempBuf.index   = i;

        if (-1 == ioctlLoop(camHal->fd, VIDIOC_QBUF, &tempBuf))
            ALOGE("%s: VIDIOC_QBUF for %d buffer failed", __func__, i);
        else
            ALOGE("%s: VIDIOC_QBUF for %d buffer success", __func__, i);
    }

    v4l2BufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctlLoop(camHal->fd, VIDIOC_STREAMON, &v4l2BufType))
        ALOGE("%s: VIDIOC_STREAMON failed", __func__);
    else
    {
        ALOGE("%s: VIDIOC_STREAMON success", __func__);
        rc = 0;
    }

    ALOGE("%s: X", __func__);
    return rc;
}

/*
This funtions gets/acquires 1 capture buffer from the camera driver
*/
static int get_buf_from_cam(camera_hardware_t *camHal)
{
    int rc = -1;

    ALOGE("%s: E", __func__);
	{
        memset(&camHal->curCaptureBuf, 0, sizeof(camHal->curCaptureBuf));

		camHal->curCaptureBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		camHal->curCaptureBuf.memory = V4L2_MEMORY_MMAP;

		if (-1 == ioctlLoop(camHal->fd, VIDIOC_DQBUF, &camHal->curCaptureBuf)) {
			switch (errno) {
			case EAGAIN:
                ALOGE("%s: EAGAIN error", __func__);
				return 1;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				ALOGE("%s: VIDIOC_DQBUF error", __func__);
			}
		}
        else
        {
            rc = 0;
            ALOGE("%s: VIDIOC_DQBUF: %d successful, %d bytes",
                 __func__, camHal->curCaptureBuf.index,
                 camHal->curCaptureBuf.bytesused);
        }
	}
    ALOGE("%s: X", __func__);
	return rc;
}

/*
This funtion puts/releases 1 capture buffer back to the camera driver
*/
static int put_buf_to_cam(camera_hardware_t *camHal)
{
    ALOGE("%s: E", __func__);

    camHal->curCaptureBuf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    camHal->curCaptureBuf.memory      = V4L2_MEMORY_MMAP;


    if (-1 == ioctlLoop(camHal->fd, VIDIOC_QBUF, &camHal->curCaptureBuf))
    {
        ALOGE("%s: VIDIOC_QBUF failed ", __func__);
        return 1;
    }
    ALOGE("%s: X", __func__);
    return 0;
}

/*
This funtion gets/acquires 1 display buffer from the display window
*/
static int get_buf_from_display(camera_hardware_t *camHal, int *buffer_id)
{
    int                     err = 0;
    preview_stream_ops      *mPreviewWindow = NULL;
    int                     stride = 0, cnt = 0;
    buffer_handle_t         *buffer_handle = NULL;
    struct private_handle_t *private_buffer_handle = NULL;

    ALOGE("%s: E", __func__);

    if (camHal == NULL) {
        ALOGE("%s: camHal = NULL", __func__);
        return -1;
    }

    mPreviewWindow = camHal->window;
    if( mPreviewWindow == NULL) {
        ALOGE("%s: mPreviewWindow = NULL", __func__);
        return -1;
    }
    err = mPreviewWindow->dequeue_buffer(mPreviewWindow,
                                    &buffer_handle,
                                    &stride);
    if(!err) {
        ALOGE("%s: dequeue buf buffer_handle: %p\n", __func__, buffer_handle);

        ALOGE("%s: mPreviewWindow->lock_buffer: %p",
             __func__, mPreviewWindow->lock_buffer);
        if(mPreviewWindow->lock_buffer) {
            err = mPreviewWindow->lock_buffer(mPreviewWindow, buffer_handle);
            ALOGE("%s: mPreviewWindow->lock_buffer success", __func__);
        }
        ALOGE("%s: camera call genlock_lock, hdl=%p",
             __func__, (*buffer_handle));

        if (GENLOCK_NO_ERROR !=
            genlock_lock_buffer((native_handle_t *)(*buffer_handle),
                                GENLOCK_WRITE_LOCK, GENLOCK_MAX_TIMEOUT)) {
           ALOGE("%s: genlock_lock_buffer(WRITE) failed", __func__);
       } else {
         ALOGE("%s: genlock_lock_buffer hdl =%p", __func__, *buffer_handle);
       }

        private_buffer_handle = (struct private_handle_t *)(*buffer_handle);

        ALOGE("%s: fd = %d, size = %d, offset = %d",
             __func__, private_buffer_handle->fd,
        private_buffer_handle->size, private_buffer_handle->offset);

        for(cnt = 0; cnt < camHal->previewMem.buffer_count + 2; cnt++) {
            if(private_buffer_handle->fd ==
               camHal->previewMem.private_buffer_handle[cnt]->fd) {
                *buffer_id = cnt;
                ALOGE("%s: deQueued fd = %d, index: %d",
                     __func__, private_buffer_handle->fd, cnt);
                break;
            }
        }
/*
        ALOGE("%s: data = %p, size = %d, handle = %p", __func__,
            camera_memory->data, camera_memory->size, camera_memory->handle);
 */
    }
    else
        ALOGE("%s: dequeue buf failed \n", __func__);

    ALOGE("%s: X", __func__);

    return err;
}

/*
This funtion puts/releases 1 buffer back to the display window
*/
static int put_buf_to_display(camera_hardware_t *camHal, int buffer_id)
{
    int err = 0;
    preview_stream_ops    *mPreviewWindow;

    ALOGE("%s: E", __func__);

    if (camHal == NULL) {
        ALOGE("%s: camHal = NULL", __func__);
        return -1;
    }

    mPreviewWindow = camHal->window;
    if( mPreviewWindow == NULL) {
        ALOGE("%s: mPreviewWindow = NULL", __func__);
        return -1;
    }

    if (GENLOCK_FAILURE ==
        genlock_unlock_buffer(
            (native_handle_t *)
            (*(camHal->previewMem.buffer_handle[buffer_id])))) {
       ALOGE("%s: genlock_unlock_buffer failed: hdl =%p",
            __func__, (*(camHal->previewMem.buffer_handle[buffer_id])) );
    } else {
      ALOGE("%s: genlock_unlock_buffer success: hdl =%p",
           __func__, (*(camHal->previewMem.buffer_handle[buffer_id])) );
    }

    err = mPreviewWindow->enqueue_buffer(mPreviewWindow,
      (buffer_handle_t *)camHal->previewMem.buffer_handle[buffer_id]);
    if(!err) {
        ALOGE("%s: enqueue buf successful: %p\n",
             __func__, camHal->previewMem.buffer_handle[buffer_id]);
    }else
        ALOGE("%s: enqueue buf failed: %p\n",
             __func__, camHal->previewMem.buffer_handle[buffer_id]);

    ALOGE("%s: X", __func__);

    return err;
}

/*
This funtion copies the content from capture buffer to preiew display buffer
*/
static int copy_data_frm_cam_to_disp(camera_hardware_t *camHal, int buffer_id)
{
    if(!camHal) {
        ALOGE("%s: camHal is NULL", __func__);
        return -1;
    }
    covert_YUYV_to_420_NV12(
        (char *)camHal->buffers[camHal->curCaptureBuf.index].data,
        (char *)camHal->previewMem.camera_memory[buffer_id]->data, PRVW_WD, PRVW_HT);
    ALOGE("%s: Copied %d bytes from camera buffer %d to display buffer: %d",
         __func__, camHal->curCaptureBuf.bytesused,
         camHal->curCaptureBuf.index, buffer_id);
    return 0;
}

/*
This is a wrapper function to start preview thread
*/
static int launch_preview_thread(camera_hardware_t *camHal)
{
    ALOGE("%s: E", __func__);
    int rc = 0;

    if(!camHal) {
        ALOGE("%s: camHal is NULL", __func__);
        return -1;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&camHal->previewThread, &attr, previewloop, camHal);

    ALOGE("%s: X", __func__);
    return rc;
}

/*
This is thread funtion for preivew loop
*/
static void * previewloop(void *hcamHal = NULL)
{
    int buffer_id = 0;
    int color = 30;
    pid_t tid = 0;
    camera_hardware_t *camHal = NULL;

    camHal = (camera_hardware_t *)hcamHal;
    ALOGE("%s: E", __func__);

    if(!camHal) {
        ALOGE("%s: camHal is NULL", __func__);
        return NULL ;
    }

    while( camHal->window == NULL) {
        ALOGE("%s: sleeping coz camHal->window = NULL",__func__);
        sleep(2);
        continue;
        //return NULL;
    }

    tid  = gettid();
    /* TBD: Set appropriate audio thread priority */
    androidSetThreadPriority(tid, ANDROID_PRIORITY_NORMAL);
    prctl(PR_SET_NAME, (unsigned long)"Camera HAL preview thread", 0, 0, 0);

	while(1) {


        for (;;) {
			fd_set fds;
			struct timeval tv;
			int r = 0;

//            Mutex::Autolock autoLock(camHal->lock);
			FD_ZERO(&fds);
#if CAPTURE
			FD_SET(camHal->fd, &fds);
#endif /* CAPTURE */

			/* Timeout. */
			tv.tv_sec = 0;
			tv.tv_usec = 100000;

            ALOGE("%s: b4 select on camHal->fd + 1, camHal->fd: %d",
                 __func__, camHal->fd);
#if CAPTURE
            r = select(camHal->fd + 1, &fds, NULL, NULL, &tv);
#else
            r = select(1, NULL, NULL, NULL, &tv);

#endif /* CAPTURE */
            ALOGE("%s: after select on camHal->fd + 1, camHal->fd: %d",
                 __func__, camHal->fd);

			if (-1 == r) {
				if (EINTR == errno)
					continue;
				ALOGE("%s: FDSelect error: %d", __func__, errno);
			}

			if (0 == r) {
				ALOGE("%s: select timeout\n", __func__);
			}

            /* TBD: should display and camera Q/deQ be on different threads */
#if DISPLAY
            if(0 == get_buf_from_display(camHal, &buffer_id)) {
                ALOGE("%s: get_buf_from_display success: %d",
                     __func__, buffer_id);
            }
            else
            {
                ALOGE("%s: get_buf_from_display failed. Skipping the loop",
                     __func__);
                continue;
            }
#endif

#if CAPTURE
			if (0 == get_buf_from_cam(camHal))
                ALOGE("%s: get_buf_from_cam success", __func__);
            else
                ALOGE("%s: get_buf_from_cam error", __func__);
#endif

#if FILE_DUMP_CAMERA
            {
                static int frame_cnt = 0;
                FILE *fp = NULL;
                char fn[128];

                strncpy(fn, "/data/USBcam.yuv", 128);

                if (0 == frame_cnt) {
                    fp = fopen(fn, "wb");
                    if (NULL == fp) {
                        ALOGE("%s: Error in opening %s", __func__, fn);
                    }
                    fclose(fp);
                }
                fp = fopen(fn, "ab");
                if (NULL == fp) {
                    ALOGE("%s: Error in opening %s", __func__, fn);
                }
                fwrite(camHal->buffers[camHal->curCaptureBuf.index].start,
                       1, camHal->curCaptureBuf.bytesused, fp);
                ALOGE("%s: Written %d frame. buf_index: %d : %d bytes",
                     __func__, frame_cnt, camHal->curCaptureBuf.index,
                     camHal->curCaptureBuf.bytesused);
                fclose(fp);
                frame_cnt++;
            }
#endif

#if MEMSET
            color += 50;
            if(color > 200) {
                color = 30;
            }
            ALOGE("%s: Setting to the color: %d\n", __func__, color);
            memset(camHal->previewMem.camera_memory[buffer_id]->data,
                   color, PRVW_WD*PRVW_HT*1.5);

            //sleep(1);
#else
            copy_data_frm_cam_to_disp(camHal, buffer_id);
            ALOGE("%s: Copied datat to buffer_id: %d", __func__, buffer_id);
#endif

#if CALL_BACK
            {
                int msgType                         = 0;
                camera_memory_t *data               = NULL;
                camera_frame_metadata_t *metadata   = NULL;
                camera_memory_t *previewMem = NULL;
                int previewBufSize = 640*480*1.5;

                msgType |=  CAMERA_MSG_PREVIEW_FRAME;

                if(previewBufSize !=
                    camHal->previewMem.private_buffer_handle[buffer_id]->size) {

                    previewMem = camHal->get_memory(
                        camHal->previewMem.private_buffer_handle[buffer_id]->fd,
                        previewBufSize,
                        1,
                        camHal->cb_ctxt);

                      if (!previewMem || !previewMem->data) {
                          ALOGE("%s: get_memory failed.\n", __func__);
                      }
                      else {
                          data = previewMem;
                          ALOGE("%s: GetMemory successful. data = %p",
                                    __func__, data);
                          ALOGE("%s: previewBufSize = %d, priv_buf_size: %d",
                            __func__, previewBufSize,
                            camHal->previewMem.private_buffer_handle[buffer_id]->size);
                      }
                }
                else{
                    data =   camHal->previewMem.camera_memory[buffer_id];
                    ALOGE("%s: No Getmemory, no invalid fmt. data = %p, idx=%d",
                        __func__, data, buffer_id);
                }

                if(msgType && camHal->data_cb) {
                    ALOGE("%s: before data callback", __func__);
                    camHal->data_cb(msgType, data, 0,metadata, camHal->cb_ctxt);
                    ALOGE("%s: after data callback: %p", __func__, camHal->data_cb);
                }
                if (previewMem)
                    previewMem->release(previewMem);
                }
#endif

#if FILE_DUMP_B4_DISP
            {
                static int frame_cnt = 0;
                FILE *fp = NULL;
                char *fn = "/sdcard/display.yuv";

                if (0 == frame_cnt) {
                    fp = fopen(fn, "wb");
                    if (NULL == fp) {
                        ALOGE("%s: Error in opening %s", __func__, fn);
                    }
                    fclose(fp);
                }
                fp = fopen(fn, "ab");
                if (NULL == fp) {
                    ALOGE("%s: Error in opening %s", __func__, fn);
                }
                fwrite(camHal->previewMem.camera_memory[buffer_id]->data,
                       1, PRVW_WD*PRVW_HT*1.5, fp);
                ALOGE("%s: Written buf_index: %d ", __func__, buffer_id);
                fclose(fp);
                frame_cnt++;
            }
#endif

#if DISPLAY
            if(0 == put_buf_to_display(camHal, buffer_id)) {
                ALOGE("%s: put_buf_to_display success: %d", __func__, buffer_id);
            }
#endif

#if CAPTURE
            if(0 == put_buf_to_cam(camHal)) {
                ALOGE("%s: put_buf_to_cam success", __func__);
            }
            else
                ALOGE("%s: put_buf_to_cam error", __func__);
#endif
			/* EAGAIN - continue select loop. */
		}
	}
    ALOGE("%s: X", __func__);
    return NULL;
}

/******************************************************************************/
}; // namespace android
