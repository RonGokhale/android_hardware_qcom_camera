/*
Copyright (c) 2014, The Linux Foundation. All rights reserved.

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
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*/

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#include <hardware/camera.h>
#include <utils/Timers.h>
#include <sys/mman.h>

/*===========================================================================
 *                               Common defines
 *==========================================================================*/
#define SET_LUA_ERROR_VAL(err) {lua_newtable(L); lua_pushliteral(L, "err"); lua_pushstring(L, err); lua_settable(L,-3);}
#define SET_LUA_NO_ERROR() {lua_newtable(L); lua_pushliteral(L, "err"); lua_pushliteral(L, "none"); lua_settable(L,-3);}

/*===========================================================================
 *                               Log Macros
 *==========================================================================*/
#if defined(USE_DLOG)
#include <dlog/dlog.h>
#elif defined(_ANDROID_)
#include <utils/Log.h>
#else
#include <stdio.h>
#endif

#undef CDBG
#define LOG_DEBUG

#ifndef LOG_DEBUG
#define CDBG(fmt, args...) do{}while(0)
#else
#if defined(_ANDROID_) || defined (USE_DLOG)
#undef LOG_NIDEBUG
#undef LOG_TAG
#define LOG_NIDEBUG 0
#define LOG_TAG "mm-camera-hal-test"
#define CDBG(fmt, args...) ALOGV(fmt, ##args)
#else
#define CDBG(fmt, args...) fprintf(stderr, "%s:%d "fmt"\n", __func__, __LINE__, ##args)
#endif
#endif

#if defined(_ANDROID_) || defined (USE_DLOG)
#define CDBG_HIGH(fmt, args...)  ALOGI(fmt, ##args)
#define CDBG_ERROR(fmt, args...)  ALOGE(fmt, ##args)
#define CDBG_LOW(fmt, args...) ALOGD(fmt, ##args)
#else
#define CDBG_HIGH(fmt, args...) fprintf(stderr, "%s:%d "fmt"\n", __func__, __LINE__, ##args)
#define CDBG_ERROR(fmt, args...) fprintf(stderr, "%s:%d "fmt"\n", __func__, __LINE__, ##args)
#define CDBG_LOW(fmt, args...) fprintf(stderr, "%s:%d "fmt"\n", __func__, __LINE__, ##args)
#endif

static pthread_cond_t  snapshot_cond  = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t snapshot_mutex = PTHREAD_MUTEX_INITIALIZER;

/*===========================================================================
 *                               Global variables
 *==========================================================================*/

static int (*qcamera_get_number_of_cameras)();
static int (*qcamera_get_info)(int camera_id, struct camera_info *info);
static int (*qcamera_release_resources)(struct camera_device *);

static int (*qcamera_camera_device_open)(const struct hw_module_t *module,
        int cam_id,
        struct hw_device_t **device);
static int (*qcamera_close_camera_device)( hw_device_t *);
static int (*qcamera_start_preview)( struct camera_device *);
static void (*qcamera_stop_preview)( struct camera_device *);
static int (*qcamera_set_callBacks) (struct camera_device *,
                                     camera_notify_callback notify_cb,
                                     camera_data_callback data_cb,
                                     camera_data_timestamp_callback data_cb_timestamp,
                                     camera_request_memory get_memory,
                                     void *user);
static int (*qcamera_enable_msg_type) (struct camera_device *, int32_t msg_type);
static int (*qcamera_disable_msg_type) (struct camera_device *, int32_t msg_type);
static int (*qcamera_start_recording)(struct camera_device *);
static int (*qcamera_stop_recording)(struct camera_device *);
static int (*qcamera_auto_focus)(struct camera_device *);
static int (*qcamera_cancel_auto_focus)(struct camera_device *);
static int (*qcamera_take_picture)(struct camera_device *);
static int (*qcamera_cancel_picture)(struct camera_device *);
static int (*qcamera_set_parameters)(struct camera_device *, const char *parms);
static char *(*qcamera_get_parameters)(struct camera_device *);
static int (*qcamera_send_command)(struct camera_device *,
                                   int32_t cmd, int32_t arg1, int32_t arg2);

