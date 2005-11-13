# $Id$

EXTRA_DIST=ascmail binmail.uue

clean-local:
	rm -f *~ .*~ *.orig *.rej core *.core TAGS .depend foo bar baz binmail *.test

all:

bin_PROGRAMS=lzhuf_1 wl2ktelnet

lzhuf_1_SOURCES=lzhuf_1.c

wl2ktelnet_SOURCES=wl2ktelnet.c strupper.c progname.h strupper.h timeout.c timeout.h wl2k.c wl2k.h

TESTS=test_lzhuf_1
