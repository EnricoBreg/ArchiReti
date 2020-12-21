SOURCES := $(wildcard *.[ch])
TODAY   := $(shell date +%Y%m%d)
ARCHIVE := rcp_naive_unix_$(TODAY).zip
TARGETS := rstrlen_client rstrlen_server

CFLAGS := -std=gnu17 -O2 -g3 -Wall -Werror -pedantic -D_FORTIFY_SOURCE=2 
LDFLAGS := -fpie -Wl,-pie

.PHONY: clean dist

all: $(TARGETS)

# dependencies
rstrlen_client.o: rstrlen_client.c utils.h
rstrlen_server.o: rstrlen_server.c utils.h
rstrlen_client: rstrlen_client.o utils.o
rstrlen_server: rstrlen_server.o utils.o

clean:
	@rm -rf $(TARGETS) *.zip *.o *.dSYM *~

dist:
	@zip $(ARCHIVE) $(SOURCES) Makefile
