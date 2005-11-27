#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
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
