#!/bin/bash
NUMCPU=`cat /proc/cpuinfo | grep processor | wc -l`

for (( i = 0 ; i < $NUMCPU ; i++ ))
do
	echo performance > /sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor
done
