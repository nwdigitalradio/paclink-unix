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

#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef __RCSID
__RCSID("$Id$");
#endif

#if HAVE_STDIO_H
# include <stdio.h>
#endif
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if HAVE_CTYPE_H
# include <ctype.h>
#endif
#if HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#if HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif
#if HAVE_ERRNO_H
# include <errno.h>
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) (strlen((dirent)->d_name))
#else
# define dirent direct
# define NAMLEN(dirent) ((dirent)->d_namlen)
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#define USE_SECURE_LOGIN 1

#include "compat.h"
#include "strutil.h"
#include "wl2k.h"
#include "timeout.h"
#include "mid.h"
#include "buffer.h"
#include "lzhuf_1.h"
#include "wl2mime.h"
#include "printlog.h"

#define PROPLIMIT 5
/* number of bytes to send through socket */
#define CHUNK_SIZE 255

/* Send messages only flag */
extern int gsendmsgonly_flag;

struct proposal {
  char code;
  char type;
  char mid[MID_MAXLEN + 1];
  unsigned long usize;
  unsigned long csize;
  struct proposal *next;
  char *path;
  struct buffer *ubuf;
  struct buffer *cbuf;
  char *title;
  unsigned long offset;
  int accepted;
  int delete;
};

static int getrawchar(FILE *fp);
static struct buffer *getcompressed(FILE *fp);
static struct proposal *parse_proposal(char *propline);
static int b2outboundproposal(FILE *ifp, FILE *ofp, char *lastcommand, struct proposal **oproplist);
static void printprop(struct proposal *prop);
static void putcompressed(struct proposal *prop, FILE *fp);
static char *tgetline(FILE *fp, int terminator, int ignore);
static void dodelete(struct proposal **oproplist, struct proposal **nproplist);

static void send_my_sid(FILE *ofp);
static char *parse_inboundsid(char *line);
static int inbound_parser(FILE *ifp, FILE *ofp, struct proposal *nproplist, struct proposal *oproplist, char *emailaddress);

#ifdef USE_SECURE_LOGIN
static char *handshake(FILE *ifp, FILE *ofp, char *sl_pass,char *mycall, char * yourcall, int opropcount);
static void compute_secure_login_response(char *challenge, char *response, char *password);
static int send_secure_login_response(FILE *ofp, char *challenge, char *sl_pass);

# include "md5.h"

/* Salt for Winlink 2000 secure login */
static const unsigned char sl_salt[] = {
  77, 197, 101, 206, 190, 249,
  93, 200, 51, 243, 93, 237,
  71, 94, 239, 138, 68, 108,
  70, 185, 225, 137, 217, 16,
  51, 122, 193, 48, 194, 195,
  198, 175, 172, 169, 70, 84,
  61, 62, 104, 186, 114, 52,
  61, 168, 66, 129, 192, 208,
  187, 249, 232, 193, 41, 113,
  41, 45, 240, 16, 29, 228,
  208, 228, 61, 20 };
#endif

/* Debug only */
void dump_hex(char *buf, size_t len);

void
dump_hex(char *buf, size_t len)
{
	size_t i;
	unsigned char *pBuf;
	pBuf = (unsigned char *)buf;

	for(i=0; i < len; i++) {
		printf("%02x ", *pBuf++);
	}
	printf("\n");
}

static int
getrawchar(FILE *fp)
{
  int c;

  resettimeout();
  c = fgetc(fp);
  if (c == EOF) {
    print_log(LOG_ERR, "lost connection in getrawchar()");
    exit(EXIT_FAILURE);
  }
  return c;
}

#define CHRNUL 0
#define CHRSOH 1
#define CHRSTX 2
#define CHREOT 4

