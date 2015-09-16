/* Copyright (c) 2015, The Linux Foundataion. All rights reserved.
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
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "qcamlib.h"

#define DEF_DUMP_COUNT_MAX    100
#define DEF_DUMP_INTERVAL     30
#define DEF_OUTPUT_DIR        "camera"

#define PRINT_FPS             0

typedef enum {
	SENSOR_IMX_4K = 0,
	SENSOR_OV_OPTIC_FLOW = 1,
	SENSOR_TYPE_MAX
} sensor_type_t;

uint32_t p_frame_count = 0;
uint32_t v_frame_count = 0;
uint32_t dump_count = 0;
uint32_t v_dump_count = 0;
uint32_t dump_count_max = DEF_DUMP_COUNT_MAX;
uint32_t dump_interval = DEF_DUMP_INTERVAL;
uint32_t sensor_type = SENSOR_IMX_4K;
uint8_t *p_buf = NULL;
uint32_t p_bufsize = 0;
uint8_t *v_buf = NULL;
uint32_t v_bufsize = 0;
bool previewing = false;
static bool streaming = false;
char *outdir = DEF_OUTPUT_DIR;

qcamlib_frame_info_t preview_info, video_info;

#define YUV420_BUF_SIZE(w, h) ((w) * (h) * 3/2)


struct timeval tv0, tv1;

int64_t p_ns0 = 0, v_ns0 = 0;
int64_t p_diff=0, v_diff=0;

FILE *video_fp;

static bool enable_video = false;

static qcamlib_t h_cam;

char usage_str[] =
  "usage: qcamlib-test [options]\n"
  "\n"
  "    -o outdir\n"
  "        path where output image files are saved (default=./camera)\n"
  "    -n dump-count\n"
  "        maximum number of frames to be saved (default=100)\n"
  "    -i dump-interval\n"
  "        save every i'th frame (default=30)\n"
  "    -s sensor-type \n"
  "        0 : IMX 4K sensor (default) \n"
  "        1 : OV OpticFlow sensor \n"
  "    -h\n"
  "        print this message\n"
;

int save_frame_to_file(char *filename, uint8_t *buf, uint32_t size)
{
  int rc;
  FILE *fp;
  fp = fopen(filename, "w");
  if (!fp) {
    perror("fopen() failed\n");
    return -1;
  }
  rc = fwrite(buf, size, 1, fp);
  if (rc != 1) {
    perror("fwrite() failed\n");
    return -1;
  }
  fclose(fp);
  return 0;
}

int copy_frame_to_buf(qcamlib_cb_data_t frame, qcamlib_frame_info_t frame_info,
                      uint8_t *buf)
{
    int i;
    uint32_t src_offset, dest_offset;
    /* copy one line at a time */
    /* Y plane */
    src_offset = frame_info.planes[0].offset;
    dest_offset = 0;
    for (i=0; i<frame_info.height; i++) {
        memcpy(buf + dest_offset, frame.buffer + src_offset,
               frame_info.width);
        src_offset += frame_info.planes[0].stride;
        dest_offset += frame_info.width;
    }
    /* UV plane */
    src_offset = frame_info.planes[1].offset;
    dest_offset = frame_info.width * frame_info.height;
    for (i=0; i<frame_info.height/2; i++) {
        memcpy(buf + dest_offset, frame.buffer + src_offset,
               frame_info.width);
        src_offset += frame_info.planes[1].stride;
        dest_offset += frame_info.width;
    }
    return 0;
}

void video_cb(qcamlib_cb_data_t frame)
{
  int rc;
  v_frame_count++;

  int64_t ns1 = frame.ts.tv_sec * 1000000000LL + frame.ts.tv_nsec;
  v_diff = (v_diff + (ns1 - v_ns0))/2;
  v_ns0 = ns1;

#if PRINT_FPS
  if (v_frame_count % 30 == 0) {
    fprintf(stderr, "video fps = %0.2f\n", 1e9/v_diff);
  }
#endif
  /* copy video frame to local buffer */
  copy_frame_to_buf(frame, video_info, v_buf);

  rc = fwrite(v_buf, v_bufsize, 1, video_fp);
  if (rc != 1) {
    perror("fwrite() failed\n");
    return;
  }
}

void preview_cb(qcamlib_cb_data_t frame)
{
  int rc;
  p_frame_count++;

#if PRINT_FPS
  int64_t ns1 = frame.ts.tv_sec * 1000000000LL + frame.ts.tv_nsec;
  p_diff = (p_diff + (ns1 - p_ns0))/2;
  p_ns0 = ns1;
  if (p_frame_count % 30 == 0) {
    fprintf(stderr, "preview fps = %0.2f\n", 1e9/p_diff);
  }
#endif

  if ((p_frame_count-1) % dump_interval == 0 && dump_count < dump_count_max) {
    /* copy the camera frame to local buffer */
    copy_frame_to_buf(frame, preview_info, p_buf);
    char filename[128];
    snprintf(filename, 128, "%s/p_frame_%d_%dx%d_NV21.yuv",
             outdir, p_frame_count, preview_info.planes[0].width,
             preview_info.planes[0].height);
    rc = save_frame_to_file(filename, p_buf, p_bufsize);
    if (rc < 0) {
      perror("save failed\n");
      return;
    }
    dump_count++;
  }
}

