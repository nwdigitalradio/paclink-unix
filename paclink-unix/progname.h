/* $Id$ */

/* system dependent */

#ifndef PROGNAME_H
#define PROGNAME_H

#if HAVE___ARGV
extern char **__Argv;
#define progname __Argv[0]
#else
#if HAVE___PROGNAME
extern char *__progname;
#define progname __progname
#else
#define progname "(unknown program)"
#endif
#endif

#endif
