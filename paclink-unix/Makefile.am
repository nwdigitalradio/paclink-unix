# $Id$

ACLOCAL_AMFLAGS=-I m4

EXTRA_DIST=ascmail binmail.uue

clean-local:
	rm -f *~ .*~ *.orig *.rej core *.core TAGS .depend foo bar baz binmail *.test

bin_PROGRAMS=lzhuf_1 wl2ktelnet test_mime_encoding mime2wl

lzhuf_1_CPPFLAGS=-DLZHUF_1_MAIN
lzhuf_1_SOURCES=lzhuf_1.c buffer.c buffer.h

wl2ktelnet_SOURCES=wl2ktelnet.c progname.h strutil.c strutil.h timeout.c timeout.h wl2k.c wl2k.h mid.c mid.h buffer.c buffer.h lzhuf_1.c lzhuf_1.h llist.c llist.h

test_mime_encoding_SOURCES=test_mime_encoding.c mime_encoding.c mime_encoding.h buffer.c buffer.h

mime2wl_SOURCES=mime2wl.c buffer.c buffer.h strutil.c strutl.h mid.c mid.h

TESTS=test_lzhuf_1 test_mime_encoding

localstate_DATA=

install-data-hook:
	$(mkdir_p) $(DESTDIR)$(localstatedir)/wl2k/pending
