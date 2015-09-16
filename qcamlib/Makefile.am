ACLOCAL_AMFLAGS = -I m4

## build libqcam.so ##
lib_LTLIBRARIES = libqcam.la

include_HEADERS = inc/qcamlib.h

libqcam_la_CFLAGS = -fPIC

libqcam_la_CFLAGS += -DCAMERA_ION_HEAP_ID=ION_IOMMU_HEAP_ID
libqcam_la_CFLAGS += -DCAMERA_GRALLOC_HEAP_ID=GRALLOC_USAGE_PRIVATE_IOMMU_HEAP
libqcam_la_CFLAGS += -DCAMERA_GRALLOC_FALLBACK_HEAP_ID=GRALLOC_USAGE_PRIVATE_IOMMU_HEAP
libqcam_la_CFLAGS += -DCAMERA_ION_FALLBACK_HEAP_ID=ION_IOMMU_HEAP_ID
libqcam_la_CFLAGS += -DCAMERA_GRALLOC_CACHING_ID=0
libqcam_la_CFLAGS += -DNUM_RECORDING_BUFFERS=9


libqcam_la_CFLAGS += -I src
libqcam_la_CFLAGS += -I inc
libqcam_la_CFLAGS += -I ../QCamera2/stack/common
libqcam_la_CFLAGS += -I ../
libqcam_la_CFLAGS += -I ../mm-image-codec/qexif
libqcam_la_CFLAGS += -I ../mm-image-codec/qomx_core
libqcam_la_CFLAGS += -I ../QCamera2/stack/mm-camera-interface/inc
libqcam_la_CFLAGS += -I ../QCamera2/stack/mm-camera-test/inc

libqcam_la_SOURCES = src/qcamlib.c
libqcam_la_SOURCES += ../QCamera2/stack/mm-camera-test/src/mm_qcamera_app.c
libqcam_la_SOURCES += ../QCamera2/stack/mm-camera-test/src/mm_qcamera_preview.c
libqcam_la_SOURCES += ../QCamera2/stack/mm-camera-test/src/mm_qcamera_socket.c
libqcam_la_SOURCES += ../QCamera2/stack/mm-camera-test/src/mm_qcamera_commands.c
libqcam_la_SOURCES += ../QCamera2/stack/mm-camera-test/src/mm_qcamera_reprocess.c
libqcam_la_SOURCES += ../QCamera2/stack/mm-camera-test/src/mm_qcamera_snapshot.c
libqcam_la_SOURCES += ../QCamera2/stack/mm-camera-test/src/mm_qcamera_queue.c
libqcam_la_SOURCES += ../QCamera2/stack/mm-camera-test/src/mm_qcamera_unit_test.c
libqcam_la_SOURCES += ../QCamera2/stack/mm-camera-test/src/mm_qcamera_video.c
libqcam_la_SOURCES += ../QCamera2/stack/mm-camera-test/src/mm_qcamera_rdi.c

libqcam_la_LDFLAGS = -shared
libqcam_la_LIBADD = -ldl -lrt
libqcam_la_LIBADD += ../QCamera2/stack/mm-camera-interface/libmmcamera_interface.la

## build qcamlib-test ##

bin_PROGRAMS = qcamlib-test

qcamlib_test_CFLAGS = -I inc
qcamlib_test_CFLAGS += -g

qcamlib_test_SOURCES = test/qcamlib_test.c

qcamlib_test_LDADD = libqcam.la
