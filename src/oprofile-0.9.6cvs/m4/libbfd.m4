dnl start here 
AC_DEFUN([LIBBFD_DO_IT_ALL],
[
	if test "`uname -i`" = "x86_64" ; then
		dnl Create a soft link to 64-bit libbfd if no libbfd.so is present
		if test -d /usr/lib64 && test ! -f /usr/lib64/libbfd.so; then
			usr_lib=/usr/lib64
			create_libbfd_so="yes"
		else
			create_libbfd_so="no"		
		fi

	else	
		dnl Create a soft link to 32-bit libbfd if no libbfd.so is present
		if test -d /usr/lib && test ! -f /usr/lib/libbfd.so; then
			usr_lib=/usr/lib
			create_libbfd_so="yes"
		else
			create_libbfd_so="no"		
		fi
	fi

        if test $create_libbfd_so = "yes" ; then        
		bfd_so=`ls $usr_lib |grep libbfd |grep so | head -n1 2>/dev/null`
                if test -n $bfd_so ; then
			ln -sf $bfd_so $usr_lib/libbfd.so
                else
                        create_libbfd_so="error"           
                fi
        fi
	
])

AC_DEFUN([CHECK_LBFD],
[
	LIBBFD=`/sbin/ldconfig -p | \
		grep "libbfd" | \
		awk '{split($[]1,a," "); \
			sub (/lib/,"", [a[1]]) ; \
			sub(/.so/,"",[a[1]]); \
			print [a[1]]; \
			exit;}' 2> /dev/null` 
        if test "$LIBBFD" != "" ; then
		BFD_LIB="$LIBBFD"
        else
		BFD_LIB="bfd"
        fi
])
