SUBDIRS = .

# libCAPerfTranslate version ( major:minor:micro)
libVersion = 1:0:0

lib_LTLIBRARIES = libCAPerfTranslate.la

include_HEADERS = \
        CAPerfTranslator.h

AM_CPPFLAGS=\
        -pipe \
	${CA_PERF_INCLUDES} \
	${COMMONPROJECTSHDR_INC} \
	${LIBCADATA_INC} \
	${LIBCSS_INC} \
	-I ${top_srcdir}/src/PerfData \
	-I ${top_srcdir}/src/CAPerfData \
	-I ${top_srcdir}/src/libcalog

METASOURCES = AUTO

libCAPerfTranslate_la_LDFLAGS = \
	-fPIC \
	-version-number ${libVersion}

libCAPerfTranslate_la_LIBADD = \
	${LIBCADATA_LIB}\
	${LIBCSS_LIB}\
	${top_srcdir}/src/PerfData/libPerfData.la \
	${top_srcdir}/src/CAPerfData/libCAPerfData.la \
	${top_srcdir}/src/libcalog/libcalog.la \
	-lelf

libCAPerfTranslate_la_SOURCES = \
        CAPerfTranslator.cpp