/*===========================================================================
 *                               callback functions
 *==========================================================================*/

static void luabind_camerahal_notifycb(int32_t msg_type, int32_t ext1, int32_t ext2, void *user)
{
    if (msg_type ==  CAMERA_MSG_ERROR) {
        CDBG_ERROR("\nSomething wrong! Error callback from Camera\n");
    }
    CDBG("Exit");
}

static void luabind_camerahal_datacb(int32_t msg_type, const camera_memory_t *data,
                                     unsigned int index,
                                     camera_frame_metadata_t *metadata,
                                     void *user)
{
    if (msg_type == CAMERA_MSG_COMPRESSED_IMAGE) {
        pthread_mutex_lock(&snapshot_mutex);
        pthread_cond_signal(&snapshot_cond);
        pthread_mutex_unlock(&snapshot_mutex);
        CDBG("Snapshot buffer callback for camera: %d", user);
    } else if (msg_type ==  CAMERA_MSG_PREVIEW_FRAME) {
        CDBG("Preview buffer callback for camera: %d", user);
    } else if (msg_type ==  CAMERA_MSG_ERROR) {
        CDBG_ERROR("\nSomething wrong! Error callback from Camera: %d\n", user);
    }

    CDBG("Exit");
}

static void luabind_camerahal_timestampcb(nsecs_t timestamp, int32_t msg_type,
        const camera_memory_t *data, unsigned int index,
        void *user)
{
    CDBG_LOW("\timestamp %ld msg_type %d, data %p,  index %d",
             (long) timestamp, msg_type, (unsigned int *)data, index);

    CDBG("Exit");
}

static void luabind_camerahal_unmapfd(camera_memory_t *handle)
{
    if (!handle)
        return;

    CDBG("Buffer fd: %d, Buffer size: %d", handle->data, handle->size);

    if (handle->handle != -1) {
        if (munmap(handle->data, handle->size) != 0) {
            CDBG_ERROR("Buffer unmap failed");
        }
    } else {
        free(handle->data);
    }

    free(handle);

    CDBG("Exit");
}


