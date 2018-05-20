#!/bin/sh
chmod 777 UW802_11
chmod 777 SFAMA
#set term enh 可扩展
#using smooth bezier
gnuplot<<!
set term pdfcairo color solid 
set key top inside
set key font "0.5"
set key samplen 1.5


set output '2a.png'
set ylabel "overload kbits/s" offset 0,0 font "20,bold"
set xlabel "generating rate pkt/s" offset 0,1 font "20,bold"
plot  "UW802_11" using 1:3 smooth bezier lt 6 lw 2 title "proposed","UW802_11" using 1:3 with points pt 6 ps 0.1 notitle, "SFAMA" using 1:3 smooth bezier lt 4 lw 2 title "SFAMA","SFAMA" using 1:3 with points pt 5 ps 0.1 notitle,"ALOHA" using 1:3 smooth bezier lt 2 lw 2 title"ALOHA","ALOHA" using 1:3 with points pt 2 ps 0.1 notitle


set output '2b.png'
set ylabel "throughput kbits/s" offset 0,0 font "40,bold"
set xlabel "generating rate pkt/s" offset 0,1 font "40,bold"
plot "theory" using 1:2 smooth bezier lt 7 lw 2 title "proposed theory","theory" using 1:2 with points lt 7 ps 0.2 notitle,"UW802_11" using 1:7 smooth bezier lt 6 lw 2 title "proposed simu","UW802_11" using 1:7 with points lt 6 ps 0.2 notitle, "SFAMA" using 1:7 smooth bezier lt 4 lw 2 title "SFAMA","SFAMA" using 1:7 with points lt 4 ps 0.2 notitle,"ALOHA" using 1:7 smooth bezier lt 2 lw 2 title"ALOHA","ALOHA" using 1:7 with points lt 2 ps 0.2 notitle


set output '2c.png'
set ylabel "average end-to-end delay/s" offset 0,0 font "40,bold"
set xlabel "generating rate pkt/s" offset 0,0 font "40,bold"
plot "UW802_11" using 1:8 smooth bezier lt 6 lw 2 title "proposed","UW802_11" using 1:8 with points lt 6 ps 0.2 notitle, "SFAMA" using 1:8 smooth bezier lt 4 lw 2 title "SFAMA","SFAMA" using 1:8 with points lt 4 ps 0.2 notitle,"ALOHA" using 1:8 smooth bezier lt 2 lw 2 title"ALOHA","ALOHA" using 1:8 with points lt 2 ps 0.2 notitle

set output '2d.png'
set ylabel "啊啊" offset 0,0 font "20,bold"
set xlabel "generating rate pkt/s" offset 0,0 font "20,bold"
set ytics 0.1 font '20,bold'
set yrange [0:1]
plot "UW802_11" using 1:9 smooth bezier lt 6 lw 2 title "proposed","UW802_11" using 1:9 with points lt 6 ps 0.2 notitle, "SFAMA" using 1:9 smooth bezier lt 4 lw 2 title "SFAMA","SFAMA" using 1:9 with points lt 4 ps 0.2 notitle,"ALOHA" using 1:9 smooth bezier lt 2 lw 2 title"ALOHA","ALOHA" using 1:9 with points lt 2 ps 0.2 notitle

set output '2e.png'
set ylabel "average energy consumption J/packet" offset 0,0 font "40,bold"
set xlabel "generating rate pkt/s" offset 0,0 font "40,bold"
set ytics 50
set yrange [0:800]
plot "UW802_11" using 1:10 smooth bezier lt 6 lw 2 title "proposed","UW802_11" using 1:10 with points lt 6 ps 0.2 notitle, "SFAMA" using 1:10 smooth bezier lt 4 lw 2 title "SFAMA","SFAMA" using 1:10 with points lt 4 ps 0.2 notitle,"ALOHA" using 1:10 smooth bezier lt 2 lw 2 title"ALOHA","ALOHA" using 1:10 with points lt 2 ps 0.2 notitle
q
!



