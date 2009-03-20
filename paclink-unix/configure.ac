dnl $Id$
dnl Process this file with autoconf to produce a configure script.

dnl Increment the package version for each release.
AC_INIT(PACLINK-UNIX, 0.2)
AC_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE

AC_LANG(C)

AC_USE_SYSTEM_EXTENSIONS

dnl =======================================================================
dnl Checks for programs.
dnl =======================================================================

AC_PROG_CC
AC_PROG_RANLIB
AM_PROG_MKDIR_P
AM_PROG_CC_C_O

AC_PATH_PROG([SENDMAIL], [sendmail], , [$PATH:/usr/libexec:/usr/sbin:/usr/lib:/usr/lib/sendmail:/usr/pkg/libexec/sendmail])
if test -z "$SENDMAIL"; then
  AC_MSG_FAILURE([sendmail not found])
fi
AC_DEFINE_UNQUOTED([SENDMAIL], ["$SENDMAIL"], [Define to full path of sendmail executable])

dnl =======================================================================
dnl Checks for libraries.
dnl =======================================================================

AC_SEARCH_LIBS(socket, socket)
AC_SEARCH_LIBS(gethostbyname, nsl)
AC_SEARCH_LIBS(herror, resolv)
AC_SEARCH_LIBS(ax25_aton, ax25)

AC_CHECK_FILE(/usr/pkg/include/db4/db.h,
[
		CPPFLAGS="$CPPFLAGS -I/usr/pkg/include/db4"
		LDFLAGS="-L/usr/pkg/lib -Wl,-rpath,/usr/pkg/lib $LDFLAGS"
])
AC_CHECK_FILE(/usr/pkg/include/db3/db.h,
[
		CPPFLAGS="$CPPFLAGS -I/usr/pkg/include/db3"
		LDFLAGS="-L/usr/pkg/lib -Wl,-rpath,/usr/pkg/lib $LDFLAGS"
])
AX_PATH_BDB([3], [
		 LIBS="$BDB_LIBS $LIBS"
		 LDFLAGS="$BDB_LDFLAGS $LDFLAGS"
		 CPPFLAGS="$CPPFLAGS $BDB_CPPFLAGS"
], AC_MSG_FAILURE([Berkeley DB not found])
)

PKG_CHECK_MODULES(GMIME, gmime-2.0 >= 2.1.19, , AC_MSG_FAILURE([gmime-2.1.19 or newer not found]))
CFLAGS="$CFLAGS $GMIME_CFLAGS"
CPPFLAGS="$CPPFLAGS $GMIME_CPPFLAGS"
LDFLAGS="$LDFLAGS $GMIME_LDFLAGS"
LIBS="$LIBS $GMIME_LIBS"

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.10.3, , AC_MSG_FAILURE([glib-2.10.13 or newer not found]))
CFLAGS="$CFLAGS $GLIB_CFLAGS"
CPPFLAGS="$CPPFLAGS $GLIB_CPPFLAGS"
LDFLAGS="$LDFLAGS $GLIB_LDFLAGS"
LIBS="$LIBS $GLIB_LIBS"

dnl =======================================================================
dnl Checks for header files.
dnl =======================================================================

AC_CHECK_HEADERS([sys/errno.h sys/time.h sys/stat.h sys/types.h sys/socket.h netinet/in.h arpa/inet.h errno.h stdio.h string.h stdlib.h string.h paths.h netdb.h unistd.h signal.h strings.h stdarg.h assert.h fcntl.h ctype.h time.h netax25/ax25.h netax25/axlib.h])

AC_HEADER_STDC
AC_HEADER_TIME
AC_HEADER_DIRENT
AC_HEADER_STAT
AC_HEADER_ASSERT

dnl =======================================================================
dnl Checks for types.
dnl =======================================================================

dnl =======================================================================
dnl Checks for structures.
dnl =======================================================================

AC_CHECK_MEMBERS([struct sockaddr_in.sin_len], , ,[
#include <sys/types.h>
#include <netinet/in.h>
])

dnl =======================================================================
dnl Checks for compiler characteristics.
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
dnl Checks for library functions.
dnl =======================================================================

AC_CHECK_FUNCS([vsnprintf])
AC_REPLACE_FUNCS([getprogname asprintf vasprintf strlcpy])

dnl =======================================================================
dnl Checks for library symbols.
dnl =======================================================================

AC_MSG_CHECKING([for __progname])
AC_LINK_IFELSE(AC_LANG_PROGRAM([[#include <stdio.h>]],
                               [[extern char *__progname; puts(__progname);]]),
ax_cv___progname=yes,
ax_cv___progname=no)
AC_MSG_RESULT($ax_cv___progname)
if test x$ax_cv___progname = xyes; then
  AC_DEFINE(HAVE___PROGNAME, 1, [Define to 1 if you have the global variable __progname])
fi

AC_MSG_CHECKING([for __Argv])
AC_LINK_IFELSE(AC_LANG_PROGRAM([[#include <stdio.h>]],
                               [[extern char **__Argv; puts(__Argv@<:@0@:>@);]]),
ax_cv___Argv=yes,
ax_cv___Argv=no)
AC_MSG_RESULT($ax_cv___Argv)
if test x$ax_cv___Argv = xyes; then
  AC_DEFINE(HAVE___ARGV, 1, [Define to 1 if you have the global variable __Argv])
fi

AC_MSG_CHECKING([for ___Argv])
AC_LINK_IFELSE(AC_LANG_PROGRAM([[#include <stdio.h>]],
                               [[extern char **___Argv; puts(___Argv@<:@0@:>@);]]),
ax_cv____Argv=yes,
ax_cv____Argv=no)
AC_MSG_RESULT($ax_cv____Argv)
if test x$ax_cv____Argv = xyes; then
  AC_DEFINE(HAVE____ARGV, 1, [Define to 1 if you have the global variable ___Argv])
fi

dnl =======================================================================
dnl Checks for system services.
dnl =======================================================================

dnl =======================================================================
dnl Output our results.
dnl =======================================================================

AC_MSG_RESULT(done)

AC_CONFIG_FILES([Makefile lib/Makefile])
AC_OUTPUT
