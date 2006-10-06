/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2; c-brace-offset: -2; c-argdecl-indent: 2 -*- */

/*  paclink-unix client for the Winlink 2000 ham radio email system.
 *
 *  Copyright 2006 Nicholas S. Castellano <n2qz@arrl.net> and others,
 *                 See the file AUTHORS for a list.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

/**************************************************************
        lzhuf.c
        written by Haruyasu Yoshizaki 11/20/1988
        some minor changes 4/6/1989
        comments translated by Haruhiko Okumura 4/7/1989

        ported to UNIX and modified for paclink-unix by
        Nicholas S. Castellano N2QZ
**************************************************************/
#include <stdio.h>
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_STRING_H
# include <string.h>
#endif
#include <ctype.h>

#include "buffer.h"
#include "lzhuf_1.h"

static void InitTree(void);
static void InsertNode(int r);
static void DeleteNode(int p);
static int GetBit(struct buffer *inbuf);
static int GetByte(struct buffer *inbuf);
static int Putcode(int l, unsigned c, struct buffer *outbuf);
static void StartHuff(void);
static void reconst(void);
static void update(int c);
static int EncodeChar(unsigned c, struct buffer *outbuf);
static int EncodePosition(unsigned c, struct buffer *outbuf);
static int EncodeEnd(struct buffer *outbuf);
static int DecodeChar(struct buffer *inbuf);
static int DecodePosition(struct buffer *inbuf);

/* crctab calculated by Mark G. Mendel, Network Systems Corporation */
static unsigned short crctab[256] = {
  0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
  0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
  0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
  0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
  0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
  0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
  0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
  0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
  0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
  0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
  0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
  0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
  0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
  0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
  0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
  0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
  0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
  0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
  0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
  0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
  0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
  0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
  0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
  0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
  0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
  0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
  0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
  0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
  0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
  0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
  0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
  0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
};

#define updcrc(cp, crc) ((crc << 8) ^ crctab[(cp & 0xff) ^ (crc >> 8)])

static unsigned long int textsize = 0, codesize = 0;
static unsigned short crc;
static unsigned putbuf;
static unsigned char putlen;

/********** LZSS compression **********/

#define N               2048    /* buffer size */
#define F               60      /* lookahead buffer size */
#define THRESHOLD       2
#define NIL             N       /* leaf of tree */

unsigned char text_buf[N + F - 1];
unsigned int match_position;
int match_length;
int lson[N + 1], rson[N + 257], dad[N + 1];

static int
crc_fputc(int c, struct buffer *crcoutbuf)
{
  crc = updcrc(c, crc);
  return(buffer_addchar(crcoutbuf, c));
}

static int
crc_fgetc(struct buffer *crcinbuf)
{
  int retour;

  if ((retour = buffer_iterchar(crcinbuf)) != EOF) {
    crc = updcrc(retour, crc);
  }
  return(retour);
}

/* initialize trees */
static void
InitTree(void)
{
  int i;

  for (i = N + 1; i <= N + 256; i++)
    rson[i] = NIL;                  /* root */
  for (i = 0; i < N; i++)
    dad[i] = NIL;                   /* node */
}

/* insert to tree */
static void
InsertNode(int r)
{
  int i, p, cmp;
  unsigned char *key;
  unsigned c;

  cmp = 1;
  key = &text_buf[r];
  p = N + 1 + key[0];
  rson[r] = lson[r] = NIL;
  match_length = 0;
  for ( ; ; ) {
    if (cmp >= 0) {
      if (rson[p] != NIL)
	p = rson[p];
      else {
	rson[p] = r;
	dad[r] = p;
	return;
      }
    } else {
      if (lson[p] != NIL)
	p = lson[p];
      else {
	lson[p] = r;
	dad[r] = p;
	return;
      }
    }
    for (i = 1; i < F; i++)
      if ((cmp = key[i] - text_buf[p + i]) != 0)
	break;
    if (i > THRESHOLD) {
      if (i > match_length) {
	match_position = ((r - p) & (N - 1)) - 1;
	if ((match_length = i) >= F)
	  break;
      }
      if (i == match_length) {
	if ((c = ((r - p) & (N - 1)) - 1) < match_position) {
	  match_position = c;
	}
      }
    }
  }
  dad[r] = dad[p];
  lson[r] = lson[p];
  rson[r] = rson[p];
  dad[lson[p]] = r;
  dad[rson[p]] = r;
  if (rson[dad[p]] == p)
    rson[dad[p]] = r;
  else
    lson[dad[p]] = r;
  dad[p] = NIL;  /* remove p */
}

