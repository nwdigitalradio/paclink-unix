#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

#include <stdio.h>
#include <ctype.h>
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif

#include "mime_encoding.h"
#include "buffer.h"

int
main(int argc ATTRIBUTE_UNUSED, char *argv[] ATTRIBUTE_UNUSED)
{
  int i;
  const unsigned char *plain = "\01testing123\r\nfoobarbaz\rhello\nthere\r\n";
  struct buffer *buf;
  struct buffer *outbuf;
  struct buffer *decbuf;
  int c;
  int c2;

  if ((buf = buffer_new()) == NULL) {
    perror("buffer_new()");
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < 200 ; i++) {
    if (buffer_addstring(buf, plain) == -1) {
      perror("buffer_addstring()");
      exit(EXIT_FAILURE);
    }
  }

  printf("base64_encode\n");
  outbuf = base64_encode(buf);
  printf("back from base64_encode\n");
  buffer_rewind(outbuf);
  while ((c = buffer_iterchar(outbuf)) != EOF) {
    fputc(c, stdout);
  }
  fputc('\n', stdout);

  printf("base64_decode\n");
  decbuf = base64_decode(outbuf);
  printf("back from base64_decode\n");
  buffer_rewind(buf);
  buffer_rewind(decbuf);
  while ((c = buffer_iterchar(decbuf)) != EOF) {
    c2 = buffer_iterchar(buf);
    if (c2 != c) {
      printf("mismatch %d %d\n", c, c2);
      exit(EXIT_FAILURE);
    }
    if (!isprint(c) && !isspace(c)) {
      c = '?';
    }
    fputc(c, stdout);
  }
  c2 = buffer_iterchar(buf);
  if (c2 != c) {
    printf("mismatch %d %d\n", c, c2);
    exit(EXIT_FAILURE);
  }
  fputc('\n', stdout);

  printf("qp_encode\n");
  outbuf = qp_encode(buf, 1);
  printf("back from qp_encode\n");
  buffer_rewind(outbuf);
  while ((c = buffer_iterchar(outbuf)) != EOF) {
    fputc(c, stdout);
  }
  fputc('\n', stdout);

  printf("qp_decode\n");
  decbuf = qp_decode(outbuf);
  printf("back from qp_decode\n");
  buffer_rewind(buf);
  buffer_rewind(decbuf);
  while ((c = buffer_iterchar(decbuf)) != EOF) {
    c2 = buffer_iterchar(buf);
    if (c2 != c) {
      printf("mismatch %d %d\n", c, c2);
      exit(EXIT_FAILURE);
    }
    if (!isprint(c) && !isspace(c)) {
      c = '?';
    }
    fputc(c, stdout);
  }
  c2 = buffer_iterchar(buf);
  if (c2 != c) {
    printf("mismatch %d %d\n", c, c2);
    exit(EXIT_FAILURE);
  }
  fputc('\n', stdout);

  exit(EXIT_SUCCESS);
}
