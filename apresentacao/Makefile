# LaTeX flags
LATEXFLAGS = -interaction=nonstopmode

%.pdf : %.tex 
	pdflatex $(LATEXFLAGS) $< 
	pdflatex $(LATEXFLAGS) $< 
	pdflatex $(LATEXFLAGS) $< 

%-handout.tex: %.tex
	set -e && (cat $< | sed -e '/^\\documentclass/s/\[/[handout,/' > $@)
	sed -i '/^\\usetheme/d' $@
	
all: slides.pdf

.PHONY: clean view

view: all
	gv slides.pdf

handout: slides-handout.pdf

clean: 
	rm -rf *.log *.aux *~ core *.ps *.dvi *.bbl *.blg *.lot *.lof *.nav *.snm *.out *.toc slides.pdf slides-handout.pdf slides-handout.tex
