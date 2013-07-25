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
#include <assert.h>

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

struct msmfb_overlay_data ov_front;
struct mdp_overlay overlay;
int vid_buf_front_id, vid_buf_back_id;
static unsigned char need_init = 1;
int num_of_ready_frames = 0;

static pthread_cond_t  sub_thread_ready_cond  = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t sub_thread_ready_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  camframe_fb_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t camframe_fb_mutex = PTHREAD_MUTEX_INITIALIZER;
static void notify_camframe_fb_thread();

void camframe_fb_thread_ready_signal(void);

void overlay_set_params(struct mdp_blit_req *blit_param)
{
  int rc = MM_CAMERA_OK;

  if (need_init) {
    overlay.id = MSMFB_NEW_REQUEST;
  }

  overlay.src.width  = blit_param->src.width;
  overlay.src.height = blit_param->src.height;
  overlay.src.format = blit_param->src.format;

  overlay.src_rect.x = blit_param->src_rect.x;
  overlay.src_rect.y = blit_param->src_rect.y;
  overlay.src_rect.w = blit_param->src_rect.w;
  overlay.src_rect.h = blit_param->src_rect.h;

  /* FIXME: Width and height are swapped for the destination
     rectangle as the camera mounting in landscape mode
     and display is in portrait */
  overlay.dst_rect.x = blit_param->dst_rect.y;
  overlay.dst_rect.y = blit_param->dst_rect.x;

  overlay.dst_rect.w = blit_param->dst_rect.h;
  overlay.dst_rect.h = blit_param->dst_rect.w;

  //TODO: Instead of hardcoding, see if the max resolutions
  //could be queried
  if (overlay.dst_rect.w > 720)
    overlay.dst_rect.w = 720;
  if (overlay.dst_rect.h > 1280)
    overlay.dst_rect.h = 1280;

  overlay.z_order = 0; // FB_OVERLAY_VID_0;
  overlay.alpha = blit_param->alpha;
  overlay.transp_mask = blit_param->transp_mask; /* 0xF81F */
  overlay.flags = blit_param->flags;
  overlay.is_fg = 1;


  rc = ioctl(fb_fd, MSMFB_OVERLAY_SET, &overlay);
  if (rc < 0) {
    CDBG_ERROR("%s: Display overlay set failed. IOCTL return: %d, src.width %d height %d; src_rect.x %d y %d w %d h %d; dst_rect.x %d y %d w %d h %d\n",
    __func__, rc,
    overlay.src.width, overlay.src.height,
    overlay.src_rect.x, overlay.src_rect.y, overlay.src_rect.w, overlay.src_rect.h,
    overlay.dst_rect.x, overlay.dst_rect.y, overlay.dst_rect.w, overlay.dst_rect.h
   );
  }
  assert(rc >= 0);

  if (need_init) {
    vid_buf_front_id = overlay.id; /* keep return id */
    ov_front.id = overlay.id;
    need_init = 0;
  }

  return;
}

void notify_camframe_fb_thread()
{
  pthread_mutex_lock(&camframe_fb_mutex);

  num_of_ready_frames ++;
  pthread_cond_signal(&camframe_fb_cond);

  pthread_mutex_unlock(&camframe_fb_mutex);
}