/* remove from tree */
static void
DeleteNode(int p)
{
  int q;

  if (dad[p] == NIL)
    return;                 /* not registered */
  if (rson[p] == NIL)
    q = lson[p];
  else
    if (lson[p] == NIL)
      q = rson[p];
    else {
      q = lson[p];
      if (rson[q] != NIL) {
	do {
	  q = rson[q];
	} while (rson[q] != NIL);
	rson[dad[q]] = lson[q];
	dad[lson[q]] = dad[q];
	lson[q] = lson[p];
	dad[lson[p]] = q;
      }
      rson[q] = rson[p];
      dad[rson[p]] = q;
    }
  dad[q] = dad[p];
  if (rson[dad[p]] == p)
    rson[dad[p]] = q;
  else
    lson[dad[p]] = q;
  dad[p] = NIL;
}

/* Huffman coding */

#define N_CHAR          (256 - THRESHOLD + F)
/* kinds of characters (character code = 0..N_CHAR-1) */
#define T               (N_CHAR * 2 - 1)        /* size of table */
#define R               (T - 1)                 /* position of root */
#define MAX_FREQ        0x8000          /* updates tree when the */
                                        /* root frequency comes to this value. */

/* table for encoding and decoding the upper 6 bits of position */

/* for encoding */
static unsigned char p_len[64] = {
  0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05,
  0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
  0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
  0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
  0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
};

static unsigned char p_code[64] = {
  0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68,
  0x70, 0x78, 0x80, 0x88, 0x90, 0x94, 0x98, 0x9C,
  0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
  0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
  0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE,
  0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,
  0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
  0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/* for decoding */
static unsigned char d_code[256] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
  0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
  0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
  0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
  0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
  0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
  0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
  0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
  0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
  0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
  0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D,
  0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F,
  0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11,
  0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13,
  0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15,
  0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17,
  0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B,
  0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F,
  0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23,
  0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27,
  0x28, 0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B,
  0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
};

static unsigned char d_len[256] = {
  0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
  0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
  0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
  0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
  0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
  0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
  0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
  0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
  0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
  0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
  0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
  0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
  0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
  0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
  0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
  0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
  0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
  0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
  0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
  0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
  0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
};

static unsigned freq[T + 1];   /* frequency table */

static int prnt[T + N_CHAR];   /* pointers to parent nodes, except for the */
                        /* elements [T..T + N_CHAR - 1] which are used to get */
                        /* the positions of leaves corresponding to the codes. */

static int son[T];             /* pointers to child nodes (son[], son[] + 1) */

static unsigned getbuf;
static unsigned char getlen;

/* get one bit */
static int
GetBit(struct buffer *inbuf)
{
  unsigned int i;

  while (getlen <= 8) {
    if ((int) (i = crc_fgetc(inbuf)) == EOF)
      i = 0;
    getbuf |= i << (8 - getlen);
    getlen += 8;
  }
  i = getbuf;
  getbuf <<= 1;
  getlen--;
  return (i & 0x8000) ? 1 : 0;
}

/* get one byte */
static int
GetByte(struct buffer *inbuf)
{
  unsigned i;

  while (getlen <= 8) {
    if ((int) (i = crc_fgetc(inbuf)) == EOF)
      i = 0;
    getbuf |= i << (8 - getlen);
    getlen += 8;
  }
  i = getbuf;
  getbuf <<= 8;
  getlen -= 8;
  return (i >> 8) & 0xff;
}

/* output c bits of code */
static int
Putcode(int l, unsigned c, struct buffer *outbuf)
{
  putbuf |= c >> putlen;
  if ((putlen += l) >= 8) {
    if (crc_fputc((int) (putbuf >> 8), outbuf) == EOF) {
      return -1;
    }
    if ((putlen -= 8) >= 8) {
      if (crc_fputc((int)putbuf, outbuf) == EOF) {
	return -1;
      }
      codesize += 2;
      putlen -= 8;
      putbuf = c << (l - putlen);
    } else {
      putbuf <<= 8;
      codesize++;
    }
  }
  return 0;
}

/* initialization of tree */
static void
StartHuff(void)
{
  int i, j;

  for (i = 0; i < N_CHAR; i++) {
    freq[i] = 1;
    son[i] = i + T;
    prnt[i + T] = i;
  }
  i = 0;
  j = N_CHAR;
  while (j <= R) {
    freq[j] = freq[i] + freq[i + 1];
    son[j] = i;
    prnt[i] = prnt[i + 1] = j;
    i += 2;
    j++;
  }
  freq[T] = 0xffff;
  prnt[R] = 0;
}

