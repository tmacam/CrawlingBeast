
all: view

LIST_OF_PDF_FIGS := plot_linkranking.pdf plot_pagerankconverge.pdf

include images.mk


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
	rm -rf *.log *.aux *~ core *.pdf *.dvi *.bbl *.blg


.PHONY: clean view
