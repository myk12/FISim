set terminal pdf
set output "multipath-test.pdf"
set title "Multipath Test"
set xlabel "Time (s)"
set ylabel "Throughput (Mbps)"
set grid

plot "Multipath_50_50.dat" using 1:2 title "Multipath 50Mbps + 50Mbps" with lines, \
    "Multipath_100_50.dat" using 1:2 title "Multipath 100Mbps + 50Mbps" with lines, \
    "Multipath_100_100.dat" using 1:2 title "Multipath 100Mbps + 100Mbps" with lines

plot    "SinglePath100_100.dat" using 1:2 title "Singlepath 100Mbps + 100Mbps" with lines, \
        "SinglePath50_100.dat" using 1:2 title "Singlepath 50Mbps + 100Mbps" with lines