struct buffer *
getcompressed(FILE *fp)
{
  int c;
  int len;
  int i;
  unsigned char title[81];
  unsigned char offset[7];
  int cksum = 0;
  struct buffer *buf;

  if ((buf = buffer_new()) == NULL) {
    return NULL;
  }
  c = getrawchar(fp);
  if (c != CHRSOH) {
    buffer_free(buf);
    return NULL;
  }
  len = getrawchar(fp);
  title[80] = '\0';
  for (i = 0; i < 80; i++) {
    c = getrawchar(fp);
    len--;
    title[i] = (unsigned char)c;
    if (c == CHRNUL) {
      ungetc(c, fp);
      len++;
      break;
    }
  }
  c = getrawchar(fp);
  len--;
  if (c != CHRNUL) {
    buffer_free(buf);
    return NULL;
  }
  print_log(LOG_DEBUG, "title: %s", title);
  offset[6] = '\0';
  for (i = 0; i < 6; i++) {
    c = getrawchar(fp);
    len--;
    offset[i] = (unsigned char)c;
    if (c == CHRNUL) {
      ungetc(c, fp);
      len++;
      break;
    }
  }
  c = getrawchar(fp);
  len--;
  if (c != CHRNUL) {
    buffer_free(buf);
    return NULL;
  }
  print_log(LOG_DEBUG,"offset: %s",  offset);
  if (len != 0) {
    buffer_free(buf);
    return NULL;
  }
  if (strcmp((const char *) offset, "0") != 0) {
    buffer_free(buf);
    return NULL;
  }

  for (;;) {
    c = getrawchar(fp);
    switch (c) {
    case CHRSTX:
      print_log(LOG_DEBUG,"STX");
      len = getrawchar(fp);
      if (len == 0) {
	len = 256;
      }
      print_log(LOG_DEBUG,"len %d", len);
      while (len--) {
	c = getrawchar(fp);
	if (buffer_addchar(buf, c) == EOF) {
	  buffer_free(buf);
	  return NULL;
	}
	cksum = (cksum + c) % 256;
      }
      break;
    case CHREOT:
      print_log(LOG_DEBUG,"EOT");
      c = getrawchar(fp);
      cksum = (cksum + c) % 256;
      if (cksum != 0) {
        print_log(LOG_ERR, "bad cksum");
	buffer_free(buf);
	return NULL;
      }
      return buf;
      break;
    default:
      print_log(LOG_ERR,"unexpected character in compressed stream");
      buffer_free(buf);
      return NULL;
      break;
    }
  }
  buffer_free(buf);
  return NULL;
}

static void
putcompressed(struct proposal *prop, FILE *fp)
{
  size_t len;
  unsigned char title[81];
  unsigned char offset[7];
  int cksum = 0;
  unsigned char *cp, *cp_buf;
  long rem;
  unsigned char msglen;
  size_t msgbuflen;

  strlcpy((char *) title, prop->title, sizeof(title));
  snprintf((char *) offset, sizeof(offset), "%lu", prop->offset);

  print_log(LOG_DEBUG,"transmitting [%s] [offset %s]",  title, offset);

  len = strlen((const char *) title) + strlen((const char *) offset) + 2;

  /* ** Send header */
  resettimeout();
  if (fprintf(fp, "%c%c%s%c%s%c", CHRSOH, (int)len, title, CHRNUL, offset, CHRNUL) == -1) {
    print_log(LOG_ERR, "fprintf() - %s", strerror(errno));
    exit(EXIT_FAILURE);
  }
  fflush(fp);

  rem = (long)prop->csize;
  cp = prop->cbuf->data;

  if (rem < 6) {
    print_log(LOG_ERR,"invalid compressed data");
    exit(EXIT_FAILURE);
  }

  cp += prop->offset;
  rem -= (long)prop->offset;

  if (rem < 0) {
    print_log(LOG_ERR,"invalid offset");
    exit(EXIT_FAILURE);
  }

  /* ** Send message */
  while (rem > 0) {
    print_log(LOG_DEBUG,"... %ld", rem);
    if (rem > CHUNK_SIZE) {
      msglen = CHUNK_SIZE;
    } else {
      msglen = (unsigned char)rem;
    }
    if (fprintf(fp, "%c%c", CHRSTX, msglen) == -1) {
      print_log(LOG_ERR, "fprintf() - %s", strerror(errno));
      exit(EXIT_FAILURE);
    }

    /* ** send buffer to ax25 stack */
    cp_buf = cp;
    msgbuflen = msglen;
    while (msglen--) {
      resettimeout();
      cksum += *cp;
      cp++;
      rem--;
    }
    if( fwrite(cp_buf, 1, msgbuflen, fp) < msgbuflen) {
      print_log(LOG_ERR, "socket write - %s", strerror(errno));
    }
    fflush(fp);
  }

  /* ** Send checksum */
  cksum = -cksum & 0xff;
  resettimeout();
  if (fprintf(fp, "%c%c", CHREOT, cksum) == -1) {
    print_log(LOG_ERR, "fprintf() - %s", strerror(errno));
    exit(EXIT_FAILURE);
  }
  fflush(fp);
  resettimeout();
}

