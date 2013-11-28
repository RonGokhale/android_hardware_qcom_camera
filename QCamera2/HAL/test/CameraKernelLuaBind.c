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
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <linux/media.h>
#include <linux/videodev2.h>

#include <media/msmb_camera.h>

#include <media/msm_cam_sensor.h>

#include <media/msmb_ispif.h>
#include <media/msmb_isp.h>
#include <media/msmb_pproc.h>
#include <media/msmb_generic_buf_mgr.h>

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

static int luabind_open(lua_State *L)
{
	int fd;

    if (lua_gettop(L) != 2) {
        SET_LUA_ERROR_VAL("Wrong/Insufficient arguments");
        goto end;
    }

	char *dev_name = lua_tostring(L, 1);
    if (!dev_name) {
        SET_LUA_ERROR_VAL("Null device name");
        goto end;
    }

    int flags = luaL_checkint(L, 2);
    if (!flags) {
        SET_LUA_ERROR_VAL("Null flags");
        goto end;
    }

    CDBG("Opening device: %s, with flags: %d", dev_name, flags);

    fd = open(dev_name, flags);
    if (fd == -1) {
        SET_LUA_ERROR_VAL(strerror(errno));
        goto end;
    }

    CDBG("Device open successful. Device fd: %d", fd);

    SET_LUA_NO_ERROR();
    lua_pushliteral(L, "fd");
    lua_pushnumber(L, fd);
    lua_settable(L, -3);

end:
    return 1;
}

static int luabind_close(lua_State *L)
{
    if (lua_gettop(L) != 1) {
        SET_LUA_ERROR_VAL("Wrong/Insufficient arguments");
        goto end;
    }

    int fd = luaL_checkint(L, 1);
    if (!fd) {
        SET_LUA_ERROR_VAL("Null fd");
        goto end;
    }

    if (close(fd) == -1) {
        SET_LUA_ERROR_VAL(strerror(errno));
        goto end;
    }

    SET_LUA_NO_ERROR();

end:
    return 1;
}

static void set_int(lua_State *L, const char *string, int val)
{
    lua_pushstring(L, string);
    lua_pushnumber(L, val);
    lua_settable(L, -3);
}

static void set_string(lua_State *L, const char *string, const char *val)
{
    lua_pushstring(L, string);
    lua_pushstring(L, val);
    lua_settable(L, -3);
}

static bool handle_enum_entities(lua_State *L, int fd)
{
    struct media_entity_desc entity;
    memset(&entity, 0, sizeof(entity));
    int id = luaL_checkint(L, 3);
    entity.id = id | MEDIA_ENT_ID_FLAG_NEXT;

    CDBG("Entity id: %x", entity.id);

    int rc = ioctl(fd, MEDIA_IOC_ENUM_ENTITIES, &entity);
    if (rc < 0) {
        SET_LUA_ERROR_VAL(strerror(errno));
        return false;
    }

    SET_LUA_NO_ERROR();

    set_int(L, "group_id", entity.group_id);
    set_string(L, "name", entity.name);

    switch (entity.type) {
        case MEDIA_ENT_T_DEVNODE:
            set_string(L, "type", "Unknown Device Node");
            break;
        case MEDIA_ENT_T_DEVNODE_V4L:
            set_string(L, "type", "V4L video, radio or vbi device node");
            break;
        case MEDIA_ENT_T_DEVNODE_FB:
            set_string(L, "type", "Frame buffer device node");
            break;
        case MEDIA_ENT_T_DEVNODE_ALSA:
            set_string(L, "type", "ALSA card");
            break;
        case MEDIA_ENT_T_DEVNODE_DVB:
            set_string(L, "type", "DVB card");
            break;
        case MEDIA_ENT_T_V4L2_SUBDEV:
            set_string(L, "type", "Unknown V4L sub-device");
            break;
        case MEDIA_ENT_T_V4L2_SUBDEV_SENSOR:
            set_string(L, "type", "Video sensor");
            break;
        case MEDIA_ENT_T_V4L2_SUBDEV_FLASH:
            set_string(L, "type", "Flash controller");
            break;
        case MEDIA_ENT_T_V4L2_SUBDEV_LENS:
            set_string(L, "type", "Lens controller");
            break;
        default:
            set_string(L, "type", "Unknown type");
            break;
    }

    CDBG("Exit");
    return true;
}

static bool handle_media_ioc(lua_State *L, int fd)
{
    struct media_device_info mdev_info;

    int rc = ioctl(fd, MEDIA_IOC_DEVICE_INFO, &mdev_info);
    if (rc < 0) {
        SET_LUA_ERROR_VAL(strerror(errno));
        return false;
    }

    SET_LUA_NO_ERROR();

    set_string(L, "model", mdev_info.model);
    return true;
}

