#$Id: configure.ac 17734 2010-03-02 19:47:46Z ssuthiku $

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

AC_INIT([CodeAnalyst],[3.4.18-public],[CodeAnalyst.support@amd.com])

CA_RELEASE_TYPE="public"
AC_SUBST([CA_RELEASE_TYPE])dnl

AC_CONFIG_SRCDIR([samples/classic/inlined_classic_stress.cpp])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([src/ca/gui/config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_DEFINE_UNQUOTED([CA_CONFIG_ARG],["${ac_configure_args}"],
                   [CodeAnalyst configuration flags])

AC_PREREQ([2.70])
AC_CANONICAL_HOST

# Initialize Libtool
LT_INIT([dlopen])
AC_PROG_RANLIB

# Fix issue with basename in gcc-4.4
AC_CHECK_DECLS([basename], [], [], [[#include <libgen.h>]])

AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE

# Default installation directory
AC_PREFIX_DEFAULT([/opt/CodeAnalyst])

###############################################
# Checking configure flags
###############################################
# Get Java configuration
AC_ARG_WITH([java],
[  --with-java=java-home        Path to Java home directory], JAVA_HOMEDIR=$withval)
AC_SUBST(JAVA_HOMEDIR)

# Get Oprofile configuration 
AC_ARG_WITH([oprofile], 
	[  --with-oprofile=DIR                  absolute location where the Oprofile is installed])
AC_ARG_WITH([oprofile-includes], 
	[  --with-oprofile-includes=DIR         absolute location where the Oprofile includes are])
AC_ARG_WITH([oprofile-libraries], 
	[  --with-oprofile-libraries=DIR        absolute location where the Oprofile libraries are])
AC_ARG_WITH([oprofile-source], 
	[  --with-oprofile-source=DIR           absolute location where the Oprofile source code is])

# Get libdwarf configuration 
AC_ARG_WITH([libdwarf-includes], 
	[  --with-libdwarf-includes=DIR         absolute location where the libdwarf includes are])
AC_ARG_WITH([libdwarf-libraries], 
	[  --with-libdwarf-libraries=DIR        absolute location where the libdwarf libraries are])

# Disable dwarf library
AC_ARG_ENABLE([dwarf],
	[  --disable-dwarf		disable usage of Dwarf Library (default is enabled)],
	enable_dwarf=$enableval, 
	enable_dwarf=yes)
AM_CONDITIONAL(enable_dwarf, test "$enable_dwarf" = yes)
if test "$enable_dwarf" = "yes"; then
	AC_DEFINE_UNQUOTED(ENABLE_DWARF,1, [Enable dwarf-symbol-engine])
fi

# Turn on debug
AC_ARG_ENABLE([debug],
	[  --enable-debug		Turn on debugging],
	[case "${enableval}" in
		yes) debug=true ;;
		no)  debug=false ;;
		*) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
	esac],
	[debug=false])
AM_CONDITIONAL(DEBUG, test x$debug = xtrue)

dnl enable code to handle separate debug info
debugdir=/usr/lib/debug

AC_ARG_WITH(separate-debug-dir,
	[  --with-separate-debug-dir=path	Look for global separate debug info in this path [LIBDIR/debug]],
[debugdir="${withval}"])

if test "z$debugdir" == "zyes"; then
	debugdir=/usr/lib/debug
fi

AC_DEFINE_UNQUOTED([DEBUGDIR], "$debugdir", [Directory with separate debug info files])

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

# Enable Oprofile XML library
AC_ARG_ENABLE([oprofile-lib],
	[  --disable-oprofile-lib	disable usage of Oprofile Library (default is enabled)],
	disable_oprofile_lib=$enableval, 
	disable_oprofile_lib=no)
AM_CONDITIONAL(disable_oprofile_lib, test "$disable_oprofile_lib" = yes)
if test "$disable_oprofile_lib" = "yes"; then
	AC_DEFINE_UNQUOTED(DISABLE_OPROFILE_LIB,1, [Disable Oprofile Lib])
fi

# disable CA CSS
AC_ARG_ENABLE([ca-css],
[  --disable-ca-css              disable CA Call-stack Sampling (CSS) and use OProfile Call-graph instead], 
	enable_ca_css=$enableval, enable_ca_css=yes)
if test "$enable_ca_css" = "yes"; then
	OP_CA_CSS="--enable-ca-css"
fi
AM_CONDITIONAL(ENABLE_CA_CSS, test "$enable_ca_css" = "yes")

# Get set of events
AC_ARG_WITH([events], 
	[  --with-events=DIR       specify the set of events to use [public|nda|internal]])
CA_EVENTS_DIR=$CA_RELEASE_TYPE
if test "$with_events" != "" ; then
	CA_EVENTS_DIR="$with_events"
fi
AC_SUBST(CA_EVENTS_DIR)

###############################################
# Environment Testing / Library Checking
###############################################

# Testing 
# Check for programs
AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_MSG_CHECKING([${CC} path])
if test -z "`which $CC`"; then 
	AC_MSG_RESULT([no])	
	AC_MSG_ERROR([C compiler not found])
else
	AC_MSG_RESULT([yes])	
fi

AC_PROG_CXX
AC_PROG_CXXCPP
AC_MSG_CHECKING([${CXX} path])
if test -z "`which $CXX`"; then 
	AC_MSG_RESULT([no])	
	AC_MSG_ERROR([C++ compiler not found])
else
	AC_MSG_RESULT([yes])	
fi
AC_PROG_OBJC
AC_PROG_OBJCXX
AM_PROG_AS
AM_PROG_GCJ
# UPC probably isn't actually needed, but conditionalizing use of this macro
# leads to errors, so we'll need to find another way to avoid this:
AM_PROG_UPC

AC_PROG_INSTALL
AC_PROG_MAKE_SET

# check pidof path
AC_CHECK_PROG([PIDOF],[pidof],[`which pidof`],[""])
if test -z "$PIDOF" ; then
	# Try to check in /sbin/pidof
	PIDOF=/sbin/pidof			
	if test ! -f "$PIDOF" ; then
		AC_MSG_ERROR([pidof not found in PATH environment variable.])
	fi
fi

AC_DEFINE_UNQUOTED(PIDOF, "$PIDOF", [path to pidof])

# check xterm path
AC_CHECK_PROG([XTERM], [xterm], [`which xterm`],[""])
if test -z "$XTERM" ; then
	AC_MSG_WARN([xterm not found in PATH environment variable.])
fi

AC_DEFINE_UNQUOTED(XTERM, "$XTERM", [path to xterm])

# check if ln -s works
AC_PROG_LN_S

# Check for language implementations
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT

# Check for types
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_CHECK_TYPES([ptrdiff_t])

# Check for headers
dnl# autoupdate says:
dnl# The preprocessor macro `STDC_HEADERS' is obsolete.
dnl# Except in unusual embedded environments, you can safely include all
dnl# ISO C90 headers unconditionally.
# Autoupdate added the next few lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
if test "x${EGREP}" = "x"; then
  test -z "${EGREP}"
  AC_PROG_EGREP
fi
AC_HEADER_STDBOOL dnl# also "_CHECK"s it
AC_CHECK_HEADERS([demangle.h fcntl.h limits.h stdio.h sys/time.h values.h])
AC_HEADER_SYS_WAIT
if test "x${ac_cv_header_stdint_h}" = "x"; then
  test -z "${ac_cv_header_stdint_h}"
  AC_CHECK_HEADERS([stdint.h])
fi
if test "x${ac_cv_header_strings_h}" = "x"; then
  test -z "${ac_cv_header_strings_h}"
  AC_CHECK_HEADERS([strings.h])
fi
if test "x${ac_cv_header_unistd_h}" = "x"; then
  test -z "${ac_cv_header_unistd_h}"
  AC_CHECK_HEADERS([unistd.h])
fi

# Check for functions
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_LSTAT dnl# also checks whether it "_FOLLOWS_SLASHED_SYMLINK"
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_CHECK_FUNCS([alarm atexit ftruncate gettimeofday memmove memset mkdir \
                mkfifo munmap realpath regcomp rmdir select setenv strchr \
                strerror strncasecmp strrchr strstr strtoul strtoull uname])
AC_CHECK_FUNCS([malloc xcalloc xmemdup])

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lCAdata':
AC_CHECK_LIB([CAdata],[main])
AC_CHECK_LIB([dwarf],[dwarf_init],[],[
  AC_CHECK_LIB([dwarf],[dwarf_finish],[],[
    AC_CHECK_LIB([dwarf],[dwarf_next_cu_header],[],[
      AC_CHECK_LIB([dwarf],[dwarf_siblingof],[],[
        AC_CHECK_LIB([dwarf],[dwarf_child],[],[
          AC_CHECK_LIB([dwarf],[dwarf_tag],[],[
            AC_CHECK_LIB([dwarf],[dwarf_attr],[],[
              AC_CHECK_LIB([dwarf],[dwarf_dealloc],[],[
                AC_CHECK_LIB([dwarf],[main],[],[])dnl
              ])dnl
            ])dnl
          ])dnl
        ])dnl
      ])dnl
    ])dnl
  ])dnl
])dnl
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m],[main])
LT_LIB_M
AC_CHECK_LIB([pthread],[pthread_mutex_lock],[],[
  AC_CHECK_LIB([pthread],[pthread_mutex_unlock],[],[
    AC_CHECK_LIB([pthread],[main],[],[])dnl
  ])dnl
])dnl

