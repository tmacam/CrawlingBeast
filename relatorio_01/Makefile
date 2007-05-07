
LIST_OF_PDF_FIGS := 20070504-min.pdf 20070506-min.pdf

include images.mk

%-min.data:ri-stats-%.gz
	zcat $< | awk -f minutestats.awk > $@

%-min.eps: %-min.gp %-min.data
	gnuplot $<
	

%.aux : %.tex $(LIST_OF_PDF_FIGS)
	pdflatex  $<
	-rm -rf `basename $< .tex`.pdf

%.bbl : %.bib %.aux
	bibtex `basename $< .bib`

%.pdf : %.tex %.bbl $(LIST_OF_PDF_FIGS)
	pdflatex $< &&\
	pdflatex $< &&\
	pdflatex $<





all: view

pdf: relatorio.pdf

view: relatorio.pdf
	gv relatorio.pdf&

clean: 
	rm -rf *.log *.aux *~ core *.pdf *.dvi *.bbl *.blg *.eps


.PHONY: clean view
