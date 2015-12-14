#!/bin/bash
CC1=src/luxdvr/luxdvr 	# compiler being tested
CC2=gcc      		# reference compiler
VM=src/luxvm/luxvm
TESTS_PATH=src/tests/execute
if uname -i | grep -q "i386"; then
	CC1="$CC1 -q -mvm32"
else
	CC1="$CC1 -q -mvm64"
fi

fail_counter=0
fail_files=""
pass_counter=0

#for file in $TESTS_PATH/*.c ; do
for file in $(find $TESTS_PATH/ | grep '\.c') ; do
	# skip 'other' tests
	if echo $file | grep -q "$TESTS_PATH/other" ; then		
		continue;
	fi

	echo $file
	
	# avoid llvm benchmarks
	if echo $file | grep -q "llvm"; then
		continue
	fi
	
	# out1	
	$CC1 $file -o $TESTS_PATH/test1.vme &>/dev/null &&
	$VM $TESTS_PATH/test1.vme >"${file%.*}.output" 2>/dev/null
	rm -f $TESTS_PATH/test1.vme

	# out2
	if [ ! "$LUX_DONT_RECALC" = "1" ] ; then	
		$CC2 $file -o $TESTS_PATH/test2 2>/dev/null
		$TESTS_PATH/test2 >"${file%.*}.expect" 2>/dev/null
		rm -f $TESTS_PATH/test2
	fi
	
	# compare
	if ! cmp -s "${file%.*}.output" "${file%.*}.expect" ; then
		echo "failed: $file"
		let fail_counter=fail_counter+1
		fail_files="$fail_files $file"		
	else
		let pass_counter=pass_counter+1
	fi
	
	# clean
	rm -f "${file%.*}.output"
done

echo "passes: $pass_counter"
echo "fails: $fail_counter"

if [ "$fail_counter" = "0" ] ; then
	exit 0
else
	exit 1
fi
