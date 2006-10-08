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
#include <sys/types.h>
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

#include "compat.h"
#include "buffer.h"
#include "mime2wl.h"

int
main(int argc, char *argv[])
{
  struct buffer *buf;
  int c;

  g_mime_init(0);

  unlink("/tmp/foo");
  if ((freopen("/tmp/foo", "w", stderr)) == NULL) {
    printf("no good patty\n");
    perror("freopen()");
    exit(EXIT_FAILURE);
  }
  setlinebuf(stderr);

#if 0
  fprintf(stderr, "argc %d\n", argc);

  while (argc--) {
    fprintf(stderr, "%s\n", argv[0]);
    argv++;
  }
#endif

#if 0
  while ((c = getchar()) != EOF) {
    putc(c, stderr);
  }
  exit(0);
#endif

  if ((buf = mime2wl(0, "N2QZ")) == NULL) {
    printf("i hate it when that happens\n");
    exit(EXIT_FAILURE);
  }
  buffer_rewind(buf);

  while ((c = buffer_iterchar(buf)) != EOF) {
    if (putc(c, stderr) == EOF) {
      perror("putc()");
      exit(EXIT_FAILURE);
    }
  }

  g_mime_shutdown();
  exit(EXIT_SUCCESS);
  return 1;
}
