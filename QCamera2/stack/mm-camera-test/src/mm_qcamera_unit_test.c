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

#include "mm_qcamera_dbg.h"
#include "mm_qcamera_app.h"
#include <assert.h>


static mm_app_tc_t mm_app_tc[MM_QCAM_APP_TEST_NUM];

int mm_app_tc_open_close(mm_camera_app_t *cam_app)
{
    int rc = MM_CAMERA_OK;
    int i;
    mm_camera_test_obj_t test_obj;

    CDBG_HIGH("\n Verifying open/close cameras...\n");
    for (i = 0; i < cam_app->num_cameras; i++) {
        memset(&test_obj, 0, sizeof(mm_camera_test_obj_t));
        rc = mm_app_open(cam_app, i, &test_obj);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_open() cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            break;
        }
        sleep(1);
        rc = mm_app_close(&test_obj);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_close() cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            break;
        }
    }
    if (rc == MM_CAMERA_OK) {
        CDBG_LOW("\nPassed\n");
    } else {
        CDBG_LOW("\nFailed\n");
    }
    CDBG_HIGH("%s:END, rc = %d\n", __func__, rc);
    return rc;
}

int mm_app_tc_start_stop_preview(mm_camera_app_t *cam_app)
{
    int rc = MM_CAMERA_OK;
    int i, j;
    mm_camera_test_obj_t test_obj;

    CDBG_HIGH("\n Verifying start/stop preview...\n");

    for (i = 0; i < cam_app->num_cameras; i++) {
        memset(&test_obj, 0, sizeof(mm_camera_test_obj_t));
        rc = mm_app_open(cam_app, i, &test_obj);
        assert(rc == MM_CAMERA_OK);

        for (j = 0; j < MM_QCAMERA_APP_UTEST_INNER_LOOP; j++) {
            rc = mm_app_start_preview(&test_obj);
            assert(rc == MM_CAMERA_OK);

            //Default Auto exposure.
            mm_app_set_params(&test_obj, CAM_INTF_PARM_AEC_ALGO_TYPE, CAM_AEC_MODE_FRAME_AVERAGE);


            sleep(MM_QCAM_APP_TEST_PREVIEW_TIME);

            rc = mm_app_stop_preview(&test_obj);
            assert(rc == MM_CAMERA_OK);
        }

        rc = mm_app_close(&test_obj);
        assert(rc == MM_CAMERA_OK);
    }

    CDBG_HIGH("%s:END, rc = %d\n", __func__, rc);
    return rc;
}

int mm_app_tc_start_stop_zsl(mm_camera_app_t *cam_app)
{
    int rc = MM_CAMERA_OK;
    int i, j;
    mm_camera_test_obj_t test_obj;

    CDBG_HIGH("\n Verifying start/stop zsl...\n");
    for (i = 0; i < cam_app->num_cameras; i++) {
        memset(&test_obj, 0, sizeof(mm_camera_test_obj_t));
        rc = mm_app_open(cam_app, i, &test_obj);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_open() cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            break;
        }

        for (j = 0; j < MM_QCAMERA_APP_UTEST_INNER_LOOP; j++) {
            rc = mm_app_start_preview_zsl(&test_obj);
            if (rc != MM_CAMERA_OK) {
                CDBG_ERROR("%s: mm_app_start_preview_zsl() cam_idx=%d, err=%d\n",
                           __func__, i, rc);
                break;
            }
             sleep(MM_QCAM_APP_TEST_PREVIEW_TIME);
            rc = mm_app_stop_preview_zsl(&test_obj);
            if (rc != MM_CAMERA_OK) {
                CDBG_ERROR("%s: mm_app_stop_preview_zsl() cam_idx=%d, err=%d\n",
                           __func__, i, rc);
                break;
            }
        }

        rc = mm_app_close(&test_obj);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_close() cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            break;
        }
    }
    if (rc == MM_CAMERA_OK) {
        CDBG_LOW("\nPassed\n");
    } else {
        CDBG_LOW("\nFailed\n");
    }
    CDBG_HIGH("%s:END, rc = %d\n", __func__, rc);
    return rc;
}

