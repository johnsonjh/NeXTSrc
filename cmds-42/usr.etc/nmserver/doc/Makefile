design.dvi: design.tex design.bbl
	pslatex design.tex

design.bbl: design.bib
	bibtex  design

print:
	dvi-ps design.dvi | lpr -P coral

spell:
	detex design.tex | spell
