lib_LTLIBRARIES = libCAJnc.la

# libCAJnc version ( major:minor:micro)
libVersion = 1:0:0

INCLUDES = \
	-I@JAVA_HOMEDIR@/include \
	-I@JAVA_HOMEDIR@/include/linux 

AM_CPPFLAGS= \
	${FLAG64} \
	${CA_CXXFLAGS}

libCAJnc_la_LDFLAGS = -fPIC -no-undefined -version-number ${libVersion}

libCAJnc_la_SOURCES = \
			jncwriter.cpp \
			jncreader.cpp