static struct proposal *
parse_proposal(char *propline)
{
  char *cp = propline;
  static struct proposal prop;
  int i;
  char *endp;

  if (!cp) {
    return NULL;
  }
  if (*cp++ != 'F') {
    return NULL;
  }
  prop.code = *cp++;
  switch (prop.code) {
  case 'C':
    if (*cp++ != ' ') {
      print_log(LOG_ERR,"malformed proposal 1");
      return NULL;
    }
    prop.type = *cp++;
    if ((prop.type != 'C') && (prop.type != 'E')) {
      print_log(LOG_ERR,"malformed proposal 2");
      return NULL;
    }
    if (*cp++ != 'M') {
      print_log(LOG_ERR," malformed proposal 3");
      return NULL;
    }
    if (*cp++ != ' ') {
      print_log(LOG_ERR,"malformed proposal 4");
      return NULL;
    }
    for (i = 0; i < MID_MAXLEN; i++) {
      prop.mid[i] = *cp++;
      if (prop.mid[i] == ' ') {
	prop.mid[i] = '\0';
	cp--;
	break;
      } else {
	if (prop.mid[i] == '\0') {
          print_log(LOG_ERR,"malformed proposal 5");
	  return NULL;
	}
      }
    }
    prop.mid[MID_MAXLEN] = '\0';
    if (*cp++ != ' ') {
      print_log(LOG_ERR,"malformed proposal 6");
      return NULL;
    }
    prop.usize = strtoul(cp, &endp, 10);
    cp = endp;
    if (*cp++ != ' ') {
      print_log(LOG_ERR,"malformed proposal 7");
      return NULL;
    }
    prop.csize = (unsigned int) strtoul(cp, &endp, 10);
    cp = endp;
    if (*cp != ' ') {
      print_log(LOG_ERR,"malformed proposal 8");
      return NULL;
    }
    break;
  case 'A':
  case 'B':
  default:
    prop.type = 'X';
    prop.mid[0] = '\0';
    prop.usize = 0;
    prop.csize = 0;
    break;
    print_log(LOG_ERR,"unsupported proposal type %c", prop.code);
    break;
  }
  prop.next = NULL;
  prop.path = NULL;
  prop.cbuf = NULL;
  prop.ubuf = NULL;
  prop.delete = 0;

  return &prop;
}

static void
printprop(struct proposal *prop)
{
  print_log(LOG_DEBUG,
	  "proposal code %c type %c mid %s usize %lu csize %lu next %p path %s ubuf %p cbuf %p",
	  prop->code,
	  prop->type,
	  prop->mid,
	  prop->usize,
	  prop->csize,
	  prop->next,
	  prop->path,
	  prop->ubuf,
	  prop->cbuf);
}

static void
dodelete(struct proposal **oproplist, struct proposal **nproplist)
{
  if ((oproplist == NULL) || (nproplist == NULL)) {
    print_log(LOG_ERR,"bad call to dodelete()");
    exit(EXIT_FAILURE);
  }
  while (*oproplist != *nproplist) {
    if (((*oproplist)->delete) && ((*oproplist)->path)) {
      print_log(LOG_ERR,"DELETING PROPOSAL: ");
      printprop(*oproplist);
#if 1
      if(unlink((*oproplist)->path) < 0) {
        print_log(LOG_ERR,"Can't delete file: %s: %s",
                (*oproplist)->path, strerror(errno));
      }
#endif
      (*oproplist)->delete = 0;
    }
    oproplist = &((*oproplist)->next);
  }
}

static struct proposal *
prepare_outbound_proposals(void)
{
  struct proposal *prop;
  struct proposal **opropnext;
  struct proposal *oproplist = NULL;
  DIR *dirp;
  struct dirent *dp;
  char *line;
  unsigned char *cp;
  struct stat sb;
  char name[MID_MAXLEN + 1];

  opropnext = &oproplist;
  if (chdir(WL2K_OUTBOX) != 0) {
    print_log(LOG_ERR,"chdir(%s): %s", WL2K_OUTBOX, strerror(errno));
    exit(EXIT_FAILURE);
  }
  if ((dirp = opendir(".")) == NULL) {
    print_log(LOG_ERR,"opendir() -%s", strerror(errno));
    exit(EXIT_FAILURE);
  }
  while ((dp = readdir(dirp)) != NULL) {
    if (NAMLEN(dp) > MID_MAXLEN) {
      print_log(LOG_ERR,
                "warning: skipping bad filename %s in outbox directory %s",
                dp->d_name, WL2K_OUTBOX);
      continue;
    }
    strlcpy(name, dp->d_name, MID_MAXLEN + 1);
    name[NAMLEN(dp)] = '\0';
    if (stat(name, &sb)) {
      continue;
    }
    if (!(S_ISREG(sb.st_mode))) {
      continue;
    }
    if ((prop = malloc(sizeof(struct proposal))) == NULL) {
      print_log(LOG_ERR,"malloc() - %s",strerror(errno));
      exit(EXIT_FAILURE);
    }
    prop->code = 'C';
    prop->type = 'E';
    strlcpy(prop->mid, name, MID_MAXLEN + 1);
    prop->path = strdup(name);

    if ((prop->ubuf = buffer_readfile(prop->path)) == NULL) {
      print_log(LOG_ERR,"%s - %s", prop->path, strerror(errno));
      exit(EXIT_FAILURE);
    }
    prop->usize = prop->ubuf->dlen;

    if ((prop->cbuf = version_1_Encode(prop->ubuf)) == NULL) {
      print_log(LOG_ERR,"version_1_Encode() %s", strerror(errno));
      exit(EXIT_FAILURE);
    }

    prop->csize = (unsigned long) prop->cbuf->dlen;

    prop->title = NULL;
    prop->offset = 0;
    prop->accepted = 0;
    prop->delete = 0;

    buffer_rewind(prop->ubuf);
    while ((line = buffer_getline(prop->ubuf, '\n')) != NULL) {
      if (strbegins(line, "Subject:")) {
	strzapcc(line);
	cp = ((unsigned char *) line) + 8;
	while (isspace(*cp)) {
	  cp++;
	}
	if (strlen((const char *) cp) > 80) {
	  cp[80] = '\0';
	}
	prop->title = strdup((const char *) cp);
      }
      free(line);
    }

    if (prop->title == NULL) {
      prop->title = strdup("No subject");
    }

    prop->next = NULL;

    *opropnext = prop;
    opropnext = &prop->next;
  }
  closedir(dirp);

  print_log(LOG_DEBUG,"---\n%s", oproplist ? " outbound proposal list" : "");

  for (prop = oproplist; prop != NULL; prop = prop->next) {
    printprop(prop);
  }

  return oproplist;
}

