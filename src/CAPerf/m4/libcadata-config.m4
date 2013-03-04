AC_DEFUN([LIBCADATA_INIT],
[
	dnl ############################################
	dnl # Setup for libCAdata
	dnl ############################################
	AC_ARG_WITH(libcadata,
		[  --with-libcadata=path   Path to libCAdata install directory], LIBCADATA_DIR=$withval)
	AC_SUBST(LIBCADATA_DIR)

	if test "$LIBCADATA_DIR" != "" ; then
		if test -f $LIBCADATA_DIR/Makefile.libCaData ; then
			. $LIBCADATA_DIR/Makefile.libCaData
			echo "Import LIBCADATA_INC = $LIBCADATA_INC"
			echo "Import LIBCADATA_LIB = $LIBCADATA_LIB"
		else
			LIBCADATA_INC="-I${LIBCADATA_DIR}/src"
			LIBCADATA_LIB="-L${LIBCADATA_DIR}/src/.libs -lCAdata"
			echo "LIBCADATA_INC = $LIBCADATA_INC"
			echo "LIBCADATA_LIB = $LIBCADATA_LIB"
		fi
		
		AC_SUBST(LIBCADATA_INC)
		AC_SUBST(LIBCADATA_LIB)
	fi
])

