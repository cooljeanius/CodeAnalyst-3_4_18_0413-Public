#!/bin/bash
RUN_NAME=""
NUM_RUN=0

OPCONTROL=/opt/CodeAnalyst/bin/opcontrol

run_app()
{
	##################################
	# EDIT: Run your apps here
	./classic &

	sleep 5

	pkill classic
	##################################
}

configure_oprofile()
{
	##################################
	# EDIT: Specify run configuration	
	$OPCONTROL --no-vmlinux
	$OPCONTROL --separate=all
	$OPCONTROL --event=CPU_CLK_UNHALTED:100000:0:1:1
	##################################
}

setup_profile()
{
	rm -rf ~/.oprofile/daemonrc
	rm -rf /var/lib/oprofile/samples/$RUN_NAME*
	$OPCONTROL --reset
	configure_oprofile
	echo "---------- STATUS --------------"
	$OPCONTROL --status
	
}

run_profile()
{
	for i in `seq 0 $((NUM_RUN-1))`
	do
		echo "---------- RUN $i --------------"
		$OPCONTROL --start
		run_app
		$OPCONTROL --shutdown
		$OPCONTROL --save=${RUN_NAME}_$i
	done
}

finished()
{
	echo "---------- FINISHED ------------"
	echo "--- Finished ${NUM_RUN}."
	echo "--- Your profiles are in :"
	echo "--- /var/lib/oprofile/samples/"
	echo "        `ls /var/lib/oprofile/samples/ | grep ${RUN_NAME} 2> /dev/null`"
	echo ""
	echo "--- Please use CodeAnalyst to import the profiles"
}

####################################################
# MAIN

if test "$#" != 2 ; then
	echo "[Usage]   run_multiple_session.sh <Run Name> <Num of Runs>"
	echo "[Example] run_multiple_session.sh test 10"
else
	RUN_NAME=$1
	NUM_RUN=$2
fi

setup_profile
run_profile
finished


