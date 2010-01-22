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

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  if HAVE_TIME_H
#   include <time.h>
#  endif
# endif
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
#if HAVE_FCNTL_H
# include <fcntl.h>
#endif

#include <unistd.h>
#include <gmime/gmime.h>

#include "compat.h"
#include "buffer.h"
#include "mid.h"
#include "mime2wl.h"
#include "conf.h"

struct wl2kmessage {
  struct buffer *hbuf; /* headers */
  struct buffer *fbuf; /* attachment filename headers */
  struct buffer *bbuf; /* body */
  struct buffer *abuf; /* attachment data */
};

static char *address_cleanup(const char *addr);
static GMimeMessage *parse_message(int fd);
static void mime_foreach_callback(GMimeObject *parent, GMimeObject *part, gpointer user_data);

static char *
address_cleanup(const char *addr)
{
  char *tmp;
  char *clean;
  char *p;
  char *a;
  char *b;
  struct buffer *buf;

  if ((tmp = strdup(addr)) == NULL) {
    return NULL;
  }
  if ((buf = buffer_new()) == NULL) {
    return NULL;
  }
  p = tmp;
  while (isspace((unsigned char) *p)) {
    p++;
  }
  if (((a = strchr(p, '<')) != NULL)
      && ((b = strchr(a, '>')) != NULL)) {
    *b = '\0';
    buffer_addstring(buf, a + 1);
  } else {
    if ((a = strchr(p, ' ')) != NULL) {
      *a = '\0';
    }
    buffer_addstring(buf, p);
  }
  buffer_addchar(buf, '\0');
  clean = buffer_getstring(buf);
  buffer_free(buf);
  free(tmp);

  return clean;
}

static GMimeMessage *
parse_message(int fd)
{
  GMimeMessage *message;
  GMimeParser *parser;
  GMimeStream *stream;
  GMimeStreamFilter *filtstream;
  GMimeFilter *filtcrlf;
  int id_filtcrlf;

  /* create a stream to read from the file descriptor */
  stream = g_mime_stream_fs_new(dup(fd));

  /*
   * Create a line terminator filter
   * 
   * First arg line-feed parameter: TRUE
   *   - lone line-feeds will be 'encoded' into CRLF 
   * Second arg dots parameter TRUE,
   *   - a '.' at the beginning of a line will be 'encoded' into ".."
   *  
   *  Returns : new GMimeFilterCRLF filter. 
   */
  filtcrlf = g_mime_filter_crlf_new(TRUE, TRUE);

  /* create a new filter object */
  filtstream = g_mime_stream_filter_new (stream);

  /*
   * Add the line terminator filter to the filter stream
   * Returns : an id for the filter.
   */
  id_filtcrlf = g_mime_stream_filter_add (filtstream, filtcrlf);

  /* create a new parser object to parse the filter stream */
  parser = g_mime_parser_new_with_stream((GMimeStream *)filtstream);

  /* If persist is FALSE, the parser will always load message content into memory.  This allows input to come from a pipe. */
  g_mime_parser_set_persist_stream (parser, FALSE);
	
  /* unref the stream (parser owns a ref, so this object does not actually get free'd until we destroy the parser) */
  g_object_unref(stream);
	
  /* parse the message from the stream */
  message = g_mime_parser_construct_message(parser);
	
  /* free the parser (and the stream) */
  g_object_unref(parser);
	
  return message;
}

