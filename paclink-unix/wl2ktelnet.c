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
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#include "strupper.h"
#include "progname.h"
#include "timeout.h"

char *wl2kgetline(FILE *fp);
int getrawchar(FILE *fp);
int getcompressed(FILE *fp, FILE *ofp);
void usage(void);

#define WL2KBUF 2048

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

int
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

#define COMPRESSED_GOOD 0
#define COMPRESSED_BAD 1

int
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
    return COMPRESSED_BAD;
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
    return COMPRESSED_BAD;
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
    return COMPRESSED_BAD;
  }
  printf("offset: %s\n", offset);
  if (len != 0) {
    return COMPRESSED_BAD;
  }
  if (strcmp(offset, "0") != 0) {
    return COMPRESSED_BAD;
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
	  return COMPRESSED_BAD;
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
	return COMPRESSED_BAD;
      }
      return COMPRESSED_GOOD;
      break;
    default:
      printf("huh?\n");
      return COMPRESSED_BAD;
      break;
    }
  }
  return COMPRESSED_BAD;
}

void
usage(void)
{
  fprintf(stderr, "usage:  %s mycall hostname port timeoutsecs password\n", progname);
}

int
main(int argc, char *argv[])
{
  char *endp;
  struct hostent *host;
  int s;
  struct sockaddr_in s_in;
  int port;
  FILE *fp;
  FILE *ofp;
  char *line;
  char *inboundsid = NULL;
  char *inboundsidcodes = NULL;
  const char *sid = "[PaclinkUNIX-1.0-B2FHM]";
  int proposals = 0;
  int i;
  char *cp;
  unsigned int timeoutsecs;

#define MYCALL argv[1]
#define HOSTNAME argv[2]
#define PORT argv[3]
#define TIMEOUTSECS argv[4]
#define PASSWORD argv[5]

  setlinebuf(stdout);

  if (argc != 6) {
    usage();
    exit(EXIT_FAILURE);
  }
  port = (int) strtol(PORT, &endp, 10);
  if (*endp != '\0') {
    usage();
    exit(EXIT_FAILURE);
  }
  timeoutsecs = (int) strtol(TIMEOUTSECS, &endp, 10);
  if (*endp != '\0') {
    usage();
    exit(EXIT_FAILURE);
  }
  if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  memset(&s_in, '\0', sizeof(struct sockaddr_in));
  s_in.sin_len = sizeof(struct sockaddr_in);
  s_in.sin_family = AF_INET;
  s_in.sin_addr.s_addr = inet_addr(HOSTNAME);
  if ((int) s_in.sin_addr.s_addr == -1) {
    host = gethostbyname(HOSTNAME);
    if (host) {
      memcpy(&s_in.sin_addr.s_addr, host->h_addr, (unsigned) host->h_length);
    } else {
      herror(HOSTNAME);
      exit(EXIT_FAILURE);
    }
  }
  s_in.sin_port = htons(port);
  printf("Connecting to %s %s ...\n", HOSTNAME, PORT);

  settimeout(timeoutsecs);
  if (connect(s, (struct sockaddr *) &s_in, sizeof(struct sockaddr_in)) != 0) {
    close(s);
    perror("connect()");
    exit(EXIT_FAILURE);
  }
  resettimeout();

  printf("Connected.\n");

  if ((fp = fdopen(s, "r+b")) == NULL) {
    close(s);
    perror("fdopen()");
    exit(EXIT_FAILURE);
  }

  while ((line = wl2kgetline(fp)) != NULL) {
    printf("%s", line);
    if (strncmp("Callsign", line, 8) == 0) {
      fprintf(fp, ".%s\r\n", MYCALL);
      printf(" %s\n", MYCALL);
      break;
    }
    putchar('\n');
  }
  if (line == NULL) {
    printf("Connection closed by foreign host.\n");
    exit(EXIT_FAILURE);
  }

  while ((line = wl2kgetline(fp)) != NULL) {
    printf("%s\n", line);
    if (strncmp("Password", line, 8) == 0) {
      fprintf(fp, "%s\r\n", PASSWORD);
      break;
    }
  }
  if (line == NULL) {
    printf("Connection closed by foreign host.\n");
    exit(EXIT_FAILURE);
  }

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
	if (getcompressed(fp, ofp) != COMPRESSED_GOOD) {
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
      goto out;
    }
  }
  if (line == NULL) {
    printf("Connection closed by foreign host.\n");
    exit(EXIT_FAILURE);
  }

 out:
  fclose(fp);
  exit(EXIT_SUCCESS);
  return 1;
}
