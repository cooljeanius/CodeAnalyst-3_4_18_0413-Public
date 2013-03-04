AC_DEFUN([CA_LIBCADATA_CONFIG],
[
	dnl Check for configure
	echo "... Configuring libCAdata"
	pushd src/libCAdata/
	echo "... running src/libCAdata/configure"
	./autogen.sh
	./configure  --with-CommonProjectsHdr=${topdir}/src/CommonProjectsHdr --prefix=$prefix --libdir=${prefix}/lib --enable-debug --enable-shared CC=$CC CFLAGS="-fPIC $CFLAGS" LDFLAGS="$LDFLAGS"
	LIBCADATA_DIR=${topdir}/src/libCAdata/
	AC_SUBST(LIBCADATA_DIR)	
	popd
])

