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
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include "buffer.h"

struct buffer *
buffer_new(void)
{
  struct buffer *b;

  if ((b = malloc(sizeof(struct buffer))) == NULL) {
    return NULL;
  }
  b->alen = 1;
  b->dlen = 0;
  b->i = 0;
  if ((b->data = malloc(b->alen * sizeof(unsigned char))) == NULL) {
    return NULL;
  }
  return b;
}

void
buffer_free(struct buffer *b)
{

  if (b->data) {
    free(b->data);
  }
  free(b);
}

int
buffer_addchar(struct buffer *b, int c)
{
  unsigned char *d;
  unsigned long newlen;

  if (b->dlen == b->alen) {
    newlen = b->alen * 2;
    if ((d = realloc(b->data, newlen * sizeof(unsigned char))) == NULL) {
      return -1;
    }
    b->data = d;
    b->alen = newlen;
  }
  b->data[b->dlen++] = (unsigned char) c;
  return 0;
}

int
buffer_addstring(struct buffer *b, const unsigned char *s)
{
  int r;

  for ( ; *s; s++) {
    if ((r = buffer_addchar(b, *s)) != 0) {
      return r;
    }
  }
  return 0;
}

int
buffer_addbuf(struct buffer *b, struct buffer *s)
{
  int c;

  buffer_rewind(s);
  while ((c = buffer_iterchar(s)) != EOF) {
    buffer_addchar(b, c);
  }
  return 0;
}

int
buffer_setstring(struct buffer *b, const unsigned char *s)
{

  buffer_truncate(b);
  return buffer_addstring(b, s);
}

void
buffer_rewind(struct buffer *b)
{

  b->i = 0;
}

int
buffer_iterchar(struct buffer *b)
{

  if (b->i >= b->dlen) {
    return EOF;
  }
  return b->data[b->i++];
}

int
buffer_lastchar(struct buffer *b)
{

  if (b->dlen == 0) {
    return EOF;
  }
  return b->data[b->dlen - 1];
}

void
buffer_truncate(struct buffer *b)
{

  b->dlen = 0;
}

char *
buffer_getstring(struct buffer *b)
{
  char *cp;

  cp = strdup(b->data);
  return cp;
}

char *
buffer_getline(struct buffer *b, int terminator)
{
  struct buffer *t;
  int c;
  char *cp;

  if ((t = buffer_new()) == NULL) {
    return NULL;
  }
  c = buffer_iterchar(b);
  if (c == EOF) {
    buffer_free(t);
    return NULL;
  }
  for (;;) {
    buffer_addchar(t, c);
    if (c == terminator) {
      break;
    }
    c = buffer_iterchar(b);
    if (c == EOF) {
      break;
    }
  }
  buffer_addchar(t, '\0');
  cp = strdup(t->data);
  buffer_free(t);
  return cp;
}

struct buffer *
buffer_readfile(const char *path)
{
  FILE *fp;
  int c;
  struct buffer *buf;

  if ((fp = fopen(path, "rb")) == NULL) {
    return NULL;
  }
  if ((buf = buffer_new()) == NULL) {
    fclose(fp);
    return NULL;
  }
  while ((c = fgetc(fp)) != EOF) {
    if (buffer_addchar(buf, c) == -1) {
      fclose(fp);
      buffer_free(buf);
      return NULL;
    }
  }
  if (fclose(fp) != 0) {
    buffer_free(buf);
    return NULL;
  }
  return buf;
}

int
buffer_writefile(const char *path, struct buffer *buf)
{
  FILE *fp;
  int c;

  if ((fp = fopen(path, "wb")) == NULL) {
    return -1;
  }
  buffer_rewind(buf);
  while ((c = buffer_iterchar(buf)) != EOF) {
    if (fputc(c, fp) == EOF) {
      fclose(fp);
      return -1;
    }
  }
  if (fclose(fp) != 0) {
    return -1;
  }
  return 0;
}

unsigned long
buffer_length(struct buffer *b)
{

  return b->dlen;
}
