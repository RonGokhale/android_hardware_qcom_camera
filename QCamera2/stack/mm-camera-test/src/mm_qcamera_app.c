/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

#include <cutils/properties.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <linux/msm_ion.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>

#include "mm_qcamera_dbg.h"
#include "mm_qcamera_app.h"

static pthread_mutex_t app_mutex;
static int thread_status = 0;
static pthread_cond_t app_cond_v;


#define MM_QCAMERA_APP_NANOSEC_SCALE 1000000000

int mm_camera_app_timedwait(uint8_t seconds)
{
    int rc = 0;
    pthread_mutex_lock(&app_mutex);
    if(FALSE == thread_status) {
        struct timespec tw;
        memset(&tw, 0, sizeof tw);
        tw.tv_sec = 0;
        tw.tv_nsec = time(0) + seconds * MM_QCAMERA_APP_NANOSEC_SCALE;

        rc = pthread_cond_timedwait(&app_cond_v, &app_mutex,&tw);
        thread_status = FALSE;
    }
    pthread_mutex_unlock(&app_mutex);
    return rc;
}

int mm_camera_app_wait()
{
    int rc = 0;
    pthread_mutex_lock(&app_mutex);
    if(FALSE == thread_status){
        pthread_cond_wait(&app_cond_v, &app_mutex);
        thread_status = FALSE;
    }
    pthread_mutex_unlock(&app_mutex);
    return rc;
}

void mm_camera_app_done()
{
  pthread_mutex_lock(&app_mutex);
  thread_status = TRUE;
  pthread_cond_signal(&app_cond_v);
  pthread_mutex_unlock(&app_mutex);
}

int mm_app_load_hal(mm_camera_app_t *my_cam_app)
{
    memset(&my_cam_app->hal_lib, 0, sizeof(hal_interface_lib_t));
    my_cam_app->hal_lib.ptr = dlopen("libmmcamera_interface.so", RTLD_NOW);
    if( NULL == my_cam_app->hal_lib.ptr) {
        CDBG_ERROR("%s: dlopen error: %s", __func__, dlerror());
    }
    assert(NULL != my_cam_app->hal_lib.ptr);

    my_cam_app->hal_lib.ptr_jpeg = dlopen("libmmjpeg_interface.so", RTLD_NOW);
    if( NULL == my_cam_app->hal_lib.ptr_jpeg) {
        CDBG_ERROR("%s: dlopen error: %s", __func__, dlerror());
    }
    assert(NULL != my_cam_app->hal_lib.ptr_jpeg);

    *(void **)&(my_cam_app->hal_lib.get_num_of_cameras) =
        dlsym(my_cam_app->hal_lib.ptr, "get_num_of_cameras");
    assert(NULL != my_cam_app->hal_lib.get_num_of_cameras);

    *(void **)&(my_cam_app->hal_lib.mm_camera_open) =
        dlsym(my_cam_app->hal_lib.ptr, "camera_open");
    assert(NULL != my_cam_app->hal_lib.mm_camera_open);

    *(void **)&(my_cam_app->hal_lib.jpeg_open) =
        dlsym(my_cam_app->hal_lib.ptr_jpeg, "jpeg_open");
    assert(NULL != my_cam_app->hal_lib.jpeg_open);

    my_cam_app->num_cameras = my_cam_app->hal_lib.get_num_of_cameras();
    CDBG_HIGH("%s: num_cameras = %d\n", __func__, my_cam_app->num_cameras);

    return MM_CAMERA_OK;
}

int mm_app_allocate_ion_memory(mm_camera_app_buf_t *buf, int ion_type)
{
    int rc = MM_CAMERA_OK;
    struct ion_handle_data handle_data;
    struct ion_allocation_data alloc;
    struct ion_fd_data ion_info_fd;
    int main_ion_fd = 0;
    void *data = NULL;

    main_ion_fd = open("/dev/ion", O_RDONLY);
    if (main_ion_fd <= 0) {
        CDBG_ERROR("%s: Ion dev open failed %s\n",__func__, strerror(errno));
        goto ION_OPEN_FAILED;
    }

    memset(&alloc, 0, sizeof(alloc));
    alloc.len = buf->mem_info.size;
    /* to make it page size aligned */
    alloc.len = (alloc.len + 4095) & (~4095);
    alloc.align = 4096;
    alloc.flags = ION_FLAG_CACHED;
    alloc.heap_mask = ion_type;
    rc = ioctl(main_ion_fd, ION_IOC_ALLOC, &alloc);
    if (rc < 0) {
        CDBG_ERROR("%s: ION allocation failed\n", __func__);
        goto ION_ALLOC_FAILED;
    }

    memset(&ion_info_fd, 0, sizeof(ion_info_fd));
    ion_info_fd.handle = alloc.handle;
    rc = ioctl(main_ion_fd, ION_IOC_SHARE, &ion_info_fd);
    if (rc < 0) {
        CDBG_ERROR("%s: ION map failed %s\n", __func__, strerror(errno));
        goto ION_MAP_FAILED;
    }

    data = mmap(NULL,
                alloc.len,
                PROT_READ  | PROT_WRITE,
                MAP_SHARED,
                ion_info_fd.fd,
                0);

    if (data == MAP_FAILED) {
        CDBG_ERROR("%s: ION_MMAP_FAILED: %s (%d)\n",__func__, strerror(errno), errno);
        goto ION_MAP_FAILED;
    }
    buf->mem_info.main_ion_fd = main_ion_fd;
    buf->mem_info.fd = ion_info_fd.fd;
    buf->mem_info.handle = ion_info_fd.handle;
    buf->mem_info.size = alloc.len;
    buf->mem_info.data = data;
    return MM_CAMERA_OK;

ION_MAP_FAILED:
    memset(&handle_data, 0, sizeof(handle_data));
    handle_data.handle = ion_info_fd.handle;
    ioctl(main_ion_fd, ION_IOC_FREE, &handle_data);
ION_ALLOC_FAILED:
    close(main_ion_fd);
ION_OPEN_FAILED:
    return -MM_CAMERA_E_GENERAL;
}

int mm_app_deallocate_ion_memory(mm_camera_app_buf_t *buf)
{
  struct ion_handle_data handle_data;
  int rc = 0;

  rc = munmap(buf->mem_info.data, buf->mem_info.size);

  if (buf->mem_info.fd > 0) {
      close(buf->mem_info.fd);
      buf->mem_info.fd = 0;
  }

  if (buf->mem_info.main_ion_fd > 0) {
      memset(&handle_data, 0, sizeof(handle_data));
      handle_data.handle = buf->mem_info.handle;
      ioctl(buf->mem_info.main_ion_fd, ION_IOC_FREE, &handle_data);
      close(buf->mem_info.main_ion_fd);
      buf->mem_info.main_ion_fd = 0;
  }
  return rc;
}

/* cmd = ION_IOC_CLEAN_CACHES, ION_IOC_INV_CACHES, ION_IOC_CLEAN_INV_CACHES */
int mm_app_cache_ops(mm_camera_app_meminfo_t *mem_info,
                     unsigned int cmd)
{
    struct ion_flush_data cache_inv_data;
    struct ion_custom_data custom_data;
    int ret = MM_CAMERA_OK;

#ifdef USE_ION
    if (NULL == mem_info) {
        CDBG_ERROR("%s: mem_info is NULL, return here", __func__);
        return -MM_CAMERA_E_GENERAL;
    }

    memset(&cache_inv_data, 0, sizeof(cache_inv_data));
    memset(&custom_data, 0, sizeof(custom_data));
    cache_inv_data.vaddr = mem_info->data;
    cache_inv_data.fd = mem_info->fd;
    cache_inv_data.handle = mem_info->handle;
    cache_inv_data.length = mem_info->size;
    custom_data.cmd = cmd;
    custom_data.arg = (unsigned long)&cache_inv_data;

    CDBG("addr = %p, fd = %d, handle = %p length = %d, ION Fd = %d",
         cache_inv_data.vaddr, cache_inv_data.fd,
         cache_inv_data.handle, cache_inv_data.length,
         mem_info->main_ion_fd);
    if(mem_info->main_ion_fd > 0) {
        if(ioctl(mem_info->main_ion_fd, ION_IOC_CUSTOM, &custom_data) < 0) {
            CDBG_ERROR("%s: Cache Invalidate failed\n", __func__);
            ret = -MM_CAMERA_E_GENERAL;
        }
    }
#endif

    return ret;
}

void mm_app_dump_frame(mm_camera_buf_def_t *frame,
                       char *name,
                       char *ext,
                       int frame_idx)
{
    char file_name[64];
    int file_fd;
    int i;
    uint32_t offset = 0;

    if ( frame != NULL) {
        snprintf(file_name, sizeof(file_name), "/data/%s_%d.%s", name, frame_idx, ext);
        file_fd = open(file_name, O_RDWR | O_CREAT, 0777);
        if (file_fd < 0) {
            CDBG_ERROR("%s: cannot open file %s \n", __func__, file_name);
        } else {
            for (i = 0; i < frame->num_planes; i++) {

                offset = offset + frame->planes[i].data_offset;

                write(file_fd,
                      (uint8_t *)((uint32_t)frame->buffer + offset),
                      frame->planes[i].length);

                offset = offset + frame->planes[i].length;
            }

            close(file_fd);
            CDBG("dump %s", file_name);
        }
    }
}

void mm_app_dump_jpeg_frame(const void * data, uint32_t size, char* name, char* ext, int index)
{
    char buf[32];
    int file_fd;
    if ( data != NULL) {
        snprintf(buf, sizeof(buf), "/data/%s_%d.%s", name, index, ext);
        CDBG("%s: %s size =%d, jobId=%d", __func__, buf, size, index);
        file_fd = open(buf, O_RDWR | O_CREAT, 0777);
        write(file_fd, data, size);
        close(file_fd);
    }
}

int mm_app_alloc_bufs(mm_camera_app_buf_t* app_bufs,
                      cam_frame_len_offset_t *frame_offset_info,
                      uint8_t num_bufs,
                      uint8_t is_streambuf)
{
    int i, j;
    int rc = MM_CAMERA_OK;
    int ion_type = 0x1 << CAMERA_ION_FALLBACK_HEAP_ID;

    CDBG("%s: Enter", __func__);

    if (is_streambuf) {
        ion_type |= 0x1 << CAMERA_ION_HEAP_ID;
    }

    for (i = 0; i < num_bufs ; i++) {
        app_bufs[i].mem_info.size = frame_offset_info->frame_len;

        rc = mm_app_allocate_ion_memory(&app_bufs[i], ion_type);
        if (MM_CAMERA_OK != rc) {
            app_bufs[i].buf.buffer = NULL;
            goto end;
        }

        app_bufs[i].buf.buf_idx = i;
        app_bufs[i].buf.num_planes = frame_offset_info->num_planes;
        app_bufs[i].buf.fd = app_bufs[i].mem_info.fd;
        app_bufs[i].buf.frame_len = app_bufs[i].mem_info.size;
        app_bufs[i].buf.buffer = app_bufs[i].mem_info.data;
        app_bufs[i].buf.mem_info = (void *)&app_bufs[i].mem_info;

        /* Plane 0 needs to be set seperately. Set other planes
             * in a loop. */
        app_bufs[i].buf.planes[0].length = frame_offset_info->mp[0].len;
        app_bufs[i].buf.planes[0].m.userptr = app_bufs[i].buf.fd;
        app_bufs[i].buf.planes[0].data_offset = frame_offset_info->mp[0].offset;
        app_bufs[i].buf.planes[0].reserved[0] = 0;
        for (j = 1; j < frame_offset_info->num_planes; j++) {
            app_bufs[i].buf.planes[j].length = frame_offset_info->mp[j].len;
            app_bufs[i].buf.planes[j].m.userptr = app_bufs[i].buf.fd;
            app_bufs[i].buf.planes[j].data_offset = frame_offset_info->mp[j].offset;
            app_bufs[i].buf.planes[j].reserved[0] =
                app_bufs[i].buf.planes[j-1].reserved[0] +
                app_bufs[i].buf.planes[j-1].length;
        }
    }

end:
    CDBG("%s: Exit", __func__);
    return rc;
}

int mm_app_release_bufs(uint8_t num_bufs,
                        mm_camera_app_buf_t* app_bufs)
{
    int i, rc = MM_CAMERA_OK;

    CDBG("%s: E", __func__);

    for (i = 0; i < num_bufs; i++) {
        rc = mm_app_deallocate_ion_memory(&app_bufs[i]);
    }
    memset(app_bufs, 0, num_bufs * sizeof(mm_camera_app_buf_t));
    CDBG("%s: X", __func__);
    return rc;
}

int mm_app_stream_initbuf(cam_frame_len_offset_t *frame_offset_info,
                          uint8_t *num_bufs,
                          uint8_t **initial_reg_flag,
                          mm_camera_buf_def_t **bufs,
                          mm_camera_map_unmap_ops_tbl_t *ops_tbl,
                          void *user_data)
{
    mm_camera_stream_t *stream = (mm_camera_stream_t *)user_data;
    mm_camera_buf_def_t *pBufs = NULL;
    uint8_t *reg_flags = NULL;
    int i, rc;

    stream->offset = *frame_offset_info;
    CDBG("%s: alloc buf for stream_id %d, len=%d",
         __func__, stream->s_id, frame_offset_info->frame_len);

    pBufs = (mm_camera_buf_def_t *)malloc(sizeof(mm_camera_buf_def_t) * stream->num_of_bufs);
    reg_flags = (uint8_t *)malloc(sizeof(uint8_t) * stream->num_of_bufs);
    if (pBufs == NULL || reg_flags == NULL) {
        CDBG_ERROR("%s: No mem for bufs", __func__);
        if (pBufs != NULL) {
            free(pBufs);
        }
        if (reg_flags != NULL) {
            free(reg_flags);
        }
        return -1;
    }

    rc = mm_app_alloc_bufs(&stream->s_bufs[0],
                           frame_offset_info,
                           stream->num_of_bufs,
                           1);

    if (rc != MM_CAMERA_OK) {
        CDBG_ERROR("%s: mm_stream_alloc_bufs err = %d", __func__, rc);
        free(pBufs);
        free(reg_flags);
        return rc;
    }

    for (i = 0; i < stream->num_of_bufs; i++) {
        /* mapping stream bufs first */
        pBufs[i] = stream->s_bufs[i].buf;
        reg_flags[i] = 1;
        rc = ops_tbl->map_ops(pBufs[i].buf_idx,
                              -1,
                              pBufs[i].fd,
                              pBufs[i].frame_len,
                              ops_tbl->userdata);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s: mapping buf[%d] err = %d", __func__, i, rc);
            break;
        }
    }

    if (rc != MM_CAMERA_OK) {
        int j;
        for (j=0; j>i; j++) {
            ops_tbl->unmap_ops(pBufs[j].buf_idx, -1, ops_tbl->userdata);
        }
        mm_app_release_bufs(stream->num_of_bufs, &stream->s_bufs[0]);
        free(pBufs);
        free(reg_flags);
        return rc;
    }

    *num_bufs = stream->num_of_bufs;
    *bufs = pBufs;
    *initial_reg_flag = reg_flags;

    CDBG("%s: X",__func__);
    return rc;
}

int32_t mm_app_stream_deinitbuf(mm_camera_map_unmap_ops_tbl_t *ops_tbl,
                                void *user_data)
{
    mm_camera_stream_t *stream = (mm_camera_stream_t *)user_data;
    int i;

    for (i = 0; i < stream->num_of_bufs ; i++) {
        /* mapping stream bufs first */
        ops_tbl->unmap_ops(stream->s_bufs[i].buf.buf_idx, -1, ops_tbl->userdata);
    }

    mm_app_release_bufs(stream->num_of_bufs, &stream->s_bufs[0]);

    CDBG("%s: X",__func__);
    return 0;
}

int32_t mm_app_stream_clean_invalidate_buf(int index, void *user_data)
{
    mm_camera_stream_t *stream = (mm_camera_stream_t *)user_data;
    return mm_app_cache_ops(&stream->s_bufs[index].mem_info,
      ION_IOC_CLEAN_INV_CACHES);
}

int32_t mm_app_stream_invalidate_buf(int index, void *user_data)
{
    mm_camera_stream_t *stream = (mm_camera_stream_t *)user_data;
    return mm_app_cache_ops(&stream->s_bufs[index].mem_info, ION_IOC_INV_CACHES);
}

static void notify_evt_cb(uint32_t camera_handle,
                          mm_camera_event_t *evt,
                          void *user_data)
{
    mm_camera_test_obj_t *test_obj =
        (mm_camera_test_obj_t *)user_data;
    if (test_obj == NULL || test_obj->cam->camera_handle != camera_handle) {
        CDBG_ERROR("%s: Not a valid test obj", __func__);
        return;
    }

    CDBG("%s:E evt = %d", __func__, evt->server_event_type);
    switch (evt->server_event_type) {
       case CAM_EVENT_TYPE_AUTO_FOCUS_DONE:
           CDBG("%s: rcvd auto focus done evt", __func__);
           break;
       case CAM_EVENT_TYPE_ZOOM_DONE:
           CDBG("%s: rcvd zoom done evt", __func__);
           break;
       default:
           break;
    }

    CDBG("%s:X", __func__);
}

int mm_app_open(mm_camera_app_t *cam_app,
                uint8_t cam_id,
                mm_camera_test_obj_t *test_obj)
{
    int32_t rc;
    cam_frame_len_offset_t offset_info;

    CDBG_HIGH("%s:Enter\n", __func__);

    test_obj->cam = cam_app->hal_lib.mm_camera_open(cam_id);
    assert(NULL != test_obj->cam);

    test_obj->cam_id = cam_id;
    CDBG("Open Camera id = %d handle = %d", cam_id, test_obj->cam->camera_handle);

    /* alloc ion mem for capability buf */
    memset(&offset_info, 0, sizeof(offset_info));
    offset_info.frame_len = sizeof(cam_capability_t);
    rc = mm_app_alloc_bufs(&test_obj->cap_buf,
                           &offset_info,
                           1,
                           0);
    assert(rc == MM_CAMERA_OK);

    /* mapping capability buf */
    rc = test_obj->cam->ops->map_buf(test_obj->cam->camera_handle,
                                     CAM_MAPPING_BUF_TYPE_CAPABILITY,
                                     test_obj->cap_buf.mem_info.fd,
                                     test_obj->cap_buf.mem_info.size);
    assert(rc == MM_CAMERA_OK);

    /* alloc ion mem for getparm buf */
    memset(&offset_info, 0, sizeof(offset_info));
    offset_info.frame_len = sizeof(parm_buffer_t);
    rc = mm_app_alloc_bufs(&test_obj->parm_buf,
                           &offset_info,
                           1,
                           0);
    assert(rc == MM_CAMERA_OK);

    /* mapping getparm buf */
    rc = test_obj->cam->ops->map_buf(test_obj->cam->camera_handle,
                                     CAM_MAPPING_BUF_TYPE_PARM_BUF,
                                     test_obj->parm_buf.mem_info.fd,
                                     test_obj->parm_buf.mem_info.size);
    assert(rc == MM_CAMERA_OK);

    test_obj->params_buffer = (parm_buffer_t*) test_obj->parm_buf.mem_info.data;
    CDBG("\n%s params_buffer=%p\n",__func__,test_obj->params_buffer);

    rc = test_obj->cam->ops->register_event_notify(test_obj->cam->camera_handle,
                                                   notify_evt_cb,
                                                   test_obj);
    assert(rc == MM_CAMERA_OK);

    rc = test_obj->cam->ops->query_capability(test_obj->cam->camera_handle);
    assert(rc == MM_CAMERA_OK);

    memset(&test_obj->jpeg_ops, 0, sizeof(mm_jpeg_ops_t));
    test_obj->jpeg_hdl = cam_app->hal_lib.jpeg_open(&test_obj->jpeg_ops);
    assert(NULL != test_obj->jpeg_hdl);

    //add reference to app handle in test object. This is needed for
    // accessing widht/height/format etc of test
    test_obj->app_handle = cam_app;

    /* alloc ion mem for jpeg output buf */
    memset(&offset_info, 0, sizeof(offset_info));
    offset_info.frame_len = cam_app->snapshot_width * cam_app->snapshot_height;

    CDBG("%s: width:%ul, height:%ul, frame_leng::%ul", __func__,
        cam_app->snapshot_width, cam_app->snapshot_height, offset_info.frame_len);

    rc = mm_app_alloc_bufs(&test_obj->jpeg_buf,
                           &offset_info,
                           1,
                           0);
    assert(rc == MM_CAMERA_OK);

    //init jpeg frame queue structure to 0
    memset(test_obj->jpeg_frame_queue, 0, sizeof(mm_camera_app_frame_stack) * CAMERA_MAX_JPEG_SESSIONS);

    CDBG_HIGH("%s:Exit\n", __func__);

    return rc;
}

int mm_app_set_params(mm_camera_test_obj_t *test_obj,
                      cam_intf_parm_type_t param_type,
                      uint32_t paramLength,
                      void *paramValue)
{
    int rc = MM_CAMERA_OK;
    assert(NULL != test_obj);
    assert(NULL != paramValue);

    CDBG(":%s: param_type =%d, param value: %s, param length =%d",__func__, param_type, paramValue, paramLength);

    rc = init_batch_update(test_obj->params_buffer);
    assert(MM_CAMERA_OK == rc);

    rc = add_parm_entry_tobatch(test_obj->params_buffer, param_type, paramLength, paramValue);
    assert(MM_CAMERA_OK == rc);

    rc = commit_set_batch(test_obj);
    assert(MM_CAMERA_OK == rc);

    return rc;
}

int init_batch_update(parm_buffer_t *p_table)
{
    int rc = MM_CAMERA_OK;
    CDBG("%s: Enter ",__func__);

    assert(NULL != p_table);

    int32_t hal_version = CAM_HAL_V1;

    memset(p_table, 0, sizeof(parm_buffer_t));
    p_table->first_flagged_entry = CAM_INTF_PARM_MAX;
    rc = add_parm_entry_tobatch(p_table, CAM_INTF_PARM_HAL_VERSION,sizeof(hal_version), &hal_version);
    assert(MM_CAMERA_OK == rc);

    CDBG("%s: Exit ",__func__);
    return rc;
}

