%.eps : %.dia
	dia --export=$@ -t eps $<

%.pdf : %.eps
	epstopdf $<

#%.fig : %.dia
#	dia --export-to-format=fig $<

%.pdf : %.fig
	fig2dev -L pdftex -p dummy $< > $@

%.eps : %.fig
	fig2dev -L pstex $< > $@

#%.eps : %.dia
#	dia --export-to-format=eps $<

