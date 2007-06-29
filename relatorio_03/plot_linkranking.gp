set encoding iso_8859_1 # pt_BR RULES!!!
set term postscript eps enhanced color#monochrome 14
set output "plot_linkranking.eps"
set title  "Distribuição de Links por documento"
set ylabel "Número de links"
set xlabel "Ranking do documento (x1000)"
set border
set nokey
set xrange[1:]
set yrange[1:]
set logscale y
plot "/tmp/c.rev" u ($0/1000):($1) smooth bezier w line
# vim:fileencoding=latin1 syn=gnuplot:
