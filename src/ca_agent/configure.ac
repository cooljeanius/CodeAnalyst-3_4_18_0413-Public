# Process this file with autoconf to produce a configure script

#
# CodeAnalyst for Open Source
# Copyright 2002 . 2005 Advanced Micro Devices, Inc.
# You may redistribute this program and/or modify this program under the terms
# of the GNU General Public License as published by the Free Software 
# Foundation; either version 2 of the License, or (at your option) any later 
# version.
#
# This program is distributed WITHOUT ANY WARRANTY; WITHOUT EVEN THE IMPLIED 
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  See the 
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA 02111-1307 USA.


###############################################
# Initialization
###############################################

AC_INIT([CodeAnalyst-agent], [1.0], [CodeAnalyst.support@amd.com])
AC_CONFIG_SRCDIR([libCAagent/CAagent_buffer.cpp])

AC_CONFIG_HEADERS([config.h])

AC_PREREQ([2.13])

# Initialize Libtool
AC_PROG_LIBTOOL

# Fix issue with basename in gcc-4.4
AC_CHECK_DECLS([basename], [], [], [[#include <libgen.h>]])

AM_INIT_AUTOMAKE

# Default installation directory
AC_PREFIX_DEFAULT([/opt/CodeAnalyst])

###############################################
# Checking configure flags
###############################################
# Detect jvmti/jvmpi #
AC_ARG_WITH([java],
[  --with-java=java-home        Path to Java home directory], JAVA_HOMEDIR=$withval)
AC_SUBST(JAVA_HOMEDIR)

if test -n "$JAVA_HOMEDIR"; then

	if test -f "$JAVA_HOMEDIR/include/jvmti.h"; then
		JVMTI_H_EXISTS="yes"
		AC_DEFINE_UNQUOTED(BUILD_WITH_JVMTI, 1, [Build with JVMTI])
	else
		JVMTI_H_EXISTS=""
	fi
	
	if test -f "$JAVA_HOMEDIR/include/jvmpi.h"; then
		JVMPI_H_EXISTS="yes"
		AC_DEFINE_UNQUOTED(BUILD_WITH_JVMPI, 1, [Build with JVMTI])
	else
		JVMPI_H_EXISTS=""
	fi

	dnl Check if jvmti/jvmpi is available
	if test "$JVMTI_H_EXISTS" != "yes"; then
	if test "$JVMPI_H_EXISTS" != "yes"; then
		AC_MSG_ERROR("Required header files (include/jvmti.h or include/jvmpi.h) not found in $JAVA_HOMEDIR")
	fi fi
fi

AM_CONDITIONAL(BUILD_JVMTI_AGENT, test -n "$JVMTI_H_EXISTS")
AM_CONDITIONAL(BUILD_JVMPI_AGENT, test -n "$JVMPI_H_EXISTS")

#Turn on debug
AC_ARG_ENABLE([debug],
	[  --enable-debug		Turn on debugging],
	[case "${enableval}" in
		yes) debug=true ;;
		no)  debug=false ;;
		*) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
	esac],
	[debug=false])
AM_CONDITIONAL(DEBUG, test x$debug = xtrue)

dnl enable -Werr flag
AC_ARG_ENABLE([werror],
	[  --enable-werror		enable -Werror flag (default is disabled)],
        enable_werror=$enableval, 
	enable_werror=no)

dnl disable-optimization flag
AC_ARG_ENABLE([optimization],
	[  --disable-optimization	disable optimization flags (default is enabled)],
	enable_optimization=$enableval, 
	enable_optimization=yes)

###############################################
# Environment Testing / Library Checking
###############################################

# Testing 
# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_MSG_CHECKING([$CC path])
if test -z `which $CC` ; then 
	AC_MSG_RESULT([no])	
	AC_MSG_ERROR(C compiler not found)
else
	AC_MSG_RESULT([yes])	
fi

AC_PROG_CXX
AC_PROG_CXXCPP
AC_MSG_CHECKING([$CXX path])
if test -z `which $CXX` ; then 
	AC_MSG_RESULT([no])	
	AC_MSG_ERROR(C++ compiler not found)
else
	AC_MSG_RESULT([yes])	
fi

AC_PROG_INSTALL

# check if ln -s works
AC_PROG_LN_S

# Check for language implementations
AC_C_CONST
AC_C_INLINE

# Check for types
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Check for headers
AC_HEADER_STDC
AC_CHECK_HEADERS([demangle.h fcntl.h limits.h memory.h stdio.h string.h strings.h sys/time.h unistd.h])
AC_HEADER_SYS_WAIT
AC_CHECK_HEADER_STDBOOL

# Check for functions
AC_FUNC_MALLOC
AC_CHECK_FUNCS([bzero gettimeofday memset mkdir strchr strncasecmp strrchr strstr strtoul])
AC_CHECK_FUNCS([malloc xcalloc xmemdup])

dnl some distro have a libiberty.a but does not have a libiberty.h
AC_CHECK_HEADERS([libiberty.h],[],[AC_MSG_ERROR([libiberty.h not found])])
AC_CHECK_LIB([iberty_pic], [cplus_demangle],
		[IBERTY_LIB="-liberty_pic";AC_SUBST(IBERTY_LIB)],
		[AC_CHECK_LIB([iberty], [cplus_demangle],
			[IBERTY_LIB="-liberty";AC_SUBST(IBERTY_LIB)], 
			[IBERTY_LIB=""; AC_MSG_ERROR([liberty library not found])])])
LIBS="$IBERTY_LIB $LIBS"

