dnl $Id$
dnl Process this file with autoconf to produce a configure script.

dnl Increment the package version for each release.
AC_INIT(TELPAC-UNIX, 1.0)
AC_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE

dnl =======================================================================
dnl Checks for programs.
dnl =======================================================================

AC_PROG_CC

CPPFLAGS="$CPPFLAGS -I/usr/pkg/include/db4 -I/usr/pkg/include/db3"
LDFLAGS="-L/usr/pkg/lib -Wl,-rpath,/usr/pkg/lib"

dnl =======================================================================
dnl Checks for libraries.
dnl =======================================================================

AC_SEARCH_LIBS(socket, socket)
AC_SEARCH_LIBS(gethostbyname, nsl)
AX_PATH_BDB([3], [
		 LIBS="$BDB_LIBS $LIBS"
		 LDFLAGS="$BDB_LDFLAGS $LDFLAGS"
		 CPPFLAGS="$CPPFLAGS $BDB_CPPFLAGS"
])

dnl =======================================================================
dnl Checks for header files.
dnl =======================================================================

dnl checks stdlib.h stdarg.h string.h float.h defines STDC_HEADERS
AC_HEADER_STDC

AC_CHECK_HEADERS([sys/socket.h netinet/in.h arpa/inet.h netdb.h unistd.h signal.h strings.h])

dnl =======================================================================
dnl Checks for typedefs, structures, and compiler characteristics.
dnl =======================================================================

AC_C_CONST

if test "$ac_compiler_gnu" = "yes" ; then
	CPPFLAGS="$CPPFLAGS -Wall -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -Wcast-align -Wwrite-strings -Wconversion -Waggregate-return -W"
fi

AC_MSG_CHECKING(whether compiler has __attribute__((__unused__)))
AC_TRY_COMPILE(, [int x __attribute__((__unused__));],
compiler_has_attribute_unused=yes,
compiler_has_attribute_unused=no)
AC_MSG_RESULT($compiler_has_attribute_unused)
if test x"$compiler_has_attribute_unused" = xyes; then
   AC_DEFINE(ATTRIBUTE_UNUSED, __attribute__((__unused__)), [Define to attribute for unused variables])
fi

dnl =======================================================================
dnl Output our results.
dnl =======================================================================

AC_MSG_RESULT(done)

AC_OUTPUT(Makefile)
