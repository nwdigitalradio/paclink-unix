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

#include "progname.h"
#include "timeout.h"
#include "wl2k.h"

static void usage(void);

static void
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
  char *line;
  int timeoutsecs;

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

  wl2kexchange(fp);

  fclose(fp);
  exit(EXIT_SUCCESS);
  return 1;
}