/*
 * returns 0
 *   - after putting compressed
 *   - outputting FF
 *
 * returns -1
 *   - after outputting FQ
 *
 */
static int
b2outboundproposal(FILE *ifp, FILE *ofp, char *lastcommand, struct proposal **oproplist)
{
  int i;
  char *sp;
  unsigned char *cp;
  int cksum = 0;
  char *line;
  struct proposal *prop;
  char *endp;


  if (*oproplist) {
    prop = *oproplist;
    for (i = 0; i < PROPLIMIT; i++) {
      if (asprintf(&sp, "F%c %cM %s %lu %lu 0\r",
		  prop->code,
		  prop->type,
		  prop->mid,
		  prop->usize,
		  prop->csize) == -1) {
        print_log(LOG_ERR, "asprintf() - %s", strerror(errno));
	exit(EXIT_FAILURE);
      }
      print_log(LOG_DEBUG, ">%s", sp);
      resettimeout();
      if (fprintf(ofp, "%s", sp) == -1) {
        print_log(LOG_ERR, "fprintf() - %s", strerror(errno));
	exit(EXIT_FAILURE);
      }
      for (cp = (unsigned char *) sp; *cp; cp++) {
	cksum += (unsigned char) *cp;
      }
      free(sp);
      if ((prop = prop->next) == NULL) {
	break;
      }
    }
    cksum = -cksum & 0xff;
    print_log(LOG_DEBUG, ">F> %02X", cksum);
    resettimeout();
    if (fprintf(ofp, "F> %02X\r", cksum) == -1) {
      print_log(LOG_ERR, "fprintf() - %s", strerror(errno));
      exit(EXIT_FAILURE);
    }
    fflush(ofp);

    if ((line = wl2kgetline(ifp)) == NULL) {
      print_log(LOG_ERR, "connection closed");
      exit(EXIT_FAILURE);
    }
    print_log(LOG_DEBUG, "<%s", line);

    /* For line beginning: ";PM: " pending messages */
    while (strbegins(line, ";PM:")) {
      /* ignore this line & get next line */
      print_log(LOG_DEBUG, "Found WL2K extension, pending msg");

      if ((line = wl2kgetline(ifp)) == NULL) {
        print_log(LOG_ERR, "connection closed");
        exit(EXIT_FAILURE);
      }
      print_log(LOG_DEBUG, "<%s", line);
    }
    /* For line beginning: ";FW: " forwarding mail */
    while (strbegins(line, ";FW:")) {
      /* ignore this line & get next line */
      print_log(LOG_DEBUG, "Found WL2K extension, forward mail");

      if ((line = wl2kgetline(ifp)) == NULL) {
        print_log(LOG_ERR, "connection closed");
        exit(EXIT_FAILURE);
      }
      print_log(LOG_DEBUG, "<%s", line);
    }
    if (!strbegins(line, "FS ")) {
      print_log(LOG_ERR, "B2 protocol error 1");
      exit(EXIT_FAILURE);
    }
    prop = *oproplist;
    i = 0;
    cp = ((unsigned char *) line) + 3;
    while (*cp && isspace(*cp)) {
      cp++;
    }
    while (*cp && prop) {
      if (i == PROPLIMIT) {
        print_log(LOG_ERR, "B2 protocol error 2");
        exit(EXIT_FAILURE);
      }
      prop->accepted = 0;
      prop->delete = 0;
      prop->offset = 0;
      switch(*cp) {
      case 'Y': case 'y':
      case 'H': case 'h':
      case '+':
        prop->accepted = 1;
        break;
      case 'N': case 'n':
      case 'R': case 'r':
      case '-':
        prop->delete = 1;
        break;
      case 'L': case 'l':
      case '=':
        break;
      case 'A': case 'a':
      case '!':
        prop->accepted = 1;
        prop->offset = strtoul((const char *) cp, &endp, 10);
        cp = ((unsigned char *) endp) - 1;
        break;
      default:
        print_log(LOG_ERR, "B2 protocol error 3");
        exit(EXIT_FAILURE);
        break;
      }
      cp++;
      prop = prop->next;
    }

    prop = *oproplist;
    for (i = 0; i < PROPLIMIT; i++) {
      if(prop->delete == 0) {
        putcompressed(prop, ofp);
        prop->delete = 1;
      }
      if ((prop = prop->next) == NULL) {
        print_log(LOG_DEBUG, "Debug: After putcompressed prop->next = NULL, i:%d", i);
        break;
      }
    }
    *oproplist = prop;
    print_log(LOG_DEBUG_VERBOSE, "Finished proplist %d putcompressed", i);
    return 0;
  } else if (strbegins(lastcommand, "FF")) {
    print_log(LOG_DEBUG, ">FQ");
    resettimeout();
    if (fprintf(ofp, "FQ\r") == -1) {
      print_log(LOG_ERR, "fprintf() - %s", strerror(errno));
      exit(EXIT_FAILURE);
    }
    fflush(ofp);
    return -1;
  } else {
    print_log(LOG_DEBUG, ">FF");
    resettimeout();
    if (fprintf(ofp, "FF\r") == -1) {
      print_log(LOG_ERR, "fprintf() - %s", strerror(errno));
      exit(EXIT_FAILURE);
    }
    fflush(ofp);
    return 0;
  }
}

