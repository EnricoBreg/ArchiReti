SOURCES := $(wildcard *.[ch])
TODAY   := $(shell date +%Y%m%d)
ARCHIVE := rstrlen_unix_$(TODAY).zip
TARGETS := rstrlen_client rstrlen_server

CFLAGS := -std=c17 -O2 -g3 -Wall -Werror -pedantic -D_FORTIFY_SOURCE=2
LDFLAGS := -fpie -Wl,-pie 

ifeq ($(USE_LIBUNISTRING),yes)
CFLAGS += -DUSE_LIBUNISTRING=1
LDFLAGS += -lunistring
endif

.PHONY: clean dist

all: $(TARGETS)

# dependencies
rstrlen_client.o: rstrlen_client.c utils.h rxb.h
rxb.o: rxb.c rxb.h
rstrlen_server.o: rstrlen_server.c utils.h rxb.h

rstrlen_client: rstrlen_client.o utils.o rxb.o
rstrlen_server: rstrlen_server.o utils.o rxb.o

# client-naive: client-naive.o
# server-iterative-naive: server-iterative-naive.o

clean:
	@rm -rf $(TARGETS) *.zip *.o *.dSYM *~

dist:
	@zip $(ARCHIVE) $(SOURCES) Makefile

