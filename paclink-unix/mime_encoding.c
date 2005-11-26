#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

#include <stdio.h>
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_STRING_H
# include <string.h>
#endif

#include "buffer.h"
#include "mime_encoding.h"

static const unsigned char *b64chr =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

struct buffer *
base64_encode(struct buffer *inbuf)
{
  struct buffer *buf;
  int c;
  unsigned char x;
  unsigned long state = 0;
  unsigned long linelen = 0;

  if ((buf = buffer_new()) == NULL) {
    perror("buffer_new()");
    exit(EXIT_FAILURE);
  }

  buffer_rewind(inbuf);

  c = buffer_iterchar(inbuf);
  while (c != EOF) {
    switch (state % 4) {
    case 0:
      x = (c >> 2) & 0x3f;
      break;
    case 1:
      x = (c << 4) & 0x30;
      if ((c = buffer_iterchar(inbuf)) != EOF) {
	x |= (c >> 4) & 0x0f;
      }
      break;
    case 2:
      x = (c << 2) & 0x3c;
      if ((c = buffer_iterchar(inbuf)) != EOF) {
	x |= (c >> 6) & 0x03;
      }
      break;
    case 3:
      x = c & 0x3f;
      c = buffer_iterchar(inbuf);
      break;
    default:
      fprintf(stderr, "not reached\n");
      exit(EXIT_FAILURE);
      break;      
    }
    if (++linelen > 76) {
      linelen %= 76;
      if (buffer_addstring(buf, "\r\n") == -1) {
	perror("buffer_addstring()");
	exit(EXIT_FAILURE);
      }
    }
    if (buffer_addchar(buf, b64chr[x]) == -1) {
      perror("buffer_addchar()");
      exit(EXIT_FAILURE);
    }
    state++;
    state = state % 4;
  }
  while (state % 4) {
    if (++linelen > 76) {
      linelen %= 76;
      if (buffer_addstring(buf, "\r\n") == -1) {
	perror("buffer_addstring()");
	exit(EXIT_FAILURE);
      }
    }
    if (buffer_addchar(buf, '=') == -1) {
      perror("buffer_addchar()");
      exit(EXIT_FAILURE);
    }
    state++;
  }
  if (buffer_addstring(buf, "\r\n") == -1) {
    perror("buffer_addstring()");
    exit(EXIT_FAILURE);
  }
  return buf;
}

struct buffer *
base64_decode(struct buffer *inbuf)
{
  struct buffer *buf;
  int c;
  unsigned char x = 0;
  unsigned char *cp;
  int i;
  int state = 0;

  if ((buf = buffer_new()) == NULL) {
    perror("buffer_new()");
    exit(EXIT_FAILURE);
  }

  buffer_rewind(inbuf);
  while ((c = buffer_iterchar(inbuf)) != EOF) {
    if (c == '=') {
      break;
    }
    cp = strchr(b64chr, c);
    if (cp == NULL) {
      continue;
    }
    i = cp - b64chr;
    switch (state) {
    case 0:
      x = (i << 2) & 0xfc;
      break;
    case 1:
      x |= (i >> 4) & 0x03;
      if (buffer_addchar(buf, x) == -1) {
	perror("buffer_addchar()");
	exit(EXIT_FAILURE);
      }
      x = (i << 4) & 0xf0;
      break;
    case 2:
      x |= (i >> 2) & 0x0f;
      if (buffer_addchar(buf, x) == -1) {
	perror("buffer_addchar()");
	exit(EXIT_FAILURE);
      }
      x = (i << 6) & 0xc0;
      break;
    case 3:
      x |= i & 0x3f;
      if (buffer_addchar(buf, x) == -1) {
	perror("buffer_addchar()");
	exit(EXIT_FAILURE);
      }
      break;
    default:
      fprintf(stderr, "not reached\n");
      exit(EXIT_FAILURE);
      break;      
    }
    state++;
    state = state % 4;
  }
  return buf;
}