static char *
tgetline(FILE *fp, int terminator, int ignore)
{
  static struct buffer *buf = NULL;
  int c;
  int notascii_buf[256]; /* debug only */
  size_t notascii_ind = 0;


  if (buf == NULL) {
    if ((buf = buffer_new()) == NULL) {
      print_log(LOG_ERR, "buffer_new() - %s", strerror(errno));
      exit(EXIT_FAILURE);
    }
  } else {
    buffer_truncate(buf);
  }
  for (;;) {
    resettimeout();
    if ((c = fgetc(fp)) == EOF) {
      return NULL;
    }
    /* Debug only */
    if (c == 0x1e || (!isascii(c) && c != terminator && c != ignore) ) {
      notascii_buf[notascii_ind] = c;
      if(notascii_ind < 255) {
        notascii_ind++;
      }
      continue;
    }
    if (c == terminator) {
      if (buffer_addchar(buf, '\0') == -1) {
        print_log(LOG_ERR, "buffer_addchar()- %s",  strerror(errno));
        exit(EXIT_FAILURE);
      }
      if (notascii_ind > 0) {
        print_log(LOG_DEBUG, "Found non ascii response:");
        dump_hex((char *)notascii_buf, notascii_ind);
      }
      return (char *) buf->data;
    }
    if ((c != ignore) && buffer_addchar(buf, c) == -1) {
      print_log(LOG_ERR, "buffer_addchar() - %s",  strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
  return NULL;
}

char *
wl2kgetline(FILE *fp)
{
  char *cp;

  cp = tgetline(fp, '\r', '\n');
  return cp;
}

void
   send_my_sid(FILE *ofp)
{
  char sidbuf[32];

  sprintf(sidbuf, "[%s-%s-B2FIHM$]", SID_NAME, PACKAGE_VERSION);
  print_log(LOG_DEBUG, ">%s", sidbuf);
  resettimeout();
  if (fprintf(ofp, "%s\r", sidbuf) == -1) {
    print_log(LOG_ERR, "fprintf() - %s",strerror(errno));
    exit(EXIT_FAILURE);
  }
}

char *
parse_inboundsid(char *line)
{
  char *cp;
  char *inboundsid = NULL;
  char *inboundsidcodes = NULL;

  inboundsid = strdup(line);
  if ((cp = strrchr(inboundsid, '-')) == NULL) {
    print_log(LOG_ERR, "bad sid %s", inboundsid);
    exit(EXIT_FAILURE);
  }
  inboundsidcodes = strdup(cp);
  if ((cp = strrchr(inboundsidcodes, ']')) == NULL) {
    print_log(LOG_ERR, "bad sid %s", inboundsid);
    exit(EXIT_FAILURE);
  }
  *cp = '\0';
  strupper(inboundsidcodes);
  if (strstr(inboundsidcodes, "B2F") == NULL) {
    print_log(LOG_ERR,  "sid %s does not support B2F protocol", inboundsid);
    exit(EXIT_FAILURE);
  }
  print_log(LOG_DEBUG, "sid %s inboundsidcodes %s", inboundsid, inboundsidcodes);
  return (inboundsidcodes);
}

/*
 * returns from:
 *   b2outboundproposal
 *     put compressed output FF 0
 *     output FQ -1
 *   Found 'B' -1
 *   Found 'FQ' -1
 *   End of Function 0
 */
static int inbound_parser(FILE *ifp, FILE *ofp, struct proposal *nproplist, struct proposal *oproplist, char *emailaddress)
{
  int i,j;
  char *cp;

  char *endp;
  char pbuf[16];
  char *line;
  unsigned long sentcksum;
  char responsechar;
  FILE *smfp;
  char *command;
  struct buffer *mimebuf;
  int c;
  int r;
  int b2outret;

  int proposalcksum = 0;
  int proposals = 0;
  struct proposal *prop;
  struct proposal ipropary[PROPLIMIT];


  while ((line = wl2kgetline(ifp)) != NULL) {
    print_log(LOG_DEBUG, "<%s [%d]", line, strlen(line));
    if (strbegins(line, ";")) {
      /* do nothing */
    } else if (strlen(line) == 0) {
      /* do nothing */
    } else if (strbegins(line, "FC")) {
      dodelete(&oproplist, &nproplist);
      for (cp = line; *cp; cp++) {
        proposalcksum += (unsigned char) *cp;
      }
      proposalcksum += '\r'; /* bletch */
      if (proposals == PROPLIMIT) {
        print_log(LOG_ERR, "too many proposals");
        exit(EXIT_FAILURE);
      }
      if ((prop = parse_proposal(line)) == NULL) {
        print_log(LOG_ERR, "wl2kexchange() failed to parse proposal");
        exit(EXIT_FAILURE);
      }
      memcpy(&ipropary[proposals], prop, sizeof(struct proposal));
      printprop(&ipropary[proposals]);
      proposals++;
    } else if (strbegins(line, "FF")) {
      dodelete(&oproplist, &nproplist);

      print_log(LOG_DEBUG, "Debug: %s(): xmit 1", __FUNCTION__);

      if ((b2outret = b2outboundproposal(ifp, ofp, line, &nproplist)) != 0) {
        print_log(LOG_DEBUG, "Debug: %s(): ret: 0x%02x, xmit 1", b2outret, __FUNCTION__);
        return(b2outret);
      }
    } else if (strbegins(line, "B")) {
      return(-1);
    } else if (strbegins(line, "FQ")) {
      dodelete(&oproplist, &nproplist);
      return(-1);
    } else if (strbegins(line, "F>")) {
      /* Send message only option -
       *  -bail out from a session after receive proposal */
      if(gsendmsgonly_flag) {
        /* create an acceptance error */
        print_log(LOG_DEBUG, ">FF");
        resettimeout();
        if (fprintf(ofp, "FF\r") == -1) {
          print_log(LOG_ERR, "fprintf() - %s", strerror(errno));
          exit(EXIT_FAILURE);
        }
        fflush(ofp);
        continue;
      }
      proposalcksum = (-proposalcksum) & 0xff;
      sentcksum = strtoul(line + 2, &endp, 16);

      print_log(LOG_DEBUG, "sentcksum=%lX proposalcksum=%lX", sentcksum, (unsigned long) proposalcksum);
      if (sentcksum != (unsigned long) proposalcksum) {
        print_log(LOG_ERR, "proposal cksum mismatch");
        exit(EXIT_FAILURE);
      }

      print_log(LOG_DEBUG, "%d proposal%s received", proposals, ((proposals == 1) ? "" : "s"));

      if (proposals != 0) {
        strcpy(pbuf, "FS ");
        j = (int)strlen(pbuf); /* used to build up proposal response */

        /* Definiton of characters used for answers in an FS line :
         *   + or Y : Yes, message accepted
         *   - or N : No, message already received
         *   = or L : Later, already receiving this message
         *   H : Message is accepted but will be held
         *   R : Message is rejected
         *   E : There is an error in the line
         *   !offset or Aoffset : Yes, message accepted from (Offset)
         *
         * Note: If the later or Reject answer is given in successive
         * sessions the message will be dropped by the CSM
         */
        for (i = 0; i < proposals; i++) {
          ipropary[i].accepted = 0;
          if (ipropary[i].code == 'C') {
            if (check_mid(ipropary[i].mid)) {
              responsechar = 'N';
            } else {
              responsechar = 'Y';
              ipropary[i].accepted = 1;
            }
          } else {
            responsechar = 'L';
          }
          pbuf[j++] = responsechar;
        }
        pbuf[j] = '\0';
        print_log(LOG_DEBUG, ">%s", pbuf);

        resettimeout();
        if (fprintf(ofp, "%s\r", pbuf) == -1) {
          print_log(LOG_ERR, "fprintf() - %s", strerror(errno));
          exit(EXIT_FAILURE);
        }
        fflush(ofp);

        for (i = 0; i < proposals; i++) {
          if (ipropary[i].accepted != 1) {
            continue;
          }

          if ((ipropary[i].cbuf = getcompressed(ifp)) == NULL) {
            print_log(LOG_ERR, "error receiving compressed data\n");
            exit(EXIT_FAILURE);
          }

          print_log(LOG_ERR, "extracting...");
          if ((ipropary[i].ubuf = version_1_Decode(ipropary[i].cbuf)) == NULL) {
            print_log(LOG_ERR, "version_1_Decode() - %s",strerror(errno));
            exit(EXIT_FAILURE);
          }

#if 0
          if (buffer_writefile(ipropary[i].mid, ipropary[i].ubuf) != 0) {
            print_log(LOG_ERR, "buffer_writefile - %s",strerror(errno));
            exit(EXIT_FAILURE);
          }
#endif

          buffer_rewind(ipropary[i].ubuf);
          if ((mimebuf = wl2mime(ipropary[i].ubuf)) == NULL) {
            print_log(LOG_ERR, "wm2mime() failed");
            exit(EXIT_FAILURE);
          }
          /* Flush all open output streams */
          fflush(NULL);

          if (asprintf(&command, "%s %s %s", SENDMAIL, SENDMAIL_FLAGS, emailaddress) == -1) {
            print_log(LOG_ERR, "asprintf() - %s",strerror(errno));
            exit(EXIT_FAILURE);
          }
          print_log(LOG_DEBUG, "calling sendmail for delivery: %s", command);

          if ((smfp = popen(command, "w")) == NULL) {
            print_log(LOG_ERR, "popen() - %s",strerror(errno));
            exit(EXIT_FAILURE);
          }
          free(command);
          buffer_rewind(mimebuf);
          while ((c = buffer_iterchar(mimebuf)) != EOF) {
            if (putc(c, smfp) == EOF) {
              exit(EXIT_FAILURE);
            }
          }
          if ((r = pclose(smfp)) != 0) {
            if (r < 0) {
              print_log(LOG_ERR, "pclose() - %s", strerror(errno));
              /* Check if process terminated before pclose was called
               * errno 10 = ECHILD No child processes
               */
              if(errno != ECHILD) {
                print_log(LOG_ERR, "pclose exiting on error");
                exit(EXIT_FAILURE);
              } else {
                print_log(LOG_ERR, "pclose error, continuing after No child processes");
              }
            } else {
              print_log(LOG_ERR, "sendmail failed - %s", strerror(errno));
              exit(EXIT_FAILURE);
            }
          }
          print_log(LOG_DEBUG, "delivery completed");
          record_mid(ipropary[i].mid);
          buffer_free(ipropary[i].ubuf);
          ipropary[i].ubuf = NULL;
          buffer_free(ipropary[i].cbuf);
          ipropary[i].cbuf = NULL;
        }
      }
      proposals = 0;
      proposalcksum = 0;
      print_log(LOG_DEBUG, "Debug: %s(): xmit 2", __FUNCTION__);
      if ((b2outret = b2outboundproposal(ifp, ofp, line, &nproplist)) != 0) {
        print_log(LOG_DEBUG, "Debug: %s(): ret: 0x%02x, xmit 2", b2outret, __FUNCTION__);
        return(b2outret);
      }
    } else if (line[strlen(line - 1)] == '>') {
      dodelete(&oproplist, &nproplist);

      print_log(LOG_DEBUG, "Debug: %s(): xmit 3", __FUNCTION__);
      if ((b2outret = b2outboundproposal(ifp, ofp, line, &nproplist)) != 0) {
        print_log(LOG_DEBUG, "Debug: %s(): ret: 0x%02x, xmit 3", b2outret, __FUNCTION__);
        return(b2outret);
      }
    } else {
      print_log(LOG_ERR, "unrecognized command (len %lu): /%s/",
                (unsigned long) strlen(line), line);
      /* Debug only */
      dump_hex(line, strlen(line));
      exit(EXIT_FAILURE);
    }
  }
  if (line == NULL) {
    print_log(LOG_ERR, "%s(): Lost connection. 4",__FUNCTION__);
    exit(EXIT_FAILURE);
  }
  return(0);
}

#ifdef USE_SECURE_LOGIN
void
compute_secure_login_response(char *challenge, char *response, char *password)
{
  char *hash_input;
  unsigned char hash_sig[16];
  size_t m, n;
  int i, pr;
  char pr_str[20];

  m = strlen(challenge) + strlen(password);
  n = m + sizeof(sl_salt);
  hash_input = (char*)malloc(n);
  strcpy(hash_input, challenge);
  strcat(hash_input, password);
  strupper(hash_input);
  memcpy(hash_input+m, sl_salt, sizeof(sl_salt));
  md5_buffer(hash_input, (unsigned int)n, hash_sig);
  free(hash_input);

  pr = hash_sig[3] & 0x3f;
  for (i=2; i>=0; i--)
    pr = (pr << 8) | hash_sig[i];

  sprintf(pr_str, "%08d", pr);
  n = strlen(pr_str);
  if (n > 8)
    strcpy(response, pr_str+(n-8));
  else
    strcpy(response, pr_str);
}

int send_secure_login_response(FILE *ofp, char *challenge, char *sl_pass)
{
  int sent_pr = 0;

  if ((strlen(challenge) > 0) && (sl_pass != NULL)) {
    char response[9];
    send_my_sid(ofp);
    compute_secure_login_response(challenge, response, sl_pass);
    print_log(LOG_DEBUG, ">;PR: %s", response);
    resettimeout();
    if (fprintf(ofp, ";PR: %s\r", response) == -1) {
      print_log(LOG_ERR, "fprintf() - %s",strerror(errno));
      exit(EXIT_FAILURE);
    }
    sent_pr = 1;
  }
  return(sent_pr);
}
#endif /* USE_SECURE_LOGIN */

static char *handshake(FILE *ifp, FILE *ofp, char *sl_pass,char *mycall, char * yourcall, int opropcount)
{
  static char *line;
  char *inboundsidcodes = NULL;
  char challenge[9];
  challenge[0] = 0;


  while ((line = wl2kgetline(ifp)) != NULL) {
    print_log(LOG_DEBUG, "<%s", line);
    if (strchr(line, '[')) {
      inboundsidcodes = parse_inboundsid(line);
    } else if (strbegins(line, ";")) {
      /* parse secure login challenge */
      if (!strncmp(line, ";PQ: ", 5)) {
        if (strlen(line+5) == 8) {
          strcpy(challenge, line+5);
          print_log(LOG_DEBUG, "Challenge received: %s", challenge);
        }
      }
    } else if (line[strlen(line) - 1] == '>') {
      int sent_pr = 0;
      if (inboundsidcodes == NULL) {
        print_log(LOG_ERR, "inboundsidcodes not set");
        exit(EXIT_FAILURE);
      }
      sent_pr = send_secure_login_response(ofp, challenge, sl_pass);
      if (strchr(inboundsidcodes, 'I')) {
        /* Identify: with QTC (I have telegrams) */
        print_log(LOG_DEBUG, ">; %s DE %s QTC %d", yourcall, mycall, opropcount);
        resettimeout();
        if (fprintf(ofp, "; %s DE %s QTC %d\r", yourcall, mycall, opropcount) == -1) {
          print_log(LOG_ERR, "fprintf() - %s",strerror(errno));
          exit(EXIT_FAILURE);
        }
      }
      if (!sent_pr) {
        send_my_sid(ofp);
      }
      break;
    }
  }
  if (line == NULL) {
    print_log(LOG_ERR, "%s(): Lost connection. 1",__FUNCTION__);
    exit(EXIT_FAILURE);
  }
  return(line);
}

void
wl2k_exchange(char *mycall, char *yourcall, FILE *ifp, FILE *ofp, char *emailaddress, char *sl_pass)
{
  static char *line;
  struct proposal *prop;
  struct proposal *oproplist;
  struct proposal *nproplist;
  int opropcount = 0;
  long unsigned int oprop_usize = 0;
  long unsigned int oprop_csize = 0;

  if (expire_mids() == -1) {
    print_log(LOG_ERR, "expire_mids() failed");
    exit(EXIT_FAILURE);
  }

  oproplist = prepare_outbound_proposals();

  for (prop = oproplist; prop; prop = prop->next) {
    opropcount++;
    oprop_usize += prop->usize;
    oprop_csize += prop->csize;
  }

  line = handshake(ifp, ofp, sl_pass, mycall, yourcall, opropcount);
  nproplist = oproplist;

  print_log(LOG_DEBUG, "Debug ex: handshake FINISHED");

  if (b2outboundproposal(ifp, ofp, line, &nproplist) != 0) {
    return;
  }
  print_log(LOG_DEBUG, "Debug: outbound parser finished");

  fflush(ofp);

  print_log(LOG_DEBUG, "Debug: Start inbound parser");
  inbound_parser(ifp, ofp, nproplist, oproplist, emailaddress);
}
