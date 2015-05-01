#!/bin/sh

#### CONFIGURATION

# Default iteration count (can be overwritten by 1st parameter)
DEFAULT_ITERATIONS=100
# Default molecule count (can be overwritten by 2nd parameter)
DEFAULT_MOLECULES=1000

#### FUNCTIONS

test_h20 () {
	RET=0
	./h2o $MOLECULES 0 0 0 &
	PID=$!
	wait $!
	STATUS=$?
	REMAINING="`ps -ef | grep \`whoami\` | grep h2o | wc -l`"
	REMAINING=`echo "$REMAINING-1" | bc`
	if [ "$STATUS" -ne 0 ]; then
		echo "Process has ended unsuccessfully with status $STATUS!" >&2
		RET=1
	fi

	if [ "$REMAINING" -gt 0 ]; then
		echo "$REMAINING processes left after the process finished!" >&2
		RET=1
	fi

	SHM="`ipcs -p | grep \`whoami\` | awk '{print $3}' | grep $PID`"

	if [ ! -z "$SHM" ]; then
		echo "Not-freed shared memory segment found!" >&2
		RET=1
	fi

	pkill -9 h2o

	ios-tests/test $MOLECULES
	if [ $? -ne 0 ]; then
		RET=1
	fi

	return $RET
}


#### CODE

if [ $# -ge 1 ]; then
	ITERATIONS="$1"
	else ITERATIONS="$DEFAULT_ITERATIONS"
fi

if [ $# -ge 2 ]; then
	MOLECULES="$2"
	else MOLECULES="$DEFAULT_MOLECULES"
fi

trap "echo ; echo 'Ending on request!' ; pkill -9 h2o ; exit 2" SIGINT

(cd ios-tests; make)
make
DIR="`pwd`"

pkill -9 h2o
	
#cd ..

I=0

while [ "$I" -lt "$ITERATIONS" ]; do

	test_h20
	if [ $? -ne 0 ]; then
		exit 1
	else
		echo "Test #$I successful!"
	fi

	I=`echo "$I+1" | bc`
done
