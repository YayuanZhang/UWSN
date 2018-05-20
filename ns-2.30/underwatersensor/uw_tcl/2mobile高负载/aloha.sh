#!/bin/bash -x
rm test_ALOHA.data
rm plotdata_ALOHA
for ((i=2; i<=30; i++))
do
     rate=`echo "scale=4; $i*0.001" | bc`
     sed "1c set opt(data_rate)    0$rate" aloha_20.tcl >ch_datarate.tcl;
     for ((j=1; j<=10; j++))
     do     
     rm test_ALOHA.data
     ns ch_datarate.tcl > aloha.txt;
     awk -f performance.awk test_ALOHA.data >> plotdata_ALOHA;
     done
done
awk -f aloha_average.awk plotdata_ALOHA > ALOHA

./plot_20.sh




