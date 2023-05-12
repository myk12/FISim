set terminal pdf
set output "multipath-test.pdf"
set title "Multipath Test"
set xlabel "Time (s)"
set ylabel "Throughput (Mbps)"
set grid

plot "MultiPath_50_50.dat" using 1:2 title "Multipath 50Mbps + 50Mbps" with lines, \
    "MultiPath_100_50.dat" using 1:2 title "Multipath 100Mbps + 50Mbps" with lines, \
    "MultiPath_100_100.dat" using 1:2 title "Multipath 100Mbps + 100Mbps" with lines

plot    "SinglePath_100_50.dat" using 1:2 title "Singlepath 100Mbps + 50Mbps" with lines, \
        "SinglePath_50_100.dat" using 1:2 title "Singlepath 50Mbps + 100Mbps" with lines
        #"SinglePath_100_100.dat" using 1:2 title "Singlepath 100Mbps + 100Mbps" with lines, \