int add_parm_entry_tobatch(parm_buffer_t *p_table,
                           cam_intf_parm_type_t paramType,
                           uint32_t paramLength,
                           void *paramValue)
{
    int rc = MM_CAMERA_OK;
    int position = paramType;
    int current, next;

    CDBG("%s: Enter ",__func__);

    current = GET_FIRST_PARAM_ID(p_table);
    if (position == current){
        //DO NOTHING
    } else if (position < current){
        SET_NEXT_PARAM_ID(position, p_table, current);
        SET_FIRST_PARAM_ID(p_table, position);
    } else {
        /* Search for the position in the linked list where we need to slot in*/
        while (position > GET_NEXT_PARAM_ID(current, p_table))
            current = GET_NEXT_PARAM_ID(current, p_table);

        /*If node already exists no need to alter linking*/
        if (position != GET_NEXT_PARAM_ID(current, p_table)) {
            next = GET_NEXT_PARAM_ID(current, p_table);
            SET_NEXT_PARAM_ID(current, p_table, position);
            SET_NEXT_PARAM_ID(position, p_table, next);
        }
    }
    if (paramLength > sizeof(parm_type_t)) {
        CDBG_ERROR("%s:Size of input larger than max entry size",__func__);
        return -1;
    }
    memcpy(POINTER_OF(paramType,p_table), paramValue, paramLength);

    CDBG("%s: Exit ",__func__);
    return rc;
}

int commit_set_batch(mm_camera_test_obj_t *test_obj)
{
    int rc = MM_CAMERA_OK;

    CDBG("%s: Enter ",__func__);

    if (test_obj->params_buffer->first_flagged_entry < CAM_INTF_PARM_MAX) {
        CDBG("\nset_param p_buffer =%p\n",test_obj->params_buffer);
        rc = test_obj->cam->ops->set_parms(test_obj->cam->camera_handle, test_obj->params_buffer);
    }

    assert(MM_CAMERA_OK == rc);

    CDBG("%s: Exit ",__func__);
    return rc;
}

int mm_app_close(mm_camera_test_obj_t *test_obj)
{
    uint32_t rc = MM_CAMERA_OK;

    CDBG_HIGH("%s:Enter\n", __func__);

    if (test_obj == NULL || test_obj->cam ==NULL) {
        CDBG_ERROR("%s: cam not opened", __func__);
        return -MM_CAMERA_E_GENERAL;
    }

    /* unmap capability buf */
    rc = test_obj->cam->ops->unmap_buf(test_obj->cam->camera_handle,
                                       CAM_MAPPING_BUF_TYPE_CAPABILITY);
    assert(MM_CAMERA_OK == rc);

    /* unmap parm buf */
    rc = test_obj->cam->ops->unmap_buf(test_obj->cam->camera_handle,
                                       CAM_MAPPING_BUF_TYPE_PARM_BUF);
    assert(MM_CAMERA_OK == rc);

    rc = test_obj->cam->ops->close_camera(test_obj->cam->camera_handle);
    test_obj->cam = NULL;
    assert(MM_CAMERA_OK == rc);

    /* close jpeg client */
    if (test_obj->jpeg_hdl && test_obj->jpeg_ops.close) {
        rc = test_obj->jpeg_ops.close(test_obj->jpeg_hdl);
        test_obj->jpeg_hdl = 0;
        assert(MM_CAMERA_OK == rc);
    }

    /* dealloc capability buf */
    rc = mm_app_release_bufs(1, &test_obj->cap_buf);
    assert(MM_CAMERA_OK == rc);

    /* dealloc parm buf */
    rc = mm_app_release_bufs(1, &test_obj->parm_buf);
    assert(MM_CAMERA_OK == rc);

     /* dealloc jpeg buf */
    rc = mm_app_release_bufs(1, &test_obj->jpeg_buf);
    assert(MM_CAMERA_OK == rc);

    CDBG_HIGH("%s:Exit\n", __func__);

    return MM_CAMERA_OK;
}

mm_camera_channel_t * mm_app_add_channel(mm_camera_test_obj_t *test_obj,
                                         mm_camera_channel_type_t ch_type,
                                         mm_camera_channel_attr_t *attr,
                                         mm_camera_buf_notify_t channel_cb,
                                         void *userdata)
{
    uint32_t ch_id = 0;
    mm_camera_channel_t *channel = NULL;

    ch_id = test_obj->cam->ops->add_channel(test_obj->cam->camera_handle,
                                            attr,
                                            channel_cb,
                                            userdata);
    assert(ch_id != 0);

    channel = &test_obj->channels[ch_type];
    channel->ch_id = ch_id;
    return channel;
}

int mm_app_del_channel(mm_camera_test_obj_t *test_obj,
                       mm_camera_channel_t *channel)
{
    test_obj->cam->ops->delete_channel(test_obj->cam->camera_handle,
                                       channel->ch_id);
    memset(channel, 0, sizeof(mm_camera_channel_t));
    return MM_CAMERA_OK;
}

mm_camera_stream_t * mm_app_add_stream(mm_camera_test_obj_t *test_obj,
                                       mm_camera_channel_t *channel)
{
    mm_camera_stream_t *stream = NULL;
    int rc = MM_CAMERA_OK;
    cam_frame_len_offset_t offset_info;

    stream = &(channel->streams[channel->num_streams++]);
    stream->s_id = test_obj->cam->ops->add_stream(test_obj->cam->camera_handle,
                                                  channel->ch_id);
    assert (stream->s_id != 0);

    /* alloc ion mem for stream_info buf */
    memset(&offset_info, 0, sizeof(offset_info));
    offset_info.frame_len = sizeof(cam_stream_info_t);
    rc = mm_app_alloc_bufs(&stream->s_info_buf,
                           &offset_info,
                           1,
                           0);
    assert(rc == MM_CAMERA_OK);

    /* mapping streaminfo buf */
    rc = test_obj->cam->ops->map_stream_buf(test_obj->cam->camera_handle,
                                            channel->ch_id,
                                            stream->s_id,
                                            CAM_MAPPING_BUF_TYPE_STREAM_INFO,
                                            0,
                                            -1,
                                            stream->s_info_buf.mem_info.fd,
                                            stream->s_info_buf.mem_info.size);
    assert(rc == MM_CAMERA_OK);
    return stream;
}

int mm_app_del_stream(mm_camera_test_obj_t *test_obj,
                      mm_camera_channel_t *channel,
                      mm_camera_stream_t *stream)
{
    test_obj->cam->ops->unmap_stream_buf(test_obj->cam->camera_handle,
                                         channel->ch_id,
                                         stream->s_id,
                                         CAM_MAPPING_BUF_TYPE_STREAM_INFO,
                                         0,
                                         -1);
    mm_app_deallocate_ion_memory(&stream->s_info_buf);
    test_obj->cam->ops->delete_stream(test_obj->cam->camera_handle,
                                      channel->ch_id,
                                      stream->s_id);
    memset(stream, 0, sizeof(mm_camera_stream_t));
    return MM_CAMERA_OK;
}

mm_camera_channel_t *mm_app_get_channel_by_type(mm_camera_test_obj_t *test_obj,
                                                mm_camera_channel_type_t ch_type)
{
    return &test_obj->channels[ch_type];
}

int mm_app_config_stream(mm_camera_test_obj_t *test_obj,
                         mm_camera_channel_t *channel,
                         mm_camera_stream_t *stream,
                         mm_camera_stream_config_t *config)
{
    return test_obj->cam->ops->config_stream(test_obj->cam->camera_handle,
                                             channel->ch_id,
                                             stream->s_id,
                                             config);
}

int mm_app_start_channel(mm_camera_test_obj_t *test_obj,
                         mm_camera_channel_t *channel)
{
    return test_obj->cam->ops->start_channel(test_obj->cam->camera_handle,
                                             channel->ch_id);
}

int mm_app_stop_channel(mm_camera_test_obj_t *test_obj,
                        mm_camera_channel_t *channel)
{
    return test_obj->cam->ops->stop_channel(test_obj->cam->camera_handle,
                                            channel->ch_id);
}

int mm_app_request_super_buf(mm_camera_test_obj_t *test_obj,
                         mm_camera_channel_t *channel,
                         uint32_t num_buf_requested)
{
    return test_obj->cam->ops->request_super_buf(test_obj->cam->camera_handle,
                                             channel->ch_id,
                                             num_buf_requested);
}

void print_usage()
{
    CDBG_HIGH("\nMandatory parameters: [-s or -d]");
    CDBG_HIGH("-s: Run single camera testcases");
    CDBG_HIGH("-d: Run dual camera testcases");

    CDBG_HIGH("\nOptional paramters");
    CDBG_HIGH("-m [0/1/2] 0 for manual mode, 1 for menu based testing, 2 for automatic regression. Defaults to manual");
    CDBG_HIGH("-t [n] testcase number. Defaults to testindex 0");
    CDBG_HIGH("-w: preview_width  Width for Image preview");
    CDBG_HIGH("-h: preview_height  Height for Image preview");
    CDBG_HIGH("-W: snapshot_width  Width for Image snapshot");
    CDBG_HIGH("-H: snapshot_height  Height for Image snapshot\n");

    CDBG_HIGH("\nSingle Camera testcases are");
    CDBG_HIGH("\t0: mm_app_tc_open_close");
    CDBG_HIGH("\t1: mm_app_tc_start_stop_preview");
    CDBG_HIGH("\t2: mm_app_tc_start_stop_zsl");
    CDBG_HIGH("\t3: mm_app_tc_start_stop_video_preview");
    CDBG_HIGH("\t4: mm_app_tc_start_stop_video_record");
    CDBG_HIGH("\t5: mm_app_tc_start_stop_live_snapshot");
    CDBG_HIGH("\t6: mm_app_tc_capture_regular");
    CDBG_HIGH("\t7: mm_app_tc_capture_exposure_burst");
    CDBG_HIGH("\t8: mm_app_tc_rdi_cont");
    CDBG_HIGH("\t9: mm_app_tc_rdi_burst");

    CDBG_HIGH("\nDual Camera testcases are");
    CDBG_HIGH("\t0 : mm_app_dtc_start_stop_preview");
}


