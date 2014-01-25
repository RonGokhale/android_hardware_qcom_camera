ACLOCAL_AMFLAGS = -I m4

#libmmjpeg_interface.so
lib_LTLIBRARIES = libmmjpeg_interface.la

libmmjpeg_interface_la_CFLAGS := -fPIC

libmmjpeg_interface_la_CFLAGS += -I inc
libmmjpeg_interface_la_CFLAGS += -I ../common
libmmjpeg_interface_la_CFLAGS += -I ../inc/mm_jpeg.h
libmmjpeg_interface_la_CFLAGS += -I ../../../mm-image-codec/qexif
libmmjpeg_interface_la_CFLAGS += -I ../../../mm-image-codec/qomx_core

libmmjpeg_interface_la_CFLAGS += -DUSE_ION
if MSM8974
libmmjpeg_interface_la_CFLAGS += -DMM_JPEG_CONCURRENT_SESSIONS_COUNT=2
else
libmmjpeg_interface_la_CFLAGS += -DMM_JPEG_CONCURRENT_SESSIONS_COUNT=1
endif

libmmjpeg_interface_la_SOURCES := src/mm_jpeg_queue.c
libmmjpeg_interface_la_SOURCES += src/mm_jpeg_exif.c
libmmjpeg_interface_la_SOURCES += src/mm_jpeg.c
libmmjpeg_interface_la_SOURCES += src/mm_jpeg_interface.c
libmmjpeg_interface_la_SOURCES += src/mm_jpeg_ionbuf.c
libmmjpeg_interface_la_SOURCES += src/mm_jpegdec_interface.c
libmmjpeg_interface_la_SOURCES += src/mm_jpegdec.c

libmmjpeg_interface_la_LDFLAGS := -static
libmmjpeg_interface_la_LIBADD := -ldl -lqomx_core -lm

dirs :=
SUBDIRS = $(dirs)

if USE_DLOG
libmmjpeg_interface_la_CFLAGS += -DUSE_DLOG @DLOG_CFLAGS@
libmmjpeg_interface_la_LIBADD += @DLOG_LIBS@
endif
