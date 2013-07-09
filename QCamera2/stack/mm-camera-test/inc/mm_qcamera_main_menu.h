/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
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

#ifndef __MM_QCAMERA_MAIN_MENU_H__
#define __MM_QCAMERA_MAIN_MENU_H__

#include "mm_camera_interface.h"
#include "mm_jpeg_interface.h"

#define VIDEO_BUFFER_SIZE       (PREVIEW_WIDTH * PREVIEW_HEIGHT * 3/2)
#define THUMBNAIL_BUFFER_SIZE   (THUMBNAIL_WIDTH * THUMBNAIL_HEIGHT * 3/2)
#define SNAPSHOT_BUFFER_SIZE    (PICTURE_WIDTH * PICTURE_HEIGHT * 3/2)

/*===========================================================================
 * Macro
 *===========================================================================*/
#define PREVIEW_FRAMES_NUM    5
#define VIDEO_FRAMES_NUM      5
#define THUMBNAIL_FRAMES_NUM  1
#define SNAPSHOT_FRAMES_NUM   1
#define MAX_NUM_FORMAT        32

typedef enum
{
  STOP_CAMERA = 1,
  PREVIEW_VIDEO_RESOLUTION = 2,
  SET_WHITE_BALANCE = 3,
  SET_EXP_METERING = 4,
  GET_CTRL_VALUE = 5,
  TOGGLE_AFR = 6,
  SET_ISO = 7,
  BRIGHTNESS_GOTO_SUBMENU = 8,
  CONTRAST_GOTO_SUBMENU = 9,
  EV_GOTO_SUBMENU = 10,
  SATURATION_GOTO_SUBMENU = 11,
  SET_ZOOM = 12,
  SET_SHARPNESS = 13,
  TAKE_YUV_SNAPSHOT = 14,
  TAKE_BURST_SNAPSHOT = 15,
  START_RECORDING = 16,
  STOP_RECORDING = 17,
  BEST_SHOT = 18,
  LIVE_SHOT = 19,
  FLASH_MODES = 20
} Camera_main_menu_t;

typedef enum
{
  ACTION_NO_ACTION,
  ACTION_STOP_CAMERA,
  ACTION_PREVIEW_VIDEO_RESOLUTION,
  ACTION_SET_WHITE_BALANCE,
  ACTION_SET_EXP_METERING,
  ACTION_GET_CTRL_VALUE,
  ACTION_TOGGLE_AFR,
  ACTION_SET_ISO,
  ACTION_BRIGHTNESS_INCREASE,
  ACTION_BRIGHTNESS_DECREASE,
  ACTION_CONTRAST_INCREASE,
  ACTION_CONTRAST_DECREASE,
  ACTION_EV_INCREASE,
  ACTION_EV_DECREASE,
  ACTION_SATURATION_INCREASE,
  ACTION_SATURATION_DECREASE,
  ACTION_SET_ZOOM,
  ACTION_SHARPNESS_INCREASE,
  ACTION_SHARPNESS_DECREASE,
  ACTION_TAKE_YUV_SNAPSHOT,
  ACTION_TAKE_BURST_SNAPSHOT,
  ACTION_START_RECORDING,
  ACTION_STOP_RECORDING,
  ACTION_SET_BESTSHOT_MODE,
  ACTION_TAKE_LIVE_SNAPSHOT,
  ACTION_SET_FLASH_MODE,
} camera_action_t;

#define INVALID_KEY_PRESS 0
#define BASE_OFFSET  ('Z' - 'A' + 1)
#define BASE_OFFSET_NUM  ('Z' - 'A' + 2)
#define PAD_TO_WORD(a)  (((a)+3)&~3)


#define SQCIF_WIDTH     128
#define SQCIF_HEIGHT     96
#define QCIF_WIDTH      176
#define QCIF_HEIGHT     144
#define QVGA_WIDTH      320
#define QVGA_HEIGHT     240
#define HD_THUMBNAIL_WIDTH      256
#define HD_THUMBNAIL_HEIGHT     144
#define CIF_WIDTH       352
#define CIF_HEIGHT      288
#define VGA_WIDTH       640
#define VGA_HEIGHT      480
#define WVGA_WIDTH      800
#define WVGA_HEIGHT     480
#define WVGA_PLUS_WIDTH      960
#define WVGA_PLUS_HEIGHT     720

