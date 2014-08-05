# mm-qcamera-app
bin_PROGRAMS := mm-qcamera-app

mm_qcamera_app_CFLAGS := -DAMSS_VERSION=$(AMSS_VERSION)
mm_qcamera_app_CFLAGS += -DUSE_ION

mm_qcamera_app_CFLAGS += -I inc
mm_qcamera_app_CFLAGS += -I ../common
mm_qcamera_app_CFLAGS += -I ../../../
mm_qcamera_app_CFLAGS += -I ../../../mm-image-codec/qexif
mm_qcamera_app_CFLAGS += -I ../../../mm-image-codec/qomx_core

mm_qcamera_app_CFLAGS += -DCAMERA_ION_HEAP_ID=ION_IOMMU_HEAP_ID
if MSM8974
mm_qcamera_app_CFLAGS += -DCAMERA_GRALLOC_HEAP_ID=GRALLOC_USAGE_PRIVATE_IOMMU_HEAP
mm_qcamera_app_CFLAGS += -DCAMERA_GRALLOC_FALLBACK_HEAP_ID=GRALLOC_USAGE_PRIVATE_IOMMU_HEAP
mm_qcamera_app_CFLAGS += -DCAMERA_ION_FALLBACK_HEAP_ID=ION_IOMMU_HEAP_ID
mm_qcamera_app_CFLAGS += -DCAMERA_GRALLOC_CACHING_ID=0
mm_qcamera_app_CFLAGS += -DNUM_RECORDING_BUFFERS=9
endif

#sources
mm_qcamera_app_SOURCES := src/mm_qcamera_app.c
mm_qcamera_app_SOURCES += src/mm_qcamera_unit_test.c
mm_qcamera_app_SOURCES += src/mm_qcamera_dual_test.c
mm_qcamera_app_SOURCES += src/mm_qcamera_video.c
mm_qcamera_app_SOURCES += src/mm_qcamera_preview.c
mm_qcamera_app_SOURCES += src/mm_qcamera_snapshot.c
mm_qcamera_app_SOURCES += src/mm_qcamera_rdi.c
mm_qcamera_app_SOURCES += src/mm_qcamera_main_menu.c
mm_qcamera_app_SOURCES += src/mm_qcamera_display.c

mm_qcamera_app_LDADD := -ldl
mm_qcamera_app_LDADD += ../../../QCamera2/stack/mm-camera-interface/libmmcamera_interface.la

if USE_DLOG
mm_qcamera_app_CFLAGS += -DUSE_DLOG @DLOG_CFLAGS@
mm_qcamera_app_LDADD += @DLOG_LIBS@
endif
