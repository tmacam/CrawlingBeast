
all: view

LIST_OF_PDF_FIGS := plot_dps.pdf plot_bps.pdf

include images.mk

indexer.data: index_data_tmp2.log.gz
	zgrep '^# doc' $< | sed -e 's/^# //' > $@

plot_%.eps: plot_%.gp indexer.data
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


pdf: relatorio.pdf

view: relatorio.pdf
	gv relatorio.pdf&

clean: 
	rm -rf *.log *.aux *~ core *.pdf *.dvi *.bbl *.blg *.eps indexer.data


.PHONY: clean view
