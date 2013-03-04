SUBDIRS = .

# libcalog version ( major:minor:micro)
libVersion = 1:0:0

lib_LTLIBRARIES = libcalog.la

include_HEADERS = \
	calog_types.h \
	calog.h 

AM_CPPFLAGS= \
	-pipe \
	$(FLAG64) $(more_flags)

libcalog_la_SOURCES = \
	calog.cpp

libcalog_la_LDFLAGS = -fPIC -version-number ${libVersion}

METASOURCES = AUTO

###############################3

bin_PROGRAMS = calog_dumper

calog_dumper_SOURCES = \
	calog_dumper.cpp

calog_dumper_LDADD= \
	-lpopt \
	libcalog.la

calog_dumper_LDFLAGS = -Wl,-rpath,'$$ORIGIN/../lib'
