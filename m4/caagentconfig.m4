AC_DEFUN([CA_AGENT_CONFIG],
[
	echo "---------- Begin Setup for ca_agent ----------"
	echo "... Configuring ca_agent"

	dnl# Check for autogen.sh
	test -f "src/ca_agent/autogen.sh" 
	AS_IF([test "$?" = "0"],[
		echo "... running src/ca_agent/autogen.sh"
		pushd src/ca_agent
		./autogen.sh
		popd
	],[
		AC_MSG_NOTICE([skipping the ca_agent autogen.sh script])dnl
	])dnl

	dnl# Check for java inline support
	AS_IF([test -f "${JAVA_HOMEDIR}/include/jvmticmlr.h"],[
		JAVAINLINE_FLAGS="-D_INLINEINFO_"	
	])dnl

	dnl# Check for configure
	test -f "src/ca_agent/configure" 
	AS_IF([test "$?" = "0"],[
		pushd src/ca_agent
		echo "... running src/ca_agent/configure"
		./configure --with-java=${JAVA_HOMEDIR} --prefix=${prefix} --libdir=${prefix}/lib CC=${CC} CXX=${CXX} CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS} ${JAVAINLINE_FLAGS}" LDFLAGS="${LDFLAGS}" 
		CA_AGENT_CONFIG_RET="$?"
		popd
	],[
		AC_MSG_NOTICE([skipping configuring for ca_agent])dnl
	])dnl
	## end
	echo "---------- Finish Setup for ca_agent ----------"
	
	AS_IF([test "${CA_AGENT_CONFIG_RET}" -ne "0"],[
		AC_MSG_ERROR(["Failed to configure ca_agent."])
	])dnl
])dnl