static void
mime_foreach_callback(GMimeObject *parent, GMimeObject *part, gpointer user_data)
{
  GMimeContentType *content_type;
  struct wl2kmessage *wl2k = user_data;
  GMimeStream *stream;
  GMimeDataWrapper *wrapper;
  const char *fn;
  char c;
  ssize_t r;
  ssize_t len;
  char *slen;
  struct buffer *buf;
	
  /* 'part' points to the current part node that g_mime_message_foreach() is iterating over */
	
  /* find out what class 'part' is... */
  if (GMIME_IS_MESSAGE_PART(part)) {
    /* message/rfc822 or message/news */
    GMimeMessage *message;
		
    /* g_mime_message_foreach() won't descend into
       child message parts, so if we want to process any
       subparts of this child message, we'll have to call
       g_mime_message_foreach() again here. */
		
    message = g_mime_message_part_get_message((GMimeMessagePart *) part);
    g_mime_message_foreach(message, mime_foreach_callback, wl2k);
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

    fn = g_mime_part_get_filename((const GMimePart *) part);
    fflush(stdout);

    buf = wl2k->abuf;

    content_type = (GMimeContentType *)g_mime_object_get_content_type (GMIME_OBJECT (part));

    if ((fn == NULL) && (buffer_length(wl2k->bbuf) == 0UL)) {
      if (g_mime_content_type_is_type(content_type, "text", "plain")) {
	buf = wl2k->bbuf;
      } else if (g_mime_content_type_is_type(content_type, "text", "html")) {
	/* XXX convert to plain text */
	buf = wl2k->bbuf;
      }
    }

    wrapper = g_mime_part_get_content_object((const GMimePart *) part);

    stream = g_mime_stream_mem_new();

    g_mime_data_wrapper_write_to_stream(wrapper, stream);

    g_mime_stream_flush(stream);

    g_mime_stream_reset(stream);

    len = g_mime_stream_length(stream);
    if (asprintf(&slen, "%ld", (long) len) == -1) {
      perror("asprintf()");
      exit(EXIT_FAILURE);
    }
    if (buf == wl2k->abuf) {
      if (fn == NULL) {
	if (g_mime_content_type_is_type(content_type, "text", "*")) {
	  fn = "attachment.txt";
	} else {
	  fn = "attachment.bin";
	}
      }
      buffer_addstring(wl2k->fbuf, "File: ");
      buffer_addstring(wl2k->fbuf, slen);
      buffer_addstring(wl2k->fbuf, " ");
      buffer_addstring(wl2k->fbuf, fn);
      buffer_addstring(wl2k->fbuf, "\r\n");
    } else {
      buffer_addstring(wl2k->hbuf, "Body: ");
      buffer_addstring(wl2k->hbuf, slen);
      buffer_addstring(wl2k->hbuf, "\r\n");
    }
    free(slen);

    buffer_addstring(buf, "\r\n");
    while (!g_mime_stream_eos(stream) && (r = g_mime_stream_read(stream, &c, 1)) >= 0) {
      buffer_addchar(buf, (int) c);
    }

    g_object_unref(stream);

    g_object_unref(wrapper);

  } else {
    g_assert_not_reached();
  }
}

