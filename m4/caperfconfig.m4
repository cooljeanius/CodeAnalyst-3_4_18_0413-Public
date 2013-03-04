AC_DEFUN([CA_CAPERF_CONFIG],
[
	dnl AX_KERNEL_VERSION(major, minor, level, comparison, action-if-true, action-if-false)
	AX_KERNEL_VERSION(2, 6, 31, >, CAPERF_SUPPORT="no", CAPERF_SUPPORT="yes")

	AC_SUBST(CAPERF_SUPPORT)

	echo "... CAPERF_SUPPORT $CAPERF_SUPPORT"

	if test "$CAPERF_SUPPORT" == "yes" ; then
		dnl Check for configure
		echo "... Configuring CAPerf"
		pushd src/CAPerf/
		echo "... running src/CAPerf/configure"
		./autogen.sh
		./configure --prefix=$prefix --with-CommonProjectsHdr=${topdir}/src/CommonProjectsHdr --with-libcadata=${topdir}/src/libCAdata --enable-debug
		CAPERF_DIR=${topdir}/src/CAPerf/
		popd
	else
		echo "... CAPerf is not configured"
	fi

	AC_SUBST(CAPERF_DIR)
])

