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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>

#include "mm_qcamera_main_menu.h"
#include "mm_qcamera_app.h"
#include "mm_qcamera_dbg.h"

/*===========================================================================
 * Macro
 *===========================================================================*/
//TODO:check this Macros with current app.
#define VIDEO_FRAMES_NUM      4
#define THUMBNAIL_FRAMES_NUM  1
#define SNAPSHOT_FRAMES_NUM   1
#define MAX_NUM_FORMAT        32
#define ZOOM_STEP             2
#define ZOOM_MIN_VALUE        0

/*===========================================================================
 * Defines
 *===========================================================================*/

const CAMERA_MAIN_MENU_TBL_T camera_main_menu_tbl[] = {
    {STOP_CAMERA,                   "Stop preview/video and exit camera."},
    {PREVIEW_VIDEO_RESOLUTION,      "Preview/Video Resolution"},
    {SET_WHITE_BALANCE,          "Set white balance mode"},
    {SET_EXP_METERING,          "Set exposure metering mode"},
    {GET_CTRL_VALUE,              "Get control value menu"},
    {TOGGLE_AFR,                 "Toggle auto frame rate. Default fixed frame rate"},
    {SET_ISO,                 "ISO changes."},
    {BRIGHTNESS_GOTO_SUBMENU,                               "Brightness changes."},
    {CONTRAST_GOTO_SUBMENU,                                 "Contrast changes."},
    {EV_GOTO_SUBMENU,                                       "EV changes."},
    {SATURATION_GOTO_SUBMENU,                               "Saturation changes."},
    {SET_ZOOM,          "Set Digital Zoom."},
    {SET_SHARPNESS,          "Set Sharpness."},
    {TAKE_SINGLE_SNAPSHOT,       "Take a snapshot"},
    {TAKE_BURST_SNAPSHOT,       "Take burst snapshot"},
    {START_RECORDING,       "Start RECORDING"},
    {STOP_RECORDING,       "Stop RECORDING"},
    {BEST_SHOT,       "Set best-shot mode"},
    {LIVE_SHOT,       "Take a live snapshot"},
    {FLASH_MODES,       "Set Flash modes"},
    {PREVIEW_FLIP,       "Set preview Flip"},
    {FLIP_CAMERAS,       "Flip front and back camera"},
    {SET_JPEG_QUALITY,       "Set quality for JPEG files"},
};

const PREVIEW_DIMENSION_TBL_T preview_video_dimension_tbl[] = {
    {  QCIF,  QCIF_WIDTH,  QCIF_HEIGHT,  "QCIF",  "Preview/Video Resolution: QCIF <176x144>"},
    {  QVGA,  QVGA_WIDTH,  QVGA_HEIGHT,  "QVGA",  "Preview/Video Resolution: QVGA <320x240>"},
    {  VGA,   VGA_WIDTH,   VGA_HEIGHT,   "VGA",  "Preview/Video Resolution: VGA <640x480>"},
    {  WVGA,  WVGA_WIDTH,  WVGA_HEIGHT,  "WVGA",  "Preview/Video Resolution: WVGA <800x480>"},
    {  WVGA_PLUS,  WVGA_PLUS_WIDTH,  WVGA_PLUS_HEIGHT,  "WVGA_PLUS",  "Preview/Video Resolution: WVGA_PLUS <960x720>"},
    {  HD720, HD720_WIDTH, HD720_HEIGHT,  "HD720", "Preview/Video Resolution: HD720 <1280x720>"},
    {  HD720VDIS, HD720VDIS_WIDTH, HD720VDIS_HEIGHT,  "HD720VDIS", "Preview/Video Resolution: HD720VDIS <1536x864>"},
    {  HD720_PLUS, HD720_PLUS_WIDTH, HD720_PLUS_HEIGHT,  "HD720_PLUS", "Preview/Video Resolution: HD720_PLUS <1440x1080>"},
    {  HD720_PLUSVDIS, HD720_PLUSVDIS_WIDTH, HD720_PLUSVDIS_HEIGHT,  "HD720_PLUSVDIS", "Preview/Video Resolution: HD720_PLUSVDIS <1728x1296>"},
    {  HD1080, HD1080_WIDTH, HD1080_HEIGHT,  "HD1080", "Preview/Video Resolution: HD1080 <1920x1080>"},
    {  HD1080VDIS, HD1080VDIS_WIDTH, HD1080VDIS_HEIGHT,  "HD1080VDIS", "Preview/Video Resolution: HD1080VIDS <2304x1296>"},
};

const CAMERA_BRIGHTNESS_TBL_T brightness_change_tbl[] = {
    {INC_BRIGHTNESS, "Increase Brightness by one step."},
    {DEC_BRIGHTNESS, "Decrease Brightness by one step."},
};

const CAMERA_CONTRST_TBL_T contrast_change_tbl[] = {
    {INC_CONTRAST, "Increase Contrast by one step."},
    {DEC_CONTRAST, "Decrease Contrast by one step."},
};

const CAMERA_EV_TBL_T camera_EV_tbl[] = {
    {INCREASE_EV, "Increase EV by one step."},
    {DECREASE_EV, "Decrease EV by one step."},
};

const CAMERA_SATURATION_TBL_T camera_saturation_tbl[] = {
    {INC_SATURATION, "Increase Satuation by one step."},
    {DEC_SATURATION, "Decrease Satuation by one step."},
};

const CAMERA_SHARPNESS_TBL_T camera_sharpness_tbl[] = {
    {INC_SHARPNESS, "Increase Sharpness."},
    {DEC_SHARPNESS, "Decrease Sharpness."},
};

const WHITE_BALANCE_TBL_T white_balance_tbl[] = {
    {   WB_AUTO,               "White Balance - Auto"},
    {   WB_INCANDESCENT,       "White Balance - Incandescent"},
    {   WB_FLUORESCENT,        "White Balance - Fluorescent"},
    {   WB_WARM_FLUORESCENT,   "White Balance - Warm Fluorescent"},
    {   WB_DAYLIGHT,           "White Balance - Daylight"},
    {   WB_CLOUDY_DAYLIGHT,    "White Balance - Cloudy Daylight"},
    {   WB_TWILIGHT,           "White Balance - Twilight"},
    {   WB_SHADE,              "White Balance - Shade"},
};

const GET_CTRL_TBL_T get_ctrl_tbl[] = {
    {     WHITE_BALANCE_STATE,           "Get white balance state (auto/off)"},
    {     WHITE_BALANCE_TEMPERATURE,      "Get white balance temperature"},
    {     BRIGHTNESS_CTRL,      "Get brightness value"},
    {     EV,      "Get exposure value"},
    {     CONTRAST_CTRL,      "Get contrast value"},
    {     SATURATION_CTRL,      "Get saturation value"},
    {     SHARPNESS_CTRL,      "Get sharpness value"},
};

const EXP_METERING_TBL_T exp_metering_tbl[] = {
    {   AUTO_EXP_FRAME_AVG,          "Exposure Metering - Frame Average"},
    {   AUTO_EXP_CENTER_WEIGHTED,    "Exposure Metering - Center Weighted"},
    {   AUTO_EXP_SPOT_METERING,      "Exposure Metering - Spot Metering"},
    {   AUTO_EXP_SMART_METERING,     "Exposure Metering - Smart Metering"},
    {   AUTO_EXP_USER_METERING,      "Exposure Metering - User Metering"},
    {   AUTO_EXP_SPOT_METERING_ADV,  "Exposure Metering - Spot Metering Adv"},
    {   AUTO_EXP_CENTER_WEIGHTED_ADV, "Exposure Metering - Center Weighted Adv"},
};

const ISO_TBL_T iso_tbl[] = {
    {   ISO_AUTO, "ISO: Auto"},
    {   ISO_DEBLUR, "ISO: Deblur"},
    {   ISO_100, "ISO: 100"},
    {   ISO_200, "ISO: 200"},
    {   ISO_400, "ISO: 400"},
    {   ISO_800, "ISO: 800"},
    {   ISO_1600, "ISO: 1600"},
};

const PREVIEW_FLIP_T preview_flip_tbl[] = {
    {   FLIP_MODE_NONE, "Normal/Default"},
    {   FLIP_MODE_H, "Horizontal Flip"},
    {   FLIP_MODE_V, "Vertical Flip"},
    {   FLIP_MODE_V_H, "Vertical and Horizontal Flip"},
};

