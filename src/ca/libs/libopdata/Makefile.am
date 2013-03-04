SUBDIRS = .
#	opxmltest

# libopdata version ( major:minor:micro)
libVersion = 1:0:0

# Suravee: Moved to configure.ac
#op_includes=\
#	-I ${OPINC} \
#	-I ${OPSRC}/libutil \
#	-I ${OPSRC}/daemon  \
#	-I ${OPSRC}/libop \
#	-I ${OPSRC}/libutil \
#	-I ${OPSRC}/libdb \
#	-I ${OPSRC}/libopt++ \
#	-I ${OPSRC}/libutil++ \
#	-I ${OPSRC}/libregex \
#	-I ${OPSRC}/libpp \
#	-I ${OPSRC}/pp

AM_CPPFLAGS=\
	@QT_INCLUDES@ \
	${op_includes} \
	-I ../../include \
	-I ../liboutput \
	-I ../libca/ \
	${LIBCADATA_INC} \
	-pipe $(FLAG64) -DQT_SHARED -DQT_THREAD_SUPPORT -DQT3_SUPPORT ${CA_CXXFLAGS}

# Suravee: Moved to configure.ac
#op_libs = \
#	${OPSRC}/libpp/libpp.a \
#	${OPSRC}/libregex/libop_regex.a \
#	${OPSRC}/libutil++/libutil++.a \
#	${OPSRC}/libop/libop.a \
#	${OPSRC}/libutil/libutil.a \
#	${OPSRC}/libdb/libodb.a 

lib_LTLIBRARIES = libopdata.la

SUFFIXES= .h .moc.cpp
.h.moc.cpp:
	$(MOC) -o $@ $*.h

 
METASOURCES = AUTO
CLEANFILES=*.ui.cpp *.moc.cpp $(gui_generated_headers)

libopdata_la_LDFLAGS = -fPIC --no-undefined -version-number ${libVersion}

libopdata_la_LIBADD = \
	-L${top_srcdir}/src/libCAdata/src/.libs -lCAdata \
	${op_libs}

if disable_oprofile_lib
libopdata_la_SOURCES = \
	opdata_handler.cpp \
	opxmldata_handler.cpp \
	OpreportXMLHandler.cpp 
else
libopdata_la_SOURCES = \
	opdata_handler.cpp \
	opxmldata_handler.cpp \
	OpreportXMLHandler.cpp \
	opsampledata_handler.cpp
endif
