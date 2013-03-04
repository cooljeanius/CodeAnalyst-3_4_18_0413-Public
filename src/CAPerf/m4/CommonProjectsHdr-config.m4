AC_DEFUN([COMMONPROJECTSHDR_INIT],
[

	dnl ###########################################
	dnl # Setup for CommonProjects stuff
	dnl ############################################
	AC_ARG_WITH(CommonProjectsHdr,
		[  --with-CommonProjectsHdr=path   Path to CommonProjectsHdr directory], COMMONPROJECTSHDR_DIR=$withval)
	AC_SUBST(COMMONPROJECTSHDR_DIR)

	if test "COMMONPROJECTSHDR_DIR" != ""; then
		COMMONPROJECTSHDR_INC="-I$COMMONPROJECTSHDR_DIR"
		AC_SUBST(COMMONPROJECTSHDR_INC)
	fi

])