dnl check for gelf
AC_CHECK_HEADERS([gelf.h],[],
	[AC_CHECK_HEADERS([libelf/gelf.h],[],
	                  [AC_MSG_WARN([gelf.h not found])
	                   AC_MSG_NOTICE([(this ought to be an error, but we are downgrading it to a warning to facilitate work on this configure script)])
	])dnl
])dnl

dnl# check for elf
LDCONFIG="`which ldconfig`"
if test "x${LDCONFIG}" != "x"; then
	CHECK_LELF
else
	ELF="elf"
fi
AC_CHECK_LIB([${ELF}],[gelf_getehdr],
	     [ELF_LIB="-l${ELF}"; AC_SUBST([ELF_LIB])], 
	     [ELF_LIB=""; AC_MSG_WARN([libelf liberary not found])
	      AC_MSG_NOTICE([(this ought to be an error, but we are downgrading it to a warning to facilitate work on this configure script)])])
LIBS="${ELF_LIB} ${LIBS}"

dnl# check for popt
AC_CHECK_LIB([popt],[poptGetContext],
	     [POPT_LIB="-lpopt"; AC_SUBST(POPT_LIB)],
	     [POPT_LIB=""; AC_MSG_WARN([popt library not found])
	      AC_MSG_NOTICE([(this ought to be an error, but we are downgrading it to a warning to facilitate work on this configure script)])])
