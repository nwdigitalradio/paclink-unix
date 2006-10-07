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

#include <gmime/gmime.h>
#include "buffer.h"

struct buffer *
wl2mime(struct buffer *ibuf)
{
  struct buffer *hbuf;
  struct buffer *bbuf;
  struct buffer *obuf;
  char *line;
  int c;

  while ((line = buffer_getline(ibuf, '\n')) != NULL) {
    if ((line[0] == '\r') || (line[0] == '\n')) {
      break;
    }
    buffer_addstring(hbuf, line);
    free(line);
  }

  while ((c = buffer_iterchar(ibuf)) != EOF) {
    buffer_addchar(bbuf, c);
  }

  if ((obuf = buffer_new()) == NULL) {
    return NULL;
  }
  return obuf;
}

int
main(int argc, char *argv[])
{
  struct buffer *ibuf;
  struct buffer *obuf;
  int c;

  g_mime_init(0);

  if (argc != 2) {
    fprintf(stderr, "Usage: %s messagefile\n", getprogname());
    exit(EXIT_FAILURE);
  }
  if ((ibuf = buffer_readfile(argv[1])) == NULL) {
    perror(getprogname());
    exit(EXIT_FAILURE);
  }
  obuf = wl2mime(ibuf);
  buffer_rewind(obuf);
  while ((c = buffer_iterchar(obuf)) != EOF) {
    if (putchar(c) == EOF) {
      exit(EXIT_FAILURE);
    }
  }

  g_mime_shutdown();

  exit(0);
  return 1;
}
