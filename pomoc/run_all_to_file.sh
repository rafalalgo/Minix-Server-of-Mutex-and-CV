#!/bin/sh

# runs all tests and saves them to /mnt/minix/pomoc/results/tests/

cd /mnt/minix/testy
echo TEST NR 1 ; ./test1.sh > /mnt/minix/pomoc/results/tests/test1.out
echo TEST NR 2 ; ./test2.sh > /mnt/minix/pomoc/results/tests/test2.out
echo TEST NR 3 ; ./test3.sh > /mnt/minix/pomoc/results/tests/test3.out
echo TEST NR 4 ; ./test4.sh > /mnt/minix/pomoc/results/tests/test4.out