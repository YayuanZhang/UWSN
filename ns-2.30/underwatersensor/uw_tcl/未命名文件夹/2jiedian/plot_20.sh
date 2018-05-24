#!/bin/sh
#set term enh 可扩展
#using smooth bezier
gnuplot<<!
set term pdfcairo color
set key top inside
set key font "1"
set key samplen 1.5

set key left
set output '2nodea.pdf'
set ylabel "吞吐量 kbits/s" offset 0,0 font "8,bold"
set xlabel "数据产生速率 pkt/s" offset 0,0 font "8,bold"
plot "UW802_11" using 1:7 smooth bezier lt 7 lw 4 title "MAPA-CSMA 自适应负载","UW802_11l" using 1:7 with points lt 7 ps 0.3 notitle,"UW802_11l" using 1:7 smooth bezier lt 5 lw 2 title "MAPA-CSMA 低负载模式","UW802_11l" using 1:7 with points lt 5 ps 0.3 notitle,"UW802_11h" using 1:7 smooth bezier lt 147 lw 2 title "MAPA-CSMA 高负载模式","UW802_11h" using 1:7 with points lt 147 ps 0.3 notitle,"SFAMA" using 1:7 smooth bezier lt 4 lw 4 title "SFAMA","SFAMA" using 1:7 with points lt 4 ps 0.3 notitle,"ALOHA" using 1:7 smooth bezier lt 10 lw 4 title "ALOHA","ALOHA" using 1:7 with points lt 10 ps 0.2 notitle

set key left
set output '2nodeb.pdf'
set ylabel "平均端到端时延 s" offset 0,0 font "8,bold"
set xlabel "数据产生速率 pkt/s" offset 0,0 font "8,bold"
plot "UW802_11" using 1:8 smooth bezier lt 7 lw 4 title "MAPA-CSMA 自适应负载","UW802_11l" using 1:8 with points lt 7 ps 0.3 notitle,"UW802_11l" using 1:8 smooth bezier lt 5 lw 2 title "MAPA-CSMA 低负载模式","UW802_11l" using 1:8 with points lt 5 ps 0.3 notitle,"UW802_11h" using 1:8 smooth bezier lt 147 lw 2 title "MAPA-CSMA 高负载模式","UW802_11h" using 1:8 with points lt 147 ps 0.3 notitle,"SFAMA" using 1:8 smooth bezier lt 4 lw 4 title "SFAMA","SFAMA" using 1:8 with points lt 4 ps 0.3 notitle,"ALOHA" using 1:8 smooth bezier lt 10 lw 4 title "ALOHA","ALOHA" using 1:8 with points lt 10 ps 0.3 notitle

set key right
set output '2nodec.pdf'
set ylabel "发送成功率" offset 0,0 font "8,bold"
set xlabel "数据产生速率 pkt/s" offset 0,0 font "8,bold"
set yrange[0:1]
plot "UW802_11" using 1:9 smooth bezier lt 7 lw 4 title "MAPA-CSMA 自适应负载","UW802_11l" using 1:9 with points lt 7 ps 0.3 notitle,"UW802_11l" using 1:9 smooth bezier lt 5 lw 2 title "MAPA-CSMA 低负载模式","UW802_11l" using 1:9 with points lt 5 ps 0.3 notitle,"UW802_11h" using 1:9 smooth bezier lt 147 lw 2 title "MAPA-CSMA 高负载模式","UW802_11h" using 1:9 with points lt 147 ps 0.3 notitle,"SFAMA" using 1:9 smooth bezier lt 4 lw 4 title "SFAMA","SFAMA" using 1:9 with points lt 4 ps 0.3 notitle,"ALOHA" using 1:9 smooth bezier lt 10 lw 4 title "ALOHA","ALOHA" using 1:9 with points lt 10 ps 0.3 notitle

set key right
set output '2noded.pdf'
set ylabel "平均能耗 J/pkt" offset 0,0 font "8,bold"
set xlabel "数据产生速率 pkt/s" offset 0,0 font "8,bold"
set yrange[0:500]
plot "UW802_11" using 1:10 smooth bezier lt 7 lw 4 title "MAPA-CSMA 自适应负载","UW802_11l" using 1:10 with points lt 7 ps 0.3 notitle,"UW802_11l" using 1:10 smooth bezier lt 5 lw 2 title "MAPA-CSMA 低负载模式","UW802_11l" using 1:10 with points lt 5 ps 0.3 notitle,"UW802_11h" using 1:10 smooth bezier lt 147 lw 2 title "MAPA-CSMA 高负载模式","UW802_11h" using 1:10 with points lt 147 ps 0.3 notitle,"SFAMA" using 1:10 smooth bezier lt 4 lw 4 title "SFAMA","SFAMA" using 1:10 with points lt 4 ps 0.3 notitle,"ALOHA" using 1:10 smooth bezier lt 10 lw 4 title "ALOHA","ALOHA" using 1:10 with points lt 10 ps 0.3 notitle

q
!