const ZOOM_TBL_T zoom_tbl[] = {
    {   ZOOM_IN, "Zoom In one step"},
    {   ZOOM_OUT, "Zoom Out one step"},
};

const BESTSHOT_MODE_TBT_T bestshot_mode_tbl[] = {
    {BESTSHOT_AUTO,           "Bestshot Mode: Auto"},
    {BESTSHOT_ACTION,         "Bestshot Mode: Action"},
    {BESTSHOT_PORTRAIT,       "Bestshot Mode: Portrait"},
    {BESTSHOT_LANDSCAPE,      "Bestshot Mode: Landscape"},
    {BESTSHOT_NIGHT,          "Bestshot Mode: Night"},
    {BESTSHOT_NIGHT_PORTRAIT, "Bestshot Mode: Night Portrait"},
    {BESTSHOT_THEATRE,        "Bestshot Mode: Theatre"},
    {BESTSHOT_BEACH,          "Bestshot Mode: Beach"},
    {BESTSHOT_SNOW,           "Bestshot Mode: Snow"},
    {BESTSHOT_SUNSET,         "Bestshot Mode: Sunset"},
    {BESTSHOT_ANTISHAKE,      "Bestshot Mode: Antishake"},
    {BESTSHOT_FIREWORKS,      "Bestshot Mode: Fireworks"},
    {BESTSHOT_SPORTS,         "Bestshot Mode: Sports"},
    {BESTSHOT_PARTY,          "Bestshot Mode: Party"},
    {BESTSHOT_CANDLELIGHT,    "Bestshot Mode: Candlelight"},
    {BESTSHOT_ASD,            "Bestshot Mode: ASD"},
    {BESTSHOT_BACKLIGHT,      "Bestshot Mode: Backlight"},
    {BESTSHOT_FLOWERS,        "Bestshot Mode: Flowers"},
    {BESTSHOT_AR,             "Bestshot Mode: Augmented Reality"},
    {BESTSHOT_HDR,            "Bestshot Mode: HDR"},
};

const FLASH_MODE_TBL_T flashmodes_tbl[] = {
    {   FLASH_MODE_OFF, "Flash Mode Off"},
    {   FLASH_MODE_AUTO, "Flash Mode Auto"},
    {   FLASH_MODE_ON, "Flash Mode On"},
    {   FLASH_MODE_TORCH, "Flash Mode Torch"},
};

/*===========================================================================
 * Forward declarations
 *===========================================================================*/
static int set_fps(int fps);
static int start_snapshot (void);
static int stop_snapshot (void);
void system_dimension_set(mm_camera_test_obj_t *test_obj);
/*===========================================================================
 * Static global variables
 *===========================================================================*/
USER_INPUT_DISPLAY_T input_display;
int preview_video_resolution_flag = 0;

//TODO: default values.
#if 0
int brightness = CAMERA_DEF_BRIGHTNESS;
int contrast = CAMERA_DEF_CONTRAST;
int saturation = CAMERA_DEF_SATURATION;
int sharpness = CAMERA_DEF_SHARPNESS;
#endif
int brightness = 0;
int contrast = 0;
int saturation = 0;
int sharpness = 0;
int cam_id = 0;
int ev_value = 0;

//TODO:
//fps_mode_t fps_mode = FPS_MODE_FIXED;
int zoom_level;
int zoom_max_value;
int is_rec = 0;

//TODO: find correct values of Contrast defines.
#define CAMERA_MIN_CONTRAST    0
#define CAMERA_DEF_CONTRAST    5
#define CAMERA_MAX_CONTRAST    10
#define CAMERA_CONTRAST_STEP   1

//TODO: find correct values of Brightness defines.
#define CAMERA_MIN_BRIGHTNESS  0
#define CAMERA_DEF_BRIGHTNESS  3
#define CAMERA_MAX_BRIGHTNESS  6
#define CAMERA_BRIGHTNESS_STEP 1

//TODO: find correct values of Saturation defines.
#define CAMERA_MIN_SATURATION  0
#define CAMERA_DEF_SATURATION  5
#define CAMERA_MAX_SATURATION  10
#define CAMERA_SATURATION_STEP 1

#define CAMERA_MIN_SHARPNESS 0
#define CAMERA_MAX_SHARPNESS 10
#define CAMERA_DEF_SHARPNESS 5
#define CAMERA_SHARPNESS_STEP 1

#define CAMERA_MAX_EXPOSURE 12
#define CAMERA_MIN_EXPOSURE -12
#define CAMERA_EXPOSURE_STEP 1

static int submain();

/*===========================================================================
 * FUNCTION    - keypress_to_event -
 *
 * DESCRIPTION:
 *==========================================================================*/
int keypress_to_event(char keypress)
{
    char out_buf = INVALID_KEY_PRESS;
    if ((keypress >= 'A' && keypress <= 'Z') ||
        (keypress >= 'a' && keypress <= 'z')) {
        out_buf = tolower(keypress);
        out_buf = out_buf - 'a' + 1;
    } else if (keypress >= '1' && keypress <= '9') {
        out_buf = keypress;
        out_buf = keypress - '1' + BASE_OFFSET_NUM;
    }
    return out_buf;
}