LIBS="${POPT_LIB} ${LIBS}"

dnl# check for dl
AC_CHECK_LIB([dl], [dlopen], 
		DL_LIB="-ldl", 
		DL_LIB=""; AC_MSG_ERROR([dl library not found]))
LIBS="$DL_LIB $LIBS"

dnl# check for libiberty
AC_CHECK_LIB([iberty],[xmalloc],[],[
  AC_CHECK_LIB([iberty],[main],[],[])
])
dnl# some distro have a libiberty.a but does not have a libiberty.h
AC_CHECK_HEADERS([libiberty.h],[],[AC_MSG_WARN([libiberty.h not found])])
AC_CHECK_LIB([iberty_pic], [cplus_demangle],
		[IBERTY_LIB="-liberty_pic";AC_SUBST(IBERTY_LIB)],
		[AC_CHECK_LIB([iberty], [cplus_demangle],
			[IBERTY_LIB="-liberty";AC_SUBST(IBERTY_LIB)], 
			[IBERTY_LIB=""; AC_MSG_WARN([liberty library not found])])])
LIBS="$IBERTY_LIB $LIBS"

dnl check for libz (needed by libbfd)
AC_CHECK_LIB([z], [compress],
		[Z_LIB="-lz"; AC_SUBST(Z_LIB)], 
		[Z_LIB=""; AC_MSG_ERROR([libz library not found; required by libbfd])])
