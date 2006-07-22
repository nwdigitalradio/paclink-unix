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
#include <time.h>
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
      retboundary = strdup(boundary);
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

int
main()
{
  struct buffer *mime;
  struct buffer *wl;

  if ((mime = buffer_readfile("mime.msg")) == NULL) {
    perror("buffer_readfile");
    exit(2);
  }
  wl = mime2wl(mime, "N2QZ");
  exit(0);
  return 1;
}
