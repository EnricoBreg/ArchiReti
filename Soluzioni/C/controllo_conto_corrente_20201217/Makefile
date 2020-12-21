JAVA_SOURCES := $(wildcard *.java)
JAVA_TARGETS := $(patsubst %.java,%.class,$(JAVA_SOURCES))
C_SOURCES    := $(wildcard *.[ch])
C_TARGETS    := client-naive server-concurrent-naive client-simple-csexp server-concurrent-simple-csexp
TODAY        := $(shell date +%Y%m%d)
ARCHIVE      := controllo_conto_corrente_$(TODAY).zip

CFLAGS := -std=c17 -O2 -g3 -Wall -Werror -pedantic -D_FORTIFY_SOURCE=2 # -march=native -mtune=native
LDFLAGS := -fpie -Wl,-pie 

ifeq ($(USE_LIBUNISTRING),yes)
CFLAGS += -DUSE_LIBUNISTRING=1
LDFLAGS += -lunistring
endif

.PHONY: clean dist

all: $(C_TARGETS) $(JAVA_TARGETS)

# dependencies
client-simple-csexp: client-simple-csexp.o simple_csexp.o rxb.o utils.o
server-concurrent-simple-csexp: server-concurrent-simple-csexp.o simple_csexp.o rxb.o utils.o

client-simple-csexp.o: client-simple-csexp.c simple_csexp.h rxb.h utils.h
server-concurrent-simple-csexp.o: server-concurrent-simple-csexp.c simple_csexp.h rxb.h utils.h

# client-naive: client-naive.o
# server-concurrent-naive: server-concurrent-naive.o

%.class: %.java
	javac $<

clean:
	@rm -rf $(C_TARGETS) $(JAVA_TARGETS) *.zip *.o *.dSYM *~

dist:
	@zip $(ARCHIVE) $(C_SOURCES) $(JAVA_SOURCES) "Testo Esercizio.txt" README.txt Makefile

