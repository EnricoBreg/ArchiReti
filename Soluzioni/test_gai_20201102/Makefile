SOURCES := $(wildcard *.c) $(wildcard *.h)
TARGETS := test_gai
TODAY   := $(shell date +%Y%m%d)
ARCHIVE := test_gai_$(TODAY).zip

CFLAGS := -std=c17 -O2 -g3 -Wall -Werror -pedantic -D_FORTIFY_SOURCE=2 
LDFLAGS := -fpie -Wl,-pie

.PHONY: clean dist

all: $(TARGETS)

test_gai: test_gai.o dump.o
	$(CC) $(LDFLAGS) -o $@ $^

dump.o: dump.c
	$(CC) $(CFLAGS) -c $<

test_gai.o: test_gai.c dump.h
	$(CC) $(CFLAGS) -c $<

clean:
	@rm -rf $(TARGETS) *.zip *.dSYM *.o *~

dist:
	@zip $(ARCHIVE) $(SOURCES) Makefile