static void *luabind_camerahal_mapfd(int fd, size_t size)
{
    int offset = 0;
    void *base = NULL;
    if (size == 0) {
        struct stat sb;
        if (fstat(fd, &sb) == 0)
            size = sb.st_size;
    }
    base = (uint8_t *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
    if (base == MAP_FAILED) {
        CDBG_ERROR("\nmmap failed!!\n");
        close(fd);
        return NULL;
    }

    CDBG("Exit");
    return base;
}

static camera_memory_t *luabind_camerahal_getmemorycb(int fd, size_t buf_size, unsigned int num_bufs,
        void *user __attribute__((unused)))
{
    //num_bufs is always set to 1, so ignore this parameter. Mapping happens for
    //one buffer at a time
    camera_memory_t *handle = (camera_memory_t *)malloc (sizeof(camera_memory_t));
    const size_t pagesize = getpagesize();
    buf_size = ((buf_size + pagesize - 1) & ~(pagesize - 1));

    //When fd is set to -1, expectation is to do a buffer allocation, else just do a map
    if (fd != -1) {
        handle->data = luabind_camerahal_mapfd(fd, buf_size);
        handle->handle = fd;
    } else {
        handle->data = malloc(buf_size);
        handle->handle = -1;
    }

    handle->size = buf_size;
    handle->release = luabind_camerahal_unmapfd;

    CDBG("Buffer fd: %d, Buffer size: %d", handle->data, handle->size);

    CDBG("Exit");
    return handle;
}

/*===========================================================================
 *                               lua binder functions
 *==========================================================================*/

static int luabind_camerahal_initialize(lua_State *L)
{
    void *hal_lib = dlopen("libmmcamera_hal.so", RTLD_LAZY);
    if (hal_lib == NULL) {
        CDBG_ERROR("dl_error=%s", dlerror());
        SET_LUA_ERROR_VAL(dlerror());
        goto end;
    }

    *(void **)&(qcamera_get_number_of_cameras) = dlsym(hal_lib, "get_number_of_cameras");
    *(void **)&(qcamera_get_info) = dlsym(hal_lib, "get_camera_info");
    *(void **)&(qcamera_release_resources) = dlsym(hal_lib, "release_resources");

    *(void **)&(qcamera_camera_device_open) = dlsym(hal_lib, "camera_device_open");
    *(void **)&(qcamera_close_camera_device) = dlsym(hal_lib, "close_camera_device");

    *(void **)&(qcamera_set_callBacks) = dlsym(hal_lib, "set_CallBacks");
    *(void **)&(qcamera_enable_msg_type) = dlsym(hal_lib, "enable_msg_type");

    *(void **)&(qcamera_start_preview) = dlsym(hal_lib, "start_preview");
    *(void **)&(qcamera_stop_preview) = dlsym(hal_lib, "stop_preview");
    *(void **)&(qcamera_take_picture) = dlsym(hal_lib, "take_picture");
    *(void **)&(qcamera_cancel_picture) = dlsym(hal_lib, "cancel_picture");
    *(void **)&(qcamera_start_recording) = dlsym(hal_lib, "start_recording");
    *(void **)&(qcamera_stop_recording) = dlsym(hal_lib, "stop_recording");

    *(void **)&(qcamera_auto_focus) = dlsym(hal_lib, "auto_focus");
    *(void **)&(qcamera_cancel_auto_focus) = dlsym(hal_lib, "cancel_auto_focus");
    *(void **)&(qcamera_set_parameters) = dlsym(hal_lib, "set_parameters");
    *(void **)&(qcamera_get_parameters) = dlsym(hal_lib, "get_parameters");
    *(void **)&(qcamera_send_command) = dlsym(hal_lib, "send_command");

    if (!qcamera_get_number_of_cameras ||
        !qcamera_get_info ||
        !qcamera_camera_device_open ||
        !qcamera_close_camera_device ||
        !qcamera_start_preview ||
        !qcamera_stop_preview ||
        !qcamera_set_callBacks ||
        !qcamera_start_recording ||
        !qcamera_stop_recording ||
        !qcamera_auto_focus  ||
        !qcamera_cancel_auto_focus ||
        !qcamera_take_picture  ||
        !qcamera_cancel_picture ||
        !qcamera_set_parameters  ||
        !qcamera_get_parameters ||
        !qcamera_send_command ||
        !qcamera_enable_msg_type) {
        CDBG_ERROR("Error loading symbols");
        SET_LUA_ERROR_VAL("Error loading symbols");
        goto end;
    }
    CDBG("\nSymbols Loading Done !!");
    int num_cam = qcamera_get_number_of_cameras();
    CDBG_LOW("Number of cameras: %d", num_cam);

    //Extract camera detail information
    struct camera_info info;
    int i;
    for (i = 0; i < num_cam; ++i) {
        qcamera_get_info(i, &info);
    }

    SET_LUA_NO_ERROR();

    lua_pushliteral(L, "num_cam");
    lua_pushnumber(L, num_cam);
    lua_settable(L, -3);

end:
    return 1;
}

static int luabind_camerahal_open(lua_State *L)
{
    if (lua_gettop(L) < 1) {
        SET_LUA_ERROR_VAL("Wrong/Insufficient arguments");
        goto end;
    }

    int cam_id = luaL_checkint(L, -1);

    CDBG("cam_id: %d", cam_id);

    struct hw_module_t module;
    memset(&module, 0, sizeof(hw_module_t));
    module.name = "QCamera Module";

    struct hw_device_t *hw_device;
    int rc = qcamera_camera_device_open(&module, cam_id, &hw_device);

    if (hw_device == NULL || rc != 0) {
        CDBG_ERROR("Device open failed");
        SET_LUA_ERROR_VAL("Device open failed");
        goto end;
    }

    CDBG("hw_device: %p", hw_device);

    SET_LUA_NO_ERROR();

    lua_pushliteral(L, "hw_device");
    lua_pushlightuserdata(L, hw_device);
    lua_settable(L, -3);

end:
    return 1;
}

static int luabind_camerahal_releaseresources(lua_State *L)
{
    struct hw_device_t *hw_device = (struct camera_device *)lua_touserdata(L, -1);
    if (hw_device == NULL) {
        SET_LUA_ERROR_VAL("Null hw_device pointer");
        goto end;
    }

    int rc = qcamera_release_resources(hw_device);
    if (rc != 0) {
        SET_LUA_ERROR_VAL("release camera API failed");
        goto end;
    }

    SET_LUA_NO_ERROR();

end:
    return 1;
}

static int luabind_camerahal_close(lua_State *L)
{
    struct hw_device_t *hw_device = (struct camera_device *)lua_touserdata(L, -1);
    if (hw_device == NULL) {
        SET_LUA_ERROR_VAL("Null hw_device pointer");
        goto end;
    }

    int rc = qcamera_close_camera_device(hw_device);
    if (rc != 0) {
        SET_LUA_ERROR_VAL("close camera API failed");
        goto end;
    }

    SET_LUA_NO_ERROR();

end:
    return 1;
}

static int luabind_camerahal_startpreview(lua_State *L)
{
    if (lua_gettop(L) < 1) {
        SET_LUA_ERROR_VAL("Wrong/Insufficient arguments");
        goto end;
    }

    struct camera_device *hw_device = (struct camera_device *)lua_touserdata(L, -1);
    if (hw_device == NULL) {
        SET_LUA_ERROR_VAL("Null hw_device pointer");
        goto end;
    }
    int rc = qcamera_enable_msg_type(hw_device, CAMERA_MSG_PREVIEW_FRAME);
    if (rc != 0) {
        SET_LUA_ERROR_VAL("Enable CAMERA_MSG_PREVIEW_FRAME failed");
        goto end;
    }
    rc = qcamera_start_preview(hw_device);
    if (rc != 0) {
        SET_LUA_ERROR_VAL("Start preview failed");
        goto end;
    }

    SET_LUA_NO_ERROR();

end:
    return 1;
}


static int luabind_camerahal_stoppreview(lua_State *L)
{
    if (lua_gettop(L) < 1) {
        SET_LUA_ERROR_VAL("Wrong/Insufficient arguments");
        goto end;
    }

    struct camera_device *hw_device = (struct camera_device *)lua_touserdata(L, -1);
    if (hw_device == NULL) {
        SET_LUA_ERROR_VAL("Null hw_device pointer");
        goto end;
    }

    qcamera_stop_preview(hw_device);

    SET_LUA_NO_ERROR();

end:
    return 1;
}

static int luabind_camerahal_setcallbacks(lua_State *L)
{
    if (lua_gettop(L) < 2) {
        SET_LUA_ERROR_VAL("Wrong/Insufficient arguments");
        goto end;
    }

    struct camera_device *hw_device = (struct camera_device *)lua_touserdata(L, -2);
    if (hw_device == NULL) {
        SET_LUA_ERROR_VAL("Null hw_device pointer");
        goto end;
    }

    void *userdata = (void *)luaL_checkint(L, -1);

    CDBG("userdata: %p", userdata);

    int rc = qcamera_set_callBacks(hw_device,
                                   luabind_camerahal_notifycb,
                                   luabind_camerahal_datacb,
                                   luabind_camerahal_timestampcb,
                                   luabind_camerahal_getmemorycb,
                                   userdata);

    if (rc != 0) {
        SET_LUA_ERROR_VAL("Set callbacks failed");
        goto end;
    }

    SET_LUA_NO_ERROR();

end:
    return 1;
}


static int luabind_camerahal_getparameters(lua_State *L)
{
    if (lua_gettop(L) < 1) {
        SET_LUA_ERROR_VAL("Wrong/Insufficient arguments");
        goto end;
    }

    struct camera_device *hw_device = (struct camera_device *)lua_touserdata(L, -1);
    if (hw_device == NULL) {
        SET_LUA_ERROR_VAL("Null hw_device pointer");
        goto end;
    }

    CDBG("HW device: %p", hw_device);

    char *getParams;
    getParams = qcamera_get_parameters(hw_device);
    if (getParams == NULL) {
        SET_LUA_ERROR_VAL("get param returned NULL");
        goto end;
    }

    SET_LUA_NO_ERROR();
    lua_pushliteral(L, "params");
    lua_pushstring(L, getParams);
    lua_settable(L, -3);

    free(getParams);

end:
    return 1;
}

static int luabind_camerahal_setparameters(lua_State *L)
{
    if (lua_gettop(L) < 2) {
        SET_LUA_ERROR_VAL("Wrong/Insufficient arguments");
        goto end;
    }
    struct camera_device *hw_device = (struct camera_device *)lua_touserdata(L, -2);
    if (hw_device == NULL) {
        SET_LUA_ERROR_VAL("Null hw_device pointer");
        goto end;
    }

    int rc;
    char *params = lua_tostring(L, -1);
    if (params) {
        rc = qcamera_set_parameters(hw_device, params);
    } else {
        SET_LUA_ERROR_VAL("Null param pointer");
        goto end;
    }

    if (rc != 0) {
        SET_LUA_ERROR_VAL("set param API failed");
        goto end;
    }

    SET_LUA_NO_ERROR();

end:
    return 1;
}

static int luabind_camerahal_takepicture(lua_State *L)
{
    if (lua_gettop(L) < 1) {
        SET_LUA_ERROR_VAL("Wrong/Insufficient arguments");
        goto end;
    }

    struct camera_device *hw_device = (struct camera_device *)lua_touserdata(L, -1);
    if (hw_device == NULL) {
        SET_LUA_ERROR_VAL("Null hw_device pointer");
        goto end;
    }

    int rc = qcamera_enable_msg_type(hw_device, CAMERA_MSG_COMPRESSED_IMAGE);
    if (rc != 0) {
        SET_LUA_ERROR_VAL("Setting CAMERA_MSG_COMPRESSED_IMAGE failed");
        goto end;
    }

    rc = qcamera_take_picture(hw_device);
    if (rc != 0) {
        SET_LUA_ERROR_VAL("Take picture returned error");
        goto end;
    }

    //wait for jpeg snapshot callback.
    pthread_mutex_lock(&snapshot_mutex);
    pthread_cond_wait(&snapshot_cond, &snapshot_mutex);
    pthread_mutex_unlock(&snapshot_mutex);

    SET_LUA_NO_ERROR();

end:
    return 1;
}

static int luabind_camerahal_cancelpicture(lua_State *L)
{
    SET_LUA_ERROR_VAL("Not implemented");
    return 1;
}

static int luabind_camerahal_startrecording(lua_State *L)
{
    if (lua_gettop(L) < 1) {
        SET_LUA_ERROR_VAL("Wrong/Insufficient arguments");
        goto end;
    }

    struct camera_device *hw_device = (struct camera_device *)lua_touserdata(L, -1);
    if (hw_device == NULL) {
        SET_LUA_ERROR_VAL("Null hw_device pointer");
        goto end;
    }

    int rc = qcamera_start_recording(hw_device);
    if (rc != 0) {
        SET_LUA_ERROR_VAL("start_recording  failed");
        goto end;
    }

    SET_LUA_NO_ERROR();

end:
    return 1;
}

static int luabind_camerahal_stoprecording(lua_State *L)
{
    if (lua_gettop(L) < 1) {
        SET_LUA_ERROR_VAL("Wrong/Insufficient arguments");
        goto end;
    }

    struct camera_device *hw_device = (struct camera_device *)lua_touserdata(L, -1);
    if (hw_device == NULL) {
        SET_LUA_ERROR_VAL("Null hw_device pointer");
        goto end;
    }

    int rc = qcamera_stop_recording(hw_device);
    if (rc != 0) {
        SET_LUA_ERROR_VAL("Stop recording failed");
        goto end;
    }

    SET_LUA_NO_ERROR();

end:
    return 1;
}

static int luabind_camerahal_autofocus(lua_State *L)
{
    if (lua_gettop(L) < 1) {
        SET_LUA_ERROR_VAL("Wrong/Insufficient arguments");
        goto end;
    }

    struct camera_device *hw_device = (struct camera_device *)lua_touserdata(L, -1);
    if (hw_device == NULL) {
        SET_LUA_ERROR_VAL("Null hw_device pointer");
        goto end;
    }

    int rc = qcamera_auto_focus(hw_device);
    if (rc != 0) {
        SET_LUA_ERROR_VAL("auto_focus failed");
        goto end;
    }

    SET_LUA_NO_ERROR();

end:
    return 1;
}

static int luabind_camerahal_cancelautofocus(lua_State *L)
{
    if (lua_gettop(L) < 1) {
        SET_LUA_ERROR_VAL("Wrong/Insufficient arguments");
        goto end;
    }

    struct camera_device *hw_device = (struct camera_device *)lua_touserdata(L, -1);
    if (hw_device == NULL) {
        SET_LUA_ERROR_VAL("Null hw_device pointer");
        goto end;
    }

    int rc = qcamera_cancel_auto_focus(hw_device);
    if (rc != 0) {
        SET_LUA_ERROR_VAL("auto_focus failed");
        goto end;
    }

    SET_LUA_NO_ERROR();

end:
    return 1;
}

static int luabind_camerahal_sendcommand(lua_State *L)
{
    SET_LUA_ERROR_VAL("Not implemented");
    return 1;
}

/* Register file's functions with the
 * luaopen_libraryname() function
 *
 * This function should contain lua_register() commands for
 * each function that needs to be made available from Lua.
 *
*/
int luaopen_libluacamerahal(lua_State *L)
{
    lua_register(L, "camera_initialize", luabind_camerahal_initialize);
    lua_register(L, "camera_open", luabind_camerahal_open);
    lua_register(L, "camera_close", luabind_camerahal_close);
    lua_register(L, "camera_releaseresources", luabind_camerahal_releaseresources);
    lua_register(L, "camera_setcallbacks", luabind_camerahal_setcallbacks);
    lua_register(L, "camera_startpreview", luabind_camerahal_startpreview);
    lua_register(L, "camera_stoppreview", luabind_camerahal_stoppreview);
    lua_register(L, "camera_startrecording", luabind_camerahal_startrecording);
    lua_register(L, "camera_stoprecording", luabind_camerahal_stoprecording);
    lua_register(L, "camera_autofocus", luabind_camerahal_autofocus);
    lua_register(L, "camera_cancelautofocus", luabind_camerahal_cancelautofocus);
    lua_register(L, "camera_takepicture", luabind_camerahal_takepicture);
    lua_register(L, "camera_cancelpicture", luabind_camerahal_cancelpicture);
    lua_register(L, "camera_getparams", luabind_camerahal_getparameters);
    lua_register(L, "camera_setparams", luabind_camerahal_setparameters);
    lua_register(L, "camera_sendcommand", luabind_camerahal_sendcommand);

    return 0;
}
