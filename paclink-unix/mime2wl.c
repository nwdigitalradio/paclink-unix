/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2; c-brace-offset: -2; c-argdecl-indent: 2 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@helixcode.com>
 *           Nicholas S. Castellano N2QZ <n2qz@arrl.net>
 *
 *  Copyright 2000 Helix Code, Inc. (www.helixcode.com)
 *  Copyright 2006 Nicholas S. Castellano <n2qz@arrl.net>
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
#if HAVE_TIME_H
# include <time.h>
#endif
#if HAVE_FCNTL_H
# include <fcntl.h>
#endif
#include <gmime/gmime.h>
#include "compat.h"
#include "llist.h"
#include "buffer.h"
#include "strutil.h"
#include "mid.h"

static char *
address_cleanup(char *addr)
{
  char *clean;
  char *p;
  char *last;
  char *a;
  char *b;
  struct buffer *buf;

  if ((buf = buffer_new()) == NULL) {
    return NULL;
  }
  p = addr;
  while (isspace((unsigned char) *p)) {
    p++;
  }
  if (((a = strchr(p, '<')) != NULL)
      && ((b = strchr(a, '>')) != NULL)) {
    *b = '\0';
    buffer_addstring(buf, a + 1);
    *b = '>';
  } else {
    if ((a = strchr(p, ' ')) != NULL) {
      *a = '\0';
      buffer_addstring(buf, p);
      *a = ' ';
    } else {
      buffer_addstring(buf, p);
    }
  }
  buffer_addchar(buf, '\0');
  clean = buffer_getstring(buf);
  buffer_free(buf);

  return clean;
}

static void
write_gmimeobject_to_screen(GMimeObject *object)
{
  GMimeStream *stream;
  GMimeDataWrapper *wrapper;
  const char *fn;
	
  fn = g_mime_part_get_filename((const GMimePart *) object);
  printf("-> %s\n", fn);
  fflush(stdout);

  wrapper = g_mime_part_get_content_object((const GMimePart *) object);
	
  /* create a new stream for writing to stdout */
  stream = g_mime_stream_fs_new(dup(1));
	
  /* write the object to the stream */
  g_mime_data_wrapper_write_to_stream(wrapper, stream);
	
  /* flush the stream (kinda like fflush() in libc's stdio) */
  g_mime_stream_flush(stream);
	
  /* free the output stream */
  g_object_unref (stream);
}

static GMimeMessage *
parse_message(int fd)
{
  GMimeMessage *message;
  GMimeParser *parser;
  GMimeStream *stream;
	
  /* create a stream to read from the file descriptor */
  stream = g_mime_stream_fs_new(dup(fd));
	
  /* create a new parser object to parse the stream */
  parser = g_mime_parser_new_with_stream(stream);
	
  /* unref the stream (parser owns a ref, so this object does not actually get free'd until we destroy the parser) */
  g_object_unref(stream);
	
  /* parse the message from the stream */
  message = g_mime_parser_construct_message(parser);
	
  /* free the parser (and the stream) */
  g_object_unref(parser);
	
  return message;
}

static void
count_foreach_callback(GMimeObject *part, gpointer user_data)
{
  GMimeContentType *content_type;
  int *count = user_data;
	
  (*count)++;
	
  /* 'part' points to the current part node that g_mime_message_foreach_part() is iterating over */
	
  /* find out what class 'part' is... */
  if (GMIME_IS_MESSAGE_PART(part)) {
    /* message/rfc822 or message/news */
    GMimeMessage *message;
		
    /* g_mime_message_foreach_part() won't descend into
       child message parts, so if we want to count any
       subparts of this child message, we'll have to call
       g_mime_message_foreach_part() again here. */
		
    message = g_mime_message_part_get_message((GMimeMessagePart *) part);
    g_mime_message_foreach_part(message, count_foreach_callback, count);
    g_object_unref(message);
  } else if (GMIME_IS_MESSAGE_PARTIAL(part)) {
    /* message/partial */
		
    /* this is an incomplete message part, probably a
       large message that the sender has broken into
       smaller parts and is sending us bit by bit. we
       could save some info about it so that we could
       piece this back together again once we get all the
       parts? */
  } else if (GMIME_IS_MULTIPART(part)) {
    /* multipart/mixed, multipart/alternative, multipart/related, multipart/signed, multipart/encrypted, etc... */
		
    /* we'll get to finding out if this is a signed/encrypted multipart later... */
  } else if (GMIME_IS_PART(part)) {
    /* a normal leaf part, could be text/plain or image/jpeg etc */
    printf("leaf part\n");
    write_gmimeobject_to_screen(part);
  } else {
    g_assert_not_reached();
  }
}

int
main(int argc, char *argv[])
{
  struct buffer *mime;
  struct buffer *wl;
  int fd;
  GMimeMessage *message;
  int count = 0;
  char *callsign = "N2QZ";
  char *header;
  char *clean;
  char *mid;
  struct buffer *hbuf;
  char date[17];
  time_t tloc;
  struct tm *tm;
  int gmt_offset;
  const InternetAddressList *ial;
  const InternetAddress *ia;

  g_mime_init(0);

  if (argc != 2) {
    fprintf(stderr, "Usage: %s messagefile\n", getprogname());
    exit(EXIT_FAILURE);
  }
  if ((fd = open(argv[1], O_RDONLY)) == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  message = parse_message(fd);
  close(fd);

  if ((hbuf = buffer_new()) == NULL) {
    return NULL;
  }

  if ((mid = generate_mid(callsign)) == NULL) {
    return NULL;
  }
  buffer_addstring(hbuf, "Mid: ");
  buffer_addstring(hbuf, mid);
  buffer_addstring(hbuf, "\r\n");

  buffer_addstring(hbuf, "Date: ");
  g_mime_message_get_date(message, &tloc, &gmt_offset);
  if (tloc == 0) {
    time(&tloc);
  }
  tm = gmtime(&tloc);
  strftime(date, 17, "%Y/%m/%d %H:%M", tm);
  buffer_addstring(hbuf, date);
  buffer_addstring(hbuf, "\r\n");

  buffer_addstring(hbuf, "Type: Private\r\n");

  header = g_mime_message_get_sender(message);
  clean = address_cleanup(header);
  buffer_addstring(hbuf, "From: SMTP:");
  buffer_addstring(hbuf, clean);
  buffer_addstring(hbuf, "\r\n");
  free(clean);

  ial = g_mime_message_get_recipients(message, GMIME_RECIPIENT_TYPE_TO);
  while ((ia = internet_address_list_get_address(ial)) != NULL) {
    header = internet_address_to_string(ia, 0);
    clean = address_cleanup(header);
    buffer_addstring(hbuf, "To: SMTP:");
    buffer_addstring(hbuf, clean);
    buffer_addstring(hbuf, "\r\n");
    free(clean);
    ial = internet_address_list_next(ial);
  }

  ial = g_mime_message_get_recipients(message, GMIME_RECIPIENT_TYPE_CC);
  while ((ia = internet_address_list_get_address(ial)) != NULL) {
    header = internet_address_to_string(ia, 0);
    clean = address_cleanup(header);
    buffer_addstring(hbuf, "Cc: SMTP:");
    buffer_addstring(hbuf, clean);
    buffer_addstring(hbuf, "\r\n");
    free(clean);
    ial = internet_address_list_next(ial);
  }

  ial = g_mime_message_get_recipients(message, GMIME_RECIPIENT_TYPE_BCC);
  while ((ia = internet_address_list_get_address(ial)) != NULL) {
    header = internet_address_to_string(ia, 0);
    clean = address_cleanup(header);
    buffer_addstring(hbuf, "Bcc: SMTP:");
    buffer_addstring(hbuf, clean);
    buffer_addstring(hbuf, "\r\n");
    free(clean);
    ial = internet_address_list_next(ial);
  }
  
  header = g_mime_message_get_subject(message);
  buffer_addstring(hbuf, "Subject: ");
  buffer_addstring(hbuf, header);
  buffer_addstring(hbuf, "\r\n");
  buffer_addstring(hbuf, "Mbo: SMTP\r\n");

  printf("%s", buffer_getstring(hbuf));
  exit(20);


  g_mime_message_foreach_part(message, count_foreach_callback, &count);
  printf("There are %d parts in the message\n", count);
  g_mime_shutdown();
  exit(0);
  return 1;
}
