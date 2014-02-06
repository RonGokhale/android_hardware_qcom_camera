ACLOCAL_AMFLAGS = -I m4

#libutils.so
lib_LTLIBRARIES = libutils.la

libutils_la_CPPFLAGS := -fPIC

libutils_la_CPPFLAGS += -I ../../include
libutils_la_CPPFLAGS += -I ../HAL
libutils_la_CPPFLAGS += -DLINUX_ENABLED=1
libutils_la_CPPFLAGS += -DHAVE_PTHREADS=1
libutils_la_CPPFLAGS += -D_ANDROID_

libutils_la_SOURCES := RefBase.cpp \
        SharedBuffer.cpp \
        StopWatch.cpp \
        String8.cpp \
        String16.cpp \
        StringArray.cpp \
        SystemClock.cpp \
        Timers.cpp \
        VectorImpl.cpp \
        misc.cpp

libutils_la_LDFLAGS := -shared
libutils_la_LIBADD := -ldl -lpthread -lcutils -llog -lrt

if USE_DLOG
libutils_la_CPPFLAGS += -DUSE_DLOG @DLOG_CFLAGS@
libutils_la_LIBADD += @DLOG_LIBS@
endif

