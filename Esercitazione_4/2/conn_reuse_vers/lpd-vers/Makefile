JAVA_SOURCES := $(wildcard *.java)
JAVA_TARGETS := $(patsubst %.java,%.class,$(JAVA_SOURCES))
C_SOURCES    := $(wildcard *.[ch])
C_TARGETS    := vdv-client vdv-server
TODAY        := $(shell date +%Y%m%d)
ARCHIVE      := ultima_lezione_$(TODAY).zip

CFLAGS := -std=c17 -O2 -g3 -Wall -Werror -pedantic -D_FORTIFY_SOURCE=2 # -march=native -mtune=native
LDFLAGS := -fpie -Wl,-pie 

ifeq ($(USE_LIBUNISTRING),yes)
CFLAGS += -DUSE_LIBUNISTRING=1
LDFLAGS += -lunistring
endif

.PHONY: clean dist

all: $(C_TARGETS) $(JAVA_TARGETS)

# dependencies
vdv-client.o: vdv-client.c utils.h
server-concurrent-lpd.o: server-concurrent-lpd.c utils.h

vdv-client: vdv-client.o utils.o
vdv-server: vdv-server.o utils.o

# client-naive: client-naive.o
# server-concurrent-naive: server-iterative-naive.o

%.class: %.java
	javac $<

clean:
	@rm -rf $(C_TARGETS) $(JAVA_TARGETS) *.zip *.o *.dSYM *~

dist:
	@zip $(ARCHIVE) $(C_SOURCES) $(JAVA_SOURCES) "Compito di Reti di Calcolatori del 9_7_2019 - errata corrige.pdf" Makefile

