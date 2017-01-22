#!/bin/sh

cc src/test4.c -o bin/test4 2> /dev/null

echo "Test na sygnaly i zabite dzieci"

bin/test4 > out/out4

if [$(diff template/test4_template out/out4) = ""]; then
  echo OK
else
  echo FAIL
fi