LIBS="$Z_LIB $LIBS"

dnl check for bfd
LDCONFIG="`which ldconfig`"
if test "x$LDCONFIG" != "x"; then
	CHECK_LBFD
else
	BFD_LIB="bfd"
fi
AC_CHECK_LIB([$BFD_LIB], [bfd_openr], 
		[BFD_LIB="-l$BFD_LIB"; AC_SUBST(BFD_LIB)], 
		[BFD_LIB=""; AC_MSG_WARN([bfd_openr bfd library not found])])
LIBS="$BFD_LIB $LIBS"

ORIG_SAVE_LIBS="$LIBS"

dnl Check for X Window stuff
AC_PATH_XTRA
X_LIBS="$X_PRE_LIBS $X_LIBS -lX11 $X_EXTRA_LIBS"
AC_SUBST(X_LIBS)
ORIG_SAVE_X_LIBS="$X_LIBS"

dnl Check for QT stuff
AX_HAVE_QT
if test "`uname`" != "Darwin"; then
	QT_DO_IT_ALL
fi

###############################################
# Setup Compiler Flags
###############################################

if test "$enable_optimization" = "no"; then
	CFLAGS=`echo $CFLAGS | sed 's/-O2//g'`
	CXXFLAGS=`echo $CXXFLAGS | sed 's/-O2//g'`
fi

if test x$debug = xfalse; then
	CFLAGS=`echo $CFLAGS | sed 's/-g//g'`
	CXXFLAGS=`echo $CXXFLAGS | sed 's/-g//g'`
fi

# Set 64-bit flags, if necessary
case ${host_cpu} in
 x86_64)
	FLAG64="-D_x86_64_"
	;;
 *)
	FLAG64=""
	;;
esac
AC_SUBST([FLAG64])dnl

dnl# get date
DATE="`date '+%a %d %B %Y'`"
AC_SUBST([DATE])dnl

dnl# get topdir
topdir="`pwd`"
AC_SUBST([topdir])dnl

dnl# Used in symbolengine
m4_ifdef([AX_BFD_VERSION],[
AX_BFD_VERSION
],[
AC_MSG_WARN([missing check for BFD version])
])dnl

dnl# Setting CFLAGS
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

AC_SUBST([CA_CXXFLAGS])dnl

dnl# bfd.h pre-1998 only checks for gnu 2.xx series, so gcc 3.0 cannot compile
dnl# it:
AC_MSG_CHECKING([whether bfd defines bool])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <bfd.h>]],[[]])],
                  [AC_MSG_RESULT(no);],
                  [AC_MSG_RESULT(yes);
                   AC_DEFINE([TRUE_FALSE_ALREADY_DEFINED],[1],
                             [whether bfd.h defines bool values])])dnl

dnl# smart demangler need to know what are the underlined type for some typedef
AX_TYPEDEFED_NAME([size_t],"unsigned" "unsigned long",[SIZE_T_TYPE])
AC_SUBST([SIZE_T_TYPE])
AX_TYPEDEFED_NAME([ptrdiff_t],"int" "long",[PTRDIFF_T_TYPE])
AC_SUBST([PTRDIFF_T_TYPE])dnl

############################################
# Setup for CA 
############################################

# fixups for config.h
if test "x${prefix}" = "xNONE"; then
	my_prefix="${ac_default_prefix}"
else
	my_prefix="${prefix}"
fi
my_datadir="${my_prefix}/share"

