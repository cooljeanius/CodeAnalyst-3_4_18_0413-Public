#!/bin/bash

#?? Doesn't seem like the count is doing anything :(
perf record --event=r076,r0c0 --count=100000 $1
perf report -n
