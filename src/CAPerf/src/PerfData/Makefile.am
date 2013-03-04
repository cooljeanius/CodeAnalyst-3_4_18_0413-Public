# libPerfData version ( major:minor:micro)
libVersion = 1:0:0

lib_LTLIBRARIES = libPerfData.la

include_HEADERS = \
        PerfDataReader.h

AM_CPPFLAGS=\
        -pipe \
	${COMMONPROJECTSHDR_INC} \
	${CA_PERF_INCLUDES} \
	${LIBCADATA_INC}

METASOURCES = AUTO

libPerfData_la_LDFLAGS= -fPIC -version-number ${libVersion}

libPerfData_la_SOURCES = \
	perfRecName.c \
        PerfDataReader.cpp
