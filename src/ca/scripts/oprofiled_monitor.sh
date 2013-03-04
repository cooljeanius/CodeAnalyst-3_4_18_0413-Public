#!/bin/bash
PIDOF=`which pidof`
OPROFILED_PID=`$PIDOF oprofiled`
if test  ${OPROFILED_PID}
then
	echo "Daemon pid                   : ${OPROFILED_PID}"
	echo ""
	echo "Daemon command line          : `cat /proc/${OPROFILED_PID}/cmdline`"
	echo ""
	echo "Daemon file descriptor count : `ls /proc/${OPROFILED_PID}/fd | wc -w`"
	echo ""
else
	echo "Daemon pid                   : N/A"
	echo ""
	echo "Daemon command line          : N/A"
	echo ""
	echo "Daemon file descriptor count : N/A"
	echo ""
fi
#-----------------------------------------------
if test -f /var/lib/oprofile/lock
then
	echo "/var/lib/oprofile/lock       : `cat /var/lib/oprofile/lock`"
	echo ""
else
	echo "/var/lib/oprofile/lock       : N/A"
	echo ""
fi
#-----------------------------------------------
if test -f /var/lib/oprofile/complete_dump
then
	echo "Daemon complete_dump time    : `stat -c"%y" /var/lib/oprofile/complete_dump`" 
	echo ""

else
	echo "Daemon complete_dump time    : N/A" 
	echo ""
fi