struct buffer *
mime2wl(int fd, const char *callsign)
{
  GMimeMessage *message;
  const char *header;
  char *clean;
  char *mid;
  struct wl2kmessage wl2k;
  struct buffer *buf;
  char date[17];
  time_t tloc;
  struct tm *tm;
  int gmt_offset;
  InternetAddressList *ial;
  InternetAddressList *ialp;
  InternetAddress *ia;
  static char *nobody = "No message body\r\n";
  ssize_t len;
  char *slen;
  int idx;

  message = parse_message(fd);
  close(fd);

  if ((wl2k.hbuf = buffer_new()) == NULL) {
    return NULL;
  }

  if ((wl2k.fbuf = buffer_new()) == NULL) {
    return NULL;
  }

  if ((wl2k.bbuf = buffer_new()) == NULL) {
    return NULL;
  }

  if ((wl2k.abuf = buffer_new()) == NULL) {
    return NULL;
  }

  if ((mid = generate_mid(callsign)) == NULL) {
    return NULL;
  }
  buffer_addstring(wl2k.hbuf, "Mid: ");
  buffer_addstring(wl2k.hbuf, mid);
  buffer_addstring(wl2k.hbuf, "\r\n");

  buffer_addstring(wl2k.hbuf, "Date: ");
  g_mime_message_get_date(message, &tloc, &gmt_offset);
  if (tloc == 0) {
    time(&tloc);
  }
  tm = gmtime(&tloc);
  if (strftime(date, sizeof(date), "%Y/%m/%d %H:%M", tm) == 0) {
    return NULL;
  }
  buffer_addstring(wl2k.hbuf, date);
  buffer_addstring(wl2k.hbuf, "\r\n");

  buffer_addstring(wl2k.hbuf, "Type: Private\r\n");

  header = g_mime_message_get_sender(message);
  clean = address_cleanup(header);
  buffer_addstring(wl2k.hbuf, "From: ");
  if (strchr(clean, '@')) {
    buffer_addstring(wl2k.hbuf, "SMTP:");
  }
  buffer_addstring(wl2k.hbuf, clean);
  buffer_addstring(wl2k.hbuf, "\r\n");
  free(clean);

  ialp = ial = g_mime_message_get_recipients(message, GMIME_RECIPIENT_TYPE_TO);
  idx = 0;
  while ((ia = internet_address_list_get_address(ial, idx)) != NULL) {
    header = internet_address_to_string(ia, 0);
    clean = address_cleanup(header);
    buffer_addstring(wl2k.hbuf, "To: ");
    if (strchr(clean, '@')) {
      buffer_addstring(wl2k.hbuf, "SMTP:");
    }
    buffer_addstring(wl2k.hbuf, clean);
    buffer_addstring(wl2k.hbuf, "\r\n");
    free(clean);
    idx++;
  }

  ialp = ial = g_mime_message_get_recipients(message, GMIME_RECIPIENT_TYPE_CC);
  idx = 0;
  while ((ia = internet_address_list_get_address(ial, idx)) != NULL) {
    header = internet_address_to_string(ia, 0);
    clean = address_cleanup(header);
    buffer_addstring(wl2k.hbuf, "Cc: ");
    if (strchr(clean, '@')) {
      buffer_addstring(wl2k.hbuf, "SMTP:");
    }
    buffer_addstring(wl2k.hbuf, clean);
    buffer_addstring(wl2k.hbuf, "\r\n");
    free(clean);
    idx++;
  }

  header = g_mime_message_get_subject(message);
  buffer_addstring(wl2k.hbuf, "Subject: ");
  buffer_addstring(wl2k.hbuf, header);
  buffer_addstring(wl2k.hbuf, "\r\n");
  buffer_addstring(wl2k.hbuf, "Mbo: SMTP\r\n");

  g_mime_message_foreach(message, mime_foreach_callback, &wl2k);

  if (buffer_length(wl2k.bbuf) == 0UL) {
    len = strlen(nobody);
    if (asprintf(&slen, "%ld", (long) len) == -1) {
      perror("asprintf()");
      exit(EXIT_FAILURE);
    }
    buffer_addstring(wl2k.hbuf, "Body: ");
    buffer_addstring(wl2k.hbuf, slen);
    buffer_addstring(wl2k.hbuf, "\r\n");
    buffer_addstring(wl2k.bbuf, nobody);
    free(slen);
  }

  if ((buf = buffer_new()) == NULL) {
    return NULL;
  }
  buffer_addbuf(buf, wl2k.hbuf);
  buffer_addbuf(buf, wl2k.fbuf);
  buffer_addbuf(buf, wl2k.bbuf);
  buffer_addbuf(buf, wl2k.abuf);
  buffer_free(wl2k.hbuf);
  buffer_free(wl2k.fbuf);
  buffer_free(wl2k.bbuf);
  buffer_free(wl2k.abuf);
  return buf;
}

#ifdef MIME2WL_MAIN
int
main(int argc, char *argv[])
{
  struct buffer *obuf;
  int c;
  int fd;
  struct conf *conf;
  char *mycall;

  g_mime_init(0);

  if (argc != 2) {
    fprintf(stderr, "Usage: %s messagefile\n", getprogname());
    exit(EXIT_FAILURE);
  }
  if ((fd = open(argv[1], O_RDONLY)) == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  conf = conf_read();
  if ((mycall = conf_get(conf, "mycall")) == NULL) {
    fprintf(stderr, "%s: failed to read mycall from configuration file\n", getprogname());
  }
  obuf = mime2wl(fd, mycall);
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
#endif