static int luabind_ioctl(lua_State *L)
{
    if (lua_gettop(L) < 2) {
        SET_LUA_ERROR_VAL("Wrong/Insufficient arguments");
        goto end;
    }

    int fd = luaL_checkint(L, 1);
    if (!fd) {
        SET_LUA_ERROR_VAL("Null fd");
        goto end;
    }

    char *type = lua_tostring(L, 2);
    if (!type) {
        SET_LUA_ERROR_VAL("Null type");
        goto end;
    }

    if (!strcmp(type, "MEDIA_IOC_ENUM_ENTITIES")) {
       if (handle_enum_entities(L, fd) == false)
            goto end;

    } else if (!strcmp(type, "MEDIA_IOC_DEVICE_INFO")) {
        if (handle_media_ioc(L, fd) == false)
            goto end;

    } else if (!strcmp(type, "VIDIOC_MSM_ACTUATOR_CFG")) {

    } else if (!strcmp(type, "VIDIOC_MSM_CSID_IO_CFG")) {

    } else if (!strcmp(type, "VIDIOC_MSM_EEPROM_CFG")) {

    } else if (!strcmp(type, "VIDIOC_MSM_FLASH_LED_DATA_CFG")) {

    } else if (!strcmp(type, "VIDIOC_MSM_SENSOR_CFG")) {

    } else if (!strcmp(type, "VIDIOC_MSM_SENSOR_GET_SUBDEV_ID")) {

    } else if (!strcmp(type, "VIDIOC_MSM_ISP_UPDATE_STREAM")) {

    } else if (!strcmp(type, "VIDIOC_MSM_ISP_RELEASE_STREAM")) {

    } else if (!strcmp(type, "VIDIOC_MSM_ISP_REQUEST_STREAM")) {

    } else if (!strcmp(type, "VIDIOC_MSM_ISP_CFG_STREAM")) {

    } else if (!strcmp(type, "VIDIOC_MSM_ISP_INPUT_CFG")) {

    } else if (!strcmp(type, "VIDIOC_MSM_VFE_REG_CFG")) {

    } else if (!strcmp(type, "VIDIOC_MSM_BUF_MNGR_GET_BUF")) {

    } else if (!strcmp(type, "VIDIOC_MSM_BUF_MNGR_PUT_BUF")) {

    } else if (!strcmp(type, "VIDIOC_MSM_BUF_MNGR_BUF_DONE")) {

    } else if (!strcmp(type, "VIDIOC_MSM_CPP_GET_INST_INFO")) {

    } else if (!strcmp(type, "VIDIOC_MSM_CPP_QUEUE_BUF")) {

    } else if (!strcmp(type, "VIDIOC_MSM_CPP_GET_HW_INFO")) {

    } else if (!strcmp(type, "VIDIOC_MSM_CPP_LOAD_FIRMWARE")) {

    } else if (!strcmp(type, "VIDIOC_MSM_CPP_ENQUEUE_STREAM_BUFF_INFO")) {

    } else if (!strcmp(type, "VIDIOC_MSM_CPP_DEQUEUE_STREAM_BUFF_INFO")) {

    } else if (!strcmp(type, "VIDIOC_MSM_CPP_GET_EVENTPAYLOAD")) {

    } else if (!strcmp(type, "VIDIOC_MSM_CPP_CFG")) {

    } else if (!strcmp(type, "VIDIOC_SUBSCRIBE_EVENT")) {

    } else if (!strcmp(type, "VIDIOC_UNSUBSCRIBE_EVENT")) {

    } else if (!strcmp(type, "VIDIOC_DQEVENT")) {

    } else {
        /* table is in the stack at index 't' */
        lua_pushnil(L);  /* first key */
        while (lua_next(L, 3) != 0) {
            /* uses 'key' (at index -2) and 'value' (at index -1) */
            printf("%s - %s\n",
            lua_typename(L, lua_type(L, -2)),
            lua_typename(L, lua_type(L, -1)));
            /* removes 'value'; keeps 'key' for next iteration */
            lua_pop(L, 1);
        }
        CDBG_ERROR("No matching case found");
    }

end:
    return 1;
}

/* Register file's functions with the
 * luaopen_libraryname() function
 *
 * This function should contain lua_register() commands for
 * each function that needs to be made available from Lua.
 *
*/
int luaopen_libluacamerakernel(lua_State *L)
{
    lua_register(L, "d_open", luabind_open);
    lua_register(L, "d_ioctl", luabind_ioctl);
    lua_register(L, "d_close", luabind_close);
    return 0;
}
