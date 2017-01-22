#!/bin/sh

cc src/test3.c -o bin/test3 2> /dev/null

echo "test sprawdzajacy sygnaly, trwa ok 10 sekund"

bin/test3 5 | sort> out/out3

if [$(diff template/test3_template out/out3) = ""]; then
	echo OK
else
	echo FAIL
fi

