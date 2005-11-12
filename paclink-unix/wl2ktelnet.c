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

char *wl2kgetline(FILE *fp);
void usage(void);

char *
wl2kgetline(FILE *fp)
{
  static char buf[2048];
  int i;
  int c;

  for (i = 0; i < 2047; i++) {
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
usage(void)
{
  fprintf(stderr, "usage:  %s mycall hostname port timeout password\n", progname);
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
  char *line;
  char *inboundsid = NULL;
  char *inboundsidcodes = NULL;
  const char *sid = "[PaclinkUNIX-1.0-B2FHM]";
  int proposals = 0;
  int i;
  int c;
  char *cp;
  int timeout;

#define MYCALL argv[1]
#define HOSTNAME argv[2]
#define PORT argv[3]
#define TIMEOUT argv[4]
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
  timeout = (int) strtol(TIMEOUT, &endp, 10);
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
  if (connect(s, (struct sockaddr *) &s_in, sizeof(struct sockaddr_in)) != 0) {
    close(s);
    perror("connect()");
    exit(EXIT_FAILURE);
  }

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
    printf("#%s#\n", line);
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

      i = proposals;
#if 1
      while ((c = fgetc(fp)) != EOF) {
	putchar(c);
      }
#endif
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
