ARCH := $(shell arch)
ifeq ($(ARCH),x86_64)
CFLAGS=-g -Wall -Wc++-compat -static -std=c99 -O3 -Wno-unused-function
else
CFLAGS=-g -Wall -Wc++-compat -std=c99 -O3 -Wno-unused-function
endif

CC=gcc
PROG=sdust
LIBS=-lz

ifneq ($(asan),)
	CFLAGS+=-fsanitize=address
	LIBS+=-fsanitize=address
endif

.SUFFIXES:.c .o
.PHONY:all clean depend

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

all:$(PROG)

sdust:sdust.o horiz.o
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -fr *.o a.out $(PROG) *~ *.a *.dSYM

depend:
	(LC_ALL=C; export LC_ALL; makedepend -Y -- $(CFLAGS) -- *.c)

# DO NOT DELETE

sdust.o: kdq.h kalloc.h kvec.h sdust.h ketopt.h kseq.h
