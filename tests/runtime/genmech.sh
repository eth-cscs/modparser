#!/bin/bash

# simple script for generating source code for mechanisms
# required to build the runtime tests

mechanisms="Ca KdShu2007 Ih NaTs2_t expsyn"
for mech in $mechanisms
do
    ../../bin/modcc ../modfiles/$mech.mod -o mechanisms/$mech.h --verbose
done