int mm_app_tc_start_stop_video_preview(mm_camera_app_t *cam_app)
{
    int rc = MM_CAMERA_OK;
    int i, j;
    mm_camera_test_obj_t test_obj;

    CDBG_HIGH("\n Verifying start/stop video preview...\n");
    for (i = 0; i < cam_app->num_cameras; i++) {
        memset(&test_obj, 0, sizeof(mm_camera_test_obj_t));

        rc = mm_app_open(cam_app, i, &test_obj);
        assert(MM_CAMERA_OK == rc);

        for (j = 0; j < MM_QCAMERA_APP_UTEST_INNER_LOOP; j++) {

            rc = mm_app_start_record_preview(&test_obj);
            assert(MM_CAMERA_OK == rc);

            //Set exposure mode to frame average
            mm_app_set_params(&test_obj, CAM_INTF_PARM_AEC_ALGO_TYPE, CAM_AEC_MODE_FRAME_AVERAGE);

            sleep(MM_QCAM_APP_TEST_PREVIEW_TIME);

            rc = mm_app_stop_record_preview(&test_obj);
            assert(MM_CAMERA_OK == rc);
        }

        rc = mm_app_close(&test_obj);
        assert(MM_CAMERA_OK == rc);

    }

    CDBG_HIGH("%s:END, rc = %d\n", __func__, rc);
    return rc;
}

int mm_app_tc_start_stop_video_record(mm_camera_app_t *cam_app)
{
    int rc = MM_CAMERA_OK;
    int i, j;
    mm_camera_test_obj_t test_obj;

    CDBG_HIGH("\n Verifying start/stop recording...\n");
    for (i = 0; i < cam_app->num_cameras; i++) {
        memset(&test_obj, 0, sizeof(mm_camera_test_obj_t));

        rc = mm_app_open(cam_app, i, &test_obj);
        assert(MM_CAMERA_OK == rc);

        rc = mm_app_start_record_preview(&test_obj);
        assert(MM_CAMERA_OK == rc);

        //Set exposure mode to frame average
        mm_app_set_params(&test_obj, CAM_INTF_PARM_AEC_ALGO_TYPE, CAM_AEC_MODE_FRAME_AVERAGE);

        sleep(MM_QCAM_APP_TEST_PREVIEW_TIME);

        for (j = 0; j < MM_QCAMERA_APP_UTEST_INNER_LOOP; j++) {
            rc = mm_app_start_record(&test_obj);
            assert(MM_CAMERA_OK == rc);

            sleep(MM_QCAM_APP_TEST_PREVIEW_TIME);

            rc = mm_app_stop_record(&test_obj);
            assert(MM_CAMERA_OK == rc);
        }

        rc = mm_app_stop_record_preview(&test_obj);
        assert(MM_CAMERA_OK == rc);

        rc = mm_app_close(&test_obj);
        assert(MM_CAMERA_OK == rc);
    }

    CDBG_HIGH("%s:END, rc = %d\n", __func__, rc);
    return rc;
}

int mm_app_tc_start_stop_live_snapshot(mm_camera_app_t *cam_app)
{
    int rc = MM_CAMERA_OK;
    int i, j;
    mm_camera_test_obj_t test_obj;

    CDBG_HIGH("\n Verifying start/stop live snapshot...\n");
    for (i = 0; i < cam_app->num_cameras; i++) {
        memset(&test_obj, 0, sizeof(mm_camera_test_obj_t));
        rc = mm_app_open(cam_app, i, &test_obj);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_open() cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            break;
        }

        rc = mm_app_start_record_preview(&test_obj);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_start_record_preview() cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            mm_app_close(&test_obj);
            break;
        }

        sleep(1);

        rc = mm_app_start_record(&test_obj);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_start_record() cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            mm_app_stop_record_preview(&test_obj);
            mm_app_close(&test_obj);
            break;
        }

        sleep(1);

        for (j = 0; j < MM_QCAMERA_APP_UTEST_INNER_LOOP; j++) {
            rc = mm_app_start_live_snapshot(&test_obj);
            if (rc != MM_CAMERA_OK) {
                CDBG_ERROR("%s:mm_app_start_live_snapshot() cam_idx=%d, err=%d\n",
                           __func__, i, rc);
                break;
            }

            /* wait for jpeg is done */
            mm_camera_app_wait();

            rc = mm_app_stop_live_snapshot(&test_obj);
            if (rc != MM_CAMERA_OK) {
                CDBG_ERROR("%s:mm_app_stop_live_snapshot() cam_idx=%d, err=%d\n",
                           __func__, i, rc);
                break;
            }
        }
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:start/stop live snapshot cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            mm_app_stop_record(&test_obj);
            mm_app_stop_record_preview(&test_obj);
            mm_app_close(&test_obj);
            break;
        }

        rc = mm_app_stop_record(&test_obj);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_stop_record() cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            mm_app_stop_record_preview(&test_obj);
            mm_app_close(&test_obj);
            break;
        }

        sleep(1);

        rc = mm_app_stop_record_preview(&test_obj);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_stop_record_preview() cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            mm_app_close(&test_obj);
            break;
        }

        rc = mm_app_close(&test_obj);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_close() cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            break;
        }
    }
    if (rc == MM_CAMERA_OK) {
        CDBG_LOW("\nPassed\n");
    } else {
        CDBG_LOW("\nFailed\n");
    }
    CDBG_HIGH("%s:END, rc = %d\n", __func__, rc);
    return rc;
}

