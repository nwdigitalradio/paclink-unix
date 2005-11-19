/* $Id$ */

#ifndef BUFFER_H
#define BUFFER_H

struct buffer {
  unsigned char *data;
  unsigned long alen;
  unsigned long dlen;
};

struct buffer *buffer_new(void);
void buffer_free(struct buffer *b);
int buffer_addchar(struct buffer *b, int c);
void buffer_truncate(struct buffer *b);

#endif
