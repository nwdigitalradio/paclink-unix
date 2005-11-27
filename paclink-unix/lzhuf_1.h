/* $Id$ */

#ifndef LZHUF_1_H
#define LZHUF_1_H

struct buffer *Encode(struct buffer *inbuf);
struct buffer *version_1_Encode(struct buffer *inbuf);
struct buffer *Decode(struct buffer *inbuf);
struct buffer *version_1_Decode(struct buffer *inbuf);

#endif