void print_usage()
{
  printf("%s", usage_str);
}

int parse_commandline(int argc, char* argv[])
{
  int c;
  while ((c = getopt (argc, argv, "hi:n:o:vs:")) != -1) {
    switch (c) {
      case 'o':
        outdir = optarg;
        break;
      case 'i':
        dump_interval = atoi(optarg);
        break;
      case 'n':
        dump_count_max = atoi(optarg);
        break;
      case 'v':
        enable_video = true;
        break;
      case 's':
        sensor_type = atoi(optarg);
        break;
      case 'h':
        print_usage();
        exit(0);
      case '?':
        print_usage();
        exit(1);
      default:
        abort();
    }
  }
}

void print_menu()
{
  printf("\nSelect an option");
  if (enable_video) {
    printf("[video enabled]");
  }
  printf("\n");
  if (streaming) {
    printf("\t1. Stop streaming\n");
  } else {
    printf("\t1. Start streaming\n");
  }
  printf("\t2. Exit\n");
  printf("\nSelect: ");
}

void start_streaming()
{
  qcamlib_start_preview(h_cam);
  if (enable_video) {
    qcamlib_start_video(h_cam);
  }
  streaming = true;
}

void stop_streaming()
{
  if (enable_video) {
    qcamlib_stop_video(h_cam);
  }
  qcamlib_stop_preview(h_cam);
  streaming = false;
}

int main(int argc, char *argv[])
{
  parse_commandline(argc, argv);

  printf("---- qcamlib test application ----\n");

  if(sensor_type >= SENSOR_TYPE_MAX) {
	printf("ERROR: Invalid sensor type (%d) (IMX_4K / OV_OPTIC_FLOW) parameter\n",sensor_type);
	return -1;
  }
 
  struct stat st = {0};
  if (stat(outdir, &st) == -1) {
    mkdir(outdir, 0777);
  }

  h_cam = qcamlib_create();
  if (h_cam == NULL) {
      perror("qcamlib_create() failed");
      return -1;
  }

  qcamlib_config_t config;
  memset(&config, 0x00, sizeof(qcamlib_config_t));

  config.snapshot_dim.width = 1024;
  config.snapshot_dim.height = 768;
  config.video_dim.width = 1920;
  config.video_dim.height = 1080;
  if (enable_video == true) {
     config.preview_dim.width = 1280;
     config.preview_dim.height = 720;
  } else {
     config.preview_dim.width = 3840;
     config.preview_dim.height = 2160;
  }

  if (sensor_type == SENSOR_OV_OPTIC_FLOW) {
      config.snapshot_dim.width = 640;
      config.snapshot_dim.height = 480;
      config.preview_dim.width = 640;
      config.preview_dim.height = 480;
      config.video_dim.width = 640;
      config.video_dim.height = 480;
  }

  qcamlib_configure(h_cam, config);

  preview_info = qcamlib_get_frame_info(h_cam, QCAMLIB_STREAM_PREVIEW);
  video_info = qcamlib_get_frame_info(h_cam, QCAMLIB_STREAM_VIDEO);

  v_bufsize = video_info.buf_size;
  p_bufsize = preview_info.buf_size;
  p_buf = (uint8_t *)malloc(p_bufsize);
  if (!p_buf) {
    perror("malloc failed\n");
    qcamlib_destroy(h_cam);
    goto exit;
  }
  v_buf = (uint8_t *)malloc(v_bufsize);
  if (!v_buf) {
    perror("v_buf malloc failed\n");
    qcamlib_destroy(h_cam);
    goto exit;
  }
  char v_filename[128];
  snprintf(v_filename, 128, "video_%dx%d_nv12.yuv",
           video_info.planes[0].width,  video_info.planes[0].width);
  video_fp = fopen(v_filename, "w");
  if (video_fp == NULL) {
    perror("fopen() failed\n");
    goto exit;
  }
  qcamlib_register_preview_cb(h_cam, preview_cb);
  qcamlib_register_video_cb(h_cam, video_cb);

  int option;
  do {
    print_menu();
    if (scanf("%d", &option) != 1) {
      printf("ERROR: use integer options\n");
      goto exit;
    }
    switch (option) {
      case 1:
        if (streaming) {
          stop_streaming();
          printf(">> streaming stopped\n");
        } else {
          start_streaming();
          printf(">> streaming started\n");
        }
        break;
      case 2:
        if (streaming) {
          stop_streaming();
          printf(">> streaming stopped\n");
        }
        goto exit;
      default:
        printf(">> invalid option\n");
        continue;
    }
  } while (1);

  fclose(video_fp);

exit:
  free(p_buf);
  free(v_buf);
  qcamlib_destroy(h_cam);
  printf(">> DONE\n");
  return 0;
}
