#!/bin/bash
if ! test -f /dev/oprofile/enable
then
	echo "Error oprofile driver is not loaded"
	exit
fi

echo "---- General Info ----"
echo "/dev/oprofile/enable                       : `cat /dev/oprofile/enable 2> /dev/null`"
echo "/dev/oprofile/cpu_type                     : `cat /dev/oprofile/cpu_type 2> /dev/null`"
echo "/dev/oprofile/buffer_size                  : `cat /dev/oprofile/buffer_size 2> /dev/null`"
echo "/dev/oprofile/buffer_watershed             : `cat /dev/oprofile/buffer_watershed 2> /dev/null`"
echo "/dev/oprofile/cpu_buffer_size              : `cat /dev/oprofile/cpu_buffer_size 2> /dev/null`"
echo "/dev/oprofile/backtrace_depth              : `cat /dev/oprofile/backtrace_depth 2> /dev/null`"
echo "/dev/oprofile/ca_css_depth                 : `cat /dev/oprofile/ca_css_depth 2> /dev/null`"
echo "/dev/oprofile/ca_css_interval              : `cat /dev/oprofile/ca_css_interval 2> /dev/null`"
echo "/dev/oprofile/ca_css_tgid                  : `cat /dev/oprofile/ca_css_tgid 2> /dev/null`"
echo "/dev/oprofile/ca_css_bitness               : `cat /dev/oprofile/ca_css_bitness 2> /dev/null`"
echo "/dev/oprofile/time_slice                   : `cat /dev/oprofile/time_slice 2> /dev/null`"
echo ""

echo "---- Lost Info ----"
echo "/dev/oprofile/stats/event_lost_overflow    : `cat /dev/oprofile/stats/event_lost_overflow 2> /dev/null`"
echo "/dev/oprofile/stats/bt_lost_no_mapping     : `cat /dev/oprofile/stats/bt_lost_no_mapping 2> /dev/null`"
echo "/dev/oprofile/stats/sample_lost_no_mapping : `cat /dev/oprofile/stats/sample_lost_no_mapping 2> /dev/null`"
echo "/dev/oprofile/stats/sample_lost_no_mm      : `cat /dev/oprofile/stats/sample_lost_no_mm 2> /dev/null`"
echo "/dev/oprofile/stats/multiplex_counter      : `cat /dev/oprofile/stats/multiplex_counter 2> /dev/null`"
echo ""

echo "---- CPU ----"
echo "  CPU  | sample_lost_overflow | sample_received"
for i in `ls /dev/oprofile/stats | grep cpu | sed "s/cpu//g" |  sort -n`
do
echo "  $i | `cat /dev/oprofile/stats/cpu$i/sample_lost_overflow 2> /dev/null` | `cat /dev/oprofile/stats/cpu$i/sample_received 2> /dev/null`"
done
echo ""

if test -d /dev/oprofile/ibs_fetch
then
echo "---- IBS FETCH ----"
echo " enable | max_count | rand_enable"
echo "    `cat /dev/oprofile/ibs_fetch/enable`   | `cat /dev/oprofile/ibs_fetch/max_count` | `cat /dev/oprofile/ibs_fetch/rand_enable` "
fi
echo ""

if test -d /dev/oprofile/ibs_op
then
echo "---- IBS OP ----"
echo " enable | max_count | dispatched_ops"
echo "    `cat /dev/oprofile/ibs_op/enable`   | `cat /dev/oprofile/ibs_op/max_count` | `cat /dev/oprofile/ibs_op/dispatched_ops`"
fi
echo ""

echo "---- PMC ----"
echo " Counter | enabled | event | unitmasks | count"
for i in `ls /dev/oprofile | grep [0-9] | sort -n`
do
echo "    $i    | `cat /dev/oprofile/$i/enabled 2> /dev/null` | `cat /dev/oprofile/$i/event 2> /dev/null` | `cat /dev/oprofile/$i/unit_mask 2> /dev/null` | `cat /dev/oprofile/$i/count 2> /dev/null`"
done
echo ""

