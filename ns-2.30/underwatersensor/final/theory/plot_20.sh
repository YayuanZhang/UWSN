#!/bin/sh
#set term enh 可扩展
#using smooth bezier
gnuplot<<!
set term pdfcairo font '/usr/share/fonts/winfonts/simsun.ttc,8'
set key top inside
set key font "0.5"
set key samplen 1.5

set output 'lilun.pdf'
set ylabel "吞吐量 kbits/s" offset 0,0 font "bold"
set xlabel "数据产生速率 pkt/s" offset 0,0 font "bold"
set yrange [0:0.7]
plot "theoryl" using 1:2 with linespoints lt 10 lw 2 ps 0.5 title "LACC-M 低负载 理论","UW802_11l" using 1:7 with linespoints lt 2 lw 2 ps 0.5 title "LACC-M 低负载 仿真","theoryh" using 1:2 with linespoints lt 28 lw 2 ps 0.5 title "LACC-M 高负载 理论","UW802_11h" using 1:7 with linespoints lt 4 lw 2 ps 0.5 title "LACC-M 高负载 仿真"

q
!