int next_menu(menu_id_change_t current_menu_id, char keypress, camera_action_t *action_id_ptr, int *action_param)
{
    char output_to_event;
    menu_id_change_t next_menu_id = MENU_ID_INVALID;
    * action_id_ptr = ACTION_NO_ACTION;

    output_to_event = keypress_to_event(keypress);
    CDBG("current_menu_id=%d\n", current_menu_id);
    CDBG("output_to_event=%d\n", output_to_event);
    switch (current_menu_id) {
        case MENU_ID_MAIN:
            switch (output_to_event) {
                case STOP_CAMERA:
                    * action_id_ptr = ACTION_STOP_CAMERA;
                    CDBG("STOP_CAMERA\n");
                    break;

                case PREVIEW_VIDEO_RESOLUTION:
                    next_menu_id = MENU_ID_PREVIEWVIDEORESOLUTIONCHANGE;
                    CDBG("next_menu_id = MENU_ID_PREVIEWVIDEORESOLUTIONCHANGE = %d\n", next_menu_id);
                    break;

                case SET_WHITE_BALANCE:
                    next_menu_id = MENU_ID_WHITEBALANCECHANGE;
                    CDBG("next_menu_id = MENU_ID_WHITEBALANCECHANGE = %d\n", next_menu_id);
                    break;

                case SET_EXP_METERING:
                    next_menu_id = MENU_ID_EXPMETERINGCHANGE;
                    CDBG("next_menu_id = MENU_ID_EXPMETERINGCHANGE = %d\n", next_menu_id);
                    break;

                case GET_CTRL_VALUE:
                    next_menu_id = MENU_ID_GET_CTRL_VALUE;
                    CDBG("next_menu_id = MENU_ID_GET_CTRL_VALUE = %d\n", next_menu_id);
                    break;

                case BRIGHTNESS_GOTO_SUBMENU:
                    next_menu_id = MENU_ID_BRIGHTNESSCHANGE;
                    CDBG("next_menu_id = MENU_ID_BRIGHTNESSCHANGE = %d\n", next_menu_id);
                    break;

                case CONTRAST_GOTO_SUBMENU:
                    next_menu_id = MENU_ID_CONTRASTCHANGE;
                    break;

                case EV_GOTO_SUBMENU:
                    next_menu_id = MENU_ID_EVCHANGE;
                    break;

                case SATURATION_GOTO_SUBMENU:
                    next_menu_id = MENU_ID_SATURATIONCHANGE;
                    break;

                case TOGGLE_AFR:
                    * action_id_ptr = ACTION_TOGGLE_AFR;
                    CDBG("next_menu_id = MENU_ID_TOGGLEAFR = %d\n", next_menu_id);
                    break;

                case SET_ISO:
                    next_menu_id = MENU_ID_ISOCHANGE;
                    CDBG("next_menu_id = MENU_ID_ISOCHANGE = %d\n", next_menu_id);
                    break;

                case PREVIEW_FLIP:
                    next_menu_id = MENU_ID_PREVIEWFLIP;
                    CDBG("next_menu_id = MENU_ID_PREVIEWFLIP  = %d\n", next_menu_id);
                    break;

                case SET_ZOOM:
                    next_menu_id = MENU_ID_ZOOMCHANGE;
                    CDBG("next_menu_id = MENU_ID_ZOOMCHANGE = %d\n", next_menu_id);
                    break;

                case BEST_SHOT:
                    next_menu_id = MENU_ID_BESTSHOT;
                    CDBG("next_menu_id = MENU_ID_BESTSHOT = %d\n", next_menu_id);
                    break;

                case LIVE_SHOT:
                    * action_id_ptr = ACTION_TAKE_LIVE_SNAPSHOT;
                    CDBG("\nTaking Live snapshot\n");
                    break;

                case FLASH_MODES:
                    next_menu_id = MENU_ID_FLASHMODE;
                    CDBG("next_menu_id = MENU_ID_FLASHMODE = %d\n", next_menu_id);
                    break;

                case SET_SHARPNESS:
                    next_menu_id = MENU_ID_SHARPNESSCHANGE;
                    CDBG("next_menu_id = MENU_ID_SHARPNESSCHANGE = %d\n", next_menu_id);
                    break;

                case TAKE_SINGLE_SNAPSHOT:
                    * action_id_ptr = ACTION_TAKE_SINGLE_SNAPSHOT;
                    printf("\n Taking YUV snapshot\n");
                    break;

                case TAKE_BURST_SNAPSHOT:
                    * action_id_ptr = ACTION_TAKE_BURST_SNAPSHOT;
                    printf("\n Taking Burst snapshot\n");
                    break;

                case START_RECORDING:
                    * action_id_ptr = ACTION_START_RECORDING;
                    CDBG("Start recording\n");
                    break;

                case STOP_RECORDING:
                    * action_id_ptr = ACTION_STOP_RECORDING;
                    CDBG("Stop recording\n");
                    break;

                case FLIP_CAMERAS:
                    * action_id_ptr = ACTION_FLIP_CAMERAS;
                    CDBG("Flipping between front and back camera\n");
                    break;

                case SET_JPEG_QUALITY:
                    * action_id_ptr = ACTION_SET_JPEG_QUALITY;
                    CDBG("Set JPEG quality\n");
                    break;

                default:
                    next_menu_id = MENU_ID_MAIN;
                    CDBG("next_menu_id = MENU_ID_MAIN = %d\n", next_menu_id);
                    break;
            }
            break;

        case MENU_ID_PREVIEWVIDEORESOLUTIONCHANGE:
            printf("MENU_ID_PREVIEWVIDEORESOLUTIONCHANGE\n");
            * action_id_ptr = ACTION_PREVIEW_VIDEO_RESOLUTION;
            if (output_to_event > RESOLUTION_PREVIEW_VIDEO_MAX ||
                output_to_event < RESOLUTION_MIN) {
                next_menu_id = current_menu_id;
            } else {
                next_menu_id = MENU_ID_MAIN;
                * action_param = output_to_event;
            }
            break;

        case MENU_ID_WHITEBALANCECHANGE:
            printf("MENU_ID_WHITEBALANCECHANGE\n");
            * action_id_ptr = ACTION_SET_WHITE_BALANCE;
            if (output_to_event > 0 &&
                output_to_event <= sizeof(white_balance_tbl) / sizeof(white_balance_tbl[0])) {
                next_menu_id = MENU_ID_MAIN;
                * action_param = output_to_event;
            } else {
                next_menu_id = current_menu_id;
            }
            break;

        case MENU_ID_EXPMETERINGCHANGE:
            printf("MENU_ID_EXPMETERINGCHANGE\n");
            * action_id_ptr = ACTION_SET_EXP_METERING;
            if (output_to_event > 0 &&
                output_to_event <= sizeof(exp_metering_tbl) / sizeof(exp_metering_tbl[0])) {
                next_menu_id = MENU_ID_MAIN;
                * action_param = output_to_event;
            } else {
                next_menu_id = current_menu_id;
            }
            break;

        case MENU_ID_GET_CTRL_VALUE:
            printf("MENU_ID_GET_CTRL_VALUE\n");
            * action_id_ptr = ACTION_GET_CTRL_VALUE;
            if (output_to_event > 0 &&
                output_to_event <= sizeof(get_ctrl_tbl) / sizeof(get_ctrl_tbl[0])) {
                next_menu_id = MENU_ID_MAIN;
                * action_param = output_to_event;
            } else {
                next_menu_id = current_menu_id;
            }
            break;

        case MENU_ID_BRIGHTNESSCHANGE:
            switch (output_to_event) {
                case INC_BRIGHTNESS:
                    * action_id_ptr = ACTION_BRIGHTNESS_INCREASE;
                    next_menu_id = MENU_ID_MAIN;
                    break;

                case DEC_BRIGHTNESS:
                    * action_id_ptr = ACTION_BRIGHTNESS_DECREASE;
                    next_menu_id = MENU_ID_MAIN;
                    break;

                default:
                    next_menu_id = MENU_ID_BRIGHTNESSCHANGE;
                    break;
            }
            break;

        case MENU_ID_CONTRASTCHANGE:
            switch (output_to_event) {
                case INC_CONTRAST:
                    * action_id_ptr = ACTION_CONTRAST_INCREASE;
                    next_menu_id = MENU_ID_MAIN;
                    break;

                case DEC_CONTRAST:
                    * action_id_ptr = ACTION_CONTRAST_DECREASE;
                    next_menu_id = MENU_ID_MAIN;
                    break;

                default:
                    next_menu_id = MENU_ID_CONTRASTCHANGE;
                    break;
            }
            break;

        case MENU_ID_EVCHANGE:
            switch (output_to_event) {
                case INCREASE_EV:
                    * action_id_ptr = ACTION_EV_INCREASE;
                    next_menu_id = MENU_ID_MAIN;
                    break;

                case DECREASE_EV:
                    * action_id_ptr = ACTION_EV_DECREASE;
                    next_menu_id = MENU_ID_MAIN;
                    break;

                default:
                    next_menu_id = MENU_ID_EVCHANGE;
                    break;
            }
            break;

        case MENU_ID_SATURATIONCHANGE:
            switch (output_to_event) {
                case INC_SATURATION:
                    * action_id_ptr = ACTION_SATURATION_INCREASE;
                    next_menu_id = MENU_ID_MAIN;
                    break;

                case DEC_SATURATION:
                    * action_id_ptr = ACTION_SATURATION_DECREASE;
                    next_menu_id = MENU_ID_MAIN;
                    break;

                default:
                    next_menu_id = MENU_ID_EVCHANGE;
                    break;
            }
            break;

        case MENU_ID_ISOCHANGE:
            printf("MENU_ID_ISOCHANGE\n");
            * action_id_ptr = ACTION_SET_ISO;
            if (output_to_event > 0 &&
                output_to_event <= sizeof(iso_tbl) / sizeof(iso_tbl[0])) {
                next_menu_id = MENU_ID_MAIN;
                * action_param = output_to_event;
            } else {
                next_menu_id = current_menu_id;
            }
            break;

        case MENU_ID_PREVIEWFLIP:
            * action_id_ptr = ACTION_PREVIEW_FLIP;
            if (output_to_event > 0 &&
                output_to_event <= sizeof(preview_flip_tbl) / sizeof(preview_flip_tbl[0])) {
                next_menu_id = MENU_ID_MAIN;
                * action_param = output_to_event - 1;
            } else {
                next_menu_id = current_menu_id;
            }
            break;

        case MENU_ID_ZOOMCHANGE:
            * action_id_ptr = ACTION_SET_ZOOM;
            if (output_to_event > 0 &&
                output_to_event <= sizeof(zoom_tbl) / sizeof(zoom_tbl[0])) {
                next_menu_id = MENU_ID_MAIN;
                * action_param = output_to_event;
            } else {
                next_menu_id = current_menu_id;
            }
            break;

        case MENU_ID_SHARPNESSCHANGE:
            switch (output_to_event) {
                case INC_SHARPNESS:
                    * action_id_ptr = ACTION_SHARPNESS_INCREASE;
                    next_menu_id = MENU_ID_MAIN;
                    break;
                case DEC_SHARPNESS:
                    * action_id_ptr = ACTION_SHARPNESS_DECREASE;
                    next_menu_id = MENU_ID_MAIN;
                    break;
                default:
                    next_menu_id = MENU_ID_SHARPNESSCHANGE;
                    break;
            }
            break;

        case MENU_ID_BESTSHOT:
            if (output_to_event >= BESTSHOT_MAX) {
                next_menu_id = current_menu_id;
                * action_id_ptr = ACTION_NO_ACTION;
            } else {
                next_menu_id = MENU_ID_MAIN;
                * action_id_ptr = ACTION_SET_BESTSHOT_MODE;
                * action_param = output_to_event;
            }
            break;

        case MENU_ID_FLASHMODE:
            if (output_to_event >= FLASH_MODE_MAX) {
                next_menu_id = current_menu_id;
                * action_id_ptr = ACTION_NO_ACTION;
            } else {
                next_menu_id = MENU_ID_MAIN;
                * action_id_ptr = ACTION_SET_FLASH_MODE;
                * action_param = output_to_event;
            }
            break;

        default:
            CDBG("menu id is wrong: %d\n", current_menu_id);
            break;
    }

    return next_menu_id;
}

