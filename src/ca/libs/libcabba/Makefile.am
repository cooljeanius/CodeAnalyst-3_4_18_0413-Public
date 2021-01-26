lib_LTLIBRARIES = libCAbba.la

# libCAbba version ( major:minor:micro)
libVersion = 1:0:0

SUBDIRS = .
#	bbatest

AM_INCLUDES = \
    ${LIBDWARF_INC} \
    -I /usr/include/libelf 

AM_CPPFLAGS = $(AM_INCLUDES) $(FLAG64)\
    	-pipe -Wno-unused -Wno-parentheses ${CA_CXXFLAGS}

libCAbba_la_LDFLAGS = -fPIC --no-undefined -version-number ${libVersion}

if enable_dwarf
libCAbba_la_LIBADD  = -L${LIBDWARF_LIB}  -ldwarf
else
libCAbba_la_LIBADD = 
endif

libCAbba_la_SOURCES = \
	disassembler.cpp \
	smafile.cpp \
	bbanalysis.cpp

METASOURCES= AUTO