/* reconstruction of tree */
static void
reconst(void)
{
  int i, j, k;
  unsigned f, l;

  /* collect leaf nodes in the first half of the table */
  /* and replace the freq by (freq + 1) / 2. */
  j = 0;
  for (i = 0; i < T; i++) {
    if (son[i] >= T) {
      freq[j] = (freq[i] + 1) / 2;
      son[j] = son[i];
      j++;
    }
  }
  /* begin constructing tree by connecting sons */
  for (i = 0, j = N_CHAR; j < T; i += 2, j++) {
    k = i + 1;
    f = freq[j] = freq[i] + freq[k];
    for (k = j - 1; f < freq[k]; k--);
    k++;
    l = (j - k) * 2;
    memmove(&freq[k + 1], &freq[k], l);
    freq[k] = f;
    memmove(&son[k + 1], &son[k], l);
    son[k] = i;
  }
  /* connect prnt */
  for (i = 0; i < T; i++) {
    if ((k = son[i]) >= T) {
      prnt[k] = i;
    } else {
      prnt[k] = prnt[k + 1] = i;
    }
  }
}

/* increment frequency of given code by one, and update tree */
static void
update(int c)
{
  int i, j;
  unsigned k;
  int l;

  if (freq[R] == MAX_FREQ) {
    reconst();
  }
  c = prnt[c + T];
  do {
    k = ++freq[c];

    /* if the order is disturbed, exchange nodes */
    if (k > freq[l = c + 1]) {
      while (k > freq[++l]);
      l--;
      freq[c] = freq[l];
      freq[l] = k;

      i = son[c];
      prnt[i] = l;
      if (i < T)
	prnt[i + 1] = l;

      j = son[l];
      son[l] = i;

      prnt[j] = c;
      if (j < T)
	prnt[j + 1] = c;
      son[c] = j;

      c = l;
    }
  } while ((c = prnt[c]) != 0);   /* repeat up to root */
}

static int
EncodeChar(unsigned c, struct buffer *outbuf)
{
  unsigned i;
  int j, k;

  i = 0;
  j = 0;
  k = prnt[c + T];

  /* travel from leaf to root */
  do {
    i >>= 1;

    /* if node's address is odd-numbered, choose bigger brother node */
    if (k & 1) i += 0x8000;

    j++;
  } while ((k = prnt[k]) != R);
  if (Putcode(j, i, outbuf) == -1) {
    return -1;
  }
  update((int) c);
  return 0;
}

static int
EncodePosition(unsigned c, struct buffer *outbuf)
{
  unsigned i;

  /* output upper 6 bits by table lookup */
  i = c >> 6;
  if (Putcode(p_len[i], (unsigned) p_code[i] << 8, outbuf) == -1) {
    return -1;
  }

  /* output lower 6 bits verbatim */
  if (Putcode(6, (c & 0x3f) << 10, outbuf) == -1) {
    return -1;
  }
  return 0;
}

static int
EncodeEnd(struct buffer *outbuf)
{
  if (putlen) {
    if (crc_fputc((int) (putbuf >> 8), outbuf) == EOF) {
      return -1;
    }
    codesize++;
  }
  return 0;
}

static int
DecodeChar(struct buffer *inbuf)
{
  unsigned c;

  c = son[R];

  /* travel from root to leaf, */
  /* choosing the smaller child node (son[]) if the read bit is 0, */
  /* the bigger (son[]+1} if 1 */
  while (c < T) {
    c += GetBit(inbuf);
    c = son[c];
  }
  c -= T;
  update((int) c);
  return c;
}

int
DecodePosition(struct buffer *inbuf)
{
  unsigned i, j, c;

  /* recover upper 6 bits from table */
  i = GetByte(inbuf);
  c = (unsigned) d_code[i] << 6;
  j = d_len[i];

  /* read lower 6 bits verbatim */
  j -= 2;
  while (j--) {
    i = (i << 1) + GetBit(inbuf);
  }
  return c | (i & 0x3f);
}

