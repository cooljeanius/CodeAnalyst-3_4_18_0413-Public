AC_DEFUN([CA_AGENT_CONFIG],
[
	echo "---------- Begin Setup for ca_agent ----------"
	echo "... Configuring ca_agent"

	dnl Check for autogen.sh
	test -f "src/ca_agent/autogen.sh" 
	if test "$?" = "0" ; then
		echo "... running src/ca_agent/autogen.sh"
		pushd src/ca_agent
		./autogen.sh
		popd
	fi

	dnl Check for java inline support
	if test -f "$JAVA_HOMEDIR/include/jvmticmlr.h"; then
		JAVAINLINE_FLAGS="-D_INLINEINFO_"	
	fi

	dnl Check for configure
	test -f "src/ca_agent/configure" 
	if test "$?" = "0" ; then
		pushd src/ca_agent
		echo "... running src/ca_agent/configure"
		./configure --with-java=$JAVA_HOMEDIR --prefix=$prefix --libdir=${prefix}/lib CC=$CC CXX=$CXX CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS $JAVAINLINE_FLAGS" LDFLAGS="$LDFLAGS" 
		CA_AGENT_CONFIG_RET="$?"
		popd
	fi
	echo "---------- Finish Setup for ca_agent ----------"
	
	if test "$CA_AGENT_CONFIG_RET" -ne "0"; then
		AC_MSG_ERROR("Failed to configure ca_agent.")
	fi
		
])

