AC_DEFUN([CA_LIBDWARF_CONFIG],
[
	dnl Check for configure
	echo "... Configuring libdwarf"
	test -f "src/dwarf-20111214/libdwarf/configure" 
	if test "$?" = "0" ; then
		pushd src/dwarf-20111214/libdwarf/
		echo "... running src/dwarf-20111214/libdwarf/configure"
		./configure --prefix=$prefix --libdir=${prefix}/lib --enable-shared CC=$CC CFLAGS="-fPIC $CFLAGS" LDFLAGS="$LDFLAGS"
		LIBDWARF_DIR=${topdir}/src/dwarf-20111214/libdwarf/
		AC_SUBST(LIBDWARF_DIR)	
		popd
	fi
		
])

