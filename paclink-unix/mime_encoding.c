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
      linelen = 1;
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
      linelen = 1;
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

struct buffer *
qp_encode(struct buffer *inbuf, int istext)
{
  struct buffer *buf;
  int c;
  int lc;
  int pc = 0;
  unsigned long linelen = 0;
  char hexbuf[4];

  if ((buf = buffer_new()) == NULL) {
    perror("buffer_new()");
    exit(EXIT_FAILURE);
  }

  while ((c = buffer_iterchar(inbuf)) != EOF) {
    if (linelen > 70) {
      linelen = 0;
      if (buffer_addstring(buf, "=\r\n") == -1) {
	perror("buffer_addstring()");
	exit(EXIT_FAILURE);
      }
    }
    if (istext) {
      if (pc) {
	pc = 0;
	if (c == '\n') {
	  lc = buffer_lastchar(buf);
	  if ((lc == 9) || (lc == 32)) {
	    if (buffer_addstring(buf, "=\r\n") == -1) {
	      perror("buffer_addstring()");
	      exit(EXIT_FAILURE);
	    }
	  }
	  if (buffer_addstring(buf, "\r\n") == -1) {
	    perror("buffer_addstring()");
	    exit(EXIT_FAILURE);
	  }
	  linelen = 0;
	  continue;
	}
	if (buffer_addstring(buf, "=0D") == -1) {
	  perror("buffer_addstring()");
	  exit(EXIT_FAILURE);
	}
	linelen += 3;
      }
      if (c == '\r') {
	pc = 1;
	continue;
      }
    }
    if (((c >= 33) && (c <= 60))
	|| ((c >= 62) && (c <= 126))
	|| (c == 9)
	|| (c == 32)) {
      if (buffer_addchar(buf, c) == -1) {
	perror("buffer_addchar()");
	exit(EXIT_FAILURE);
      }
      linelen += 1;
    } else {
      sprintf(hexbuf, "=%02X", c);
      if (buffer_addstring(buf, hexbuf) == -1) {
	perror("buffer_addstring()");
	exit(EXIT_FAILURE);
      }
      linelen += 3;
    }
  }
  if (buffer_addstring(buf, "=\r\n") == -1) {
    perror("buffer_addstring()");
    exit(EXIT_FAILURE);
  }
  if (istext && pc) {
    if (buffer_addstring(buf, "=0D=\r\n") == -1) {
      perror("buffer_addstring()");
      exit(EXIT_FAILURE);
    }
  }
  return buf;
}

struct buffer *
qp_decode(struct buffer *inbuf)
{
  struct buffer *buf;
  int c;
  int c1;
  int c2;
  unsigned long x;
  char hexbuf[3];
  char *endp;

  if ((buf = buffer_new()) == NULL) {
    perror("buffer_new()");
    exit(EXIT_FAILURE);
  }

  while ((c = buffer_iterchar(inbuf)) != EOF) {
    if (c == '=') {
      if ((c1 = buffer_iterchar(inbuf)) == EOF) {
	if (buffer_addchar(buf, c) == -1) {
	  perror("buffer_addchar()");
	  exit(EXIT_FAILURE);
	}
	return buf;
      }
      if ((c2 = buffer_iterchar(inbuf)) == EOF) {
	if (buffer_addchar(buf, c) == -1) {
	  perror("buffer_addchar()");
	  exit(EXIT_FAILURE);
	}
	if (buffer_addchar(buf, c1) == -1) {
	  perror("buffer_addchar()");
	  exit(EXIT_FAILURE);
	}
	return buf;
      }
      if ((c1 == '\r') && (c2 == '\n')) {
	continue;
      }
      sprintf(hexbuf, "%c%c", c1, c2);
      x = strtoul(hexbuf, &endp, 16);
      if ((endp != hexbuf + 2) || (*endp != '\0')) {
	if (buffer_addchar(buf, c) == -1) {
	  perror("buffer_addchar()");
	  exit(EXIT_FAILURE);
	}
	if (buffer_addchar(buf, c1) == -1) {
	  perror("buffer_addchar()");
	  exit(EXIT_FAILURE);
	}
	if (buffer_addchar(buf, c2) == -1) {
	  perror("buffer_addchar()");
	  exit(EXIT_FAILURE);
	}
	continue;
      }
      c = x;
    }
    if (buffer_addchar(buf, c) == -1) {
      perror("buffer_addchar()");
      exit(EXIT_FAILURE);
    }
  }
  return buf;
}
