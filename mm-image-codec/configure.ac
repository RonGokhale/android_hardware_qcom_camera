#                                               -*- Autoconf -*-
# configure.ac -- Autoconf script for mm-image-codec
#

# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([mm-image-codec],1.0.0)

AM_INIT_AUTOMAKE([-Wall -Werror gnu foreign subdir-objects])
AM_MAINTAINER_MODE

AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AM_PROG_CC_C_O
AC_PROG_LIBTOOL
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

AC_ARG_WITH([dlog],
      AC_HELP_STRING([--with-dlog],
         [enable dlog, building HLOS systems which use dlog]))

if (test "x${with_dlog}" = "xyes"); then
      AC_DEFINE(ENABLE_USEDLOG, 1, [Define if HLOS systems uses dlog])
      PKG_CHECK_MODULES(DLOG, [dlog])
      AC_SUBST(DLOG_CFLAGS)
      AC_SUBST(DLOG_LIBS)
fi

AM_CONDITIONAL(USE_DLOG, test "x${with_dlog}" = "xyes")

AC_CHECK_TOOL(OBJCOPY, objcopy, false)

AC_ARG_WITH([sanitized-headers],
	[AS_HELP_STRING([--with-sanitized-headers=DIR],[location of the sanitized Linux kernel headers])],
	[CFLAGS="$CFLAGS -I $withval"])

AC_ARG_WITH([omx-includes],
        [AS_HELP_STRING([--with-omx-includes=DIR],[location of OMX interface headers])],
        [CPPFLAGS="$CPPFLAGS -I$withval"])

AC_CONFIG_FILES([ \
    Makefile
   ])
AC_OUTPUT
