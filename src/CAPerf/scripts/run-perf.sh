#!/bin/bash

PERF=/usr/bin/perf

#$PERF record -e r076,r0c0 taskset 0x2 $1
#$PERF record -e r076 -c 100000 taskset 0x2 $1
#$PERF record -e r0c0,r076 -c 100000 taskset 0x2 $1
#$PERF record -e r0c0,r076 -c 100000 $1

#$PERF record -a -e r076,r0c0 -c 100000 taskset 0x2 $1
$PERF record -a -e r076,r0c0 -c 100000 $1

#$PERF report -n > $PERF.data.report
#$PERF report -D > $PERF.data.dump
#xxd -c8 -g8 $PERF.data > $PERF.data.xxd
/opt/CAPerf/bin/perf2ca -i `pwd`/perf.data -d > perf2ca.dump
