set encoding iso_8859_1 # pt_BR RULES!!!
set term postscript eps enhanced color#monochrome 14
set title  "Desempenho do crawler"
set ylabel "# de páginas"
set xlabel "Tempo (minutos)"
set output "20070504-min.eps"
set border
set yrange [1:10000]
set logscale y
# "20070504-min.data" u 1:2 t "Descobertas" w lines , 
plot	\
	"20070504-min.data" u 1:3 t "Visitadas" w lines,\
	 "20070504-min.data" u 1:4 w lines t "Recuperadas" 

# vim:fileencoding=latin1 syn=gnuplot:
