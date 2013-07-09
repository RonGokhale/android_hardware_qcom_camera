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

#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <inttypes.h>
#include <linux/msm_mdp.h>
#include <linux/msm_rotator.h>
#include <linux/fb.h>

#include "mm_qcamera_app.h"
#include "mm_qcamera_dbg.h"

#define LE_FB0 "/dev/fb0"

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
int fb_fd = 0;
int rot_fd = 0;
union {
  char dummy[sizeof(struct mdp_blit_req_list) +
    sizeof(struct mdp_blit_req) * 1];
  struct mdp_blit_req_list list;
} yuv;

static pthread_t cam_frame_fb_thread_id;
static int camframe_fb_exit;
static int is_camframe_fb_thread_ready;

//struct msmfb_overlay_data ov_front;
struct msmfb_overlay_data ov_front, ov_back, *ovp_front, *ovp_back;
struct mdp_overlay overlay, *overlayp;
int vid_buf_front_id, vid_buf_back_id;
static unsigned char please_initialize = 1;
int num_of_ready_frames = 0;
//struct msm_rotator_img_info iinfo;
//struct msm_rotator_data_info dinfo;
int cam_id = 0;

static pthread_cond_t  sub_thread_ready_cond  = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t sub_thread_ready_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  camframe_fb_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t camframe_fb_mutex = PTHREAD_MUTEX_INITIALIZER;
static void notify_camframe_fb_thread();

#if 0
static int overlay_vsync_ctrl(int enable)
{
        int ret;
        int vsync_en = enable;
        ret = ioctl(fb_fd, MSMFB_OVERLAY_VSYNC_CTRL, &vsync_en);
        if (ret)
           CDBG("\n MSMFB_OVERLAY_VSYNC_CTRL failed! (Line %d)\n",__LINE__);
        return ret;
}
void overlay_set_params(struct mdp_blit_req *e)
{
  int result;
  unsigned int oWidth; // output width & height after buffer rotation.
  unsigned int oHeight;

  if (please_initialize) {
    overlayp = &overlay;
    overlayp->id = MSMFB_NEW_REQUEST;
  }

  //Set up iinfo and dinfo structures for rotation ioctls.
  iinfo.downscale_ratio = 0;
  iinfo.src.width  = e->src.width;
  iinfo.src.height = e->src.height;
  iinfo.src.format = iinfo.dst.format = e->src.format;
  //Set source and destination memory fd.
  dinfo.src.memory_id = e->src.memory_id;
  dinfo.dst.memory_id = dest_fd;
  dinfo.src.flags =0;
  dinfo.dst.flags =0;
  dinfo.src.offset = 0;
  dinfo.dst.offset = 0;

  iinfo.src_rect.x = e->src_rect.x;
  iinfo.src_rect.y = e->src_rect.y;
  iinfo.dst_x = iinfo.src_rect.x;
  iinfo.dst_y = iinfo.src_rect.y;
  iinfo.src_rect.w = e->src_rect.w;
  iinfo.src_rect.h = e->src_rect.h;
  iinfo.rotations = 0;

  if(cam_id == 0) {
    iinfo.rotations |= MDP_ROT_90;
    //Reverse width & height from src to dst.
    iinfo.dst.width = iinfo.src.height;
    iinfo.dst.height = iinfo.src.width;
    oWidth = iinfo.src.height;
    oHeight = iinfo.src.width;
  }
  else {
    //for front camera use flip LR to make preview like mirror image.
    iinfo.rotations |= MDP_FLIP_LR;
    iinfo.dst.width = iinfo.src.width;
    iinfo.dst.height = iinfo.src.height;
    oWidth = iinfo.src.width;
    oHeight = iinfo.src.height;
  }

  iinfo.enable = 1;
  iinfo.secure = 0;

  overlay_vsync_ctrl(TRUE);
  result = ioctl(rot_fd, MSM_ROTATOR_IOCTL_START, &iinfo);
  if (result < 0) {
     CDBG("[ROTATE] MSM_ROTATOR_IOCTL_START failed result = %d\n", result);
  }
  dinfo.session_id = iinfo.session_id;

  result = ioctl(rot_fd, MSM_ROTATOR_IOCTL_ROTATE, &dinfo);
  if (result < 0) {
     CDBG("[ROTATE] MSM_ROTATOR_IOCTL_ROTATE failed result = %d\n", result);
  }
  //Set overlay parameters.
  overlayp->src.width  = oWidth;
  overlayp->src.height = oHeight;
  overlayp->src_rect.w = oWidth;
  overlayp->src_rect.h = oHeight;
  overlayp->dst_rect.w = oWidth;
  overlayp->dst_rect.h = oHeight;

  if(needtocrop)
  {
    if(cam_id == 0) {
      //Reverse src rectangle width & height as buffer is rotated.
      overlayp->src_rect.w = e->src_rect.h;
      overlayp->src_rect.h = e->src_rect.w;
    } else {
      overlayp->src_rect.w = e->src_rect.w;
      overlayp->src_rect.h = e->src_rect.h;
    }
    overlayp->src_rect.x = e->src_rect.x;
    overlayp->src_rect.y = e->src_rect.y;
  }

  overlayp->src.format = iinfo.dst.format;
  overlayp->flags = 0;
  overlayp->z_order = 0;
  overlayp->alpha = 0xFF;
  overlayp->transp_mask = MDP_TRANSP_NOP;
  overlayp->transp_mask = 0;
  overlayp->is_fg = 1;

  if (overlayp->dst_rect.w > 480)
    overlayp->dst_rect.w = 480;
  if (overlayp->dst_rect.h > 800)
    overlayp->dst_rect.h = 800;

  result = ioctl(fb_fd, MSMFB_OVERLAY_SET, overlayp);
  if (result < 0) {
      CDBG("\n MSMFB_OVERLAY_SET failed!, result =%d \n", result);
  }
  overlay_vsync_ctrl(FALSE);

  if (please_initialize) {
    vid_buf_front_id = overlayp->id; /* keep return id */
    ov_front.id = overlayp->id;
    ov_front.data.flags = 0;
    ov_front.data.offset = 0;
    ov_front.data.memory_id = dest_fd;
    please_initialize = 0;
  }

  return;
}
#endif

int overlay_set_params(struct mdp_blit_req *e)
{
  int rc = MM_CAMERA_OK;

  if (please_initialize) {
    overlayp = &overlay;
    ovp_front = &ov_front;
    ovp_back = &ov_back;

    overlayp->id = MSMFB_NEW_REQUEST;
  }

  overlayp->src.width  = e->src.width;
  overlayp->src.height = e->src.height;
  overlayp->src.format = e->src.format;

  overlayp->src_rect.x = e->src_rect.x;
  overlayp->src_rect.y = e->src_rect.y;
  overlayp->src_rect.w = e->src_rect.w;
  overlayp->src_rect.h = e->src_rect.h;

  overlayp->dst_rect.x = e->dst_rect.x;
  overlayp->dst_rect.y = e->dst_rect.y;

  overlayp->dst_rect.w = e->dst_rect.h;
  overlayp->dst_rect.h = e->dst_rect.w;

  if (overlayp->dst_rect.w > 720)
    overlayp->dst_rect.w = 720;
  if (overlayp->dst_rect.h > 1280)
    overlayp->dst_rect.h = 1280;

  overlayp->z_order = 0; // FB_OVERLAY_VID_0;
  overlayp->alpha = e->alpha;
  overlayp->transp_mask = e->transp_mask; /* 0xF81F */
  overlayp->flags = e->flags;
  overlayp->is_fg = 1;

  CDBG("src.width %d height %d; src_rect.x %d y %d w %d h %d; dst_rect.x %d y %d w %d h %d\n",
    overlayp->src.width, overlayp->src.height,
    overlayp->src_rect.x, overlayp->src_rect.y, overlayp->src_rect.w, overlayp->src_rect.h,
    overlayp->dst_rect.x, overlayp->dst_rect.y, overlayp->dst_rect.w, overlayp->dst_rect.h
   );

  rc = ioctl(fb_fd, MSMFB_OVERLAY_SET, overlayp);
  if (rc < 0) {
    CDBG_HIGH("ERROR: MSMFB_OVERLAY_SET failed! rc=%d\n",rc);
    return rc;
  }

  if (please_initialize) {
    vid_buf_front_id = overlayp->id; /* keep return id */

    ov_front.id = overlayp->id;
    ov_back.id = overlayp->id;
    please_initialize = 0;
  }

  return rc;
}

void notify_camframe_fb_thread()
{
  pthread_mutex_lock(&camframe_fb_mutex);

  num_of_ready_frames ++;
  pthread_cond_signal(&camframe_fb_cond);

  pthread_mutex_unlock(&camframe_fb_mutex);
}

void camframe_fb_thread_ready_signal(void);

void *camframe_fb_thread(void *data)
{
  int result = 0;
  static struct timeval td1, td2;
  struct timezone tz;

  fb_fd = open(LE_FB0, O_RDWR);
  CDBG_HIGH("%s:LE_FB0 dl, '%s', fd=%d\n", __func__, LE_FB0, fb_fd);

#if 0
  rot_fd = open("/dev/msm_rotator", O_RDWR);
  if (rot_fd < 0) {
    CDBG_HIGH("%s:/dev/msm_rotator failed , fd=%d\n", __func__, rot_fd);
  }
#endif
  if (fb_fd < 0) {
    CDBG_HIGH("cannot open framebuffer %s file node\n", LE_FB0);
    goto fail1;
  }

  if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
    CDBG_HIGH("cannot retrieve vscreenInfo!\n");
    goto fail;
  }

  if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
    CDBG_HIGH("can't retrieve fscreenInfo!\n");
    goto fail;
  }
  //To avoid display flickers.
  if(ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo) < 0) {
    CDBG_HIGH("%s FBIOPAN_DISPLAY failed!!\n",__func__);
  }
  vinfo.activate = FB_ACTIVATE_VBL;

  camframe_fb_thread_ready_signal();

  pthread_mutex_lock(&camframe_fb_mutex);
  while (!camframe_fb_exit) {
    CDBG("cam_frame_fb_thread: num_of_ready_frames: %d\n", num_of_ready_frames);
    if (num_of_ready_frames <= 0) {
      pthread_cond_wait(&camframe_fb_cond, &camframe_fb_mutex);
    }
    if (num_of_ready_frames > 0) {
      num_of_ready_frames --;

      gettimeofday(&td1, &tz);
      if( ioctl(fb_fd, MSMFB_OVERLAY_PLAY, ovp_front) < 0) {
        CDBG_HIGH("\n%s MSMFB_OVERLAY_PLAY failed!!\n",__func__);
      }

      if( result = ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo) < 0) {
        CDBG_HIGH("\n%s FBIOPAN_DISPLAY failed!!\n",__func__);
      }
#if 0
      if(result = ioctl(rot_fd, MSM_ROTATOR_IOCTL_FINISH, &dinfo.session_id) < 0) {
        CDBG_HIGH("[ROTATE] rotator finish ioctl failed result = %d\n",result);
      }
#endif
    }
  }

  pthread_mutex_unlock(&camframe_fb_mutex);

  if (ioctl(fb_fd, MSMFB_OVERLAY_UNSET, &vid_buf_front_id)) {
    CDBG("\nERROR! MSMFB_OVERLAY_UNSET failed! (Line %d)\n", __LINE__);
    goto fail;
  }

  return NULL;

  fail:
  close(fb_fd);
  fail1:
  camframe_fb_exit = -1;
  camframe_fb_thread_ready_signal();
  return NULL;
}

int launch_camframe_fb_thread(void)
{
  camframe_fb_exit = 0;
  is_camframe_fb_thread_ready = 0;
  pthread_create(&cam_frame_fb_thread_id, NULL, camframe_fb_thread, NULL);

  /* Waiting for launching sub thread ready signal. */
  CDBG("launch_camframe_fb_thread(), call pthread_cond_wait\n");

  pthread_mutex_lock(&sub_thread_ready_mutex);
  if (!is_camframe_fb_thread_ready) {
    pthread_cond_wait(&sub_thread_ready_cond, &sub_thread_ready_mutex);
  }
  pthread_mutex_unlock(&sub_thread_ready_mutex);

  CDBG("launch_camframe_fb_thread(), call pthread_cond_wait done\n");
  CDBG("%s:fb rc=%d\n", __func__, camframe_fb_exit);
  return camframe_fb_exit;
}

void release_camframe_fb_thread(void)
{
  camframe_fb_exit = 1;
  please_initialize = 1;

  /* Notify the camframe fb thread to wake up */
  if (cam_frame_fb_thread_id != 0) {
     pthread_mutex_lock(&camframe_fb_mutex);
     pthread_cond_signal(&camframe_fb_cond);
     pthread_mutex_unlock(&camframe_fb_mutex);

     if (pthread_join(cam_frame_fb_thread_id, NULL) != 0) {
       CDBG("cam_frame_fb_thread exit failure!\n");
     }
     close(fb_fd);
  }
}

void camframe_fb_thread_ready_signal(void)
{
  /* Send the signal to control thread to indicate that the cam frame fb
   * ready.
   */
  CDBG("cam_frame_fb_thread() is ready, call pthread_cond_signal\n");

  pthread_mutex_lock(&sub_thread_ready_mutex);
  is_camframe_fb_thread_ready = 1;
  pthread_cond_signal(&sub_thread_ready_cond);
  pthread_mutex_unlock(&sub_thread_ready_mutex);

  CDBG("cam_frame_fb_thread() is ready, call pthread_cond_signal done\n");
}

#if 0
int mm_app_dl_render(int frame_fd, int dest_fd, mm_camera_ch_crop_t *crop, int camid)
{
  struct mdp_blit_req *e;
  int croplen = 0;

  /* Initialize yuv structure */
  yuv.list.count = 1;
  e = &yuv.list.req[0];
  cam_id = camid;
  e->src.width = input_display.user_input_display_width;
  e->src.height = input_display.user_input_display_height;
  e->src.format = MDP_Y_CRCB_H2V2;
  e->src.offset = 0;
  e->src.memory_id = frame_fd;

  e->dst.width = vinfo.xres;
  e->dst.height = vinfo.yres;
  e->dst.format = MDP_RGBA_8888;
  e->dst.offset = 0;
  e->dst.memory_id = fb_fd;

  e->transp_mask = 0xffffffff;
  e->flags = 0;
  e->alpha = 0xff;
#if 0
  if (crop != NULL && (crop->crop.width != 0 || crop->crop.height != 0)) {
    e->src_rect.x = crop->crop.left;
    e->src_rect.y = crop->crop.top;
    e->src_rect.w = crop->crop.width;
    e->src_rect.h = crop->crop.height;
    e->dst_rect.w = input_display.user_input_display_width;
    e->dst_rect.h = input_display.user_input_display_height;
    needtocrop = 1;
  } else {
#endif
    e->dst_rect.w = input_display.user_input_display_width;
    e->dst_rect.h = input_display.user_input_display_height;

    e->src_rect.x = 0;
    e->src_rect.y = 0;
    e->src_rect.w  = input_display.user_input_display_width;
    e->src_rect.h  = input_display.user_input_display_height;
    needtocrop = 0;
 // }
  //moving preview 10 pixels from top left corner.
  e->dst_rect.x = 10;
  e->dst_rect.y = 10;
  overlay_set_params(e);
  notify_camframe_fb_thread();
  return TRUE;
}
#endif

int mm_app_dl_render(int frame_fd, mm_camera_test_obj_t *test_obj)
{
  struct mdp_blit_req *e;
  int croplen = 0;
  int rc = MM_CAMERA_OK;

  /* Initialize yuv structure */
  yuv.list.count = 1;
  e = &yuv.list.req[0];

  e->src.width = test_obj->preview_resolution.user_input_display_width;
  e->src.height = test_obj->preview_resolution.user_input_display_height;
  e->src.format = MDP_Y_CRCB_H2V2;
  e->src.offset = 0;
  e->src.memory_id = frame_fd;

  e->dst.width = vinfo.xres;
  e->dst.height = vinfo.yres;
  e->dst.format = MDP_RGBA_8888;
  e->dst.offset = 0;
  e->dst.memory_id = fb_fd;

  e->transp_mask = MDP_TRANSP_NOP;
  e->flags = 0;
  e->alpha = 0xff;
  e->dst_rect.w = test_obj->preview_resolution.user_input_display_width;
  e->dst_rect.h = test_obj->preview_resolution.user_input_display_height;

  e->src_rect.x = 0;
  e->src_rect.y = 0;
  e->src_rect.w  = test_obj->preview_resolution.user_input_display_width;
  e->src_rect.h  = test_obj->preview_resolution.user_input_display_height;

  e->dst_rect.x = 0;
  e->dst_rect.y = 0;
  if(MM_CAMERA_OK != (rc = overlay_set_params(e))) {
       CDBG_HIGH("\n%s overlay_set_params failed=%d\n",__func__, rc);
       return rc;
  }

  ov_front.data.offset = 0;
  ov_front.data.memory_id = frame_fd;
  notify_camframe_fb_thread();
  return rc;
}
