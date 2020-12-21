SOURCES := $(wildcard *.[ch])
TODAY   := $(shell date +%Y%m%d)
ARCHIVE := rstrcmp_unix_$(TODAY).zip
TARGETS := client-naive server-concurrent-naive client-lpd server-concurrent-lpd

CFLAGS := -std=c17 -O2 -g3 -Wall -Werror -pedantic -D_FORTIFY_SOURCE=2
LDFLAGS := -fpie -Wl,-pie

.PHONY: clean dist

all: $(TARGETS)

# dependencies
client-lpd.o: client-lpd.c utils.h
server-concurrent-lpd.o: server-concurrent-lpd.c utils.h
client-lpd: client-lpd.o utils.o
server-concurrent-lpd: server-concurrent-lpd.o utils.o
# implicitly inferred from default
# client-naive.o: client-naive.c
# server-concurrent-naive.o: server-concurrent-naive.c
# client-naive: client-naive.o
# server-concurrent-naive: server-concurrent-naive.o

clean:
	@rm -rf $(TARGETS) *.zip *.o *.dSYM *~

dist:
	@zip $(ARCHIVE) $(SOURCES) Makefile

