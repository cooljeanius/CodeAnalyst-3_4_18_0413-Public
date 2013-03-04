AC_DEFUN([CA_OPROFILE_CONFIG],
[
	echo "... Configuring CodeAnalyst Oprofile"

	dnl Default to oprofile-0.9.6cvs
	test -h "src/oprofile"
	if test "$?" != "0" ; then
		ln -s oprofile-0.9.6cvs src/oprofile
	fi

	dnl Set release type for AMD processor family
	echo "... set release type $CA_RELEASE_TYPE"

	dnl Secretly pass information to OProfile
	echo $CA_RELEASE_TYPE > src/oprofile/.CA_RELEASE_TYPE

	dnl Check for autogen.sh
	test -f "src/oprofile/autogen.sh" 
	if test "$?" = "0" ; then

		dnl generate configure.ac to overwrite the existing configure.in
		sed 's/CA-X/CA-AC_PACKAGE_VERSION/g' < src/oprofile/configure.in > src/oprofile/configure.ac
	
		echo "... running src/oprofile/autogen.sh"
		pushd src/oprofile
		./autogen.sh
		popd
	fi

	dnl Check for configure
	test -f "src/oprofile/configure" 
	if test "$?" = "0" ; then
		echo "... running src/oprofile/configure"
		pushd src/oprofile
		./configure --with-kernel-support $OP_CA_CSS --with-ca-agent=${topdir}/src/ca_agent --prefix=$prefix --libdir=${prefix}/lib CC=$CC CXX=$CXX CFLAGS="-fPIC $CFLAGS" CXXFLAGS="-fPIC $CFLAGS" LDFLAGS="-fPIC $LDFLAGS" 
		popd
	fi
		
])

