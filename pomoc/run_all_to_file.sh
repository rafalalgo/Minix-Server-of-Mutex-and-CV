#!/bin/sh

# runs all tests and saves them to /mnt/stuff/results/tests/

cd /mnt/tests
echo TEST NR 1 ; ./test1.sh > /mnt/stuff/results/tests/test1.out
echo TEST NR 2 ; ./test2.sh > /mnt/stuff/results/tests/test2.out
echo TEST NR 3 ; ./test3.sh > /mnt/stuff/results/tests/test3.out
echo TEST NR 4 ; ./test4.sh > /mnt/stuff/results/tests/test4.out