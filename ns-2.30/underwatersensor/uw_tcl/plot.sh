#!/bin/sh
chmod 777 UW802_11
chmod 777 SFAMA
#set term enh 可扩展
gnuplot<<!
set term post eps color solid 
set output '1.eps'
set key top inside
set key font "0.5"
set key samplen 1.5
set ylabel "throughput bits/s" offset 0,0 font "40,bold"
set xlabel "overload packet/s" offset 0,1 font "40,bold"
set xtics 0.2 font "40,bold"
set ytics 0.05 font "40,bold"
plot "UW802_11" using 3:7 w lp lt 3 lw 7 title "UW802.11","SFAMA" using 3:7 w lp lt 7 lw 7 title "SFAMA","ALOHA" using 3:7 w lp lt 4 lw 7 title "ALOHA"


set ylabel "average end-to-end delay/s" offset 0,0 font "40,bold"
set xlabel "overload packet/s" offset 0,0 font "40,bold"
set xtics 0.2 font '40,bold'
set ytics 50 font '40,bold'
set yrange [0:600]
plot "UW802_11" using 3:8 w lp lt 3 lw 7 title "UW802.11","SFAMA" using 3:8 w lp lt 7 lw 7 title "SFAMA","ALOHA" using 3:8 w lp lt 4 lw 7 title "ALOHA"

set ylabel "successful delivery rate" offset 0,0 font "40,bold"
set xlabel "overload packet/s" offset 0,0 font "40,bold"
set xtics 0.2 font '40,bold'
set ytics 0.1 font '40,bold'
set yrange [0:1]
plot "UW802_11" using 3:9 w lp lt 3 lw 7 title "UW802.11" ,"SFAMA" using 3:9 w lp lt 7 lw 7 title "SFAMA","ALOHA" using 3:9 w lp lt 4 lw 7 title "ALOHA"


set ylabel "average energy consumption J/packet" offset 0,0 font "40,bold"
set xlabel "overload packet/s" offset 0,0 font "40,bold"
set xtics 0.2 font '40,bold'
set yrange [0:80]
set ytics 5 font '40,bold'
plot "UW802_11" using 3:10 w lp lt 3 lw 7 title "UW802.11" ,"SFAMA" using 3:10 w lp lt 7 lw 7 title "SFAMA","ALOHA" using 3:10 w lp lt 4 lw 7 title "ALOHA"
q
!