int mm_app_tc_capture_regular(mm_camera_app_t *cam_app)
{
    int rc = MM_CAMERA_OK;
    int i, j;
    mm_camera_test_obj_t test_obj;

    cam_app->num_snapshot = 1;

    CDBG_HIGH(":%s: \n Verifying capture...\n", __func__);

    for (i = 0; i < cam_app->num_cameras; i++) {
        memset(&test_obj, 0, sizeof(mm_camera_test_obj_t));

        cam_app->num_rcvd_snapshot = 0;

        rc = mm_app_open(cam_app, i, &test_obj);
        assert(MM_CAMERA_OK == rc);

        for (j = 0; j < MM_QCAMERA_APP_UTEST_INNER_LOOP; j++) {

            rc = mm_app_start_preview(&test_obj);
            assert(MM_CAMERA_OK == rc);

            //Set exposure mode to frame average
            mm_app_set_params(&test_obj, CAM_INTF_PARM_AEC_ALGO_TYPE, CAM_AEC_MODE_FRAME_AVERAGE);

            sleep(MM_QCAM_APP_TEST_PREVIEW_TIME);

            rc = mm_app_stop_preview(&test_obj);
            assert(MM_CAMERA_OK == rc);

            rc = mm_app_start_capture(&test_obj, cam_app->num_snapshot);
            assert(MM_CAMERA_OK == rc);

            //wait till required snapshots are captured
            do {
                mm_camera_app_wait();
                CDBG_HIGH("%s: current snapshot count:%d, required count: %d", __func__, cam_app->num_rcvd_snapshot, cam_app->num_snapshot);
            } while (cam_app->num_rcvd_snapshot < cam_app->num_snapshot);

            rc = mm_app_stop_capture(&test_obj);
            assert(MM_CAMERA_OK == rc);
        }

        rc = mm_app_close(&test_obj);
        assert(MM_CAMERA_OK == rc);

    }

    CDBG_HIGH("%s:END, rc = %d\n", __func__, rc);
    return rc;
}

int mm_app_tc_capture_burst(mm_camera_app_t *cam_app)
{
    int rc = MM_CAMERA_OK;
    int i, j;
    mm_camera_test_obj_t test_obj;
    cam_app->num_rcvd_snapshot = 0;
    cam_app->num_snapshot = 3;

    CDBG_HIGH("\n Verifying Burst capture...\n");
    for (i = 0; i < cam_app->num_cameras; i++) {
        memset(&test_obj, 0, sizeof(mm_camera_test_obj_t));
        rc = mm_app_open(cam_app, i, &test_obj);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_open() cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            break;
        }

        for (j = 0; j < MM_QCAMERA_APP_UTEST_INNER_LOOP; j++) {
            rc = mm_app_start_capture(&test_obj, cam_app->num_snapshot);
            if (rc != MM_CAMERA_OK) {
                CDBG_ERROR("%s: mm_app_start_capture() cam_idx=%d, err=%d\n",
                           __func__, i, rc);
                break;
            }

            //wait till required snapshots are captured
            do {
                mm_camera_app_wait();
            } while (cam_app->num_rcvd_snapshot < cam_app->num_snapshot);

            rc = mm_app_stop_capture(&test_obj);
            if (rc != MM_CAMERA_OK) {
                CDBG_ERROR("%s: mm_app_stop_capture() cam_idx=%d, err=%d\n",
                           __func__, i, rc);
                break;
            }
        }

        rc = mm_app_close(&test_obj);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_close() cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            break;
        }
    }
    if (rc == MM_CAMERA_OK) {
        CDBG_LOW("\nPassed\n");
    } else {
        CDBG_LOW("\nFailed\n");
    }
    CDBG_HIGH("%s:END, rc = %d\n", __func__, rc);
    return rc;
}