int main(int argc, char **argv)
{
    int c;
    int rc;
    bool run_tc = false;
    bool run_dual_tc = false;
    mm_camera_app_t mm_camera_app_handle;

    //initialize defaults for tests
    memset(&mm_camera_app_handle, 0, sizeof(mm_camera_app_t));
    mm_camera_app_handle.preview_format = DEFAULT_PREVIEW_FORMAT;
    mm_camera_app_handle.preview_width = DEFAULT_PREVIEW_WIDTH;
    mm_camera_app_handle.preview_height = DEFAULT_PREVIEW_HEIGHT;

    mm_camera_app_handle.snapshot_format = DEFAULT_SNAPSHOT_FORMAT;
    mm_camera_app_handle.snapshot_width = DEFAULT_SNAPSHOT_WIDTH;
    mm_camera_app_handle.snapshot_height = DEFAULT_SNAPSHOT_HEIGHT;

    mm_camera_app_handle.video_format = DEFAULT_VIDEO_FORMAT;
    mm_camera_app_handle.video_width = DEFAULT_VIDEO_WIDTH;
    mm_camera_app_handle.video_height = DEFAULT_VIDEO_HEIGHT;
    mm_camera_app_handle.flip_mode = DEFAULT_FLIP_MODE;

    mm_camera_app_handle.test_mode = 0;
    mm_camera_app_handle.test_idx = 0;
    mm_camera_app_handle.no_display = false;

    while ((c = getopt(argc, argv, "sdm:t:w:h:W:H:")) != -1) {
        switch (c) {
            case 's':
            run_tc = true;
            break;

            case 'd':
            run_dual_tc = true;
            break;

            case 'm':
            mm_camera_app_handle.test_mode = atoi(optarg);
            break;

            case 't':
            mm_camera_app_handle.test_idx = atoi(optarg);
            break;

            case 'w':
            mm_camera_app_handle.preview_width = atoi(optarg);
            break;

            case 'h':
            mm_camera_app_handle.preview_height = atoi(optarg);
            break;

            case 'W':
            mm_camera_app_handle.snapshot_width = atoi(optarg);
            break;

            case 'H':
            mm_camera_app_handle.snapshot_height = atoi(optarg);
            break;

            default:
            print_usage();
            goto end;
        }
    }

    CDBG_HIGH("\nStarting Test with following configuration: \n\tpreview width: %d \n\tpreview height: %d \n\tsnapshot width: %d \n\tsnapshot height: %d\n",
        mm_camera_app_handle.preview_width, mm_camera_app_handle.preview_height,
        mm_camera_app_handle.snapshot_width, mm_camera_app_handle.snapshot_height);

    //if video/preview width is greater than what display can support force no display mode
    if((mm_camera_app_handle.video_width > MM_QCAMERA_APP_MAX_DISPLAY_WIDTH) ||
        (mm_camera_app_handle.preview_width > MM_QCAMERA_APP_MAX_DISPLAY_WIDTH)) {

        CDBG_HIGH("\t\t!! Video/Preview will not be displayed !! \n\t\tresolution exceeds max display resolution");
        CDBG_HIGH("\nPlease check the file dump to verify output\n");

        //Pause for user to see the configs printed above
        sleep(2);
        mm_camera_app_handle.no_display = true;
    }

    //Pause for user to see the configs printed above
    sleep(1);

    if((mm_app_load_hal(&mm_camera_app_handle) != MM_CAMERA_OK)) {
        CDBG_ERROR("%s:mm_app_init err\n", __func__);
        goto end;
    }

    //branch to menu based, single camera or dual camera test app
    if(1 == mm_camera_app_handle.test_mode) {
        menu_based_test_main(&mm_camera_app_handle);
        goto end;
    } else if(true == run_tc) {
        rc = mm_app_unit_test_entry(&mm_camera_app_handle);
        goto end;
    } else if(true == run_dual_tc) {
        rc = mm_app_dual_test_entry(&mm_camera_app_handle);
        goto end;
    } else {
        print_usage();
        goto end;
    }

end:
    /* Clean up and exit. */
    CDBG("Exiting test app\n");
    return 0;
}
