#!/bin/bash
  
VERSION=1.0

do_options()
{
    MYPWD=`pwd`

    OPPATH="/var/lib/oprofile/samples/current/"
    JAVAPATH="/var/lib/oprofile/Java"
    OUTPUT="capacked"

    while [ "$#" -ne 0 ]
    do
   		arg=`printf %s $1 | awk -F= '{print $1}'`
		val=`printf %s $1 | awk -F= '{print $2}'`
		shift
		if test -z "$val"; then
			local possibleval=$1
			printf %s $1 "$possibleval" | grep ^- >/dev/null 2>&1
			if test "$?" != "0"; then
				val=$possibleval
				shift
			fi
		fi

        case "$arg" in 
            --help)
                do_help
                exit 1
            ;;
            --op-path)
                OPPATH=$val
            ;;
            --java-path)
                JAVAPATH=$val
            ;;
            --output-name)
                OUTPUT=$val
            ;;
	    --version)
		do_print_version
		exit 1
        esac
    done
}

do_help()
{
    echo "capackage usage:
    --prd-path  Path of the .prd file to be imported
                Default: /var/lib/oprofile/anon_samples.prd

    --java-path Directory containing JNC files directory 
                Default: /var/lib/oprofile/Java

    --op-path   Directory containing OProfile samples
                Default: /var/lib/oprofile/samples/current/

    --output    Name of the tar.gz file to be created
                Default: capacked

    --version   Print version number
    " >&2
}

verify_path()
{
    to_exit=0

    if !(test -d $OPPATH); then
        echo "$OPPATH is not a directory" >&2
        to_exit=1
    fi

    if !(test -d $JAVAPATH); then
        echo "Warning: $JAVAPATH is not available." >&2
    fi
    
    if test $to_exit -eq 1 ; then
        do_help
        exit 1
    fi
}

do_pack_binaries()
{
    if (test -d $1); then
        do_pack_binaries 
    fi
}


pack_binaries()
{
    cd $1
    DIRS=`ls`
    for d in $DIRS; do
        if (test -d $d); then
            pack_binaries $d
        else
            MYPATH=`pwd`    
            TARGET=`echo $MYPATH | sed -e "s/[a-zA-Z/_]*{root}//"`
           
            # target has names such as
            # /var/lib/oprofile/samples/current/{root}/usr/lib64/libpango-1.0. so.0.600.0/{dep}/{root}/usr/lib64/libpango-1.0.so.0.600.0
            # We need to split the string by {root} and {dep}
            TARGET1=`echo $TARGET | awk '{split($0,a,"/{root}|/{dep}"); print a[1]}'`
            TARGET2=`echo $TARGET | awk '{split($0,a,"/{root}|/{dep}"); print a[3]}'`

            if (test -f $TARGET1); then
                if [ "$TARGET1" ]; then
                	cp --parents $TARGET1 $MYPWD/$OUTPUT/binary/\{root\}
                fi
            fi

            if (test -f $TARGET2); then 
                if [ $TARGET2 ]; then
                	cp --parents $TARGET2 $MYPWD/$OUTPUT/binary/\{root\}
                fi
            fi

        fi
    done
    cd ..
}


do_pack()
{
    rm -rf $OUTPUT $OUTPUT.tar.gz
    mkdir -p $OUTPUT/binary/\{root\}
    pack_binaries $OPPATH
    cd $MYPWD

    cp -r  $OPPATH $OUTPUT
    if (test -d $JAVAPATH); then
	    cp -r  $OPPATH $JAVAPATH $OUTPUT
    fi

    cat /proc/cpuinfo > $OUTPUT/cpuinfo

    tar cf $OUTPUT.tar $OUTPUT 
    gzip $OUTPUT.tar
    rm -rf $OUTPUT
}

do_print_version()
{
	echo "capackage.sh: Version 1.0"	
}

do_print_message()
{
	if test -f "$OUTPUT.tar.gz" ; then
		echo "capackage.sh: Done package Oprofile samples and binaries."
		echo "              Please use CodeAnalyst to remotely import" 
		echo "              the $OUTPUT.tar.gz file."
		echo ""
		echo " ***************************************************************************"
		echo " * NOTE:                                                                   *"
		echo " * - Do not cross import between the 32-bit and 64-bit system.             *"
		echo " * - Cross import between different Linux distribution is not recommended. *"
		echo " *                                                                         *"
		echo " ***************************************************************************"
	else
		echo "ERROR: capackage.sh failed to create the $OUTPUT.tar.gz file."
	fi
	
}

do_options $@
verify_path
do_pack
do_print_message