#define MP1_WIDTH      1280
#define MP1_HEIGHT      960
#define MP2_WIDTH      1600
#define MP2_HEIGHT     1200
#define MP3_WIDTH      2048
#define MP3_HEIGHT     1536
#define MP5_WIDTH      2592
#define MP5_HEIGHT     1944

#define SVGA_WIDTH      800
#define SVGA_HEIGHT     600
#define XGA_WIDTH      1024
#define XGA_HEIGHT      768
#define HD720_WIDTH    1280
#define HD720_HEIGHT    720
#define HD720_PLUS_WIDTH    1440
#define HD720_PLUS_HEIGHT   1080
#define WXGA_WIDTH     1280
#define WXGA_HEIGHT     768
#define HD1080_WIDTH   1920
#define HD1080_HEIGHT  1080

typedef enum
{
  RESOLUTION_MIN         = 1,
  QCIF                  = RESOLUTION_MIN,
  QVGA                   = 2,
  VGA                    = 3,
  WVGA                   = 4,
  WVGA_PLUS              = 5,
  HD720                  = 6,
  HD720_PLUS             = 7,
  HD1080                 = 8,
  RESOLUTION_PREVIEW_VIDEO_MAX = HD1080,
  WXGA                   = 9,
  MP1                    = 10,
  MP2                    = 11,
  MP3                    = 12,
  MP5                    = 13,
  RESOLUTION_MAX         = MP5,
} Camera_Resolution;


typedef enum {
    WHITE_BALANCE_STATE = 1,
    WHITE_BALANCE_TEMPERATURE = 2,
    BRIGHTNESS_CTRL = 3,
    EV = 4,
    CONTRAST_CTRL = 5,
    SATURATION_CTRL = 6,
    SHARPNESS_CTRL = 7,
} Get_Ctrl_modes;

typedef enum {
    AUTO_EXP_FRAME_AVG = 1,
    AUTO_EXP_CENTER_WEIGHTED = 2,
    AUTO_EXP_SPOT_METERING = 3,
    AUTO_EXP_SMART_METERING = 4,
    AUTO_EXP_USER_METERING = 5,
    AUTO_EXP_SPOT_METERING_ADV = 6,
    AUTO_EXP_CENTER_WEIGHTED_ADV = 7,
} Exp_Metering_modes;

typedef enum {
  ISO_AUTO = 1,
  ISO_DEBLUR = 2,
  ISO_100 = 3,
  ISO_200 = 4,
  ISO_400 = 5,
  ISO_800 = 6,
  ISO_1600 = 7,
} ISO_modes;

typedef enum {
  BESTSHOT_AUTO = 1,
  BESTSHOT_ACTION = 2,
  BESTSHOT_PORTRAIT = 3,
  BESTSHOT_LANDSCAPE = 4,
  BESTSHOT_NIGHT = 5,
  BESTSHOT_NIGHT_PORTRAIT = 6,
  BESTSHOT_THEATRE = 7,
  BESTSHOT_BEACH = 8,
  BESTSHOT_SNOW = 9,
  BESTSHOT_SUNSET = 10,
  BESTSHOT_ANTISHAKE = 11,
  BESTSHOT_FIREWORKS = 12,
  BESTSHOT_SPORTS = 13,
  BESTSHOT_PARTY = 14,
  BESTSHOT_CANDLELIGHT = 15,
  BESTSHOT_ASD = 16,
  BESTSHOT_BACKLIGHT = 17,
  BESTSHOT_FLOWERS = 18,
  BESTSHOT_AR = 19,
  BESTSHOT_HDR = 20,
  BESTSHOT_MAX = 21,
}Bestshot_modes;

typedef enum {
    FLASH_MODE_OFF = 1,
    FLASH_MODE_AUTO = 2,
    FLASH_MODE_ON = 3,
    FLASH_MODE_TORCH = 4,
    FLASH_MODE_MAX = 5,
}Flash_modes;

typedef enum {
  WB_AUTO = 1,
  WB_INCANDESCENT = 2,
  WB_FLUORESCENT = 3,
  WB_WARM_FLUORESCENT = 4,
  WB_DAYLIGHT = 5,
  WB_CLOUDY_DAYLIGHT = 6,
  WB_TWILIGHT = 7,
  WB_SHADE = 8,
} White_Balance_modes;

