/* $Id$ */

#ifndef BUFFER_H
#define BUFFER_H

struct buffer {
  unsigned char *data;
  unsigned long alen;
  unsigned long dlen;
  unsigned int i;
};

struct buffer *buffer_new(void);
void buffer_free(struct buffer *b);
int buffer_addchar(struct buffer *b, int c);
int buffer_addstring(struct buffer *b, const unsigned char *s);
int buffer_addbuf(struct buffer *b, struct buffer *s);
int buffer_setstring(struct buffer *b, const unsigned char *s);
void buffer_rewind(struct buffer *b);
int buffer_iterchar(struct buffer *b);
int buffer_lastchar(struct buffer *b);
void buffer_truncate(struct buffer *b);
char *buffer_getstring(struct buffer *b);
char *buffer_getline(struct buffer *b, int terminator);
struct buffer *buffer_readfile(const char *path);
int buffer_writefile(const char *path, struct buffer *buf);
unsigned long buffer_length(struct buffer *b);

#endif
