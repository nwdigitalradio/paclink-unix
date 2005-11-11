dnl Process this file with autoconf to produce a configure script.

AC_INIT(TELPAC-UNIX, 1.0)
AM_INIT_AUTOMAKE

AC_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_PROG_CPP
dnl AM_PROG_LIBTOOL

OLDLIBS="$LIBS"
LIBS=""
AC_SEARCH_LIBS(socket, socket)
AC_SEARCH_LIBS(gethostbyname, nsl)
NETWORK_LIBS="$LIBS"
LIBS="$OLDLIBS"
AC_SUBST(NETWORK_LIBS)

AC_HEADER_STDC
AC_HEADER_TIME
AC_CHECK_HEADERS([unistd.h errno.h wchar.h sys/time.h malloc.h sys/types.h \
sys/socket.h arpa/inet.h netdb.h netinet/in.h netinet/tcp.h langinfo.h \
paths.h sys/ioctl.h libgen.h sys/param.h locale.h sys/select.h \
regex.h readpassphrase.h signal.h])
dnl AC_HAVE_INADDR_NONE

AC_CHECK_SIZEOF(char)
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(float)
AC_CHECK_SIZEOF(double)
AC_CHECK_SIZEOF(long double)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(__int64)
AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(wchar_t,,[$ac_includes_default
#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif])

# ------------------------------------------------------------
# Checks for typedefs and structures
# ------------------------------------------------------------

# ---- Solaris needs -DBSD_COMP to get FIONBIO defined ----
AC_MSG_CHECKING(is FIONBIO defined)
AC_EGREP_CPP(yes,
[#include <sys/ioctl.h>
#ifdef FIONBIO
 yes
#endif
],
    AC_MSG_RESULT(yes),
    [ AC_EGREP_CPP(yes,
[#define BSD_COMP
#include <sys/ioctl.h>
#ifdef FIONBIO
 yes
#endif
],
    AC_DEFINE(BSD_COMP, 1, [Define to 1 if you need BSD_COMP defined to get FIONBIO defined.]) AC_MSG_RESULT(need -DBSD_COMP),
    AC_MSG_RESULT(no))
    ])

TYPE_SOCKLEN_T

AC_C_BIGENDIAN
AC_C_CONST


AC_CHECK_FUNCS([vsnprintf gettimeofday \
nl_langinfo locale_charset setenv putenv \
getuid getpwuid getpwuid_r])
OLD_LIBS="$LIBS"
LIBS="$LIBS $NETWORK_LIBS"
AC_CHECK_FUNCS([inet_ntoa_r getipnodebyaddr getipnodebyname freehostent \
getaddrinfo getnameinfo inet_ntop inet_pton \
gethostname sethostent_r gethostent_r])
LIBS="$OLD_LIBS"
dnl AC_REPLACE_FUNCS([asprintf vasprintf atoll strtok_r readpassphrase \
dnl strlcpy strlcat basename])

dnl AC_HAVE_MALLOC_OPTIONS

if test "$ac_compiler_gnu" = "yes" ; then
	CPPFLAGS="$CPPFLAGS -Wall -Wstrict-prototypes -Wmissing-prototypes -Wno-long-long"
fi

AC_MSG_RESULT(done)

AC_OUTPUT(Makefile)
