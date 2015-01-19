ACLOCAL_AMFLAGS = -I m4

#libmmcamera_interface.so
lib_LTLIBRARIES = libmmcamera_interface.la

libmmcamera_interface_la_CFLAGS := -fPIC
libmmcamera_interface_la_CFLAGS += -DUSE_ION
if MSM8974
libmmcamera_interface_la_CFLAGS += -DVENUS_PRESENT
endif

libmmcamera_interface_la_CFLAGS += -I inc
libmmcamera_interface_la_CFLAGS += -I ../common
#TODO:
#LOCAL_C_INCLUDES += hardware/qcom/media/mm-core/inc

h_sources = ../common/cam_intf.h \
            ../common/cam_types.h \
            ../common/cam_list.h \
            ../common/cam_queue.h \
            ../common/cam_semaphore.h \
            ../common/mm_camera_interface.h \
            ../common/mm_jpeg_interface.h \
            inc/mm_camera_dbg.h \
            inc/mm_camera.h \
            inc/mm_camera_sock.h
pkgincludedir = $(includedir)/mm_camera_interface
library_includedir = $(pkgincludedir)
library_include_HEADERS = $(h_sources)

libmmcamera_interface_la_SOURCES := src/mm_camera_interface.c
libmmcamera_interface_la_SOURCES += src/mm_camera.c
libmmcamera_interface_la_SOURCES += src/mm_camera_channel.c
libmmcamera_interface_la_SOURCES += src/mm_camera_stream.c
libmmcamera_interface_la_SOURCES += src/mm_camera_thread.c
libmmcamera_interface_la_SOURCES += src/mm_camera_sock.c
libmmcamera_interface_la_SOURCES += src/cam_intf.c

#TODO:
#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = camera-hal.pc
#EXTRA_DIST = $(pkgconfig_DATA)

libmmcamera_interface_la_LDFLAGS := -shared
libmmcamera_interface_la_LIBADD := -ldl -lpthread

dirs :=
SUBDIRS = $(dirs)

if USE_DLOG
libmmcamera_interface_la_CFLAGS += -DUSE_DLOG @DLOG_CFLAGS@
libmmcamera_interface_la_LIBADD += @DLOG_LIBS@
endif
