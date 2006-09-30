/* $Id$ */

/* system dependent */

#ifndef PROGNAME_H
#define PROGNAME_H

#if HAVE_GETPROGNAME
#define progname getprogname()
#else
#if HAVE___PROGNAME
extern char *__progname;
#define progname __progname
#else
#if HAVE___ARGV
extern char **__Argv;
#define progname __Argv[0]
#else
#if HAVE____ARGV
extern char **___Argv;
#define progname ___Argv[0]
#else
#define progname "(unknown program)"
#endif
#endif
#endif
#endif
