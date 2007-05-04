
LIST_OF_PDF_FIGS := #figures/percurso.pdf


%.aux : %.tex
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
	rm -rf *.log *.aux *~ core *.pdf *.dvi *.bbl *.blg


.PHONY: clean view
