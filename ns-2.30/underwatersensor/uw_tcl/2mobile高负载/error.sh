#!/bin/bash -x
awk '{printf("1 %f\n",$1)}' plotdata_802  >> error
