#!/bin/bash

TEST_PATH=src/tests/self
DVR=src/luxdvr/luxdvr
CC1=$TEST_PATH/luxcc1.out
CC2=$TEST_PATH/luxcc2.out
ASM_TMP=$TEST_PATH/asm_tmp.asm
CFLAGS="-m$1 -q -alt-asm-tmp $ASM_TMP"

/bin/bash scripts/self_copy.sh

# phase 1
$DVR $CFLAGS $TEST_PATH/*.c $TEST_PATH/vm32_cgen/*.c $TEST_PATH/vm64_cgen/*.c $TEST_PATH/x86_cgen/*.c $TEST_PATH/x64_cgen/*.c -o $CC1 &>/dev/null
if [ "$?" != "0" ] ; then
	echo "Phase 1 failed!"
	exit 1
else
	echo "Phase 1 succeeded!"
fi

# phase 2
mv src/luxcc src/luxcc_tmp
cp $CC1 src/luxcc
$DVR $CFLAGS $TEST_PATH/*.c $TEST_PATH/vm32_cgen/*.c $TEST_PATH/vm64_cgen/*.c $TEST_PATH/x86_cgen/*.c $TEST_PATH/x64_cgen/*.c -o $CC2 &>/dev/null
if [ "$?" != "0" ] ; then
	echo "Phase 2 failed!"
	mv src/luxcc_tmp src/luxcc
	exit 1
else
	echo "Phase 2 succeeded!"
	mv src/luxcc_tmp src/luxcc
fi

# compare binaries
if cmp -s $CC1 $CC2 ; then
	echo "PASSED!"
	exit 0
else
	echo "FAILED!"
	exit 1
fi
