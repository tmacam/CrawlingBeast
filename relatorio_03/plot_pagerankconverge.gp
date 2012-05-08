set encoding iso_8859_1 # pt_BR RULES!!!
set term postscript eps enhanced color#monochrome 14
set output "plot_pagerankconverge.eps"
set title  "Converg�ncia no c�lculo do PageRank"
set ylabel "Valor da norma L1"
set xlabel "itera��o"
set border
set nokey
set logscale y
plot "pr.log" 
# vim:fileencoding=latin1 syn=gnuplot:
