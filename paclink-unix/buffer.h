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
int buffer_addstring(struct buffer *b, unsigned char *s);
int buffer_setstring(struct buffer *b, unsigned char *s);
void buffer_truncate(struct buffer *b);

#endif
