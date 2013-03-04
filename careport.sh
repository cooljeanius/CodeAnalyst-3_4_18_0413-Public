#!/bin/bash

OUTPUT_FILE=careport.txt
VERSION=1.3

echo "$0:"
echo "        This script will help the CodeAnalyst team analyze basic system configuration"
echo "        in order to provide supports for CodeAnalyst installation."
echo ""
echo "        Please send the output \"$OUTPUT_FILE\" back to CodeAnalyst team "
echo "        for further investigation."
echo "--------------------------------------------------------"

## version
echo "SECTION: version ------------------------------------------------" > ${OUTPUT_FILE}
echo careport.sh version: $VERSION >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

## date
echo "SECTION: date ---------------------------------------------------" >> ${OUTPUT_FILE}
echo "Running date"
date >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

## cat /proc/cpuinfo 
echo "SECTION: cat /proc/cpuinfo --------------------------------------" >> ${OUTPUT_FILE}
echo "Running cat /proc/cpuinfo"
cat /proc/cpuinfo >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

## /etc/...-release
echo "Checking Linux Distribution "
echo "SECTION: Check Linux Distribution -------------------------------" >> ${OUTPUT_FILE}
if test -e "/etc/redhat-release"
then
	echo "cat /etc/redhat-release" >> ${OUTPUT_FILE}
	cat /etc/redhat-release >> ${OUTPUT_FILE}
else 
	if test -e "/etc/SuSE-release"
	then
		echo "cat /etc/SuSE-release" >> ${OUTPUT_FILE}
		cat /etc/SuSE-release >> ${OUTPUT_FILE}
	else
		if test -e "/etc/lsb-release"
		then
			echo "cat /etc/lsb-release" >> ${OUTPUT_FILE}
			cat /etc/lsb-release >> ${OUTPUT_FILE}
		else
			echo"Not Supported Linux Distribution" >> ${OUTPUT_FILE}
		fi
	fi
fi
echo "" >> ${OUTPUT_FILE}

## uname -a
echo "SECTION: uname -a -----------------------------------------------" >> ${OUTPUT_FILE}
echo "Running uname -a "
uname -ra >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

## config.log
echo "SECTION: config.log ---------------------------------------------" >> ${OUTPUT_FILE}
echo "Copying config.log"
if test -e "config.log"
then
	cat config.log >> ${OUTPUT_FILE}
fi
echo "" >> ${OUTPUT_FILE}

## env
echo "SECTION: env | sort ---------------------------------------------" >> ${OUTPUT_FILE}
echo "Running env | sort "
env | sort >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

## rpm -qa
RPM=`which rpm 2> /dev/null`
if test "$RPM" != "" ; then 
	echo "SECTION: $RPM -qa | sort -----------------------------------------" >> ${OUTPUT_FILE}
	echo "Running $RPM -qa | sort"
	$RPM -qa | sort >> ${OUTPUT_FILE}
	echo "" >> ${OUTPUT_FILE}
fi

## dpkg
DPKG=`which dpkg 2> /dev/null`
if test "$DPKG" != "" ; then 
	echo "SECTION: $DPKG --get-selections | sort -----------------------------------------" >> ${OUTPUT_FILE}
	echo "Running $DPKG --get-selections | sort"
	$DPKG --get-selections | sort >> ${OUTPUT_FILE}
	echo "" >> ${OUTPUT_FILE}
fi

## gcc --version
echo "SECTION: gcc --version ------------------------------------------" >> ${OUTPUT_FILE}
echo "Running gcc --version"
gcc --version >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

## lsmod
echo "SECTION: lsmod --------------------------------------------------" >> ${OUTPUT_FILE}
echo "Running lsmod"
/sbin/lsmod  >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

## dmesg
echo "SECTION: dmesg --------------------------------------------------" >> ${OUTPUT_FILE}
echo "Running dmesg"
echo "dmesg" >> ${OUTPUT_FILE}
dmesg >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

## sysctl kernel.unknown_nmi_panic
echo "SECTION: Running sysctl kernel.unknown_nmi_panic ---------------" >> ${OUTPUT_FILE}
echo "Running sysctl kernel.unknown_nmi_panic"

echo "sysctl kernel.unknown_nmi_panic " >> ${OUTPUT_FILE}
/sbin/sysctl kernel.unknown_nmi_panic >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

## sysctl -a
echo "SECTION: Running sysctl -a -----------------------------" >> ${OUTPUT_FILE}
echo "Running sysctl -a"

echo "sysctl -a " >> ${OUTPUT_FILE}
sysctl -a >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

## sysctl -a
echo "SECTION: Running cat /proc/interrupts | grep NMI -----------------------------" >> ${OUTPUT_FILE}
echo "Running cat /proc/interrupts | grep NMI"

echo "cat /proc/interrupts | grep NMI" >> ${OUTPUT_FILE}
cat /proc/interrupts | grep NMI >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

## CA Installation
echo "SECTION: Checking CodeAnalyst Installation --------------------------" >> ${OUTPUT_FILE}
echo "Checking CodeAnalyst Installation"

echo "Checking CodeAnalyst source version" >> ${OUTPUT_FILE}
if [ -f configure.ac ]; then
	fgrep "AC_INIT(CodeAnalyst" configure.ac >> ${OUTPUT_FILE}
	echo "" >> ${OUTPUT_FILE}
