# $Id$

ACLOCAL_AMFLAGS=-I m4

SUBDIRS=lib
LDADD=lib/libcompat.a

EXTRA_DIST=ascmail binmail.uue

clean-local:
	rm -f *~ .*~ *.orig *.rej core *.core TAGS .depend foo bar baz binmail *.test

bin_PROGRAMS=lzhuf_1 wl2ktelnet
libexec_PROGRAMS=mail.wl2k
noinst_PROGRAMS=mime2wl wl2mime

lzhuf_1_CPPFLAGS=-DLZHUF_1_MAIN
lzhuf_1_SOURCES=lzhuf_1.c buffer.c buffer.h compat.h

mime2wl_CPPFLAGS=-DMIME2WL_MAIN
mime2wl_SOURCES=mime2wl.c mime2wl.h buffer.c buffer.h mid.c mid.h compat.h

wl2mime_CPPFLAGS=-DWL2MIME_MAIN
wl2mime_SOURCES=wl2mime.c wl2mime.h buffer.c buffer.h strutil.c strutil.h compat.h

wl2ktelnet_SOURCES=wl2ktelnet.c progname.h strutil.c strutil.h timeout.c timeout.h wl2k.c wl2k.h mid.c mid.h buffer.c buffer.h lzhuf_1.c lzhuf_1.h wl2mime.c wl2mime.h compat.h

mail_wl2k_SOURCES=mail.wl2k.c buffer.c buffer.h mime2wl.c mime2wl.h mid.c mid.h compat.h

TESTS=test_lzhuf_1

localstate_DATA=

install-data-hook:
	$(mkdir_p) $(DESTDIR)$(localstatedir)/wl2k/pending
