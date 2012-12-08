PAPER_NAME=cs316_paper

default:
	pdflatex $(PAPER_NAME)
	bibtex $(PAPER_NAME)
	pdflatex $(PAPER_NAME)
	pdflatex $(PAPER_NAME)

clean:
	rm $(PAPER_NAME).blg $(PAPER_NAME).bbl $(PAPER_NAME).aux $(PAPER_NAME).log $(PAPER_NAME).pdf
tidy:
	rm $(PAPER_NAME).blg $(PAPER_NAME).bbl $(PAPER_NAME).aux $(PAPER_NAME).log
