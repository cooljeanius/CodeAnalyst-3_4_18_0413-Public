#!/bin/bash
rm *.data*

perf record --event=cpu-clock --output=perf-cpu-clock.data $1
perf record --event=cpu-cycles --output=perf-cpu-cycles.data $1
perf record --event=r076 --count=100000 --output=perf-76-100000.data $1
perf record --event=r076,r0C0 --count=100000 --output=perf-76-C0-100000.data $1

for i in *.data ;
do
	xxd -c 8 $i > $i.xxd
	perf report -D -i $i > $i.dmp
done
