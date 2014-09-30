ACLOCAL_AMFLAGS = -I m4

#libqomx_core.so
lib_LTLIBRARIES = libqomx_core.la

libqomx_core_la_CFLAGS := -fPIC

libqomx_core_la_CFLAGS += -g -O0
libqomx_core_la_CPPFLAGS := -DLOGE=ALOGE
libqomx_core_la_CPPFLAGS += -DLOGE=ALOGD

libqomx_core_la_CPPFLAGS += -I ../qexif
libqomx_core_la_CPPFLAGS += -I qomx_core

libqomx_core_la_SOURCES := qomx_core/qomx_core.c

# Build explicit static lib only to avoid multple omx name conflict
#LOCAL_SHARED_LIBRARIES := libcutils libdl
libqomx_core_la_LDFLAGS := -static
libqomx_core_la_LIBADD := -ldl -lrt

if USE_DLOG
libqomx_core_la_CFLAGS += -DUSE_DLOG @DLOG_CFLAGS@
libqomx_core_la_LIBADD += @DLOG_LIBS@
endif
