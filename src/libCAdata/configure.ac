dnl#                                               -*- Autoconf -*-
dnl# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.70])
AC_INIT([libCAdata],[1.0],[suravee.suthikulpanit@amd.com])
AC_CONFIG_SRCDIR([unitTest/stdafx.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign])dnl

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
LT_INIT([disable-shared])
AC_PATH_PROG([SCONS],[scons],[scons])dnl

AC_ARG_WITH([CommonProjectsHdr],
            [AS_HELP_STRING([--with-CommonProjectsHdr],
                            [use CommonProjectsHdr])],
            [],[])
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug],[enable debugging])],
              [],[])

# Checks for header files.
AC_HEADER_STDBOOL dnl# also "_CHECK"s it

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Output.
AC_CONFIG_FILES([m4/Makefile])
AC_CONFIG_COMMANDS([default],[scons])
AC_OUTPUT
