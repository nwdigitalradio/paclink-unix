/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2; c-brace-offset: -2; c-argdecl-indent: 2 -*- */

/*  paclink-unix client for the Winlink 2000 ham radio email system.
 *
 *  Copyright 2006 Nicholas S. Castellano <n2qz@arrl.net> and others,
 *                 See the file AUTHORS for a list.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef __RCSID
__RCSID("$Id$");
#endif

#if HAVE_STDIO_H
# include <stdio.h>
#endif
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if HAVE_ERRNO_H
# include <errno.h>
#endif
#if HAVE_STRING_H
# include <string.h>
#endif

#include "compat.h"
#include "printlog.h"


/*
 * Print an error message to either stderr or syslog.
 *
 * Messages with priority LOG_DEBUG_VERBOSE will only be printed
 *  if the global verbose flag is set TRUE.
 */
void
print_log(int priority, const char *fmt, ...)
{
  va_list args;
  char *sp;

  if(priority == LOG_DEBUG_VERBOSE) {
    if(!gverbose_flag) {
      return;
    } else {
      priority = LOG_DEBUG;
    }
  }
  va_start(args, fmt);

  if (vasprintf(&sp, fmt, args) < 0) {
    syslog(LOG_ERR, "vasprintf() - %s", strerror(errno));
    va_end(args);
    return;
  }

#ifdef  WL2KAX25_DAEMON
  syslog(priority, "%s", sp);
#else
  fprintf(stderr, "%s: %s\n",  getprogname(), sp);
#endif /*   WL2KAX25_DAEMON */

  va_end(args);
}
