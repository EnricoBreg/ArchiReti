# Get list of all *.java source code files, create corresponding *.class target list.
SOURCES := $(wildcard *.java)
OBJECTS := $(notdir ${SOURCES})
TARGETS := $(patsubst %.java,%.class,$(OBJECTS))

TODAY   := $(shell date +%Y%m%d)
ARCHIVE := remote_head_java_$(TODAY).zip


.PHONY: clean dist

all: ${TARGETS}

%.class: %.java
	@javac $<

clean:
	@rm -f *.class *.zip *~

dist:
	@zip $(ARCHIVE) $(SOURCES) *.rb *.txt Makefile

