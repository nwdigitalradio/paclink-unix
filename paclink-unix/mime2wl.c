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

static struct buffer *conv_addrlist(char *al);
static char *getboundary(char *ct);
static char *makeendboundary(char *boundary);
static char *getheader(struct buffer *buf, const char *header);
static struct buffer *getmimeheaders(struct buffer *mime);

struct buffer *mime2wl(struct buffer *mime, const char *callsign);

static struct buffer *
conv_addrlist(char *al)
{
  struct buffer *buf;
  char *p;
  char *last;
  char *a;
  char *b;

  if ((buf = buffer_new()) == NULL) {
    return NULL;
  }

  for ((p = strtok_r(al, ",", &last));
       p;
       p = strtok_r(NULL, ",", &last)) {
    while (isspace((unsigned char) *p)) {
      p++;
    }
    buffer_addstring(buf, "SMTP:");

    if (((a = strchr(p, '<')) != NULL)
	&& ((b = strchr(a, '>')) != NULL)) {
      *b = '\0';
      buffer_addstring(buf, a + 1);
      *b = '>';
    } else {
      buffer_addstring(buf, p);
    }
    buffer_addstring(buf, "\r\n");
  }

  return buf;
}

static char *
getboundary(char *ct)
{
  char *tmp = NULL;
  unsigned char *boundary = NULL;
  char *next;
  size_t span;
  unsigned char *tail;
  char *retboundary;

  if ((tmp = strdup(ct)) == NULL) {
    return NULL;
  }
  next = strchr(tmp, ';');
  while ((boundary = next) != NULL) {
    span = strspn(boundary, "; \t");
    boundary += span;
    next = strchr(boundary, ';');
    if (next) {
      *next++ = '\0';
    }
    if (strcasebegins(boundary, "boundary=")) {
      boundary += 9;
      tail = strrchr(boundary, '\0') - 1;
      while ((tail >= boundary) && isspace(*tail)) {
	*tail-- = '\0';
      }
      if (boundary[0] == '"') {
	boundary++;
	tail = strrchr(boundary, '\0') - 1;
	if ((tail < boundary) || (*tail != '"')) {
	  free(tmp);
	  return NULL;
	}
	*tail = '\0';
      }
      if ((retboundary = malloc(strlen(boundary) + 3)) == NULL) {
	return NULL;
      }
      strcpy(retboundary, "--");
      strcat(retboundary, boundary);
      free(tmp);
      return retboundary;
    }
  }
  free(tmp);
  return NULL;  
}

static char *
makeendboundary(char *boundary)
{
  char *eb;

  if ((eb = malloc(strlen(boundary) + 3)) == NULL) {
    return NULL;
  }
  strcpy(eb, boundary);
  strcat(eb, "--");
  return eb;
}

static char *
getheader(struct buffer *buf, const char *header)
{
  char *line;
  size_t off;
  char *hdata;
  unsigned char *cp;

  buffer_rewind(buf);
  off = strlen(header);
  while ((line = buffer_getline(buf, '\n')) != NULL) {
    strzapcc(line);
    if ((strncasecmp(line, header, off) == 0) && (line[off] == ':')) {
      for (cp = line + off + 1; isspace(*cp); cp++)
	;
      hdata = strdup(cp);
      free(line);
      return hdata;
    }
    free(line);
  }
  return NULL;
}

static struct buffer *
getmimeheaders(struct buffer *mime)
{
  struct buffer *hbuf;
  char *line;

  if ((hbuf = buffer_new()) == NULL) {
    return NULL;
  }
  while ((line = buffer_getline(mime, '\n')) != NULL) {
    strzapcc(line);
    printf("line: /%s/\n", line);
    if ((line[0] != ' ') && (line[0] != '\t')) {
      buffer_addchar(hbuf, '\n');
    }
    buffer_addstring(hbuf, line);
    if (line[0] == '\0') {
      break;
    }
    free(line);
  }
  buffer_addchar(hbuf, '\0');
  return hbuf;
}

