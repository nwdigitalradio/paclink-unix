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
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_CTYPE_H
# include <ctype.h>
#endif

#include "compat.h"
#include "buffer.h"
#include "conf.h"

#define CONFFILE "wl2k.conf"
#define CONFPATH SYSCONFDIR "/" CONFFILE

struct conf *
conf_read(void)
{
  struct buffer *b;
  char *cp;
  char *var;
  char *value;
  char *last;
  struct conf *conflist = NULL;
  struct conf **confnext = NULL;

  confnext = &conflist;
  if ((b = buffer_readfile(CONFPATH)) == NULL) {
    printf("Could not read %s\n", CONFPATH);
    exit(2);
  }
  while ((cp = buffer_getline(b, '\n')) != NULL) {
    while (isspace((unsigned char) *cp)) {
      cp++;
    }
    if (*cp == '#') {
            continue;
    }
    if (((cp = strtok_r(cp, "#", &last)))
      && ((var = strtok_r(cp, "= \n", &last)))
        && ((value = strtok_r(NULL, "= \n", &last)))) {
      *confnext = (struct conf *) malloc(sizeof(struct conf));
      if (*confnext == NULL) {
        printf("Out of memory\n");
        exit(2);
      }
      (*confnext)->var = strdup(var);
      (*confnext)->value = strdup(value);
      (*confnext)->next = NULL;
      confnext = &((*confnext)->next);
    }
  }

  return conflist;
}

char *
conf_get(struct conf *conf, const char *var)
{
  while (conf) {
    if (strcmp(conf->var, var) == 0) {
      return conf->value;
    }
    conf = conf->next;
  }
  return NULL;
}