typedef enum
{
  MENU_ID_MAIN,
  MENU_ID_PREVIEWVIDEORESOLUTIONCHANGE,
  MENU_ID_WHITEBALANCECHANGE,
  MENU_ID_EXPMETERINGCHANGE,
  MENU_ID_GET_CTRL_VALUE,
  MENU_ID_TOGGLEAFR,
  MENU_ID_ISOCHANGE,
  MENU_ID_BRIGHTNESSCHANGE,
  MENU_ID_CONTRASTCHANGE,
  MENU_ID_EVCHANGE,
  MENU_ID_SATURATIONCHANGE,
  MENU_ID_ZOOMCHANGE,
  MENU_ID_SHARPNESSCHANGE,
  MENU_ID_BESTSHOT,
  MENU_ID_FLASHMODE,
  MENU_ID_INVALID,
} menu_id_change_t;

typedef enum
{
  INCREASE_ZOOM      = 1,
  DECREASE_ZOOM      = 2,
  INCREASE_STEP_ZOOM = 3,
  DECREASE_STEP_ZOOM = 4,
} Camera_Zoom;

typedef enum
{
  INC_CONTRAST = 1,
  DEC_CONTRAST = 2,
} Camera_Contrast_changes;

typedef enum
{
  INC_BRIGHTNESS = 1,
  DEC_BRIGHTNESS = 2,
} Camera_Brightness_changes;

typedef enum
{
  INCREASE_EV = 1,
  DECREASE_EV = 2,
} Camera_EV_changes;

typedef enum {
  INC_SATURATION = 1,
  DEC_SATURATION = 2,
} Camera_Saturation_changes;

typedef enum
{
  INC_ISO = 1,
  DEC_ISO = 2,
} Camera_ISO_changes;

typedef enum
{
  INC_SHARPNESS = 1,
  DEC_SHARPNESS = 2,
} Camera_Sharpness_changes;

typedef enum {
  ZOOM_IN = 1,
  ZOOM_OUT = 2,
} Zoom_direction;

typedef struct{
    Camera_main_menu_t main_menu;
    char * menu_name;
} CAMERA_MAIN_MENU_TBL_T;

typedef struct{
    Camera_Resolution cs_id;
    uint16_t width;
    uint16_t  height;
    char * name;
    char * str_name;
} PREVIEW_DIMENSION_TBL_T;

typedef struct {
  White_Balance_modes wb_id;
  char * wb_name;
} WHITE_BALANCE_TBL_T;

typedef struct {
  Get_Ctrl_modes get_ctrl_id;
  char * get_ctrl_name;
} GET_CTRL_TBL_T;

typedef struct{
  Exp_Metering_modes exp_metering_id;
  char * exp_metering_name;
} EXP_METERING_TBL_T;

typedef struct {
  Bestshot_modes bs_id;
  char *name;
} BESTSHOT_MODE_TBT_T;

typedef struct {
  Flash_modes bs_id;
  char *name;
} FLASH_MODE_TBL_T;

typedef struct {
  ISO_modes iso_modes;
  char *iso_modes_name;
} ISO_TBL_T;

typedef struct {
  Zoom_direction zoom_direction;
  char * zoom_direction_name;
} ZOOM_TBL_T;

typedef struct {
  Camera_Sharpness_changes sharpness_change;
  char *sharpness_change_name;
} SHARPNESS_TBL_T;

typedef struct {
  Camera_Brightness_changes bc_id;
  char * brightness_name;
} CAMERA_BRIGHTNESS_TBL_T;

typedef struct {
  Camera_Contrast_changes cc_id;
  char * contrast_name;
} CAMERA_CONTRST_TBL_T;

typedef struct {
  Camera_EV_changes ec_id;
  char * EV_name;
} CAMERA_EV_TBL_T;

typedef struct {
  Camera_Saturation_changes sc_id;
  char * saturation_name;
} CAMERA_SATURATION_TBL_T;

typedef struct {
  Camera_Sharpness_changes bc_id;
  char * sharpness_name;
} CAMERA_SHARPNESS_TBL_T;

#endif /* __MM_QCAMERA_MAIN_MENU_H__ */
