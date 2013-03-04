#!/bin/bash
NUMCPU=`cat /proc/cpuinfo | grep processor | wc -l`

read_cpu_speed()
{
echo " $1 :  `cat /sys/devices/system/cpu/cpu$1/cpufreq/cpuinfo_min_freq` :  `cat /sys/devices/system/cpu/cpu$1/cpufreq/cpuinfo_cur_freq` :  `cat /sys/devices/system/cpu/cpu$1/cpufreq/cpuinfo_max_freq`"
}

echo " CPU : MIN : CUR : MAX "
for (( i = 0 ; i < $NUMCPU ; i++ ))
do
	read_cpu_speed $i 
done
