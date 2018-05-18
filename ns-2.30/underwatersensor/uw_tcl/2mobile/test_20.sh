#!/bin/bash -x
rm test_802.data
rm test_SF.data
rm test_ALOHA.data
for ((i=1; i<=30; i++))
do
     rate=`echo "scale=4; $i*0.001" | bc`
     sed "1c set opt(data_rate)    0$rate" 802_11_20.tcl >ch_datarate.tcl;
     for ((j=1; j<=10; j++))
     do
     rm test_802.data
     ns ch_datarate.tcl > 802_11.txt;
     awk -f performance.awk test_802.data >> plotdata_802;
     done
done
awk -f mul_average.awk plotdata_802 > UW802_11

for ((i=1; i<=30; i++))
do
     rate=`echo "scale=4; $i*0.001" | bc`
     sed "1c set opt(data_rate)    0$rate" sFAMA_20.tcl >ch_datarate.tcl;
     for ((j=1; j<=10; j++))
     do     
     rm test_SF.data
     ns ch_datarate.tcl > SFAMA.txt;
     awk -f performance.awk test_SF.data >> plotdata_SF;
     done
done
awk -f mul_average.awk plotdata_SF > SFAMA

for ((i=1; i<=30; i++))
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
awk -f mul_average.awk plotdata_ALOHA > ALOHA

./plot_20.sh




