dnl $Id$
dnl Process this file with autoconf to produce a configure script.

AC_INIT(TELPAC-UNIX, 1.0)
AM_INIT_AUTOMAKE

AC_CONFIG_HEADER(config.h)

AC_PROG_CC

AC_SEARCH_LIBS(socket, socket)
AC_SEARCH_LIBS(gethostbyname, nsl)

AC_HEADER_STDC
AC_HEADER_TIME

AC_CHECK_HEADERS([stdlib.h string.h sys/socket.h netinet/in.h arpa/inet.h netdb.h unistd.h fcntl.h bstring.h])

AC_C_CONST

if test "$ac_compiler_gnu" = "yes" ; then
	CPPFLAGS="$CPPFLAGS -Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -Wcast-align -Wwrite-strings -Wconversion -Waggregate-return -W"
fi

AC_MSG_RESULT(done)

AC_OUTPUT(Makefile)