/* compression */
struct buffer *
Encode(struct buffer *inbuf)
{
  int i, c, len, r, s, last_match_length;
#ifdef LZHUF_1_MAIN
  unsigned int printcount;
#endif
  struct buffer *outbuf;

  crc = 0;
  putbuf = 0;
  putlen = 0;
#ifdef LZHUF_1_MAIN
  printcount = 1;
#endif

  textsize = inbuf->dlen;

  if ((outbuf = buffer_new()) == NULL) {
    return NULL;
  }
  if (crc_fputc((int)(textsize & 0xff), outbuf) == -1) {
    buffer_free(outbuf);
    return NULL;
  }
  if (crc_fputc((int)((textsize >> 8) & 0xff), outbuf) == -1) {
    buffer_free(outbuf);
    return NULL;
  }
  if (crc_fputc((int)((textsize >> 16) & 0xff), outbuf) == -1) {
    buffer_free(outbuf);
    return NULL;
  }
  if (crc_fputc((int)((textsize >> 24) & 0xff), outbuf) == -1) {
    buffer_free(outbuf);
    return NULL;
  }

  if (textsize == 0)
    return outbuf;
  buffer_rewind(inbuf);
  textsize = 0;                   /* rewind and re-read */
  StartHuff();
  InitTree();
  s = 0;
  r = N - F;
  for (i = s; i < r; i++)
    text_buf[i] = ' ';
  for (len = 0; len < F && (c = buffer_iterchar(inbuf)) != EOF; len++)
    text_buf[r + len] = c;
  textsize = len;
  for (i = 1; i <= F; i++)
    InsertNode(r - i);
  InsertNode(r);
  do {
    if (match_length > len)
      match_length = len;
    if (match_length <= THRESHOLD) {
      match_length = 1;
      if (EncodeChar(text_buf[r], outbuf) == -1) {
	buffer_free(outbuf);
	return NULL;
      }
    } else {
      if (EncodeChar((unsigned) (255 - THRESHOLD + match_length), outbuf) == -1) {
	buffer_free(outbuf);
	return NULL;
      }
      if (EncodePosition((unsigned) match_position, outbuf) == -1) {
	buffer_free(outbuf);
	return NULL;
      }
    }
    last_match_length = match_length;
    for (i = 0; i < last_match_length &&
	   (c = buffer_iterchar(inbuf)) != EOF; i++) {
      DeleteNode(s);
      text_buf[s] = c;
      if (s < F - 1)
	text_buf[s + N] = c;
      s = (s + 1) & (N - 1);
      r = (r + 1) & (N - 1);
      InsertNode(r);
    }
#ifdef LZHUF_1_MAIN
    if ((textsize += i) > printcount) {
      fprintf(stderr, "%12ld\r", textsize);
      printcount += 1024;
    }
#endif
    while (i++ < last_match_length) {
      DeleteNode(s);
      s = (s + 1) & (N - 1);
      r = (r + 1) & (N - 1);
      if (--len)
	InsertNode(r);
    }
  } while (len > 0);
  if (EncodeEnd(outbuf) == -1) {
    buffer_free(outbuf);
    return NULL;
  }

#ifdef LZHUF_1_MAIN
  fprintf(stderr, "In : %ld bytes\n", textsize);
  fprintf(stderr, "Out: %ld bytes\n", codesize);
  fprintf(stderr, "Out/In: %.3f\n", (double) codesize / textsize);
#endif
  return outbuf;
}

struct buffer *
version_1_Encode(struct buffer *inbuf)
{
  int c;
  struct buffer *tmpoutbuf;
  struct buffer *outbuf;

  if ((tmpoutbuf = Encode(inbuf)) == NULL) {
    return NULL;
  }
  if ((outbuf = buffer_new()) == NULL) {
    buffer_free(tmpoutbuf);
    return NULL;
  }
  if (buffer_addchar(outbuf, (int) (crc & 0xff)) == -1) {
    buffer_free(tmpoutbuf);
    buffer_free(outbuf);
    return NULL;
  }
  if (buffer_addchar(outbuf, (int) ((crc >> 8) & 0xff)) == -1) {
    buffer_free(tmpoutbuf);
    buffer_free(outbuf);
    return NULL;
  }
#ifdef LZHUF_1_MAIN
  fprintf(stderr, "CRC: %04x\n", crc);
#endif
  buffer_rewind(tmpoutbuf);
  while ((c = buffer_iterchar(tmpoutbuf)) != EOF) {
    if (buffer_addchar(outbuf, c) == -1) {
      buffer_free(tmpoutbuf);
      buffer_free(outbuf);
      return NULL;
    }
  }
  buffer_free(tmpoutbuf);
  return outbuf;
}

