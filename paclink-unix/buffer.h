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

/* $Id$ */

#ifndef BUFFER_H
#define BUFFER_H

struct buffer {
  unsigned char *data;
  unsigned long alen;
  unsigned long dlen;
  unsigned int i;
};

struct buffer *buffer_new(void);
void buffer_free(struct buffer *b);
int buffer_addchar(struct buffer *b, int c);
int buffer_addstring(struct buffer *b, const unsigned char *s);
int buffer_addbuf(struct buffer *b, struct buffer *s);
int buffer_setstring(struct buffer *b, const unsigned char *s);
void buffer_rewind(struct buffer *b);
int buffer_iterchar(struct buffer *b);
int buffer_lastchar(struct buffer *b);
void buffer_truncate(struct buffer *b);
char *buffer_getstring(struct buffer *b);
char *buffer_getline(struct buffer *b, int terminator);
struct buffer *buffer_readfile(const char *path);
int buffer_writefile(const char *path, struct buffer *buf);
unsigned long buffer_length(struct buffer *b);

#endif
