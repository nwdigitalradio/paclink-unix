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

#include "strupper.h"
#include "wl2k.h"
#include "timeout.h"

#define WL2KBUF 2048

static int getrawchar(FILE *fp);
static int getcompressed(FILE *fp, FILE *ofp);

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
	  printf("write error\n");
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
	printf("bad cksum\n");
	return WL2K_COMPRESSED_BAD;
      }
      return WL2K_COMPRESSED_GOOD;
      break;
    default:
      printf("huh?\n");
      return WL2K_COMPRESSED_BAD;
      break;
    }
  }
  return WL2K_COMPRESSED_BAD;
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
  FILE *ofp;
  char *cp;
  int proposals = 0;
  int i;
  const char *sid = "[PaclinkUNIX-1.0-B2FHM]";
  char *inboundsid = NULL;
  char *inboundsidcodes = NULL;
  char *line;

  while ((line = wl2kgetline(fp)) != NULL) {
    printf("%s\n", line);
    if (line[0] == '[') {
      inboundsid = strdup(line);
      if ((cp = strrchr(inboundsid, '-')) == NULL) {
	printf("bad sid %s\n", inboundsid);
	exit(EXIT_FAILURE);
      }
      inboundsidcodes = strdup(cp);
      if ((cp = strrchr(inboundsidcodes, ']')) == NULL) {
	printf("bad sid %s\n", inboundsid);
	exit(EXIT_FAILURE);
      }
      *cp = '\0';
      strupper(inboundsidcodes);
      if (strchr(inboundsidcodes, 'F') == NULL) {
	printf("sid %s does not support FBB protocol\n", inboundsid);
	exit(EXIT_FAILURE);
      }
      break;
    }
  }
  if (line == NULL) {
    printf("Connection closed by foreign host.\n");
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
    printf("Connection closed by foreign host.\n");
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
	ofp = fopen("bin.out", "w");
	if (ofp == NULL) {
	  perror("fopen()");
	  exit(EXIT_FAILURE);
	}
	if (getcompressed(fp, ofp) != WL2K_COMPRESSED_GOOD) {
	  printf("error receiving compressed data\n");
	  exit(EXIT_FAILURE);
	}
	if (fclose(ofp) != 0) {
	  printf("error closing compressed data\n");
	  exit(EXIT_FAILURE);
	}
	system("ls -l bin.out");
	printf("extracting...\n");
	if (system("./lzhuf_1 d1 bin.out txt.out") != 0) {
	  printf("error uncompressing received data\n");
	  exit(EXIT_FAILURE);
	}
	printf("displaying...\n");
	system("cat txt.out");
	printf("\n");
	printf("Finished!\n");
#if 0
	while ((line = wl2kgetline(fp)) != NULL) {
	  printf("%s\n", line);
	  if (line[0] == '\x1a') {
	    printf("yeeble\n");
	  }
	}
	if (line == NULL) {
	  printf("Connection closed by foreign host.\n");
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
    printf("Connection closed by foreign host.\n");
    exit(EXIT_FAILURE);
  }
}