int mm_app_tc_rdi_burst(mm_camera_app_t *cam_app)
{
    int rc = MM_CAMERA_OK, rc2 = MM_CAMERA_OK;
    int i, j;
    mm_camera_test_obj_t test_obj;

    CDBG_HIGH("\n Verifying rdi burst (3) capture...\n");
    for (i = 0; i < cam_app->num_cameras; i++) {
        memset(&test_obj, 0, sizeof(mm_camera_test_obj_t));
        rc = mm_app_open(cam_app, i, &test_obj);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_open() cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            break;
        }

        for (j = 0; j < MM_QCAMERA_APP_UTEST_INNER_LOOP; j++) {
            rc = mm_app_start_rdi(&test_obj, 3);
            if (rc != MM_CAMERA_OK) {
                CDBG_ERROR("%s: mm_app_start_preview() cam_idx=%d, err=%d\n",
                           __func__, i, rc);
                break;
            }
            sleep(1);
            rc = mm_app_stop_rdi(&test_obj);
            if (rc != MM_CAMERA_OK) {
                CDBG_ERROR("%s: mm_app_stop_preview() cam_idx=%d, err=%d\n",
                           __func__, i, rc);
                break;
            }
        }

        rc2 = mm_app_close(&test_obj);
        if (rc2 != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_close() cam_idx=%d, err=%d\n",
                       __func__, i, rc2);
            if (rc == MM_CAMERA_OK) {
                rc = rc2;
            }
            break;
        }
    }
    if (rc == MM_CAMERA_OK) {
        CDBG_LOW("\nPassed\n");
    } else {
        CDBG_LOW("\nFailed\n");
    }
    CDBG_HIGH("%s:END, rc = %d\n", __func__, rc);
    return rc;
}

int mm_app_tc_rdi_cont(mm_camera_app_t *cam_app)
{
    int rc = MM_CAMERA_OK, rc2 = MM_CAMERA_OK;
    int i, j;
    mm_camera_test_obj_t test_obj;

    CDBG_HIGH("\n Verifying rdi continuous capture...\n");
    for (i = 0; i < cam_app->num_cameras; i++) {
        memset(&test_obj, 0, sizeof(mm_camera_test_obj_t));
        rc = mm_app_open(cam_app, i, &test_obj);
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_open() cam_idx=%d, err=%d\n",
                       __func__, i, rc);
            break;
        }

        for (j = 0; j < MM_QCAMERA_APP_UTEST_INNER_LOOP; j++) {
            rc = mm_app_start_rdi(&test_obj, 0);
            if (rc != MM_CAMERA_OK) {
                CDBG_ERROR("%s: mm_app_start_preview() cam_idx=%d, err=%d\n",
                           __func__, i, rc);
                break;
            }
            sleep(1);
            rc = mm_app_stop_rdi(&test_obj);
            if (rc != MM_CAMERA_OK) {
                CDBG_ERROR("%s: mm_app_stop_preview() cam_idx=%d, err=%d\n",
                           __func__, i, rc);
                break;
            }
        }

        rc2 = mm_app_close(&test_obj);
        if (rc2 != MM_CAMERA_OK) {
            CDBG_ERROR("%s:mm_app_close() cam_idx=%d, err=%d\n",
                       __func__, i, rc2);
            if (rc == MM_CAMERA_OK) {
                rc = rc2;
            }
            break;
        }
    }
    if (rc == MM_CAMERA_OK) {
        CDBG_LOW("\nPassed\n");
    } else {
        CDBG_LOW("\nFailed\n");
    }
    CDBG_HIGH("%s:END, rc = %d\n", __func__, rc);
    return rc;
}

