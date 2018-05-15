#!/bin/bash +

#set term enh 可扩展

for ((i=1; i<=14; i++))
do
awk "NR==11*($i-1)+1, NR==11*$i" SFAMA_opti > $i
done

gnuplot<<!
set term post eps color solid 
set output 'opti.eps'
set key top inside
set key font "0.5"
set key samplen 1.5
set ylabel "delivery rate" offset 0,0 font "40,bold"
set xlabel "num" offset 0,1 font "40,bold"
set xtics font "40,bold"
set ytics font "40,bold"
set autoscale y
plot "1" using 3:9 w lp lt 3 lw 7 title "1","2" using 3:9 w lp lt 4 lw 7 title "2","3" using 3:9 w lp lt 5 lw 7 title "3", "4" using 3:9 w lp lt 6 lw 7 title "4", "5" using 3:9 w lp lt 7 lw 7 title "5","6" using 3:9 w lp lt 8 lw 7 title "6","7" using 3:9 w lp lt 9 lw 7 title "7","8" using 3:9 w lp lt 1 lw 7 title "8","9" using 3:9 w lp lt 2 lw 7 title "9","10" using 3:9 w lp lt 10 lw 7 title "10","11" using 3:9 w lp lt 11 lw 7 title "11", "12" using 3:9 w lp lt 12 lw 7 title "12","13" using 3:9 w lp lt 13 lw 7 title "13","14" using 3:9 w lp lt 14 lw 7 title "14"
q
!