void *camframe_fb_thread(void *data)
{
  int rc;

  fb_fd = open(LE_FB0, O_RDWR);
  assert(fb_fd >= 0);

  CDBG("%s:LE_FB0 dl, '%s', fd=%d\n", __func__, LE_FB0, fb_fd);

  rc = ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
  assert(rc >= 0);

  rc = ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);
  assert(rc >= 0);

  rc = ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo);
  if (rc < 0) {
      CDBG_HIGH("ERROR: FBIOPAN_DISPLAY failed! rc=%d\n",rc);
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

      rc = ioctl(fb_fd, MSMFB_OVERLAY_PLAY, &ov_front);
      if (rc < 0) {
         CDBG_HIGH("ERROR: MSMFB_OVERLAY_PLAY failed! rc=%d\n",rc);
      }

      rc = ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo);
      if (rc < 0) {
         CDBG_HIGH("ERROR: FBIOPAN_DISPLAY failed! rc=%d\n",rc);
      }
    }
  }

  pthread_mutex_unlock(&camframe_fb_mutex);

  rc = ioctl(fb_fd, MSMFB_OVERLAY_UNSET, &vid_buf_front_id);
  if (rc < 0) {
      CDBG_HIGH("ERROR: MSMFB_OVERLAY_UNSET failed! rc=%d\n",rc);
  }

  return;
}

void launch_camframe_fb_thread()
{
  is_camframe_fb_thread_ready = 0;
  camframe_fb_exit = 0;
  CDBG_HIGH("%s: Enter. Camera FB thread ready:%d", __func__, is_camframe_fb_thread_ready);

  pthread_create(&cam_frame_fb_thread_id, NULL, camframe_fb_thread, NULL);

  pthread_mutex_lock(&sub_thread_ready_mutex);

  if (!is_camframe_fb_thread_ready) {
    pthread_cond_wait(&sub_thread_ready_cond, &sub_thread_ready_mutex);
  }

  pthread_mutex_unlock(&sub_thread_ready_mutex);

  CDBG_HIGH("%s: Exit. Camera FB thread ready:%d", __func__, is_camframe_fb_thread_ready);
  return ;
}

void release_camframe_fb_thread(void)
{
  camframe_fb_exit = 1;
  need_init = 1;

  CDBG_HIGH("%s: Enter", __func__);

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

  CDBG_HIGH("%s: Exit", __func__);
}

/* Sends signal to control thread to indicate that the cam frame fb
   * ready.
   */
void camframe_fb_thread_ready_signal(void)
{
  CDBG("cam_frame_fb_thread() is ready, call pthread_cond_signal\n");

  pthread_mutex_lock(&sub_thread_ready_mutex);
  is_camframe_fb_thread_ready = 1;
  pthread_cond_signal(&sub_thread_ready_cond);
  pthread_mutex_unlock(&sub_thread_ready_mutex);

  CDBG("cam_frame_fb_thread() is ready, call pthread_cond_signal done\n");
}


int mm_app_dl_render(int frame_fd, mm_camera_test_obj_t *test_obj)
{
  struct mdp_blit_req *blit_param;
  int croplen = 0;
  int rc = MM_CAMERA_OK;
  int width = test_obj->app_handle->preview_width;
  int height = test_obj->app_handle->preview_height;

  /* Initialize yuv structure */
  yuv.list.count = 1;
  blit_param = &yuv.list.req[0];

  blit_param->src.width = width;
  blit_param->src.height = height;
  blit_param->src.format = MDP_Y_CRCB_H2V2;
  blit_param->src.offset = 0;
  blit_param->src.memory_id = frame_fd;

  blit_param->dst.width = vinfo.xres;
  blit_param->dst.height = vinfo.yres;
  blit_param->dst.format = MDP_RGBA_8888;
  blit_param->dst.offset = 0;
  blit_param->dst.memory_id = fb_fd;

  blit_param->transp_mask = MDP_TRANSP_NOP;
  blit_param->flags = 0;
  blit_param->alpha = 0xff;
  blit_param->dst_rect.w = width;
  blit_param->dst_rect.h = height;

  blit_param->src_rect.x = 0;
  blit_param->src_rect.y = 0;
  blit_param->src_rect.w  = width;
  blit_param->src_rect.h  = height;

  blit_param->dst_rect.x = 0;
  blit_param->dst_rect.y = 0;
  overlay_set_params(blit_param);

  ov_front.data.offset = 0;
  ov_front.data.memory_id = frame_fd;
  notify_camframe_fb_thread();
  return rc;
}