int mm_app_gen_test_cases()
{
    int tc = 0;
    memset(mm_app_tc, 0, sizeof(mm_app_tc));
    if (tc < MM_QCAM_APP_TEST_NUM) mm_app_tc[tc++].f = mm_app_tc_open_close;
    if (tc < MM_QCAM_APP_TEST_NUM) mm_app_tc[tc++].f = mm_app_tc_start_stop_preview;
    if (tc < MM_QCAM_APP_TEST_NUM) mm_app_tc[tc++].f = mm_app_tc_start_stop_zsl;
    if (tc < MM_QCAM_APP_TEST_NUM) mm_app_tc[tc++].f = mm_app_tc_start_stop_video_preview;
    if (tc < MM_QCAM_APP_TEST_NUM) mm_app_tc[tc++].f = mm_app_tc_start_stop_video_record;
    if (tc < MM_QCAM_APP_TEST_NUM) mm_app_tc[tc++].f = mm_app_tc_start_stop_live_snapshot;
    if (tc < MM_QCAM_APP_TEST_NUM) mm_app_tc[tc++].f = mm_app_tc_capture_regular;
    if (tc < MM_QCAM_APP_TEST_NUM) mm_app_tc[tc++].f = mm_app_tc_capture_burst;
    if (tc < MM_QCAM_APP_TEST_NUM) mm_app_tc[tc++].f = mm_app_tc_rdi_cont;
    if (tc < MM_QCAM_APP_TEST_NUM) mm_app_tc[tc++].f = mm_app_tc_rdi_burst;

    return tc;
}

int mm_app_unit_test_entry(mm_camera_app_t *cam_app)
{
    int rc = MM_CAMERA_OK;
    int i = 0, j = 0, k = 0, tc = 0;

    tc = mm_app_gen_test_cases();

    //lower 8 bits of test mode indicate, manual, automatic or menu based
    //next 8 bits to indicate single camera or dual camera mode
    cam_app->test_mode = cam_app->test_mode | (MM_QCAMERA_APP_SINGLE_MODE << 8);

    if(cam_app->test_mode && 0xFF == 1) {

        CDBG_HIGH("\nRunning single camera Automatic regression for %d test cases",tc);
        for(k = 0; k < MM_QCAMERA_APP_UTEST_MAX_MAIN_LOOP; k++)
        {
            for (i = 0; i < tc; i++) {
                for (j = 0; j < MM_QCAMERA_APP_UTEST_OUTER_LOOP; j++) {
                    mm_app_tc[i].r = mm_app_tc[i].f(cam_app);
                    rc = mm_app_tc[i].r;
                    if (rc != MM_CAMERA_OK) {
                        CDBG_ERROR("%s: test case %d (iteration %d) error = %d, abort unit testing engine!!!!\n",
                           __func__, i, j, mm_app_tc[i].r);
                        goto end;
                    }
                }
            }
        }
    } else {
        i = cam_app->test_idx;
        CDBG_HIGH("\nRunning single camera manual tests. Testcase number:%d ",i);
        mm_app_tc[i].r = mm_app_tc[i].f(cam_app);
        rc = mm_app_tc[i].r;
        if (rc != MM_CAMERA_OK) {
            CDBG_ERROR("%s: error = %d, abort unit testing engine!!!!\n",
               __func__, mm_app_tc[i].r);
            goto end;
        }
    }

    end:
    CDBG_HIGH("\nTotal testcases = %d\n, Max Outer Iterations = %d\n, Max Per Test Iterations = %d",
        tc, MM_QCAMERA_APP_UTEST_MAX_MAIN_LOOP, MM_QCAMERA_APP_UTEST_OUTER_LOOP);

    CDBG_HIGH("\nLast RC = %d\n, Outer Iteration Count = %d\n, Test Idx = %d\n, Testcase Iteration Count = %d\n\n", rc, k, i, j);
    return rc;

}