dnl check for libz (needed by libbfd)
AC_CHECK_LIB([z], [compress],
		[Z_LIB="-lz"; AC_SUBST(Z_LIB)], 
		[Z_LIB=""; AC_MSG_ERROR([libz library not found; required by libbfd])])
LIBS="$Z_LIB $LIBS"

dnl check for bfd (force shared library)
CHECK_LBFD
AC_CHECK_LIB([$BFD_LIB], [bfd_openr], 
		BFD_LIB="-l$BFD_LIB"; AC_SUBST(BFD_LIB), 
		BFD_LIB=""; AC_MSG_ERROR([bfd_openr bfd library not found]))
LIBS="$BFD_LIB $LIBS"

ORIG_SAVE_LIBS="$LIBS"

###############################################
# Setup Compiler Flags
###############################################

if test "$enable_optimization" = "no"; then
	CFLAGS=$CFLAGS`echo $CFLAGS | sed 's/-O2//g'`
	CXXFLAGS=$CXXFLAGS `echo $CXXFLAGS | sed 's/-O2//g'`
fi

if test x$debug = xfalse; then
	CFLAGS=$CFLAGS `echo $CFLAGS | sed 's/-g//g'`
	CXXFLAGS=$CXXFLAGS `echo $CXXFLAGS | sed 's/-g//g'`
fi

#Set 64-bit flags, if necessary
case $host_cpu in
	x86_64)
	FLAG64="-D_x86_64_"
	;;
	*)
	FLAG64=""
	;;
esac

AC_SUBST(FLAG64)
AM_CONDITIONAL(BUILD_JVMTIA_64, test -n "$FLAG64")

dnl get date
DATE="`date '+%a %d %B %Y'`"
AC_SUBST(DATE)

dnl get topdir
topdir=`pwd`
AC_SUBST(topdir)

dnl Setting CFLAGS
AX_CFLAGS_OPTION(CA_CFLAGS,[-W])
AX_CFLAGS_OPTION(CA_CFLAGS,[-Wall])
AX_CFLAGS_OPTION(CA_CFLAGS,[-fno-common])
AX_CFLAGS_OPTION(CA_CFLAGS,[-Wdeclaration-after-statement])
AX_CFLAGS_OPTION(CA_CFLAGS,[-fPIC])
if test x$debug = xtrue; then
	AX_CFLAGS_OPTION(CA_CFLAGS,[-DCA_ENABLE_DEBUG])
fi

if test "$enable_werror" = "yes"; then
	# enable -Werror for non-release versions.
	ls `pwd`/.svn > /dev/null
	if test "$?" = "0" ; then
		AX_CFLAGS_OPTION(CA_CFLAGS,[-Werror])
	fi
fi

if test -z "$FLAG64"; then
	AX_CFLAGS_OPTION(CA_CFLAGS,[-m32])
fi

AC_SUBST(CA_CFLAGS)

dnl Setting CXXFLAGS
AX_CXXFLAGS_OPTION(CA_CXXFLAGS,[-W])
AX_CXXFLAGS_OPTION(CA_CXXFLAGS,[-Wall])
AX_CXXFLAGS_OPTION(CA_CXXFLAGS,[-fno-common])
AX_CXXFLAGS_OPTION(CA_CXXFLAGS,[-ftemplate-depth-50])
AX_CXXFLAGS_OPTION(CA_CXXFLAGS,[-fPIC])
if test x$debug = xtrue; then
	AX_CXXFLAGS_OPTION(CA_CXXFLAGS,[-DCA_ENABLE_DEBUG])
fi

if test "$enable_werror" = "yes"; then
	# enable -Werror for non-release versions.
	ls `pwd`/.svn > /dev/null
	if test "$?" = "0" ; then
		AX_CXXFLAGS_OPTION(CA_CXXFLAGS,[-Werror])
	fi
fi

if test -z "$FLAG64"; then
	AX_CXXFLAGS_OPTION(CA_CXXFLAGS,[-m32])
fi
AC_SUBST(CA_CXXFLAGS)

dnl bfd.h pre 1998 check only for gnu 2.xx series, so gcc 3.0 can't compile it
AC_MSG_CHECKING([whether bfd defines bool])
AC_TRY_COMPILE([#include <bfd.h>], [], 
AC_MSG_RESULT([no]);,
AC_MSG_RESULT([yes]); AC_DEFINE(TRUE_FALSE_ALREADY_DEFINED, 1, [whether bfd.h defines bool values]))

dnl smart demangler need to know what are the underlined type for some typedef
AX_TYPEDEFED_NAME(size_t, "unsigned" "unsigned long", SIZE_T_TYPE)
AC_SUBST(SIZE_T_TYPE)
AX_TYPEDEFED_NAME(ptrdiff_t, "int" "long", PTRDIFF_T_TYPE)
AC_SUBST(PTRDIFF_T_TYPE)

dnl Directory required by agent
CA_OPROFILE_DIR="/var/lib/oprofile/"
CA_JIT_DIR="/var/lib/oprofile/jit"
CA_JAVA_DIR="/var/lib/oprofile/Java"
AC_DEFINE_UNQUOTED(CA_OPROFILE_DIR, "$CA_OPROFILE_DIR", [Oprofile data directory])
AC_DEFINE_UNQUOTED(CA_JIT_DIR, "$CA_JIT_DIR", [Jit data directory])
AC_DEFINE_UNQUOTED(CA_JAVA_DIR, "$CA_JAVA_DIR", [Java info directory])

AC_CONFIG_FILES([Makefile \
	libCAagent/Makefile \
	jvmti/Makefile \
	jvmpi/Makefile \
	jnc/Makefile \
	utils/Makefile \
	]) 
AC_OUTPUT