fi

echo "which CodeAnalyst" >> ${OUTPUT_FILE}
CA_BINARY=`which CodeAnalyst 2> /dev/null`
if test "$CA_BINARY" = "" ; then
	echo "---- Warning: which CodeAnalyst failed" >> ${OUTPUT_FILE}
	echo "----          force check in default location /opt/CodeAnalyst/bin" >> ${OUTPUT_FILE}
	CA_BINARY=/opt/CodeAnalyst/bin/CodeAnalyst
else
	echo "$CA_BINARY" >> ${OUTPUT_FILE}
fi
echo "" >> ${OUTPUT_FILE}

echo "Checking CodeAnalyst installed version" >> ${OUTPUT_FILE}
$CA_BINARY --version >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

echo "ldd CodeAnalyst" >> ${OUTPUT_FILE}
ldd $CA_BINARY >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

echo "cat /etc/group | grep amdca" >> ${OUTPUT_FILE}
cat /etc/group | grep amdca >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}


echo "cat /etc/sudoers | grep oprofiled" >> ${OUTPUT_FILE}
cat /etc/sudoers | grep oprofiled >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}


## Oprofile Installation
echo "SECTION: Checking Oprofile Installation -----------------------------" >> ${OUTPUT_FILE}
echo "Checking Oprofile Installation"

echo "which opcontrol" >> ${OUTPUT_FILE}
which opcontrol >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

echo "opcontrol --version" >> ${OUTPUT_FILE}
opcontrol --version >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

echo "SECTION: Oprofile events ----------------------------------------" >> ${OUTPUT_FILE}
echo "opcontrol -l" >> ${OUTPUT_FILE}
opcontrol -l >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

echo "SECTION: Oprofile device file -----------------------------------" >> ${OUTPUT_FILE}

DEVOPROFILE=/dev/oprofile

read_dir()
{
	for i in `ls $1/$2`
	do
		if [ -f $1/$2/$i ]; then
			echo "`ls -l $1/$2/$i`" >> ${OUTPUT_FILE}
			echo `cat $1/$2/$i 2>&1 >> ${OUTPUT_FILE}` >> ${OUTPUT_FILE}
		else
			read_dir $1/$2 $i			
		fi

	done
}

read_file()
{
	for i in `ls ${DEVOPROFILE}`
	do
		if [ -f ${DEVOPROFILE}/$i ]; then
			echo "`ls -l ${DEVOPROFILE}/$i`" >> ${OUTPUT_FILE}
		 	echo `cat ${DEVOPROFILE}/$i 2>&1 >> ${OUTPUT_FILE}` >> ${OUTPUT_FILE}
		else
			read_dir $DEVOPROFILE $i			
		fi

	done
}

read_file

echo "" >> ${OUTPUT_FILE}

## Checking oprofiled.log
echo "SECTION: Checking oprofiled.log -----------------------------" >> ${OUTPUT_FILE}
echo "Checking oprofiled.log"

echo "cat /var/lib/oprofile/oprofiled.log" >> ${OUTPUT_FILE}
cat /var/lib/oprofile/oprofiled.log >> ${OUTPUT_FILE} 2> /dev/null
echo "" >> ${OUTPUT_FILE}

## Checking oprofile lock file
echo "SECTION: Checking oprofile lock file -----------------------------" >> ${OUTPUT_FILE}
echo "Checking oprofile lock file"

echo "cat /var/lib/oprofile/lock" >> ${OUTPUT_FILE}
if test -f "var/lib/oprofile/lock";
then
	cat /var/lib/oprofile/lock >> ${OUTPUT_FILE}
else
	echo "/var/lib/oprofile/lock does not exist." >> ${OUTPUT_FILE}
fi
echo "" >> ${OUTPUT_FILE}

## Checking oprofile daemon
echo "SECTION: Checking oprofile daemon -----------------------------" >> ${OUTPUT_FILE}
echo "Checking oprofile daemon"

echo "ps aux | grep oprofiled" >> ${OUTPUT_FILE}
ps aux | grep oprofiled >> ${OUTPUT_FILE}
echo "" >> ${OUTPUT_FILE}

## Checking oprofile JIT 
echo "SECTION: Checking oprofile JIT -----------------------------" >> ${OUTPUT_FILE}
echo "Checking oprofile JIT"

echo "ls -l /var/lib/oprofile/jit" >> ${OUTPUT_FILE}
if test -e "/var/lib/oprofile/jit"
then
	ls -l /var/lib/oprofile/jit >> ${OUTPUT_FILE} 2> /dev/null
	ls /var/lib/oprofile/jit/* >> ${OUTPUT_FILE} 2> /dev/null
else
	echo "/var/lib/oprofile/jit does not exist." >> ${OUTPUT_FILE}
fi
echo "" >> ${OUTPUT_FILE}

## END 
echo "END REPORT ---------------------------------------------" >> ${OUTPUT_FILE}

#############
# CHANGELOG #
#############
# V.1.3 (04/22/2009)
# Suravee Suthikulpanit <suravee.suthikulpanit@amd.com>
# - change output filename to careport.txt
# - add cat /proc/interrupts

# V.1.2 (03/18/2009)
# Suravee Suthikulpanit <suravee.suthikulpanit@amd.com>
# - Hide error messages
# - Fixed which CodeAnalyst
