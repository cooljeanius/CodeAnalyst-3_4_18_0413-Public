SUBDIRS = .

# libCA version ( major:minor:micro)
libVersion = 1:0:0

# Suravee: Moved to configure.ac
# Need for op_config.h
#op_includes = \
#	-I ${OPINC} \
#	-I ${OPSRC}/libop

AM_CPPFLAGS= \
	@QT_INCLUDES@ \
	${op_includes} \
	${LIBDWARF_INC} \
	${LIBCADATA_INC} \
	-I ../../include \
	-I /usr/share/qt4/include \
	-I /usr/share/qt4/include/QtCore \
	-I /usr/share/qt4/include/QtGui \
	-I /usr/share/qt4/include/Qt3Support \
	-I /usr/include/libelf \
	-I ../libopdata \
	-I ../liboutput \
	-I ../../gui \
	-I ../../../ca_agent/jnc/ \
	-pipe -DQT_THREAD_SUPPORT -DQT3_SUPPORT -DQT_SHARED $(FLAG64) ${CA_CXXFLAGS}

lib_LTLIBRARIES = libCA.la

SUFFIXES=.h .moc.cpp
.h.moc.cpp:
	$(MOC) -o $@ $*.h

METASOURCES = AUTO
CLEANFILES=*.ui.cpp *.moc.cpp $(gui_generated_headers)

libCA_la_LDFLAGS = -fPIC --no-undefined -version-number ${libVersion}


if enable_dwarf
libCA_la_LIBADD = \
	${op_libs} \
	-L${LIBDWARF_LIB} -ldwarf \
	${top_srcdir}/${CADIR}/libs/libopdata/libopdata.la \
	${top_srcdir}/src/ca_agent/jnc/libCAJnc.la

libCA_la_SOURCES = \
    elfreader.cpp \
    symbolengine.cpp \
    symbolengine.moc.cpp \
    catranslate.cpp \
    catranslate_gui_display.cpp \
    catranslate_console_display.cpp \
    dwarfengine.cpp  

else
libCA_la_LIBADD = \
	${op_libs} \
	${top_srcdir}/${CADIR}/libs/libopdata/libopdata.la \
	${top_srcdir}/src/ca_agent/jnc/libCAJnc.la

libCA_la_SOURCES = \
    elfreader.cpp \
    symbolengine.cpp \
    symbolengine.moc.cpp \
    catranslate.cpp \
    catranslate_gui_display.cpp \
    catranslate_console_display.cpp 

endif