/*===========================================================================
 * FUNCTION    - print_menu_preview_video -
 *
 * DESCRIPTION:
 * ===========================================================================*/
static void print_menu_preview_video(void)
{
    unsigned int i;
    if (!is_rec) {
        printf("\n");
        printf("===========================================\n");
        printf("      Camera is in preview/video mode now        \n");
        printf("===========================================\n\n");
    } else {
        printf("\n");
        printf("===========================================\n");
        printf("      Camera is in RECORDING mode now       \n");
        printf("        Press 'Q' To Stop Recording          \n");
        printf("        Press 'S' To Take Live Snapshot       \n");
        printf("===========================================\n\n");
    }
    char menuNum = 'A';
    for (i = 0; i < sizeof(camera_main_menu_tbl) / sizeof(camera_main_menu_tbl[0]); i++) {
        if (i == BASE_OFFSET) {
            menuNum = '1';
        }

        printf("%c.  %s\n", menuNum, camera_main_menu_tbl[i].menu_name);
        menuNum++;
    }

    printf("\nPlease enter your choice: ");

    return;
}

static void camera_preview_video_resolution_change_tbl(void)
{
    unsigned int i;

    printf("\n");
    printf("==========================================================\n");
    printf("      Camera is in preview/video resolution mode       \n");
    printf("==========================================================\n\n");

    char previewVideomenuNum = 'A';
    for (i = 0; i < sizeof(preview_video_dimension_tbl) /
         sizeof(preview_video_dimension_tbl[0]); i++) {
        printf("%c.  %s\n", previewVideomenuNum,
               preview_video_dimension_tbl[i].str_name);
        previewVideomenuNum++;
    }

    printf("\nPlease enter your choice for Preview/Video Resolution: ");
    return;
}

static void camera_preview_video_wb_change_tbl(void)
{
    unsigned int i;
    printf("\n");
    printf("==========================================================\n");
    printf("      Camera is in white balance change mode       \n");
    printf("==========================================================\n\n");

    char submenuNum = 'A';
    for (i = 0 ; i < sizeof(white_balance_tbl) /
         sizeof(white_balance_tbl[0]); i++) {
        printf("%c.  %s\n", submenuNum, white_balance_tbl[i].wb_name);
        submenuNum++;
    }
    printf("\nPlease enter your choice for White Balance modes: ");
    return;
}

static void camera_preview_video_get_ctrl_value_tbl(void)
{
    unsigned int i;
    printf("\n");
    printf("==========================================================\n");
    printf("      Camera is in get control value mode       \n");
    printf("==========================================================\n\n");

    char submenuNum = 'A';
    for (i = 0 ; i < sizeof(get_ctrl_tbl) /
         sizeof(get_ctrl_tbl[0]); i++) {
        printf("%c.  %s\n", submenuNum, get_ctrl_tbl[i].get_ctrl_name);
        submenuNum++;
    }
    printf("\nPlease enter your choice for control value you want to get: ");
    return;
}

static void camera_preview_video_exp_metering_change_tbl(void)
{
    unsigned int i;
    printf("\n");
    printf("==========================================================\n");
    printf("      Camera is in exposure metering change mode       \n");
    printf("==========================================================\n\n");

    char submenuNum = 'A';
    for (i = 0 ; i < sizeof(exp_metering_tbl) /
         sizeof(exp_metering_tbl[0]); i++) {
        printf("%c.  %s\n", submenuNum, exp_metering_tbl[i].exp_metering_name);
        submenuNum++;
    }
    printf("\nPlease enter your choice for exposure metering modes: ");
    return;
}

static void camera_contrast_change_tbl(void)
{
    unsigned int i;

    printf("\n");
    printf("==========================================================\n");
    printf("      Camera is in change contrast resolution mode       \n");
    printf("==========================================================\n\n");

    char contrastmenuNum = 'A';
    for (i = 0; i < sizeof(contrast_change_tbl) /
         sizeof(contrast_change_tbl[0]); i++) {
        printf("%c.  %s\n", contrastmenuNum,
               contrast_change_tbl[i].contrast_name);
        contrastmenuNum++;
    }

    printf("\nPlease enter your choice for contrast Change: ");
    return;
}

static void camera_EV_change_tbl(void)
{
    unsigned int i;

    printf("\n");
    printf("===========================================\n");
    printf("      Camera is in EV change mode now       \n");
    printf("===========================================\n\n");

    char submenuNum = 'A';
    for (i = 0; i < sizeof(camera_EV_tbl) / sizeof(camera_EV_tbl[0]); i++) {
        printf("%c.  %s\n", submenuNum, camera_EV_tbl[i].EV_name);
        submenuNum++;
    }

    printf("\nPlease enter your choice for EV changes: ");
    return;
}

static void camera_preview_video_zoom_change_tbl(void)
{
    unsigned int i;
    zoom_max_value = MAX_ZOOMS_CNT;
    printf("\nCurrent Zoom Value = %d ,Max Zoom Value = %d\n", zoom_level, zoom_max_value);
    char submenuNum = 'A';
    for (i = 0 ; i < sizeof(zoom_tbl) /
         sizeof(zoom_tbl[0]); i++) {
        printf("%c.  %s\n", submenuNum, zoom_tbl[i].zoom_direction_name);
        submenuNum++;
    }
    printf("\nPlease enter your choice for zoom change direction: ");
    return;
}

static void camera_brightness_change_tbl(void)
{
    unsigned int i;

    printf("\n");
    printf("==========================================================\n");
    printf("      Camera is in change brightness mode       \n");
    printf("==========================================================\n\n");

    char brightnessmenuNum = 'A';
    for (i = 0; i < sizeof(brightness_change_tbl) /
         sizeof(brightness_change_tbl[0]); i++) {
        printf("%c.  %s\n", brightnessmenuNum,
               brightness_change_tbl[i].brightness_name);
        brightnessmenuNum++;
    }

    printf("\nPlease enter your choice for Brightness Change: ");
    return;
}

static void camera_saturation_change_tbl(void)
{
    unsigned int i;

    printf("\n");
    printf("==========================================================\n");
    printf("      Camera is in change saturation mode       \n");
    printf("==========================================================\n\n");

    char saturationmenuNum = 'A';
    for (i = 0; i < sizeof(camera_saturation_tbl) /
         sizeof(camera_saturation_tbl[0]); i++) {
        printf("%c.  %s\n", saturationmenuNum,
               camera_saturation_tbl[i].saturation_name);
        saturationmenuNum++;
    }

    printf("\nPlease enter your choice for Saturation Change: ");
    return;
}

char *set_preview_video_dimension_tbl(Camera_Resolution cs_id, uint16_t *width, uint16_t *height)
{
    unsigned int i;
    char *ptr = NULL;
    for (i = 0; i < sizeof(preview_video_dimension_tbl) /
         sizeof(preview_video_dimension_tbl[0]); i++) {
        if (cs_id == preview_video_dimension_tbl[i].cs_id) {
            *width = preview_video_dimension_tbl[i].width;
            *height = preview_video_dimension_tbl[i].height;
            ptr = preview_video_dimension_tbl[i].name;
            break;
        }
    }
    return ptr;
}

