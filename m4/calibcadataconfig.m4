AC_DEFUN([CA_LIBCADATA_CONFIG],
[
	dnl# Check for configure
	echo "... Configuring libCAdata"
	pushd src/libCAdata/
	echo "... running src/libCAdata/configure"
	test -e ./autogen.sh && test -x ./autogen.sh && ./autogen.sh
	if test -e ./configure && test -x ./configure; then
	  ./configure  --with-CommonProjectsHdr=${topdir}/src/CommonProjectsHdr --prefix=${prefix} --libdir=${prefix}/lib --enable-debug --enable-shared CC=${CC} CFLAGS="-fPIC ${CFLAGS}" LDFLAGS="${LDFLAGS}"
	else
	  echo "configure script is missing; unable to configure"
	fi
	LIBCADATA_DIR=${topdir}/src/libCAdata/
	AC_SUBST([LIBCADATA_DIR])	
	popd
])

