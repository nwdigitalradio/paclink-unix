#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

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
buffer_addstring(struct buffer *b, unsigned char *s)
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
buffer_setstring(struct buffer *b, unsigned char *s)
{

  buffer_truncate(b);
  return buffer_addstring(b, s);
}

void
buffer_truncate(struct buffer *b)
{

  b->dlen = 0;
}