static void camera_preview_video_iso_change_tbl(void)
{
    unsigned int i;
    printf("\n");
    printf("==========================================================\n");
    printf("      Camera is in ISO change mode       \n");
    printf("==========================================================\n\n");

    char submenuNum = 'A';
    for (i = 0 ; i < sizeof(iso_tbl) /
         sizeof(iso_tbl[0]); i++) {
        printf("%c.  %s\n", submenuNum, iso_tbl[i].iso_modes_name);
        submenuNum++;
    }
    printf("\nPlease enter your choice for iso modes: ");
    return;
}


static void camera_preview_flip_tbl(void)
{
    unsigned int i;
    printf("\n");
    printf("==========================================================\n");
    printf("      Camera is in preview flip change mode       \n");
    printf("==========================================================\n\n");

    char submenuNum = 'A';
    for (i = 0 ; i < sizeof(preview_flip_tbl) /
         sizeof(preview_flip_tbl[0]); i++) {
        printf("%c.  %s\n", submenuNum, preview_flip_tbl[i].preview_flip_name);
        submenuNum++;
    }
    printf("\nPlease enter your choice for preview flip: ");
    return;
}


static void camera_preview_video_sharpness_change_tbl(void)
{
    unsigned int i;
    printf("\n");
    printf("==========================================================\n");
    printf("      Camera is in sharpness change mode       \n");
    printf("==========================================================\n\n");

    char submenuNum = 'A';
    for (i = 0 ; i < sizeof(camera_sharpness_tbl) /
         sizeof(camera_sharpness_tbl[0]); i++) {
        printf("%c.  %s\n", submenuNum, camera_sharpness_tbl[i].sharpness_name);
        submenuNum++;
    }
    printf("\nPlease enter your choice for sharpness modes: ");
    return;
}

static void camera_set_bestshot_tbl(void)
{
    unsigned int i;

    printf("\n");
    printf("===========================================\n");
    printf("      Camera is in set besthot mode now       \n");
    printf("===========================================\n\n");


    char bsmenuNum = 'A';
    for (i = 0; i < sizeof(bestshot_mode_tbl) / sizeof(bestshot_mode_tbl[0]); i++) {
        printf("%c.  %s\n", bsmenuNum,
               bestshot_mode_tbl[i].name);
        bsmenuNum++;
    }

    printf("\nPlease enter your choice of Bestshot Mode: ");
    return;
}

static void camera_set_flashmode_tbl(void)
{
    unsigned int i;

    printf("\n");
    printf("===========================================\n");
    printf("      Camera is in set flash mode now       \n");
    printf("===========================================\n\n");


    char bsmenuNum = 'A';
    for (i = 0; i < sizeof(flashmodes_tbl) / sizeof(flashmodes_tbl[0]); i++) {
        printf("%c.  %s\n", bsmenuNum,
               flashmodes_tbl[i].name);
        bsmenuNum++;
    }

    printf("\nPlease enter your choice of Bestshot Mode: ");
    return;
}

/*===========================================================================
 * FUNCTION     - increase_contrast -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int increase_contrast (mm_camera_test_obj_t *test_obj)
{
    contrast += CAMERA_CONTRAST_STEP;
    if (contrast > CAMERA_MAX_CONTRAST) {
        contrast = CAMERA_MAX_CONTRAST;
        printf("Reached max CONTRAST. \n");
    }
    printf("Increase Contrast to %d\n", contrast);

    return mm_app_set_params(test_obj, CAM_INTF_PARM_CONTRAST, sizeof(contrast), &contrast);
}

/*===========================================================================
 * FUNCTION     - decrease_contrast -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int decrease_contrast (mm_camera_test_obj_t *test_obj)
{
    contrast -= CAMERA_CONTRAST_STEP;
    if (contrast < CAMERA_MIN_CONTRAST) {
        contrast = CAMERA_MIN_CONTRAST;
        printf("Reached min CONTRAST. \n");
    }
    printf("Decrease Contrast to %d\n", contrast);

    return mm_app_set_params(test_obj, CAM_INTF_PARM_CONTRAST, sizeof(contrast), &contrast);
}

/*===========================================================================
 * FUNCTION     - decrease_brightness -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int decrease_brightness (mm_camera_test_obj_t *test_obj)
{
    brightness -= CAMERA_BRIGHTNESS_STEP;
    if (brightness < CAMERA_MIN_BRIGHTNESS) {
        brightness = CAMERA_MIN_BRIGHTNESS;
        printf("Reached min BRIGHTNESS. \n");
    }
    printf("Decrease Brightness to %d\n", brightness);

    return mm_app_set_params(test_obj, CAM_INTF_PARM_BRIGHTNESS, sizeof(brightness), &brightness);
}

/*===========================================================================
 * FUNCTION     - increase_brightness -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int increase_brightness (mm_camera_test_obj_t *test_obj)
{
    brightness += CAMERA_BRIGHTNESS_STEP;
    if (brightness > CAMERA_MAX_BRIGHTNESS) {
        brightness = CAMERA_MAX_BRIGHTNESS;
        printf("Reached max BRIGHTNESS. \n");
    }
    printf("Increase Brightness to %d\n", brightness);

    return mm_app_set_params(test_obj, CAM_INTF_PARM_BRIGHTNESS, sizeof(brightness), &brightness);
}

/*===========================================================================
 * FUNCTION     - increase_EV -
 *
 * DESCRIPTION:
 * ===========================================================================*/

int increase_EV (mm_camera_test_obj_t *test_obj)
{
    ev_value += 1;

    if(ev_value >= CAMERA_MAX_EXPOSURE) {
        ev_value = CAMERA_MAX_EXPOSURE;
        CDBG_HIGH("%s: Reached Max exposure: %d. Cannot increase further", __func__, ev_value);
    }

    return mm_app_set_params(test_obj, CAM_INTF_PARM_EXPOSURE_COMPENSATION, sizeof(ev_value), &ev_value);
}

/*===========================================================================
 * FUNCTION     - decrease_EV -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int decrease_EV (mm_camera_test_obj_t *test_obj)
{
    ev_value -= 1;

    if(ev_value < CAMERA_MIN_EXPOSURE) {
        ev_value = CAMERA_MIN_EXPOSURE;
        CDBG_HIGH("%s: Reached Min exposure: %d Cannot decrease further", __func__, ev_value);
    }

    return mm_app_set_params(test_obj, CAM_INTF_PARM_EXPOSURE_COMPENSATION, sizeof(ev_value), &ev_value);
}

/*===========================================================================
 * FUNCTION     - increase_saturation -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int increase_saturation (mm_camera_test_obj_t *test_obj)
{
    saturation += CAMERA_SATURATION_STEP;

    if (saturation > CAMERA_MAX_SATURATION) {
        saturation = CAMERA_MAX_SATURATION;
        CDBG_HIGH("%s: Reached max saturation: %d", __func__, saturation);
    }

    CDBG("%s:Increase Saturation to %d", __func__, saturation);

    return mm_app_set_params(test_obj, CAM_INTF_PARM_SATURATION, sizeof(saturation), &saturation);
}

/*===========================================================================
 * FUNCTION     - decrease_saturation -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int decrease_saturation (mm_camera_test_obj_t *test_obj)
{
    saturation -= CAMERA_SATURATION_STEP;

    if (saturation < CAMERA_MIN_SATURATION) {
        saturation = CAMERA_MIN_SATURATION;
        CDBG_HIGH("%s: Reached min saturation: %d", __func__, saturation);
    }

    CDBG("Decrease Saturation to %d", __func__, saturation);

    return mm_app_set_params(test_obj, CAM_INTF_PARM_SATURATION, sizeof(saturation), &saturation);
}

void set_jpeg_quality (mm_camera_test_obj_t *test_obj)
{
    printf("\nEnter JPEG Quality Value between 1 - 100: ");
    scanf("%d", &(test_obj->app_handle->jpeg_quality));

    if(test_obj->app_handle->jpeg_quality < 1 || test_obj->app_handle->jpeg_quality > 100) {
     CDBG_HIGH("%s: Wrong JPEG quality value: %d, resetting to default", __func__, test_obj->app_handle->jpeg_quality);
     test_obj->app_handle->jpeg_quality = DEFAULT_JPEG_QUALITY;
    }

}

/*===========================================================================
 * FUNCTION    - main -
 *
 * DESCRIPTION:
 *==========================================================================*/
