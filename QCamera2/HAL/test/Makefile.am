ACLOCAL_AMFLAGS = -I m4

#### camera hal binding to lua
lib_LTLIBRARIES := libluacamerahal.la

libluacamerahal_la_CFLAGS := $(DEBUG_CFLAGS) -DAMSS_VERSION=$(AMSS_VERSION) -Dstrcat=g_strlcat
libluacamerahal_la_CFLAGS +=  -Wall -fno-short-enums -O0
libluacamerahal_la_CFLAGS += -I ..
libluacamerahal_la_SOURCES := QCamera2HalLuaBind.c

libluacamerahal_la_LDFLAGS := -shared
libluacamerahal_la_LIBADD := -llog -ldl -lpthread -lrt -lm
libluacamerahal_la_LIBADD += -llua

if USE_DLOG
libluacamerahal_la_CFLAGS += -DUSE_DLOG @DLOG_CFLAGS@
libluacamerahal_la_LIBADD += @DLOG_LIBS@
endif

#### camera ioctl binding to lua
lib_LTLIBRARIES += libluacamerakernel.la

libluacamerakernel_la_CFLAGS := $(DEBUG_CFLAGS) -DAMSS_VERSION=$(AMSS_VERSION) -Dstrcat=g_strlcat
libluacamerakernel_la_CFLAGS +=  -Wall -fno-short-enums -O0

libluacamerakernel_la_CFLAGS += -I ..
libluacamerakernel_la_SOURCES := CameraKernelLuaBind.c

libluacamerakernel_la_LDFLAGS := -shared
libluacamerakernel_la_LIBADD := -llog -ldl -lpthread -lrt -lm
libluacamerakernel_la_LIBADD += -llua

if USE_DLOG
libluacamerakernel_la_CFLAGS += -DUSE_DLOG @DLOG_CFLAGS@
libluacamerakernel_la_LIBADD += @DLOG_LIBS@
endif
