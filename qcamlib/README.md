# qcamlib camera API usage

### example call-flow for preview and video streaming using qcamlib API

~~~{.c}
#include "qcamlib.h"

qcamlib_t h_cam;
qcamlib_frame_info_t preview_info, video_info;

h_cam = qcamlib_create();

preview_info = qcamlib_get_frame_info(h_cam, QCAMLIB_STREAM_PREVIEW);
video_info = qcamlib_get_frame_info(h_cam, QCAMLIB_STREAM_VIDEO);

qcamlib_register_preview_cb(h_cam, preview_cb);
qcamlib_register_video_cb(h_cam, video_cb);

qcamlib_start_preview(h_cam);
qcamlib_start_video(h_cam);

// camera is streaming

qcamlib_stop_preview(h_cam);
qcamlib_stop_video(h_cam);

qcamlib_destroy(h_cam);

void preview_cb(qcamlib_cb_data_t frame)
{
    // copy frame to user buffer
    qcamlib_copy_frame_to_buf(frame, preview_info, buf);
    // process the frame
}

void video_cb(qcamlib_cb_data_t frame)
{
    // copy frame to user buffer
    qcamlib_copy_frame_to_buf(frame, video_info, buf);

    // process the frame

    // release the frame
    qcamlib_release_video_frame(h_cam, frame);
}
~~~