int menu_based_test_main(mm_camera_app_t *cam_app)
{
    int keep_on_going = 1;
    int rc = 0;
    int mode = 0;

    mm_camera_test_obj_t test_obj;
    memset(&test_obj, 0, sizeof(mm_camera_test_obj_t));

    do {
        rc = mm_app_open(cam_app, cam_id, &test_obj);
        assert(MM_CAMERA_OK == rc);

        CDBG("Camera opened successfully!!\n");

        keep_on_going = submain(&test_obj);

        rc = mm_app_close(&test_obj);
        assert(rc == MM_CAMERA_OK);

    } while ( keep_on_going );

    CDBG_HIGH("Exiting app\n");
    return 0;
}


/*===========================================================================
 * FUNCTION     - submain -
 *
 * DESCRIPTION:
 * ===========================================================================*/
static int submain(mm_camera_test_obj_t *test_obj)
{
    int rc = 0;
    int back_mainflag = 0;
    char tc_buf[3];
    menu_id_change_t current_menu_id = MENU_ID_MAIN, next_menu_id;
    camera_action_t action_id;
    int action_param;
    int i;

    //update preview width and height if user updated it
    if (preview_video_resolution_flag == 1) {
        test_obj->app_handle->preview_width = input_display.user_input_display_width;
        test_obj->app_handle->preview_height = input_display.user_input_display_height;
    } else {
        input_display.user_input_display_width = test_obj->app_handle->preview_width;
        input_display.user_input_display_height = test_obj->app_handle->preview_height;
    }

    //if video/preview width is greater than what display can support force no display mode
    if(input_display.user_input_display_width > MM_QCAMERA_APP_MAX_DISPLAY_WIDTH) {
        CDBG_HIGH("\n\t!!Video/Preview will not be displayed!!\nThe resolution exceeds max display supported width");
        CDBG_HIGH("Please check the file dump to verify output\n");
        test_obj->app_handle->no_display = true;
    } else {
        test_obj->app_handle->no_display = false;
    }

    rc = mm_app_start_preview(test_obj);
    assert(MM_CAMERA_OK == rc);

    uint32_t param = CAM_AEC_MODE_FRAME_AVERAGE;
    //Default Auto exposure.
    mm_app_set_params(test_obj, CAM_INTF_PARM_AEC_ALGO_TYPE, sizeof(param), &param);


    do {
        print_current_menu (current_menu_id);
        fgets(tc_buf, 3, stdin);

        next_menu_id = next_menu(current_menu_id, tc_buf[0], & action_id, & action_param);

        if (next_menu_id != MENU_ID_INVALID) {
            current_menu_id = next_menu_id;
        }
        if (action_id == ACTION_NO_ACTION) {
            continue;
        }

        switch (action_id) {
            case ACTION_STOP_CAMERA:
                CDBG("ACTION_STOP_CAMERA \n");
                break;

            case ACTION_PREVIEW_VIDEO_RESOLUTION:
                back_mainflag = 1;
                CDBG("Selection for the preview/video resolution change\n");
                preview_video_resolution (test_obj, action_param);
                break;

            case ACTION_SET_WHITE_BALANCE:
                CDBG("Selection for the White Balance changes\n");
                set_whitebalance(test_obj, action_param);
                break;

            case ACTION_SET_EXP_METERING:
                CDBG("Selection for the Exposure Metering changes\n");
                set_exp_metering(test_obj, action_param);
                break;

            case ACTION_GET_CTRL_VALUE:
                CDBG("Selection for getting control value\n");
                get_ctrl_value(action_param);
                break;

            case ACTION_BRIGHTNESS_INCREASE:
                printf("Increase brightness\n");
                increase_brightness(test_obj);
                break;

            case ACTION_BRIGHTNESS_DECREASE:
                printf("Decrease brightness\n");
                decrease_brightness(test_obj);
                break;

            case ACTION_CONTRAST_INCREASE:
                CDBG("Selection for the contrast increase\n");
                increase_contrast (test_obj);
                break;

            case ACTION_CONTRAST_DECREASE:
                CDBG("Selection for the contrast decrease\n");
                decrease_contrast (test_obj);
                break;

            case ACTION_EV_INCREASE:
                CDBG("Selection for the EV increase\n");
                increase_EV (test_obj);
                break;

            case ACTION_EV_DECREASE:
                CDBG("Selection for the EV decrease\n");
                decrease_EV (test_obj);
                break;

            case ACTION_SATURATION_INCREASE:
                CDBG("Selection for the EV increase\n");
                increase_saturation (test_obj);
                break;

            case ACTION_SATURATION_DECREASE:
                CDBG("Selection for the EV decrease\n");
                decrease_saturation (test_obj);
                break;

            case ACTION_TOGGLE_AFR:
                CDBG("Select for auto frame rate toggling\n");
                toggle_afr();
                break;

            case ACTION_SET_ISO:
                CDBG("Select for ISO changes\n");
                set_iso(test_obj, action_param);
                break;

            case ACTION_PREVIEW_FLIP:
                CDBG("Select for PREVIEW Flip changes\n");
                flip_preview(test_obj, action_param);
                back_mainflag = 1;
                break;

            case ACTION_FLIP_CAMERAS:
                CDBG("Select front back camera flip\n");
                flip_cameras(test_obj, action_param);
                back_mainflag = 1;
                break;

            case ACTION_SET_JPEG_QUALITY:
                CDBG("Select front back camera flip\n");
                set_jpeg_quality(test_obj);
                break;

            case ACTION_SET_ZOOM:
                CDBG("Selection for the zoom direction changes\n");
                set_zoom(test_obj , action_param);
                break;

            case ACTION_SHARPNESS_INCREASE:
                CDBG("Selection for sharpness increase\n");
                increase_sharpness(test_obj);
                break;

            case ACTION_SHARPNESS_DECREASE:
                CDBG("Selection for sharpness decrease\n");
                decrease_sharpness(test_obj);
                break;

            case ACTION_TAKE_SINGLE_SNAPSHOT:
                CDBG("\n Selection for Single snapshot\n");
                mm_app_take_picture(test_obj, 0);
                break;

            case ACTION_TAKE_BURST_SNAPSHOT:
                CDBG("\n Selection for Burst snapshot\n");
                mm_app_take_picture(test_obj, 1);
                break;

            case ACTION_START_RECORDING:
                //CDBG_HIGH("Start recording action\n");
                printf("\n!Not implemented. Pls use manual mode to test this feature!\n");

                break;
            case ACTION_STOP_RECORDING:
                //CDBG("Stop recording action\n");
                printf("\n!Not implemented. Pls use manual mode to test this feature!\n");
                break;

            case ACTION_SET_BESTSHOT_MODE:
                CDBG("Selection for bestshot\n");
                set_bestshot_mode(test_obj, action_param);
                break;

            case ACTION_TAKE_LIVE_SNAPSHOT:
                printf("\n!Not implemented. Pls use manual mode to test this feature!\n");
                break;

            case ACTION_SET_FLASH_MODE:
                CDBG("\n Selection for flashmode\n");
                set_flash_mode(test_obj, action_param);
                break;

            case ACTION_NO_ACTION:
                CDBG("Go back to main menu");
                break;

            default:
                CDBG("\n\n!!!!!WRONG INPUT: %d!!!!\n", action_id);
                break;
        }

        usleep(1000 * 1000);
        CDBG("action_id = %d\n", action_id);

    } while ((action_id != ACTION_STOP_CAMERA) &&
             (action_id != ACTION_PREVIEW_VIDEO_RESOLUTION) &&
             (action_id != ACTION_PREVIEW_FLIP) &&
             (action_id != ACTION_FLIP_CAMERAS));

    action_id = ACTION_NO_ACTION;

    rc = mm_app_stop_preview(test_obj);
    assert(MM_CAMERA_OK == rc);

    return back_mainflag;

ERROR:
    back_mainflag = 0;
    return back_mainflag;
}