if test "x${exec_prefix}" = "xNONE"; then
	my_exec_prefix="${my_prefix}"
else
	my_exec_prefix="${exec_prefix}"
fi
my_bindir="${my_exec_prefix}/bin"

CA_DATA_DIR="${my_datadir}/codeanalyst/"
AC_DEFINE_DIR([CA_DATA_DIR],[CA_DATA_DIR],[Defines the data directory])
AC_DEFINE_DIR([CA_BIN_DIR],[bindir],[Defines the binary directory])
AC_DEFINE_DIR([CA_SBIN_DIR],[sbindir],
              [Defines the super-user binary directory])
AC_DEFINE_DIR([CA_LIB_DIR],[libdir],[Defines the library directory])dnl

############################################
# Setup for ca_agent
############################################
CA_AGENT_CONFIG([])

############################################
# Setup for libdwarf
############################################

dnl# Check with_libdwarf_includes
if test "x${with_libdwarf_includes}" != "x"; then
  test -n "${with_libdwarf_includes}" && export LIBDWARF_INC="-I ${with_libdwarf_includes}"
fi

dnl Check with_libdwarf_libraries
if test "x${with_libdwarf_libraries}" != "x"; then
  test -n "${with_libdwarf_libraries}" && export LIBDWARF_LIB="${with_libdwarf_libraries}"
fi

if test "x${LIBDWARF_INC}" = "x"; then
if test "x${LIBDWARF_LIB}" = "x"; then
  echo "---------- Begin Setup for libdwarf ----------"
  CA_LIBDWARF_CONFIG([])
  if test "x${LIBDWARF_DIR}" != "x"; then
    test -n "${LIBDWARF_DIR}" && export LIBDWARF_INC="-I ${LIBDWARF_DIR}"
    test -n "${LIBDWARF_DIR}" && export LIBDWARF_LIB="${LIBDWARF_DIR}"
  fi
  echo "---------- Finish Setup for libdwarf ----------"
fi fi
AC_SUBST([LIBDWARF_INC])dnl
AC_SUBST([LIBDWARF_LIB])dnl

############################################
# Setup for libCAJnc
############################################
LIBCAJNC_INC="-I ${topdir}/src/ca_agent/jnc"
AC_SUBST([LIBCAJNC_INC])
jnc_lib="${topdir}/src/ca_agent/jnc/libCAJnc.la"
AC_SUBST([jnc_lib])dnl

############################################
# Setup for Oprofile
############################################

dnl# Check with_oprofile
if test ! -z "${with_oprofile}"; then
	OPINST="${with_oprofile}"

	# TODO: check where the default bin should be
	OPBIN=${OPINST}/bin

	# TODO: check where the default headers should be
	OPINC=${OPINST}/include

	# TODO: check where the default libraries should be
	OPLIB=${OPINST}/lib
fi

dnl# Check with_oprofile_includes
if test ! -z "${with_oprofile_includes}"; then
	OPINC="${with_oprofile_includes}/"
fi

dnl# Check with_oprofile_libraries
if test ! -z "${with_oprofile_libraries}"; then
	OPLIB="${with_oprofile_libraries}/"
fi

dnl# Check with_oprofile_bin
if test ! -z "${with_oprofile_bin}"; then
	OPBIN="${with_oprofile_bin}/"
	AC_SUBST([OPBIN])
fi

