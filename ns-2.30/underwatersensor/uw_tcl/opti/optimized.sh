#!/bin/bash -x
rm SFAMA_opti
for ((num=1; num<=14; num++))
do
	rm test_SF.data;
	for ((i=1; i<=12; i++))
	do
        rate=`echo "scale=4; $i*0.01" | bc`
        sed "1c set opt(data_rate)  0$rate" sFAMA.tcl | sed "2c set opt(max_backoff_slots) $num"  >1.tcl;
     	for ((j=1; j<=10; j++))
     	do     
     	ns 1.tcl > SFAMA.txt;
     	done
	done
        awk -f performance.awk test_SF.data > plotdata_SF;
        awk -f mul_average.awk plotdata_SF >> SFAMA_opti;       	
done