/*===========================================================================
 * FUNCTION     - preview_resolution -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int preview_video_resolution (mm_camera_test_obj_t *test_obj, int preview_video_action_param)
{
    char *resolution_name;
    CDBG(" Selecting the action for preview/video resolution = %d ", preview_video_action_param);
    resolution_name = set_preview_video_dimension_tbl(preview_video_action_param,
                      & input_display.user_input_display_width,
                      & input_display.user_input_display_height);

    CDBG("Selected preview/video resolution is %s", resolution_name);

    if (resolution_name == NULL) {
        CDBG("main:%d set_preview_dimension failed!\n", __LINE__);
        goto ERROR;
    }

    CDBG_HIGH("\nSelected Preview Resolution: display_width = %d, display_height = %d",
              input_display.user_input_display_width, input_display.user_input_display_height);

    preview_video_resolution_flag = 1;

    return 0;

ERROR:
    return -1;
}

/*===========================================================================
 * FUNCTION     - set_whitebalance -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int set_whitebalance (mm_camera_test_obj_t *test_obj, int wb_action_param)
{
    cam_wb_mode_type type;
    switch (wb_action_param) {
        case WB_AUTO:
            printf("\n WB_AUTO\n");
            type = CAM_WB_MODE_AUTO;
            break;
        case WB_INCANDESCENT:
            printf("\n WB_INCANDESCENT\n");
            type = CAM_WB_MODE_INCANDESCENT;
            break;
        case WB_FLUORESCENT:
            printf("\n WB_FLUORESCENT\n");
            type = CAM_WB_MODE_FLUORESCENT;
            break;
        case WB_WARM_FLUORESCENT:
            printf("\n WB_WARM_FLUORESCENT\n");
            type = CAM_WB_MODE_WARM_FLUORESCENT;
            break;
        case WB_DAYLIGHT:
            printf("\n WB_DAYLIGHT\n");
            type = CAM_WB_MODE_DAYLIGHT;
            break;
        case WB_CLOUDY_DAYLIGHT:
            printf("\n WB_CLOUDY_DAYLIGHT\n");
            type = CAM_WB_MODE_CLOUDY_DAYLIGHT;
            break;
        case WB_TWILIGHT:
            printf("\n WB_TWILIGHT\n");
            type = CAM_WB_MODE_TWILIGHT;
            break;
        case WB_SHADE:
            printf("\n WB_SHADE\n");
            type = CAM_WB_MODE_SHADE;
            break;
        default:
            break;
    }
    return mm_app_set_params(test_obj, CAM_INTF_PARM_WHITE_BALANCE, sizeof(type), &type);
}


/*===========================================================================
 * FUNCTION     - set_exp_metering -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int set_exp_metering (mm_camera_test_obj_t *test_obj, int exp_metering_action_param)
{
    cam_auto_exposure_mode_type type;
    switch (exp_metering_action_param) {
        case AUTO_EXP_FRAME_AVG:
            printf("\nAUTO_EXP_FRAME_AVG\n");
            type = CAM_AEC_MODE_FRAME_AVERAGE;
            break;
        case AUTO_EXP_CENTER_WEIGHTED:
            printf("\n AUTO_EXP_CENTER_WEIGHTED\n");
            type = CAM_AEC_MODE_CENTER_WEIGHTED;
            break;
        case AUTO_EXP_SPOT_METERING:
            printf("\n AUTO_EXP_SPOT_METERING\n");
            type = CAM_AEC_MODE_SPOT_METERING;
            break;
        case AUTO_EXP_SMART_METERING:
            printf("\n AUTO_EXP_SMART_METERING\n");
            type = CAM_AEC_MODE_SMART_METERING;
            break;
        case AUTO_EXP_USER_METERING:
            printf("\n AUTO_EXP_USER_METERING\n");
            type = CAM_AEC_MODE_USER_METERING;
            break;
        case AUTO_EXP_SPOT_METERING_ADV:
            printf("\n AUTO_EXP_SPOT_METERING_ADV\n");
            type = CAM_AEC_MODE_SPOT_METERING_ADV;
            break;
        case AUTO_EXP_CENTER_WEIGHTED_ADV:
            printf("\n AUTO_EXP_CENTER_WEIGHTED_ADV\n");
            type = CAM_AEC_MODE_CENTER_WEIGHTED_ADV;
            break;
        default:
            break;
    }

    return mm_app_set_params(test_obj, CAM_INTF_PARM_AEC_ALGO_TYPE, sizeof(type), &type);
}

int get_ctrl_value (int ctrl_value_mode_param)
{
#if 0
    int rc = 0;
    struct v4l2_control ctrl;

    if (ctrl_value_mode_param == WHITE_BALANCE_STATE) {
        printf("You chose WHITE_BALANCE_STATE\n");
        ctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;
    } else if (ctrl_value_mode_param == WHITE_BALANCE_TEMPERATURE) {
        printf("You chose WHITE_BALANCE_TEMPERATURE\n");
        ctrl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
    } else if (ctrl_value_mode_param == BRIGHTNESS_CTRL) {
        printf("You chose brightness value\n");
        ctrl.id = V4L2_CID_BRIGHTNESS;
    } else if (ctrl_value_mode_param == EV) {
        printf("You chose exposure value\n");
        ctrl.id = V4L2_CID_EXPOSURE;
    } else if (ctrl_value_mode_param == CONTRAST_CTRL) {
        printf("You chose contrast value\n");
        ctrl.id = V4L2_CID_CONTRAST;
    } else if (ctrl_value_mode_param == SATURATION_CTRL) {
        printf("You chose saturation value\n");
        ctrl.id = V4L2_CID_SATURATION;
    } else if (ctrl_value_mode_param == SHARPNESS_CTRL) {
        printf("You chose sharpness value\n");
        ctrl.id = V4L2_CID_SHARPNESS;
    }

    //  rc = ioctl(camfd, VIDIOC_G_CTRL, &ctrl);
    return rc;
#endif
}

/*===========================================================================
 * FUNCTION     - toggle_afr -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int toggle_afr ()
{
#if 0
    if (fps_mode == FPS_MODE_AUTO) {
        printf("\nSetting FPS_MODE_FIXED\n");
        fps_mode = FPS_MODE_FIXED;
    } else {
        printf("\nSetting FPS_MODE_AUTO\n");
        fps_mode = FPS_MODE_AUTO;
    }
    return mm_app_set_config_parm(cam_id, MM_CAMERA_PARM_FPS_MODE, fps_mode);
#endif
}

int set_zoom (mm_camera_test_obj_t *test_obj, int zoom_action_param)
{

    if (zoom_action_param == ZOOM_IN) {
        zoom_level += ZOOM_STEP;
        if (zoom_level > zoom_max_value)
            zoom_level = zoom_max_value;
    } else if (zoom_action_param == ZOOM_OUT) {
        zoom_level -= ZOOM_STEP;
        if (zoom_level < ZOOM_MIN_VALUE)
            zoom_level = ZOOM_MIN_VALUE;
    } else {
        CDBG("%s: Invalid zoom_action_param value\n", __func__);
        return -EINVAL;
    }

    return mm_app_set_params(test_obj, CAM_INTF_PARM_ZOOM, sizeof(zoom_level), &zoom_level);
}

/*===========================================================================
 * FUNCTION     - set_iso -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int set_iso (mm_camera_test_obj_t *test_obj, int iso_action_param)
{
    cam_iso_mode_type type;
    switch (iso_action_param) {
        case ISO_AUTO:
            printf("\n ISO_AUTO\n");
            type = CAM_ISO_MODE_AUTO;
            break;
        case ISO_DEBLUR:
            printf("\n ISO_DEBLUR\n");
            type = CAM_ISO_MODE_DEBLUR;
            break;
        case ISO_100:
            printf("\n ISO_100\n");
            type = CAM_ISO_MODE_100;
            break;
        case ISO_200:
            printf("\n ISO_200\n");
            type = CAM_ISO_MODE_200;
            break;
        case ISO_400:
            printf("\n ISO_400\n");
            type = CAM_ISO_MODE_400;
            break;
        case ISO_800:
            printf("\n ISO_800\n");
            type = CAM_ISO_MODE_800;
            break;
        case ISO_1600:
            printf("\n ISO_1600\n");
            type = CAM_ISO_MODE_1600;
            break;
        default:
            break;
    }

    return mm_app_set_params(test_obj, CAM_INTF_PARM_ISO, sizeof(type), &type);
}


void flip_preview(mm_camera_test_obj_t *test_obj, int flip_action)
{

    CDBG("\nFlip_action=%d\n", flip_action);
    test_obj->app_handle->flip_mode = flip_action;
}

void flip_cameras(mm_camera_test_obj_t *test_obj, int flip_action)
{
    CDBG("Flip_action = %d\n", flip_action);

    if (test_obj->app_handle->num_cameras == 2)
        cam_id = !cam_id;
    else
        CDBG_HIGH("%s: Cannot flip. Only one camera present", __func__);
}


/*===========================================================================
 * FUNCTION     - increase_sharpness -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int increase_sharpness (mm_camera_test_obj_t *test_obj)
{
    sharpness += CAMERA_SHARPNESS_STEP;
    if (sharpness > CAMERA_MAX_SHARPNESS) {
        sharpness = CAMERA_MAX_SHARPNESS;
        printf("Reached max SHARPNESS. \n");
    }
    printf("Increase Sharpness to %d\n", sharpness);

    return mm_app_set_params(test_obj, CAM_INTF_PARM_SHARPNESS, sizeof(sharpness), &sharpness);
}

/*===========================================================================
 * FUNCTION     - decrease_sharpness -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int decrease_sharpness (mm_camera_test_obj_t *test_obj)
{
    sharpness -= CAMERA_SHARPNESS_STEP;
    if (sharpness < CAMERA_MIN_SHARPNESS) {
        sharpness = CAMERA_MIN_SHARPNESS;
        printf("Reached min SHARPNESS. \n");
    }
    printf("Decrease Sharpness to %d\n", sharpness);

    return mm_app_set_params(test_obj, CAM_INTF_PARM_SHARPNESS, sizeof(sharpness), &sharpness);
}

int set_flash_mode (mm_camera_test_obj_t *test_obj, int action_param)
{
    cam_flash_mode_t type;
    switch (action_param) {
        case FLASH_MODE_OFF:
            printf("\n FLASH_MODE_OFF\n");
            type = CAM_FLASH_MODE_OFF;
            break;
        case FLASH_MODE_AUTO:
            printf("\n FLASH_MODE_AUTO\n");
            type = CAM_FLASH_MODE_AUTO;
            break;
        case FLASH_MODE_ON:
            printf("\n FLASH_MODE_ON\n");
            type = CAM_FLASH_MODE_ON;
            break;
        case FLASH_MODE_TORCH:
            printf("\n FLASH_MODE_TORCH\n");
            type = CAM_ISO_MODE_100;
            break;
        default:
            break;
    }

    return mm_app_set_params(test_obj, CAM_INTF_PARM_LED_MODE, sizeof(type), &type);
}

int set_bestshot_mode(mm_camera_test_obj_t *test_obj, int action_param)
{
    cam_scene_mode_type type;
    switch (action_param) {
        case BESTSHOT_AUTO:
            printf("\n BEST SHOT AUTO\n");
            type = CAM_SCENE_MODE_OFF;
            break;
        case BESTSHOT_ACTION:
            printf("\n BEST SHOT ACTION\n");
            type = CAM_SCENE_MODE_ACTION;
            break;
        case BESTSHOT_PORTRAIT:
            printf("\n BEST SHOT PORTRAIT\n");
            type = CAM_SCENE_MODE_PORTRAIT;
            break;
        case BESTSHOT_LANDSCAPE:
            printf("\n BEST SHOT LANDSCAPE\n");
            type = CAM_SCENE_MODE_LANDSCAPE;
            break;
        case BESTSHOT_NIGHT:
            printf("\n BEST SHOT NIGHT\n");
            type = CAM_SCENE_MODE_NIGHT;
            break;
        case BESTSHOT_NIGHT_PORTRAIT:
            printf("\n BEST SHOT NIGHT PORTRAIT\n");
            type = CAM_SCENE_MODE_NIGHT_PORTRAIT;
            break;
        case BESTSHOT_THEATRE:
            printf("\n BEST SHOT THREATRE\n");
            type = CAM_SCENE_MODE_THEATRE;
            break;
        case BESTSHOT_BEACH:
            printf("\n BEST SHOT BEACH\n");
            type = CAM_SCENE_MODE_BEACH;
            break;
        case BESTSHOT_SNOW:
            printf("\n BEST SHOT SNOW\n");
            type = CAM_SCENE_MODE_SNOW;
            break;
        case BESTSHOT_SUNSET:
            printf("\n BEST SHOT SUNSET\n");
            type = CAM_SCENE_MODE_SUNSET;
            break;
        case BESTSHOT_ANTISHAKE:
            printf("\n BEST SHOT ANTISHAKE\n");
            type = CAM_SCENE_MODE_ANTISHAKE;
            break;
        case BESTSHOT_FIREWORKS:
            printf("\n BEST SHOT FIREWORKS\n");
            type = CAM_SCENE_MODE_FIREWORKS;
            break;
        case BESTSHOT_SPORTS:
            printf("\n BEST SHOT SPORTS\n");
            type = CAM_SCENE_MODE_SPORTS;
            break;
        case BESTSHOT_PARTY:
            printf("\n BEST SHOT PARTY\n");
            type = CAM_SCENE_MODE_PARTY;
            break;
        case BESTSHOT_CANDLELIGHT:
            printf("\n BEST SHOT CANDLELIGHT\n");
            type = CAM_SCENE_MODE_CANDLELIGHT;
            break;
        case BESTSHOT_ASD:
            printf("\n BEST SHOT ASD\n");
            type = CAM_SCENE_MODE_AUTO;
            break;
        case BESTSHOT_BACKLIGHT:
            printf("\n BEST SHOT BACKLIGHT\n");
            type = CAM_SCENE_MODE_BACKLIGHT;
            break;
        case BESTSHOT_FLOWERS:
            printf("\n BEST SHOT FLOWERS\n");
            type = CAM_SCENE_MODE_FLOWERS;
            break;
        case BESTSHOT_AR:
            printf("\n BEST SHOT AR\n");
            type = CAM_SCENE_MODE_AR;
            break;
        case BESTSHOT_HDR:
            printf("\n BEST SHOT HDR\n");
            type = CAM_SCENE_MODE_OFF;
            break;
        default:
            break;
    }

    return mm_app_set_params(test_obj, CAM_INTF_PARM_BESTSHOT_MODE, sizeof(type), &type);
}
/*===========================================================================
 * FUNCTION     - print_current_menu -
 *
 * DESCRIPTION:
 * ===========================================================================*/
