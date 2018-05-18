#!/bin/sh
chmod 777 UW802_11
chmod 777 SFAMA
#set term enh 可扩展
#using smooth bezier
gnuplot<<!
set term post eps color solid 
set output '2.eps'
set key top inside
set key font "0.5"
set key samplen 1.5

set ylabel "overload kbits/s" offset 0,0 font "40,bold"
set xlabel "generating rate pkt/s" offset 0,1 font "40,bold"
plot "SFAMA" using 1:3  smooth bezier lw 7  title "SFAMA", "UW802_11" using 1:3  smooth bezier lw 7 title "UW802.11","ALOHA" using 1:3  smooth bezier lw 3 title "ALOHA"


set ylabel "throughput kbits/s" offset 0,0 font "40,bold"
set xlabel "generating rate pkt/s" offset 0,1 font "40,bold"
plot "SFAMA" using 1:7 smooth bezier lw 7 title "SFAMA","UW802_11" using 1:7 smooth bezier lw 7 title "UW802.11","ALOHA" using 1:7 smooth bezier lw 7 title "ALOHA"


set ylabel "average end-to-end delay/s" offset 0,0 font "40,bold"
set xlabel "generating rate pkt/s" offset 0,0 font "40,bold"
plot "SFAMA" using 1:8 smooth bezier lw 7 title "SFAMA","UW802_11" using 1:8 smooth bezier lw 7 title "UW802.11","ALOHA" using 1:8 smooth bezier lw 7  title "ALOHA"


set ylabel "successful delivery rate" offset 0,0 font "40,bold"
set xlabel "generating rate pkt/s" offset 0,0 font "40,bold"
set ytics 0.1 font '40,bold'
plot "SFAMA" using 1:9 smooth bezier lw 7 title "SFAMA","UW802_11" using 1:9 smooth bezier lw 7 title "UW802.11" ,"ALOHA" using 1:9 smooth bezier lw 7 title "ALOHA"


set ylabel "average energy consumption J/packet" offset 0,0 font "40,bold"
set xlabel "generating rate pkt/s" offset 0,0 font "40,bold"
set yrange[0:800]
set ytics 50
plot "SFAMA" using 1:10 smooth bezier lw 7 title "SFAMA","UW802_11" using 1:10 smooth bezier lw 7 title "UW802.11" ,"ALOHA" using 1:10 smooth bezier lw 7 title "ALOHA"
q
!



