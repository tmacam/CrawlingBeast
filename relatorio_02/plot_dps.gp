set encoding iso_8859_1 # pt_BR RULES!!!
set term postscript eps enhanced color#monochrome 14
set title  "Desempenho do Indexador"
set ylabel "Total de documentos indexados (x1000)"
set xlabel "Tempo (segundos)"
set output "plot_dps.eps"
set border
set nokey
plot "indexer.data" u 10:($2/1000) w lines
# vim:fileencoding=latin1 syn=gnuplot:
