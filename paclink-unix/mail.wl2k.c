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
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_CTYPE_H
# include <ctype.h>
#endif
#if HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif
#if HAVE_ERRNO_H
# include <errno.h>
#endif

#include <gmime/gmime.h>

#include "compat.h"
#include "buffer.h"
#include "mime2wl.h"
#include "strutil.h"
#include "timeout.h"
#include "conf.h"
#include "printlog.h"

int gverbose_flag=FALSE;

/* prototypes */
void usage(void);


int
main(int argc, char *argv[])
{
  struct buffer *buf;
  int c;
  char *mid;
  FILE *fp;
  int i;
  struct conf *conf;
  char *mycall;
  bool bRecordMid = TRUE;   /* default is record mid on mail send */

  g_mime_init(0);

  opterr = 0;

  while ((c = getopt (argc, argv, "m")) != -1) {
    switch (c) {
      case 'm':
        bRecordMid = FALSE;
        break;
      case '?':
        if (isprint (optopt)) {
          fprintf (stderr, "%s: Unknown option `-%c'.\n",
             getprogname(), optopt);
        } else {
          fprintf (stderr,"%s: Unknown option character `\\x%x'.\n",
             getprogname(), optopt);
        }
        usage();
      default:
        usage();
    }
  }

  setlinebuf(stdout);
  setlinebuf(stderr);

  /* XXX remove this when we get more stable and stop hanging so much */
  settimeout(60);

  fprintf(stderr, "%s: euid: %lu egid: %lu\n", getprogname(), (unsigned long) geteuid(), (unsigned long) getegid());

  fprintf(stderr, "%s: argc: %d\n", getprogname(), argc);
  for (i = 0; i < argc; i++) {
    fprintf(stderr, "%s: argv[%d]: %s\n", getprogname(), i, argv[i]);
  }

  conf = conf_read();
  if ((mycall = conf_get(conf, "mycall")) == NULL) {
    fprintf(stderr, "%s: failed to read mycall from configuration file\n", getprogname());
  }

  if ((buf = mime2wl(0, mycall, bRecordMid)) == NULL) {
    fprintf(stderr, "%s: mime2wl() failed\n", getprogname());
    exit(EXIT_FAILURE);
  }
  buffer_rewind(buf);

  if ((mid = buffer_getline(buf, '\n')) == NULL) {
    fprintf(stderr, "%s: malformed message was produced\n", getprogname());
    exit(EXIT_FAILURE);
  }
  if (!strcasebegins(mid, "Mid:")) {
    fprintf(stderr, "%s: malformed message was produced\n", getprogname());
    exit(EXIT_FAILURE);
  }
  mid += 4;
  while (isspace((unsigned char) *mid)) {
    mid++;
  }
  strzapcc(mid);
  if (chdir(WL2K_OUTBOX) != 0) {
    fprintf(stderr, "%s: chdir(%s): %s\n", getprogname(), WL2K_OUTBOX, strerror(errno));
    exit(EXIT_FAILURE);
  }

  buffer_rewind(buf);
  /* XXX security: untaint mid */
  if ((fp = fopen(mid, "w")) == NULL) {
    fprintf(stderr, "%s: fopen(%s): %s\n", getprogname(), mid, strerror(errno));
    exit(EXIT_FAILURE);
  }
  while ((c = buffer_iterchar(buf)) != EOF) {
    if (putc(c, fp) == EOF) {
      fprintf(stderr, "%s: putc(): %s\n", getprogname(), strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
  fclose(fp);

  g_mime_shutdown();
  exit(EXIT_SUCCESS);
  return 1;
}

void usage(void)
{
  fprintf(stderr, "Usage: %s [-m]\n", getprogname());
  exit(EXIT_FAILURE);
}
