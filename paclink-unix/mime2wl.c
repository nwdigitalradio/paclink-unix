#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "llist.h"
#include "buffer.h"
#include "strutil.h"

char *getheader(struct buffer *buf, const char *header);
struct buffer *mime2wl(struct buffer *mime);
char *getboundary(char *ct);

char *
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
      retboundary = strdup(boundary);
      free(tmp);
      return retboundary;
    }
  }
  free(tmp);
  return NULL;  
}

char *
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

struct buffer *
mime2wl(struct buffer *mime)
{
  char *line;
  struct buffer *hbuf;
  struct buffer *bbuf;
  struct buffer *wbuf;
  char *ct = NULL;
  char *boundary;

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
  printf("\n\nHeaders:\n\n%s\nEnd of headers\n\n", buffer_getstring(hbuf));

  if ((bbuf = buffer_new()) == NULL) {
    return NULL;
  }
  while ((line = buffer_getline(mime, '\n')) != NULL) {
    buffer_addstring(bbuf, line);
    free(line);
  }

  if ((line = getheader(hbuf, "content-type")) == NULL) {
    printf("no content-type\n");
  } else {
    ct = line;
    printf("ct: %s\n", ct);
  }

  if ((line = getheader(hbuf, "subject")) == NULL) {
    printf("no subject\n");
  } else {
    printf("subj: %s\n", line);
  }

  if (ct && strcasebegins(ct, "multipart/mixed")) {
    printf("it is multipart/mixed\n");
    boundary = getboundary(ct);
    if (boundary) {
      printf("boundary is: %s\n", boundary);
    } else {
      printf("no boundary\n");
      /* XXX error */
    }
  } else {
    printf("it is NOT multipart/mixed\n");
  }

  buffer_free(hbuf);
  buffer_free(bbuf);

  /* XXX finish me */
}

int
main()
{
  struct buffer *mime;
  struct buffer *wl;

  if ((mime = buffer_readfile("mime.msg")) == NULL) {
    perror("buffer_readfile");
    exit(2);
  }
  wl = mime2wl(mime);
  exit(0);
  return 1;
}