dnl# Setup oprofile specific dir
if test ! -z "${OPBIN}"; then
	AC_DEFINE_UNQUOTED([OP_DATADIR],["${OPINST}/share/oprofile"],
	                   [The defined directory of oprofile binaries])
	AC_DEFINE_UNQUOTED([OP_BINDIR],["${OPBIN}"],
	                   [The defined directory of oprofile binaries])dnl
	
	dnl# Check if oprofiled supports option "--ibs-op-dispatch-op"
	OPD_NO_DISPATCH_OP_OPTION=`fgrep ibs-op-dispatch-op ${OPBIN}/opcontrol 2> /dev/null > /dev/null; echo $?`
	AC_DEFINE_UNQUOTED([OPD_NO_DISPATCH_OP_OPTION],
	                   [${OPD_NO_DISPATCH_OP_OPTION}],
	   [Oprofile daemon does not support IBS-Op dispatched-op option])dnl
	
	dnl# Check if oprofiled supports option "--ibs-op-dcmissinfo-enable"
	OPD_NO_DCMISSINFO_OPTION=`fgrep ibs-op-dcmissinfo-enable ${OPBIN}/opcontrol 2> /dev/null > /dev/null; echo $?`
	AC_DEFINE_UNQUOTED([OPD_NO_DCMISSINFO_OPTION],
	                   [${OPD_NO_DCMISSINFO_OPTION}],
	   [Oprofile daemon does not support IBS-Op DC Miss Info option])dnl
	
	dnl# Check Oprofile version, if possible
	m4_ifdef([CHECK_OPROFILE_VERSION],[
	CHECK_OPROFILE_VERSION([${OPBIN}])
	],[
	AC_MSG_WARN([unable to check Oprofile version])dnl
	])
fi

dnl# If no oprofile specified, used default CA Oprofile
if test -z ${with_oprofile_source}; then
if test -z ${OPINST}; then
if test -z ${OPINC}; then
if test -z ${OPLIB}; then
if test -z ${OPBIN}; then
if test "x${disable_oprofile_lib}" != "xyes"; then
	echo "---------- Begin Setup for CA Oprofile ----------"
	with_oprofile_source="${topdir}/src/oprofile/"
	echo " .... with_oprofile_source = ${with_oprofile_source}"

	CA_OPROFILE_CONFIG([])
	CA_OPROFILE_DIR="oprofile"
	AC_SUBST([CA_OPROFILE_DIR])dnl

	echo "---------- Finish Setup for CA Oprofile ----------"
fi 
fi fi fi fi fi

dnl# Setup oprofile src dir
if test ! -z "${with_oprofile_source}"; then
	OPSRC="${with_oprofile_source}/"
	AC_SUBST([OPSRC])dnl
	m4_ifdef([CHECK_OPROFILE_CONFIG],[
	CHECK_OPROFILE_CONFIG([${OPSRC}])
	],[
	AC_MSG_WARN([unable to check Oprofile config])dnl
	])
fi

dnl# Setup include/library flags
if test -z "${OPINC}" -a -z "${OPLIB}"; then
	# Use default value
	op_includes="-I ${OPSRC}/libutil -I ${OPSRC}/daemon -I ${OPSRC}/libop -I ${OPSRC}/libutil -I ${OPSRC}/libdb -I ${OPSRC}/libopt++ -I ${OPSRC}/libutil++ -I ${OPSRC}/libregex -I ${OPSRC}/libpp -I ${OPSRC}/pp"

	op_libs="${OPSRC}/libpp/libpp.a ${OPSRC}/libregex/libop_regex.a ${OPSRC}/libutil++/libutil++.a ${OPSRC}/libop/libop.a ${OPSRC}/libutil/libutil.a ${OPSRC}/libdb/libodb.a"
	
	### FUTURE OPTIONS: Oprofile shared object.
	future_op_libs_so="${OPSRC}/libpp/libpp.la ${OPSRC}/libregex/libop_regex.la ${OPSRC}/libutil++/libutil++.la ${OPSRC}/libop/libop.la ${OPSRC}/libutil/libutil.la ${OPSRC}/libdb/libodb.la"
	AC_SUBST([future_op_libs_so])
else 
	# Use user-specified value
	op_includes="-I ${OPINC}"
	op_libs="-L${OPLIB} -lpp -lop_regex -lutil++ -lop -lutil -lodb"
fi

if test "x${disable_oprofile_lib}" = "xyes"; then
	op_libs=""
fi

AC_SUBST([op_includes])dnl
AC_SUBST([op_libs])dnl

