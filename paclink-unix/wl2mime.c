/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2; c-brace-offset: -2; c-argdecl-indent: 2 -*- */

/*  paclink-unix client for the Winlink 2000 ham radio email system.
 *
 *  Copyright 2006 Nicholas S. Castellano <n2qz@arrl.net> and others,
 *                 See the file AUTHORS for a list.
 *
 *  Authors: Jeffrey Stedfast <fejj@helixcode.com>
 *  Copyright 2000 Helix Code, Inc. (www.helixcode.com)
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
#if HAVE_CTYPE_H
# include <ctype.h>
#endif
#if HAVE_STRING_H
# include <string.h>
#endif

#include <gmime/gmime.h>

#include "buffer.h"
#include "strutil.h"

static void
write_message_to_screen (GMimeMessage *message)
{
  GMimeStream *stream;
	
  /* create a new stream for writing to stdout */
  stream = g_mime_stream_fs_new(dup(1));
	
  /* write the message to the stream */
  g_mime_object_write_to_stream((GMimeObject *) message, stream);
	
  /* flush the stream (kinda like fflush() in libc's stdio) */
  g_mime_stream_flush(stream);
	
  /* free the output stream */
  g_object_unref(stream);
}

struct buffer *
wl2mime(struct buffer *ibuf)
{
  struct buffer *hbuf;
  struct buffer *bbuf;
  struct buffer *obuf;
  char *line;
  char *linedata;
  int c;
  char ch;
  unsigned long len;
  char *endp;
  GMimePart *mime_part;
  GMimeStream *stream;
  GMimeDataWrapper *content;
  GMimeMultipart *multipart;
  GMimeMessage *message;
  struct tm tm;
  time_t date;

  if ((hbuf = buffer_new()) == NULL) {
    return NULL;
  }
  if ((bbuf = buffer_new()) == NULL) {
    free(hbuf);
    return NULL;
  }

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

  message = g_mime_message_new(0);
  multipart = g_mime_multipart_new_with_subtype("mixed");

  buffer_rewind(hbuf);
  buffer_rewind(bbuf);
  while ((line = buffer_getline(hbuf, '\n')) != NULL) {
    strzapcc(line);
    printf("%s\n", line);
    if ((linedata = strchr(line, ':')) != NULL) {
      linedata++;
      while (isspace((unsigned char) *linedata)) {
	linedata++;
      }
    }
    if (strcasebegins(line, "Mid:")) {
      g_mime_message_set_message_id(message, linedata);
    } else if (strcasebegins(line, "Date:")) {
      memset(&tm, 0, sizeof(struct tm));
      if (strptime(linedata, "%Y/%m/%d %R", &tm) != NULL) {
	date = mktime(&tm);
	g_mime_message_set_date(message, date, 0);
      }
    } else if (strcasebegins(line, "From:")) {
      if (strcasebegins(linedata, "SMTP:")) {
	linedata += 5;
      }
      g_mime_message_set_sender(message, linedata);
    } else if (strcasebegins(line, "To:")) {
      if (strcasebegins(linedata, "SMTP:")) {
	linedata += 5;
      }
      g_mime_message_add_recipient(message, GMIME_RECIPIENT_TYPE_TO, "", linedata);
    } else if (strcasebegins(line, "Cc:")) {
      if (strcasebegins(linedata, "SMTP:")) {
	linedata += 5;
      }
      g_mime_message_add_recipient(message, GMIME_RECIPIENT_TYPE_CC, "", linedata);
    } else if (strcasebegins(line, "Subject:")) {
      g_mime_message_set_subject(message, linedata);
    } else if (strcasebegins(line, "Body:")) {
      mime_part = g_mime_part_new_with_type("text", "plain");
      stream = g_mime_stream_mem_new();
      len = strtoul(linedata, &endp, 10);
      while (len--) {
	c = buffer_iterchar(bbuf);
	ch = (char) c;
	g_mime_stream_write(stream, &ch, 1);
      }
      content = g_mime_data_wrapper_new_with_stream(stream, GMIME_PART_ENCODING_DEFAULT);
      g_object_unref(stream);
      g_mime_part_set_content_object(mime_part, content);
      g_object_unref(content);
      g_mime_multipart_add_part(multipart, (GMimeObject *) mime_part);
    } else if (strcasebegins(line, "File:")) {
      mime_part = g_mime_part_new_with_type("application", "octet-stream");
      stream = g_mime_stream_mem_new();
      len = strtoul(linedata, &endp, 10);
      while (isspace((unsigned char) *endp)) {
	endp++;
      }
      g_mime_part_set_filename(mime_part, endp);
      while (len--) {
	c = buffer_iterchar(bbuf);
	ch = (char) c;
	g_mime_stream_write(stream, &ch, 1);
      }
      content = g_mime_data_wrapper_new_with_stream(stream, GMIME_PART_ENCODING_DEFAULT);
      g_object_unref(stream);
      g_mime_part_set_content_object(mime_part, content);
      g_object_unref(content);
      g_mime_part_set_encoding(mime_part, GMIME_PART_ENCODING_BASE64);
      g_mime_multipart_add_part(multipart, (GMimeObject *) mime_part);
    }
    free(line);
  }

  g_mime_message_set_mime_part(message, (GMimeObject *) multipart);

  write_message_to_screen(message);

  if ((obuf = buffer_new()) == NULL) {
    free(hbuf);
    free(bbuf);
    return NULL;
  }
  free(hbuf);
  free(bbuf);
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
