CC=cc

WARN=-Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -Wcast-align -Wwrite-strings -Wconversion -Waggregate-return -W #-Werror

OPTIMIZE=-O2 -fomit-frame-pointer -fstrength-reduce -finline-functions -funroll-loops

DEFS= 

CFLAGS= -g -O2 $(WARN) $(OPTIMIZE) $(DEFS)

LIBOBJS= 

LIBS=

OBJS=lzhuf_1.o

LZHBIN=lzhuf_1

all: $(LZHBIN) binmail

lzhuf_1: $(OBJS)
	$(CC) $(CFLAGS) -o $(LZHBIN) $(OBJS) $(LIBS)

binmail: binmail.uue
	uudecode binmail.uue

test: lzhuf_1 binmail
	./$(LZHBIN) e2 ascmail binmail.test
	cmp binmail binmail.test
	./$(LZHBIN) e1 ascmail binmail1.test
	./$(LZHBIN) d1 binmail1.test ascmail1.test
	cmp ascmail ascmail1.test
	echo regression tests passed

clean:
	rm -f *~ .*~ *.orig *.rej *.o core *.core TAGS .depend foo bar baz binmail *.test

realclean: clean
	rm -f $(BIN)

distclean: realclean
