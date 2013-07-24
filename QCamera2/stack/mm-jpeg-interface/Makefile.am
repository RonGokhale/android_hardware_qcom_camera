ACLOCAL_AMFLAGS = -I m4

#libmmjpeg_interface.so
lib_LTLIBRARIES = libmmjpeg_interface.la

libmmjpeg_interface_la_CFLAGS := -fPIC

libmmjpeg_interface_la_CFLAGS += -I inc
libmmjpeg_interface_la_CFLAGS += -I ../common
libmmjpeg_interface_la_CFLAGS += -I ../../../mm-image-codec/qexif
libmmjpeg_interface_la_CFLAGS += -I ../../../mm-image-codec/qomx_core

libmmjpeg_interface_la_CFLAGS += -DUSE_ION

libmmjpeg_interface_la_SOURCES := src/mm_jpeg_queue.c
libmmjpeg_interface_la_SOURCES += src/mm_jpeg_exif.c
libmmjpeg_interface_la_SOURCES += src/mm_jpeg.c
libmmjpeg_interface_la_SOURCES += src/mm_jpeg_interface.c
libmmjpeg_interface_la_SOURCES += src/mm_jpeg_ionbuf.c


libmmjpeg_interface_la_LDFLAGS := -shared
libmmjpeg_interface_la_LIBADD := -ldl -lqomx_core -lm

dirs :=
SUBDIRS = $(dirs)