dnl# Disable CA JVMTIA if not using CA version of Oprofile
if test -e ${with_oprofile_source}/configure.in; then
  USE_CA_OPROFILE="`cat ${with_oprofile_source}/configure.in | grep "CA-X" 2>/dev/null`"
elif test -e ${with_oprofile_source}/configure.ac; then
  USE_CA_OPROFILE="`cat ${with_oprofile_source}/configure.ac | grep "CA-X" 2>/dev/null`"
fi
if test -z "${USE_CA_OPROFILE}"; then
	AC_DEFINE([DISABLE_CA_JVMTIA],[],[Disable CA Java profiling Agent])
fi

############################################
# Setup for libCAdata
############################################
echo "---------- Begin Setup for libCAdata ----------"
CA_LIBCADATA_CONFIG([])
if test "x${LIBCADATA_DIR}" != "x"; then
  test -n "${LIBCADATA_DIR}" && export LIBCADATA_INC="-I ${LIBCADATA_DIR}/src -I ${topdir}/src/CommonProjectsHdr"
  test -n "${LIBCADATA_DIR}" && export LIBCADATA_LIB="${LIBCADATA_DIR}/src"
fi
AC_SUBST([LIBCADATA_INC])dnl
AC_SUBST([LIBCADATA_LIB])dnl

echo "---------- Finish Setup for libCAdata ----------"

############################################
# Setup for CAPerf
############################################
echo "---------- Begin Setup for CAPerf ----------"
CA_CAPERF_CONFIG([])
if test "x${CAPERF_DIR}" != "x"; then
  test -n "${CAPERF_DIR}" && export CAPERF_INC="-I ${CAPERF_DIR}/src/include -I ${CAPERF_DIR}/src/CAPerfControl -I ${CAPERF_DIR}/src/CAPerfData -I ${CAPERF_DIR}/src/CAPerfTranslate -I ${CAPERF_DIR}/src/PerfData"
fi
AC_SUBST([CAPERF_INC])dnl

AM_CONDITIONAL([caperf_support],[test "x${CAPERF_SUPPORT}" = "xyes"])dnl

echo "---------- Finish Setup for CAPerf ----------"

############################################
# Define Misc variable
############################################
CADIR="src/ca/"
AC_SUBST([CADIR])dnl

PACKAGE="oprofile"

if test "x${CA_SAMPLE_DIR}" = "x"; then
	#note if you change the CA_SAMPLE_DIR here, you also need to change it in the Setup.sh
	CA_SAMPLE_DIR="/var/lib/oprofile/Java"
fi
AC_DEFINE_UNQUOTED([CA_SAMPLE_DIR],["${CA_SAMPLE_DIR}"],
                   [CodeAnalyst java sample directory])dnl

if test "xyes" == "x${create_libbfd_so}"; then
	echo "Warning: no libbfd.so found. Soft link created linking to ${usr_lib}/${bfd_so}"
fi
if test "xerror" == "x${create_libbfd_so}"; then
	echo "Warning: Could not create softlink for libbfd.so."
fi

if test "xyes" == "x${create_libelf_so}"; then
	echo "Warning: no libelf.so found. Soft link created linking to ${usr_lib}/${elf_so}"
fi
if test "xerror" == "${create_libelf_so}"; then
	echo "Warning: Could not create softlink for libelf.so."
fi

############################################
# Makefiles
############################################

AC_CONFIG_FILES([ Makefile \
	src/Makefile \
	src/ca/Makefile \
	src/ca/libs/Makefile
	src/ca/libs/libca/Makefile \
	src/ca/libs/libopdata/Makefile \
	src/ca/libs/libopdata/opxmltest/Makefile \
	src/ca/libs/libcabba/Makefile \
	src/ca/libs/libcabba/bbatest/Makefile \
	src/ca/libs/liboutput/Makefile \
	src/ca/events/Makefile \
	doc/Makefile \
	doc/cadoc/Makefile
	])
AC_OUTPUT
