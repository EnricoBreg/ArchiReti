JAVA_SOURCES := $(wildcard *.java)
JAVA_TARGETS := $(patsubst %.java,%.class,$(JAVA_SOURCES))
C_SOURCES    := $(wildcard *.[ch])
C_TARGETS    := rps_client rps_server
TODAY        := $(shell date +%Y%m%d)
ARCHIVE      := rps_$(TODAY).zip

CFLAGS := -std=c17 -O2 -g3 -Wall -Werror -pedantic -D_FORTIFY_SOURCE=2 # -march=native -mtune=native
LDFLAGS := -fpie -Wl,-pie 

ifeq ($(USE_LIBUNISTRING),yes)
CFLAGS += -DUSE_LIBUNISTRING=1
LDFLAGS += -lunistring
endif

.PHONY: clean dist

all: $(C_TARGETS) $(JAVA_TARGETS)

# dependencies
rps_client.o: rps_client.c utils.h rxb.h
rxb.o: rxb.c rxb.h
server-concurrent-td.o: server-concurrent-td.c utils.h rxb.h

rps_client: rps_client.o utils.o rxb.o
rps_server: rps_server.o utils.o rxb.o

%.class: %.java
	javac $<

clean:
	@rm -rf $(C_TARGETS) $(JAVA_TARGETS) *.zip *.o *.dSYM *~

dist:
	@zip $(ARCHIVE) $(C_SOURCES) $(JAVA_SOURCES) "Testo Esercizio.txt" README.txt Makefile

