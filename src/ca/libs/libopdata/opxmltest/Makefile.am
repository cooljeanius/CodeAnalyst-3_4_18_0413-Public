bin_PROGRAMS=opxmltest

SUBDIRS = .

INCLUDES = \
	-I ../ -I ../../../include \
	-I ../../libca/ @QT_INCLUDES@ 
		 

AM_CPPFLAGS = -g $(FLAG64)\
	-W -Wall  -g -pipe -Wno-unused -Wno-parentheses \
	${CA_CXXFLAGS}

opxmltest_SOURCES= \
	opxmltest.cpp

opxmltest_LDADD= \
	@QT_LIB@ \
	${top_srcdir}/${CADIR}/libs/libopdata/libopdata.la 
	

opxmltest_LDFLAGS= -Wl -fPIC -g --no-undefined @QT_LDFLAGS@

METASOURCES= AUTO
