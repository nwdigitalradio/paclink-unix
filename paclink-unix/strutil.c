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

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

#include <ctype.h>
#include <string.h>
#include "strutil.h"

char *
strupper(unsigned char *s)
{
  unsigned char *cp;

  for (cp = s; *cp; cp++) {
    if (islower(*cp)) {
      *cp = toupper(*cp);
    }
  }
  return s;
}

int
strbegins(const char *s, const char *prefix)
{
  size_t plen;

  plen = strlen(prefix);
  if (strncmp(s, prefix, plen) == 0) {
    return 1;
  }
  return 0;
}

int
strcasebegins(const char *s, const char *prefix)
{
  size_t plen;

  plen = strlen(prefix);
  if (strncasecmp(s, prefix, plen) == 0) {
    return 1;
  }
  return 0;
}

char *
strzapcc(char *s)
{
  char *p;

  if ((p = strchr(s, '\r')) != NULL) {
    *p = '\0';
  }
  if ((p = strchr(s, '\n')) != NULL) {
    *p = '\0';
  }
  return s;
}
