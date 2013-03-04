#
# CodeAnalyst for Open Source
# Copyright 2002 . 2007 Advanced Micro Devices, Inc.
# You may redistribute this program and/or modify this program under the terms
# of the GNU General Public License as published by the Free Software 
# Foundation; either version 2 of the License, or (at your option) any later 
# version.
#
# This program is distributed WITHOUT ANY WARRANTY; WITHOUT EVEN THE IMPLIED 
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  See the 
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA 02111-1307 USA.

#!/bin/bash

# prefix must be the first parameter
# bindir must be the second parameter
# They have be  passed separately b/c bindir is not always $prefix/bin
PREFIX=$1
GREP=/bin/grep
AMDCA_GRP=amdca
GROUPADD="/usr/sbin/groupadd"
LN="/bin/ln -sf" 
#ETC="/etc"
#INITD="$ETC/init.d"
#RCD="$ETC/rc.d"
#CODEANALYST_INIT="$INITD/codeanalyst"
#CODEANALYST_LN="S99codeanalyst"
family10=$PREFIX/share/oprofile/x86-64/family10
family10h=$PREFIX/share/oprofile/x86-64/family10h


print_action()
{
	echo -n "* .... $1 ... "
}

print_result()
{
	echo "$1"
}

print_ok_no()
{
	if [ "$1" -eq "0" ] ; then
		echo "OK"
	else	
		echo "NO"
		exit 1
	fi
}

print_error()
{
	echo "$1"
	exit 1;
}

echo "***********************************"
echo "*  CodeAnalyst Post-Install Setup *"
echo "***********************************"

#
# Adding amdca group
#
print_action "Checking amdca group"
getent group amdca >/dev/null
if [ "$?" -ne "0" ] ; then 
	print_result "not exist"
	print_action  "Adding amdca group"
	$GROUPADD amdca
	RETVAL=$?; print_ok_no $RETVAL 
else
	print_result "OK"
fi


#
# Create softlink b/w x86_64/family10 and family10h
#
#if test -e $family10 ; then
#	rm -rf $family10 
#fi
#if test ! -L $family10 -a -d $family10h ; then
#	print_action "Create softlink for $family10 "
#	$LN $family10h $family10
#	RETVAL=$?; print_ok_no $RETVAL 
#fi


#
# Adding $PREFIX/lib to /etc/ld.so.conf
#
CA_LIB="$PREFIX/lib"
#print_action "Adding $CA_LIB to /etc/ld.so.conf"
#grep -w "^$CA_LIB$" /etc/ld.so.conf > /dev/null 2> /dev/null
#if test $? -ne 0; then
#	echo $CA_LIB >> /etc/ld.so.conf 
#	RETVAL=$?; print_ok_no $RETVAL 
#else
#	print_result "exists"
#fi
ARCH=`uname -m`
print_action "Adding $CA_LIB to /etc/ld.so.conf.d/codeanalyst-$ARCH.conf"
if test -d /etc/ld.so.conf.d/ ; then
	echo $CA_LIB > /etc/ld.so.conf.d/codeanalyst-$ARCH.conf
	RETVAL=$?; print_ok_no $RETVAL 
else
	print_result "directory /etc/ld.so.conf.d does not exists"
fi


#
# Running ldconfig
#
print_action "Running ldconfig"
/sbin/ldconfig
RETVAL=$?; print_ok_no $RETVAL 


#
# Installing codeanalyst service
#
#print_action "Install codeanalyst service."
#if [ -f scripts/codeanalyst ] ; then 
#	install -m 755 scripts/codeanalyst $INITD 
#	RETVAL=$?; print_ok_no $RETVAL 
#else
#	print_error "scripts/codeanalyst not found."
#fi

#
# Installing service at runlevel 3 and 5
#
#if test -e $RCD/rc3.d ; then
#	# SUSE and REDHAT
#	RC3D="$RCD/rc3.d"
#	RC5D="$RCD/rc5.d"
#else if test -e $ETC/rc3.d ; then 
#	# UBUNTU
#	RC3D="$ETC/rc3.d"
#	RC5D="$ETC/rc5.d"
#fi fi

#print_action "Install codeanalyst service in $RC3D"
#$LN $CODEANALYST_INIT  $RC3D/$CODEANALYST_LN
#RETVAL=$?; print_ok_no $RETVAL 

#print_action "Install codeanalyst service in $RC5D"
#$LN $CODEANALYST_INIT  $RC5D/$CODEANALYST_LN
#RETVAL=$?; print_ok_no $RETVAL 

#
# Restart codeanalyst service
#
#$CODEANALYST_INIT restart


echo "* Post-Install Setup Finished."
exit 0
