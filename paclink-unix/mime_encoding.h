/* $Id$ */

#ifndef MIME_ENCODING_H
#define MIME_ENCODING_H

struct buffer *base64_encode(struct buffer *inbuf);
struct buffer *base64_decode(struct buffer *inbuf);
struct buffer *qp_encode(struct buffer *inbuf, int istext);
struct buffer *qp_decode(struct buffer *inbuf);

#endif
