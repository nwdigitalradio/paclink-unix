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

#endif