struct buffer *
mime2wl(struct buffer *mime, const char *callsign)
{
  char *line;
  struct buffer *mhbuf; /* mime headers */
  struct buffer *mbbuf; /* mime body */
  struct buffer *wmbuf; /* final wl2k message */
  struct buffer *wabuf; /* wl2k attachment accumulaor */
  struct buffer *wbbuf; /* wl2k body */
  struct buffer *wcbuf; /* wl2k current attachment */
  struct buffer *albuf; /* address list */
  char *aline;
  char *ct = NULL;
  char *boundary;
  char *endboundary;
  int gotboundary = 0;
  int gottext = 0;
  char *mid;
  char date[17];
  time_t tloc;
  struct tm *tm;

  buffer_rewind(mime);

  if ((wmbuf = buffer_new()) == NULL) {
    return NULL;
  }

  if ((mid = generate_mid(callsign)) == NULL) {
    return NULL;
  }

  buffer_addstring(wmbuf, "Mid: ");
  buffer_addstring(wmbuf, mid);
  buffer_addstring(wmbuf, "\r\nDate: ");
  time(&tloc);
  tm = gmtime(&tloc);
  strftime(date, 17, "%Y/%m/%d %H:%M", tm);
  buffer_addstring(wmbuf, date);
  buffer_addstring(wmbuf, "\r\nType: Private\r\n");

  if ((mhbuf = getmimeheaders(mime)) == NULL) {
    return NULL;
  }

  printf("\n\nHeaders:\n\n%s\nEnd of headers\n\n", buffer_getstring(mhbuf));

  if ((mbbuf = buffer_new()) == NULL) {
    return NULL;
  }
  while ((line = buffer_getline(mime, '\n')) != NULL) {
    buffer_addstring(mbbuf, line);
    free(line);
  }

  if ((line = getheader(mhbuf, "from")) == NULL) {
    printf("no from\n");
  } else {
    /* XXX envelope */
    albuf = conv_addrlist(line);
    if (albuf) {
      buffer_rewind(albuf);
      while ((aline = buffer_getline(albuf, '\n')) != NULL) {
	buffer_addstring(wmbuf, "From: ");
	buffer_addstring(wmbuf, aline);
      }
      buffer_free(albuf);
    }
  }

  if ((line = getheader(mhbuf, "to")) == NULL) {
    printf("no to\n");
  } else {
    /* XXX envelope */
    albuf = conv_addrlist(line);
    if (albuf) {
      buffer_rewind(albuf);
      while ((aline = buffer_getline(albuf, '\n')) != NULL) {
	buffer_addstring(wmbuf, "To: ");
	buffer_addstring(wmbuf, aline);
      }
      buffer_free(albuf);
    }
  }

  if ((line = getheader(mhbuf, "cc")) == NULL) {
    printf("no cc\n");
  } else {
    /* XXX envelope */
    albuf = conv_addrlist(line);
    if (albuf) {
      buffer_rewind(albuf);
      while ((aline = buffer_getline(albuf, '\n')) != NULL) {
	buffer_addstring(wmbuf, "Cc: ");
	buffer_addstring(wmbuf, aline);
      }
      buffer_free(albuf);
    }
  }

  if ((line = getheader(mhbuf, "subject")) == NULL) {
    printf("no subject\n");
  } else {
    buffer_addstring(wmbuf, "Subject: ");
    buffer_addstring(wmbuf, line);
    buffer_addstring(wmbuf, "\r\n");
    printf("subj: %s\n", line);
  }

  buffer_addstring(wmbuf, "Mbo: SMTP\r\n");

  if ((line = getheader(mhbuf, "content-type")) == NULL) {
    printf("no content-type\n");
  } else {
    ct = line;
    printf("ct: %s\n", ct);
  }

  if (ct && strcasebegins(ct, "multipart/mixed")) {
    printf("it is multipart/mixed\n");
    boundary = getboundary(ct);
    if (!boundary) {
      printf("no boundary\n");
      return NULL;
    }
    printf("boundary is: %s\n", boundary);
    endboundary = makeendboundary(boundary);
    printf("endboundary is: %s\n", endboundary);

    buffer_rewind(mbbuf);
    while ((line = buffer_getline(mbbuf, '\n')) != NULL) {
      strzapcc(line);
      printf("body line: /%s/\n", line);
      if (!gotboundary) {
	printf("(skipped)\n");
      }
      if (strcmp(line, boundary) == 0) {
	printf("*** boundary line\n");
	gotboundary = 1;
      } else if (strcmp(line, endboundary) == 0) {
	printf("*** endboundary line\n");
	gotboundary = 0;
      }
    }

  } else {
    printf("it is NOT multipart/mixed\n");
  }

  buffer_free(mhbuf);
  buffer_free(mbbuf);

  buffer_writefile("/tmp/foobar", wmbuf);
  exit(2);

  return(wmbuf);

  /* XXX finish me */
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
  g_mime_message_foreach_part(message, count_foreach_callback, &count);
  printf("There are %d parts in the message\n", count);
  g_mime_shutdown();
  exit(0);
  return 1;
}
