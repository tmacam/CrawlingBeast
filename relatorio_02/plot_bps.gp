set encoding iso_8859_1 # pt_BR RULES!!!
set term postscript eps enhanced color#monochrome 14
set title  "Desempenho do Indexador"
set ylabel "KB indexados"
set xlabel "Tempo (segundos)"
set output "plot_bps.eps"
set border
set nokey
plot "indexer.data" u 10:($8/1024) w lines
# vim:fileencoding=latin1 syn=gnuplot:
