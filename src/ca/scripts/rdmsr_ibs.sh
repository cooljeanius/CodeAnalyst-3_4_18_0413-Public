#!/bin/bash
PROC_COUNT=`ls /dev/cpu`
RDMSR='rdmsr -0 --processor'
IBS0="0xc0011030"
MODE="vert"
MSRS="0x0 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8 0x9 0xa"


print_help () 
{
	echo "Options:"
	echo "   --hor            Horizontal Mode (Default:vertical)"
	echo "   --msrs|-m        List IBS MSRs offset (i.e. 0x0, 0x1, .. , 0xa) (Default: all)"
	echo "   --help|-h        Print this message"
}

print_header_horizontal ()
{
	echo "core : $MSRS"
}

per_core_rdmsr () 
{
	echo -n "$1 :" 
	for i in $MSRS
	do
		MSR=$(($IBS0 + $i))
		echo -n " `$RDMSR $1 $MSR`" 
	done
	echo ""
}

per_msr_rdmsr ()
{
	MSR=$(($IBS0 + $1))
	echo -n "$1 :"
	for i in $PROC_COUNT
	do
		echo -n " `$RDMSR $i $MSR`"
	done
	echo ""
}

horizontal () {
	print_header_horizontal
	for i in $PROC_COUNT
	do
		per_core_rdmsr $i
	done
}

vertical () {
	for i in $MSRS
	do
		per_msr_rdmsr $i
	done
}

do_options ()
{
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
				if [ "$#" -ge 1 ]; then
					shift
				fi
			fi
		fi

		case "$arg" in
			--hor)
				MODE="hor"
				;;
			--msrs|-m)
				MSRS=$val
				;;
			--help|-h)
				print_help
				exit 0;
				;;
			*)
				echo "ERROR: Unknown option \"$arg\". See help " >&2
				print_help
				exit 1;
				;;
		esac
	done
}

do_options $@
if [ "$MODE" == "vert" ]; then
	vertical
else	
	horizontal
fi
