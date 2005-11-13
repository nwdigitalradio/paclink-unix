#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

#include <stdio.h>
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "strupper.h"
#include "wl2k.h"
#include "timeout.h"

#define WL2KBUF 2048

static int getrawchar(FILE *fp);
static int getcompressed(FILE *fp, FILE *ofp);
static struct proposal *parse_proposal(char *propline);

static int
getrawchar(FILE *fp)
{
  int c;

  resettimeout();
  c = fgetc(fp);
  return c;
}

#define CHRNUL 0
#define CHRSOH 1
#define CHRSTX 2
#define CHREOT 4

static int
getcompressed(FILE *fp, FILE *ofp)
{
  int c;
  int len;
  int i;
  unsigned char title[81];
  unsigned char offset[7];
  int cksum = 0;

  c = getrawchar(fp);
  if (c != CHRSOH) {
    return WL2K_COMPRESSED_BAD;
  }
  len = getrawchar(fp);
  title[80] = '\0';
  for (i = 0; i < 80; i++) {
    c = getrawchar(fp);
    len--;
    title[i] = c;
    if (c == CHRNUL) {
      ungetc(c, fp);
      len++;
      break;
    }
  }
  c = getrawchar(fp);
  len--;
  if (c != CHRNUL) {
    return WL2K_COMPRESSED_BAD;
  }
  printf("title: %s\n", title);
  offset[6] = '\0';
  for (i = 0; i < 6; i++) {
    c = getrawchar(fp);
    len--;
    offset[i] = c;
    if (c == CHRNUL) {
      ungetc(c, fp);
      len++;
      break;
    }
  }
  c = getrawchar(fp);
  len--;
  if (c != CHRNUL) {
    return WL2K_COMPRESSED_BAD;
  }
  printf("offset: %s\n", offset);
  if (len != 0) {
    return WL2K_COMPRESSED_BAD;
  }
  if (strcmp(offset, "0") != 0) {
    return WL2K_COMPRESSED_BAD;
  }

  for (;;) {
    c = getrawchar(fp);
    switch (c) {
    case CHRSTX:
      printf("STX\n");
      len = getrawchar(fp);
      if (len == 0) {
	len = 256;
      }
      printf("len %d\n", len);
      while (len--) {
	c = getrawchar(fp);
	if (fputc(c, ofp) == EOF) {
	  perror("fputc()");
	  return WL2K_COMPRESSED_BAD;
	}
	cksum = (cksum + c) % 256;
      }
      break;
    case CHREOT:
      printf("EOT\n");
      c = getrawchar(fp);
      cksum = (cksum + c) % 256;
      if (cksum != 0) {
	fprintf(stderr, "bad cksum\n");
	return WL2K_COMPRESSED_BAD;
      }
      return WL2K_COMPRESSED_GOOD;
      break;
    default:
      fprintf(stderr, "unexpected character in compressed stream\n");
      return WL2K_COMPRESSED_BAD;
      break;
    }
  }
  return WL2K_COMPRESSED_BAD;
}

struct proposal {
  char code;
  char type;
  char id[13];
  unsigned int usize;
  unsigned int csize;
};

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
      fprintf(stderr, "malformed proposal 1\n");
      return NULL;
    }
    prop.type = *cp++;
    if ((prop.type != 'C') && (prop.type != 'E')) {
      fprintf(stderr, "malformed proposal 2\n");
    }
    if (*cp++ != 'M') {
      fprintf(stderr, "malformed proposal 3\n");
    }
    if (*cp++ != ' ') {
      fprintf(stderr, "malformed proposal 4\n");
      return NULL;
    }
    for (i = 0; i < 12; i++) {
      prop.id[i] = *cp++;
      if (prop.id[i] == ' ') {
	prop.id[i] = '\0';
	cp--;
	break;
      } else {
	if (prop.id[i] == '\0') {
	  fprintf(stderr, "malformed proposal 5\n");
	  return NULL;
	}
      }
    }
    prop.id[12] = '\0';
    if (*cp++ != ' ') {
      fprintf(stderr, "malformed proposal 6\n");
      return NULL;
    }
    prop.usize = (unsigned int) strtoul(cp, &endp, 10);
    cp = endp;
    if (*cp++ != ' ') {
      fprintf(stderr, "malformed proposal 7\n");
      return NULL;
    }
    prop.csize = (unsigned int) strtoul(cp, &endp, 10);
    cp = endp;
    if (*cp != ' ') {
      fprintf(stderr, "malformed proposal 8\n");
      return NULL;
    }
    return &prop;
    break;
  case 'A':
  case 'B':
  default:
    fprintf(stderr, "unsupported proposal type %c\n", prop.code);
    return NULL;
    break;
  }

}

char *
wl2kgetline(FILE *fp)
{
  static char buf[WL2KBUF];
  int i;
  int c;

  for (i = 0; i < WL2KBUF; i++) {
    resettimeout();
    if ((c = fgetc(fp)) == EOF) {
      return NULL;
    }
    if (c == '\r') {
      buf[i] = '\0';
      return buf;
    }
    buf[i] = c;
  }
  return NULL;
}

