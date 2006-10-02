/*
 * vasprintf(3)
 * 20020809 entropy@entropy.homeip.net
 * public domain.  no warranty.  use at your own risk.  have a nice day.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef __RCSID
__RCSID("$Id$");
#endif

#if HAVE_STDIO_H
#include <stdio.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_STDARG_H
#include <stdarg.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_PATHS_H
#include <paths.h>
#endif

#include "compat.h"

#ifndef _PATH_DEVNULL
#define _PATH_DEVNULL "/dev/null"
#endif

#define CHUNKSIZE 512
int
vasprintf(char **ret, const char *fmt, va_list ap)
{
#if HAVE_VSNPRINTF
	int chunks;
	size_t buflen;
	char *buf;
	int len;

	chunks = ((strlen(fmt) + 1) / CHUNKSIZE) + 1;
	buflen = chunks * CHUNKSIZE;
	for (;;) {
		if ((buf = malloc(buflen)) == NULL) {
			*ret = NULL;
			return -1;
		}
		len = vsnprintf(buf, buflen, fmt, ap);
		if ((len >= 0) && ((unsigned) len < (buflen - 1))) {
			break;
		}
		free(buf);
		buflen = (++chunks) * CHUNKSIZE;
		/* 
		 * len >= 0 are required for vsnprintf implementation that 
		 * return -1 of buffer insufficient
		 */
		if ((len >= 0) && ((unsigned) len >= buflen)) {
			buflen = len + 1;
		}
	}
	*ret = buf;
	return len;
#else /* HAVE_VSNPRINTF */
#ifdef _REENTRANT
	FILE *fp;
#else /* !_REENTRANT */
	static FILE *fp = NULL;
#endif /* !_REENTRANT */
	int len;
	char *buf;

	*ret = NULL;

#ifdef _REENTRANT

# ifdef WIN32
#  error Win32 do not have /dev/null, should use vsnprintf version
# endif

	if ((fp = fopen(_PATH_DEVNULL, "w")) == NULL)
		return -1;
#else /* !_REENTRANT */
	if ((fp == NULL) && ((fp = fopen(_PATH_DEVNULL, "w")) == NULL))
		return -1;
#endif /* !_REENTRANT */

	len = vfprintf(fp, fmt, ap);

#ifdef _REENTRANT
	if (fclose(fp) != 0)
		return -1;
#endif /* _REENTRANT */

	if (len < 0)
		return len;
	if ((buf = malloc(len + 1)) == NULL)
		return -1;
	if (vsprintf(buf, fmt, ap) != len)
		return -1;
	*ret = buf;
	return len;
#endif /* HAVE_VSNPRINTF */
}
