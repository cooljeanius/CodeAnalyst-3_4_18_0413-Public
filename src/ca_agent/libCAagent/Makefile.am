lib_LTLIBRARIES = libCAagent.la

# libCAJVMTIA version ( major:minor:micro)
libVersion = 1:0:0

AM_CPPFLAGS= \
	${FLAG64} \
	${CA_CXXFLAGS}

libCAagent_la_LDFLAGS = -fPIC -no-undefined -version-number ${libVersion}

libCAagent_la_SOURCES = \
			smm.cpp \
			slock.cpp \
			CAagent_buffer.cpp
