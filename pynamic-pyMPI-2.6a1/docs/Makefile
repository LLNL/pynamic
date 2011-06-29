.PHONY: docs
SOURCES= \
	how_it_works.xml \
	developers_overview.xml \
	developers_guidance.xml \
	extenders_guide.xml

POSTSCRIPT=$(SOURCES:.xml=.ps)

.PHONY: docs
docs: $(POSTSCRIPT)

%.ps: %.xml
	python xml2ps.py $<

.DELETE_ON_ERROR: $(POSTSCRIPT)
$(POSTSCRIPT): $(SOURCES) xml2ps.py