int print_current_menu (menu_id_change_t current_menu_id)
{
    if (current_menu_id == MENU_ID_MAIN) {
        print_menu_preview_video ();
    } else if (current_menu_id == MENU_ID_PREVIEWVIDEORESOLUTIONCHANGE) {
        camera_preview_video_resolution_change_tbl ();
    } else if (current_menu_id == MENU_ID_WHITEBALANCECHANGE) {
        camera_preview_video_wb_change_tbl();
    } else if (current_menu_id == MENU_ID_EXPMETERINGCHANGE) {
        camera_preview_video_exp_metering_change_tbl();
    } else if (current_menu_id == MENU_ID_GET_CTRL_VALUE) {
        camera_preview_video_get_ctrl_value_tbl();
    } else if (current_menu_id == MENU_ID_ISOCHANGE) {
        camera_preview_video_iso_change_tbl();
    } else if (current_menu_id == MENU_ID_BRIGHTNESSCHANGE) {
        camera_brightness_change_tbl ();
    } else if (current_menu_id == MENU_ID_CONTRASTCHANGE) {
        camera_contrast_change_tbl ();
    } else if (current_menu_id == MENU_ID_EVCHANGE) {
        camera_EV_change_tbl ();
    } else if (current_menu_id == MENU_ID_SATURATIONCHANGE) {
        camera_saturation_change_tbl ();
    } else if (current_menu_id == MENU_ID_ZOOMCHANGE) {
        camera_preview_video_zoom_change_tbl();
    } else if (current_menu_id == MENU_ID_SHARPNESSCHANGE) {
        camera_preview_video_sharpness_change_tbl();
    } else if (current_menu_id == MENU_ID_BESTSHOT) {
        camera_set_bestshot_tbl();
    } else if (current_menu_id == MENU_ID_FLASHMODE) {
        camera_set_flashmode_tbl();
    } else if (current_menu_id == MENU_ID_PREVIEWFLIP) {
        camera_preview_flip_tbl();
    }
    return 0;
}

