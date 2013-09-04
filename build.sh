#!/bin/bash

rm -f ./*.pbw

echo "#define INVERTED" > src/config.h
./waf build
if [ $? -eq 0 ]; then
    mv build/Gravity.pbw Gravity-inverted.pbw
else
    exit 1
fi

echo > src/config.h
./waf build
if [ $? -eq 0 ]; then
    mv build/Gravity.pbw Gravity.pbw
else
    exit 1
fi
