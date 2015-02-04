#!/bin/bash

# simple script for generating source code for mechanisms
# required to build the runtime tests

args=""
args+=" --verbose"
mechanisms="Ca KdShu2007 Ih NaTs2_t expsyn Ca_HVA"
for mech in $mechanisms
do
    echo ../../bin/modcc ../modfiles/$mech.mod -o mechanisms/$mech.h $args
    ../../bin/modcc ../modfiles/$mech.mod -o mechanisms/$mech.h $args
done

