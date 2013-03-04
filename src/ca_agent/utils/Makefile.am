bin_PROGRAMS= jncdumper

jncdumper_SOURCES = \
        jncdumper.cpp

jncdumper_LDFLAGS = \
        -fPIC

jncdumper_LDADD = \
        -lpopt \
        ${top_srcdir}/jnc/libCAJnc.la

#############################################

sbin_PROGRAMS= ca_agent_cleanup

ca_agent_cleanup_SOURCES = \
        ca_agent_cleanup.cpp

ca_agent_cleanup_LDFLAGS = \
        -fPIC

ca_agent_cleanup_LDADD = \
        ${top_srcdir}/libCAagent/libCAagent.la

#############################################

INCLUDES = \
	-I@JAVA_HOMEDIR@/include \
	-I@JAVA_HOMEDIR@/include/linux \
	-I../jnc/


AM_CPPFLAGS= \
        -I . \
        -I${top_srcdir}/jnc \
        -I${top_srcdir}/libCAagent \
        -pipe \
        $(FLAG64) $(more_flags)

