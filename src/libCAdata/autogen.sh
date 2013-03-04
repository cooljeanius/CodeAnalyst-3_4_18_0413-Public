#!/bin/bash
do_error()
{
	echo "!!! Please check if automake and autoconf packages are installed. !!!"
}

check_space_in_path()
{
	echo ".... checking current path"
	`pwd | grep ' ' > /dev/null`	
	if test $? = 0
	then
		echo "WARNING: The current path :"
		echo ""
		echo "              `pwd`" 
		echo ""
		echo "         might cause problem with libtool due to the space in"
		echo "         directory name. Please remove any spaces in the the"
		echo "         path to current directory."
		exit 1
	fi

}

echo "***********************************************************"
echo "*                   C O D E A N A L Y S T                 *"
echo "*                                                         *"
echo "*                         libCAdata                       *"
echo "*                                                         *"
echo "*   - Please see README file before installation.         *"
echo "*                                                         *"
echo "*   - Please refer to INSTALLATION file for building      *"
echo "*     and installation instruction.                       *"              
echo "*                                                         *"
echo "***********************************************************"

# Check space in path. This can fail libtool.
check_space_in_path

# Check the necessary tools for autogen.sh
echo ".... checking aclocal"
ACLOCAL=`which aclocal 2> /dev/null`
if test $? != 0
then
	echo "ERROR: aclocal does not exist"
	do_error
	exit 1	
fi

echo ".... checking autoheader"
AUTOHEADER=`which autoheader 2> /dev/null`
if test $? != 0
then
	echo "ERROR: autoheader does not exist"	
	do_error
	exit 1	
fi

echo ".... checking libtoolize"
LIBTOOLIZE=`which libtoolize 2> /dev/null`
if test $? != 0
then
	echo "ERROR: libtoolize does not exist"	
	do_error
	exit 1	
fi

echo ".... checking automake "
AUTOMAKE=`which automake 2> /dev/null`
if test $? != 0
then
	echo "ERROR: automake does not exist"	
	do_error
	exit 1	
fi

echo ".... checking autoconf"
AUTOCONF=`which autoconf 2> /dev/null`
if test $? != 0
then
	echo "ERROR: autoconf does not exist"	
	do_error
	exit 1	
fi

echo ".... creating configure"

rm -rf autom4te.cache/*
${ACLOCAL} -I m4
${AUTOHEADER}
${LIBTOOLIZE} --automake --force
${AUTOMAKE} --add-missing
${AUTOCONF}

echo ""
echo "Configuration script is ready. Next please  run"
echo ""
echo "    ./configure"
echo ""
echo "to configure CodeAnalyst. See ./configure --help for list of options."
