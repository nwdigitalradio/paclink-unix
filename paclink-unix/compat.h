/* $Id$ */

#ifndef COMPAT_H
#define COMPAT_H

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#if !HAVE_ASPRINTF
int asprintf(char **ret, const char *fmt, ...);
#endif

#if !HAVE_VASPRINTF
int vasprintf(char **ret, const char *fmt, va_list ap);
#endif

#if !HAVE_GETPROGNAME
const char *getprogname(void);
#endif

#if !HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t size);
#endif

#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

#endif
