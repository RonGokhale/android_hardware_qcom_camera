ACLOCAL_AMFLAGS = -I m4

#libmmcamera_hal.so
lib_LTLIBRARIES = libmmcamera_hal.la

libmmcamera_hal_la_CPPFLAGS := -fPIC -fvisibility-inlines-hidden -fvisibility=hidden
libmmcamera_hal_la_CPPFLAGS += -DUSE_ION
libmmcamera_hal_la_CPPFLAGS += -DLINUX_ENABLED=1
libmmcamera_hal_la_CPPFLAGS += -g -O2
libmmcamera_hal_la_CPPFLAGS += -DDEFAULT_DENOISE_MODE_ON

libmmcamera_hal_la_CPPFLAGS += -I helper
libmmcamera_hal_la_CPPFLAGS += -I ../stack/common
libmmcamera_hal_la_CPPFLAGS += -I ../stack/mm-camera-interface/inc
libmmcamera_hal_la_CPPFLAGS += -I ../../mm-image-codec/qexif
libmmcamera_hal_la_CPPFLAGS += -I ../../mm-image-codec/qomx_core
libmmcamera_hal_la_CPPFLAGS += -I ../util

libmmcamera_hal_la_SOURCES := QCamera2Factory.cpp
libmmcamera_hal_la_SOURCES += QCamera2Hal.cpp
libmmcamera_hal_la_SOURCES += QCamera2HWI.cpp
libmmcamera_hal_la_SOURCES += QCameraMem.cpp
libmmcamera_hal_la_SOURCES += ../util/QCameraQueue.cpp
libmmcamera_hal_la_SOURCES += ../util/QCameraCmdThread.cpp
libmmcamera_hal_la_SOURCES += QCameraStateMachine.cpp
libmmcamera_hal_la_SOURCES += QCameraChannel.cpp
libmmcamera_hal_la_SOURCES += QCameraStream.cpp
libmmcamera_hal_la_SOURCES += QCameraPostProc.cpp
libmmcamera_hal_la_SOURCES += QCamera2HWICallbacks.cpp
libmmcamera_hal_la_SOURCES += QCameraParameters.cpp
libmmcamera_hal_la_SOURCES += QCameraThermalAdapter.cpp
libmmcamera_hal_la_SOURCES += wrapper/QualcommCamera.cpp
libmmcamera_hal_la_SOURCES += CameraParameters.cpp

libmmcamera_hal_la_LDFLAGS := -shared -Wl,--exclude-libs=ALL
libmmcamera_hal_la_LIBADD := -ldl -lpthread -llog -lglib-2.0 -lutils -lcutils
libmmcamera_hal_la_LIBADD += ../stack/mm-camera-interface/libmmcamera_interface.la ../stack/mm-jpeg-interface/libmmjpeg_interface.la

if USE_DLOG
libmmcamera_hal_la_CPPFLAGS += -DUSE_DLOG @DLOG_CFLAGS@
libmmcamera_hal_la_LIBADD += @DLOG_LIBS@
endif