void
wl2kexchange(FILE *fp)
{
  char *cp;
  int proposals = 0;
  int i;
  const char *sid = "[PaclinkUNIX-1.0-B2FHM]";
  char *inboundsid = NULL;
  char *inboundsidcodes = NULL;
  char *line;
  struct proposal *prop;
  struct proposal proplist[5];
  char sfn[17] = "";
  FILE *sfp;
  int fd = -1;
  char *cmd;

  while ((line = wl2kgetline(fp)) != NULL) {
    printf("%s\n", line);
    if (line[0] == '[') {
      inboundsid = strdup(line);
      if ((cp = strrchr(inboundsid, '-')) == NULL) {
	fprintf(stderr, "bad sid %s\n", inboundsid);
	exit(EXIT_FAILURE);
      }
      inboundsidcodes = strdup(cp);
      if ((cp = strrchr(inboundsidcodes, ']')) == NULL) {
	fprintf(stderr, "bad sid %s\n", inboundsid);
	exit(EXIT_FAILURE);
      }
      *cp = '\0';
      strupper(inboundsidcodes);
      if (strchr(inboundsidcodes, 'F') == NULL) {
	fprintf(stderr, "sid %s does not support FBB protocol\n", inboundsid);
	exit(EXIT_FAILURE);
      }
      break;
    }
  }
  if (line == NULL) {
    fprintf(stderr, "Lost connection.\n");
    exit(EXIT_FAILURE);
  }

  while ((line = wl2kgetline(fp)) != NULL) {
    printf("%s\n", line);
    if (strchr(line, '>')) {
      fprintf(fp, "%s\r\n", sid);
      printf("%s\n", sid);
      fprintf(fp, "FF\r\n");
      printf("FF\n");
      break;
    }
  }
  if (line == NULL) {
    fprintf(stderr, "Lost connection.\n");
    exit(EXIT_FAILURE);
  }

  proposals = 0;

  while ((line = wl2kgetline(fp)) != NULL) {
    printf("%s\n", line);
    if (strncmp("FA", line, 2) == 0) {
      proposals++;
    } else if (strncmp("FB", line, 2) == 0) {
      proposals++;
    } else if (strncmp("FC", line, 2) == 0) {
      if ((prop = parse_proposal(line)) == NULL) {
	fprintf(stderr, "failed to parse proposal\n");
	exit(EXIT_FAILURE);
      }
      printf("proposal code %c type %c id %s usize %u csize %u\n",
	     prop->code, prop->type, prop->id, prop->usize, prop->csize);
      memcpy(&proplist[proposals], prop, sizeof(struct proposal));
      printf("proposal code %c type %c id %s usize %u csize %u\n",
	     proplist[proposals].code,
	     proplist[proposals].type,
	     proplist[proposals].id,
	     proplist[proposals].usize,
	     proplist[proposals].csize);
      proposals++;
    } else if (strncmp("F>", line, 2) == 0) {
      printf("%d proposals\n", proposals);
      fprintf(fp, "FS ");
      printf("FS ");
      for (i = 0; i < proposals; i++) {
	putc('+', fp);
	putchar('+');
      }
      fprintf(fp, "\r\n");
      printf("\n");

      for (i = 0; i < proposals; i++) {
	strlcpy(sfn, "/tmp/wl2k.XXXXXX", sizeof(sfn));
	if ((fd = mkstemp(sfn)) == -1 ||
	    (sfp = fdopen(fd, "w+")) == NULL) {
	  if (fd != -1) {
	    unlink(sfn);
	    close(fd);
	  }
	  perror(sfn);
	  exit(EXIT_FAILURE);
	}

	if (getcompressed(fp, sfp) != WL2K_COMPRESSED_GOOD) {
	  fprintf(stderr, "error receiving compressed data\n");
	  exit(EXIT_FAILURE);
	}
	if (fclose(sfp) != 0) {
	  fprintf(stderr, "error closing compressed data\n");
	  exit(EXIT_FAILURE);
	}
	printf("extracting...\n");
	if (asprintf(&cmd, "./lzhuf_1 d1 %s %s", sfn, proplist[i].id) == -1) {
	  perror("asprintf()");
	  exit(EXIT_FAILURE);
	}
	if (system(cmd) != 0) {
	  fprintf(stderr, "error uncompressing received data\n");
	  exit(EXIT_FAILURE);
	}
	free(cmd);
	printf("\n");
	printf("Finished!\n");
	unlink(sfn);
#if 0
	while ((line = wl2kgetline(fp)) != NULL) {
	  printf("%s\n", line);
	  if (line[0] == '\x1a') {
	    printf("yeeble\n");
	  }
	}
	if (line == NULL) {
	  fprintf(stderr, "Lost connection.\n");
	  exit(EXIT_FAILURE);
	}
#endif
      }
      fprintf(fp, "FF\r\n");
      printf("FF\n");
    } else if (strncmp("FQ", line, 2) == 0) {
      return;
    }
  }
  if (line == NULL) {
    fprintf(stderr, "Lost connection.\n");
    exit(EXIT_FAILURE);
  }
}
