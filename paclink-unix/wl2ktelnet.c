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
#include <sys/types.h>
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
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#if HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#if HAVE_NETDB_H
# include <netdb.h>
#endif

#include "compat.h"
#include "timeout.h"
#include "wl2k.h"
#include "strutil.h"

static void usage(void);

static void
usage(void)
{
  fprintf(stderr, "usage:  %s mycall yourcall hostname port timeoutsecs password\n", getprogname());
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
#define YOURCALL argv[2]
#define HOSTNAME argv[3]
#define PORT argv[4]
#define TIMEOUTSECS argv[5]
#define PASSWORD argv[6]

  setlinebuf(stdout);

  if (argc != 7) {
    usage();
    exit(EXIT_FAILURE);
  }

  strupper(MYCALL);
  strupper(YOURCALL);

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
#if HAVE_SOCKADDR_IN_SIN_LEN
  s_in.sin_len = sizeof(struct sockaddr_in);
#endif
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
    fprintf(stderr, "Connection closed by foreign host.\n");
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
    fprintf(stderr, "Connection closed by foreign host.\n");
    exit(EXIT_FAILURE);
  }

  wl2kexchange(MYCALL, YOURCALL, fp);

  fclose(fp);
  exit(EXIT_SUCCESS);
  return 1;
}
