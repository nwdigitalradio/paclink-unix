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

#define BUFFER_CHUNK 1024

struct buffer *
buffer_new(void)
{
  struct buffer *b;

  if ((b = malloc(sizeof(struct buffer))) == NULL) {
    return NULL;
  }
  b->alen = BUFFER_CHUNK;
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

  if (b->dlen == b->alen) {
    b->alen += BUFFER_CHUNK;
    if ((d = realloc(b->data, b->alen * sizeof(unsigned char))) == NULL) {
      b->alen -= BUFFER_CHUNK;
      return -1;
    }
    b->data = d;
  }
  b->data[b->dlen++] = (unsigned char) c;
  return 0;
}

void
buffer_truncate(struct buffer *b)
{

  b->dlen = 0;
}
