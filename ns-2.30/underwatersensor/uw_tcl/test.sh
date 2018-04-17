#!/bin/bash -x
rm test_802.data
rm test_SF.data
rm test_ALOHA.data
for ((i=1; i<=12; i++))
do
     rate=`echo "scale=4; $i*0.01" | bc`
     sed "1c set opt(data_rate)    0$rate" 802_11.tcl >ch_datarate.tcl;
     ns ch_datarate.tcl > 802_11.txt;
     for ((j=1; j<=10; j++))
     do
     ns ch_datarate.tcl > 802_11.txt;
     awk -f performance.awk test_802.data > plotdata_802;
     done
done
awk -f mul_average.awk plotdata_802 > UW802_11

for ((i=1; i<=12; i++))
do
     rate=`echo "scale=4; $i*0.01" | bc`
     sed "1c set opt(data_rate)    0$rate" sFAMA.tcl >ch_datarate.tcl;
     for ((j=1; j<=10; j++))
     do     
     ns ch_datarate.tcl > SFAMA.txt;
     awk -f performance.awk test_SF.data > plotdata_SF;
     done
done
awk -f mul_average.awk plotdata_SF > SFAMA

for ((i=1; i<=12; i++))
do
     rate=`echo "scale=4; $i*0.01" | bc`
     sed "1c set opt(data_rate)    0$rate" aloha.tcl >ch_datarate.tcl;
     for ((j=1; j<=10; j++))
     do     
     ns ch_datarate.tcl > aloha.txt;
     awk -f performance.awk test_ALOHA.data > plotdata_ALOHA;
     done
done
awk -f mul_average.awk plotdata_ALOHA > ALOHA

./plot.sh




