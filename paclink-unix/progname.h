/* $Id$ */

/* system dependent */

#ifndef PROGNAME_H
#define PROGNAME_H

#if defined(__osf__) || defined(__solaris__)
extern char **__Argv;
#define progname __Argv[0]
#else
#if defined(__NetBSD__) || defined(__linux__)
extern char *__progname;
#define progname __progname
#endif
#endif

#endif
