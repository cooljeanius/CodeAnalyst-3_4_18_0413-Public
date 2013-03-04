bin_PROGRAMS= perf2ca

perf2ca_SOURCES = \
        perf2ca.cpp

perf2ca_LDFLAGS = \
        -fPIC -Wl,-rpath,'$$ORIGIN/../lib'

perf2ca_LDADD = \
        -lpopt \
	${LIBCADATA_LIB} \
	${LIBCSS_LIB}\
	${top_srcdir}/src/PerfData/libPerfData.la \
	${top_srcdir}/src/CAPerfData/libCAPerfData.la \
	${top_srcdir}/src/CAPerfTranslate/libCAPerfTranslate.la

AM_CPPFLAGS= \
	-I . \
        -I ../include \
        -I ${top_srcdir}/src/CAPerfTranslate \
	${LIBCADATA_INC} \
	${COMMONPROJECTSHDR_INC} \
        -pipe \
        $(FLAG64) $(more_flags)


METASOURCES = AUTO