/* recover */
struct buffer *
Decode(struct buffer *inbuf)
{
  struct buffer *outbuf;
  int i, j, k, r, c;
  unsigned long int count;
  int x;
#ifdef LZHUF_1_MAIN
  unsigned int printcount = 0;
#endif

  getbuf = 0;
  getlen = 0;
  crc = 0;

  if ((x = crc_fgetc(inbuf)) == EOF) {
    return NULL;
  }
  textsize = x;
  if ((x = crc_fgetc(inbuf)) == EOF) {
    return NULL;
  }
  textsize |= (x << 8);
  if ((x = crc_fgetc(inbuf)) == EOF) {
    return NULL;
  }
  textsize |= (x << 16);
  if ((x = crc_fgetc(inbuf)) == EOF) {
    return NULL;
  }
  textsize |= (x << 24);

#ifdef LZHUF_1_MAIN
  fprintf(stderr, "File Size = %lu\n", textsize);
#endif
  if ((outbuf = buffer_new()) == NULL) {
    perror("buffer_new()");
    exit(EXIT_FAILURE);
  }

  if (textsize == 0)
    return outbuf;

  StartHuff();
  for (i = 0; i < N - F; i++)
    text_buf[i] = ' ';
  r = N - F;
  for (count = 0; count < textsize; ) {
    c = DecodeChar(inbuf);
    if (c < 256) {
      if (buffer_addchar(outbuf, c) == EOF) {
	buffer_free(outbuf);
	return NULL;
      }
      text_buf[r++] = c;
      r &= (N - 1);
      count++;
    } else {
      i = (r - DecodePosition(inbuf) - 1) & (N - 1);
      j = c - 255 + THRESHOLD;
      for (k = 0; k < j; k++) {
	c = text_buf[(i + k) & (N - 1)];
	if (buffer_addchar(outbuf, c) == EOF) {
	  buffer_free(outbuf);
	  return NULL;
	}
	text_buf[r++] = c;
	r &= (N - 1);
	count++;
      }
    }
#ifdef LZHUF_1_MAIN
    if (count > printcount) {
      fprintf(stderr, "%12ld\r", count);
      printcount += 1024;
    }
#endif
  }
#ifdef LZHUF_1_MAIN
  fprintf(stderr, "%12ld\n", count);
#endif
  return outbuf;
}

struct buffer *
version_1_Decode(struct buffer *inbuf)
{
  int x;
  unsigned short crc_read;
  struct buffer *outbuf;

  if ((x = crc_fgetc(inbuf)) == EOF) {
    return NULL;
  }
  crc_read = x;
  if ((x = crc_fgetc(inbuf)) == EOF) {
    return NULL;
  }
  crc_read |= (x << 8);
#ifdef LZHUF_1_MAIN
  fprintf(stderr, "File CRC  = %04x\n", crc_read);
#endif

  outbuf = Decode(inbuf);
  
#ifdef LZHUF_1_MAIN
  fprintf(stderr, "Computed CRC = %04x\n", crc);
#endif

  if (crc != crc_read) {
#ifdef LZHUF_1_MAIN
    fprintf(stderr, "CRC mismatch\n");
#endif
    buffer_free(outbuf);
    return NULL;
  }

  return outbuf;
}

#ifdef LZHUF_1_MAIN
void usage(void);

void
usage(void)
{
  fprintf(stderr, "'lzhuf e[1] file1 file2' encodes file1 into file2.\n"
	  "'lzhuf d[1] file2 file1' decodes file2 into file1.\n");
  exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
  int version_1;
  struct buffer *inbuf;
  struct buffer *outbuf;

  if ((argc != 4)
      || (strlen(argv[1]) < 1)
      || (strlen(argv[1]) > 2)
      || (strchr("EeDd", argv[1][0]) == NULL)
      || ((strlen(argv[1]) == 2) && (argv[1][1] != '1'))) {
    usage();
  }

  version_1 = (argv[1][1] == '1');

  if ((inbuf = buffer_readfile(argv[2])) == NULL) {
    perror("buffer_readfile()");
    exit(EXIT_FAILURE);
  }

  if (toupper((unsigned char) *argv[1]) == 'E') {
    if (version_1) {
      outbuf = version_1_Encode(inbuf);
    } else {
      outbuf = Encode(inbuf);
    }
  } else {
    if (version_1) {
      outbuf = version_1_Decode(inbuf);
    } else {
      outbuf = Decode(inbuf);
    }
  }
  fprintf(stderr, "\n");

  if (outbuf == NULL) {
    exit(EXIT_FAILURE);
  }

  if (buffer_writefile(argv[3], outbuf) != 0) {
    perror("buffer_writefile()");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
  return 0;
}
#endif
