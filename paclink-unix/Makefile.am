## Process this file with automake to produce Makefile.in

# $Id$

ACLOCAL_AMFLAGS=-I m4

SUBDIRS=lib
LDADD=lib/libcompat.a

EXTRA_DIST=ascmail binmail.uue test_lzhuf_1 lzhuf_1.zip.uue

clean-local:
	rm -f *~ .*~ *.orig *.rej core *.core TAGS .depend foo bar baz binmail *.test

bin_PROGRAMS=lzhuf_1 wl2ktelnet wl2kserial

if HAVE_AX25
  bin_PROGRAMS+=wl2kax25 wl2kax25d
endif

libexec_PROGRAMS=mail.wl2k
noinst_PROGRAMS=mime2wl wl2mime

dist_man_MANS=doc/wl2kax25.1 doc/wl2ktelnet.1 doc/wl2k.conf.5 doc/wl2kserial.1

WL2K_STATEDIR=$(localstatedir)/wl2k
WL2K_OUTBOX=$(WL2K_STATEDIR)/outbox
WL2K_MID_DB=$(WL2K_STATEDIR)/mid.db
WL2K_CPPFLAGS=-DWL2K_OUTBOX=\"$(WL2K_OUTBOX)\" -DWL2K_MID_DB=\"$(WL2K_MID_DB)\" -DSYSCONFDIR=\"$(sysconfdir)\"
if ENABLE_POSTFIX
  WL2K_CPPFLAGS+= -DSENDMAIL_FLAGS=\"-bm\"
else
  WL2K_CPPFLAGS+= -DSENDMAIL_FLAGS=\"-ba\"
endif
AM_CPPFLAGS=$(WL2K_CPPFLAGS)

lzhuf_1_CPPFLAGS=-DLZHUF_1_MAIN $(WL2K_CPPFLAGS)
lzhuf_1_SOURCES=lzhuf_1.c buffer.c buffer.h compat.h

mime2wl_CPPFLAGS=-DMIME2WL_MAIN $(WL2K_CPPFLAGS)
mime2wl_SOURCES=mime2wl.c mime2wl.h buffer.c buffer.h mid.c mid.h compat.h conf.c conf.h

wl2mime_CPPFLAGS=-DWL2MIME_MAIN $(WL2K_CPPFLAGS)
wl2mime_SOURCES=wl2mime.c wl2mime.h buffer.c buffer.h strutil.c strutil.h compat.h

wl2ktelnet_SOURCES=wl2ktelnet.c strutil.c strutil.h timeout.c timeout.h wl2k.c wl2k.h mid.c mid.h buffer.c buffer.h lzhuf_1.c lzhuf_1.h wl2mime.c wl2mime.h compat.h conf.c conf.h

wl2kax25_SOURCES=wl2kax25.c strutil.c strutil.h timeout.c timeout.h wl2k.c wl2k.h mid.c mid.h buffer.c buffer.h lzhuf_1.c lzhuf_1.h wl2mime.c wl2mime.h compat.h conf.c conf.h

wl2kserial_SOURCES=wl2kserial.c strutil.c strutil.h timeout.c timeout.h wl2k.c wl2k.h mid.c mid.h buffer.c buffer.h lzhuf_1.c lzhuf_1.h wl2mime.c wl2mime.h compat.h conf.c conf.h

wl2kax25d_CPPFLAGS=-DWL2KAX25_DAEMON $(WL2K_CPPFLAGS)
wl2kax25d_SOURCES=wl2kax25d.c strutil.c strutil.h timeout.c timeout.h wl2k.c wl2k.h mid.c mid.h buffer.c buffer.h lzhuf_1.c lzhuf_1.h wl2mime.c wl2mime.h compat.h conf.c conf.h

mail_wl2k_SOURCES=mail.wl2k.c buffer.c buffer.h mime2wl.c mime2wl.h mid.c mid.h strutil.c strutil.h timeout.c timeout.h compat.h conf.c conf.h

TESTS=test_lzhuf_1

localstate_DATA=

dist_sysconf_DATA=wl2k.conf.dist

install-exec-hook:
	[ -f $(DESTDIR)$(sysconfdir)/wl2k.conf ] || cp $(DESTDIR)$(sysconfdir)/wl2k.conf.dist $(DESTDIR)$(sysconfdir)/wl2k.conf

install-data-hook:
	$(INSTALL) -d -m 0775 -o $(OWNER) -g $(GROUP) $(DESTDIR)$(WL2K_STATEDIR)
	$(INSTALL) -d -m 0775 -o $(OWNER) -g $(GROUP) $(DESTDIR)$(WL2K_OUTBOX)

