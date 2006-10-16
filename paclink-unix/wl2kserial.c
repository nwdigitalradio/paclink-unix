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
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include <termios.h>

#include <gmime/gmime.h>

#include "compat.h"
#include "timeout.h"
#include "wl2k.h"
#include "strutil.h"

#if defined(EXTA) && !defined(B19200)
# define B19200 EXTA
#endif

#if defined(EXTB) && !defined(B38400)
# define B38400 EXTB
#endif

const struct baudrate {
  const char *asc;
  int num;
} baudrates[] = {
  {"50", B50},
  {"75", B75},
  {"110", B110},
  {"134", B134},
  {"150", B150},
  {"200", B200},
  {"300", B300},
  {"600", B600},
  {"1200", B1200},
  {"1800", B1800},
  {"2400", B2400},
  {"4800", B4800},
#ifdef B7200
  {"7200", B7200},
#endif
  {"9600", B9600},
#ifdef B14400
  {"14400", B14400},
#endif
#ifdef B19200
  {"19200", B19200},
#endif
#ifdef B28800
  {"28800", B28800},
#endif
#ifdef B38400
  {"38400", B38400},
#endif
#ifdef B57600
  {"57600", B57600},
#endif
#ifdef B76800
  {"76800", B76800},
#endif
#ifdef B115200
  {"115200", B115200},
#endif
#ifdef B230400
  {"230400", B230400},
#endif
#ifdef B460800
  {"460800", B460800},
#endif
#ifdef B921600
  {"921600", B921600},
#endif
  {NULL, 0}
};

static void usage(void);

static void
usage(void)
{
  fprintf(stderr, "usage:  %s mycall yourcall device baud timeoutsecs emailaddress\n", getprogname());
}

int
main(int argc, char *argv[])
{
  char *endp;
  int fd;
  FILE *fp;
  char *line;
  int timeoutsecs;
  struct termios init;
  struct termios t;
  int flags;
  struct baudrate *bp;
  int baud;

#define MYCALL argv[1]
#define YOURCALL argv[2]
#define DEVICE argv[3]
#define BAUD argv[4]
#define TIMEOUTSECS argv[5]
#define EMAILADDRESS argv[6]

  g_mime_init(0);

  setlinebuf(stdout);

  if (argc != 7) {
    usage();
    exit(EXIT_FAILURE);
  }

  strupper(MYCALL);
  strupper(YOURCALL);

  baud = B0;
  for (bp = baudrates; bp->asc != NULL; bp++) {
    if (strcmp(bp->asc, BAUD) == 0) {
      baud = bp->num;
    }
  }
  if (baud == B0) {
    usage();
    fprintf(stderr, "invalid baud '%s' -- valid baud rates are: ", BAUD);
    for (bp = baudrates; bp->asc != NULL; bp++) {
      fprintf(stderr, "%s ", bp->asc);
    }
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
  }

  timeoutsecs = (int) strtol(TIMEOUTSECS, &endp, 10);
  if (*endp != '\0') {
    usage();
    exit(EXIT_FAILURE);
  }

  /* open device */
  settimeout(timeoutsecs);

  printf("Opening %s ...\n", DEVICE);
  if ((fd = open(DEVICE, O_RDWR | O_NONBLOCK)) == -1) {
    perror("open()");
    exit(EXIT_FAILURE);
  }

  flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

  if ((tcgetattr(fd, &init)) == -1) {
    perror("tcgetattr()");
    exit(EXIT_FAILURE);
  }
  if ((tcgetattr(fd, &t)) == -1) {
    perror("tcgetattr()");
    exit(EXIT_FAILURE);
  }
  if ((cfsetspeed(&t, B9600)) == -1) {
    perror("cfsetspeed()");
    exit(EXIT_FAILURE);
  }
#if 0
  cfmakeraw(&t);
#endif

#ifndef CRTSCTS
# define CRTSCTS 0
#endif
#ifndef CCTS_OFLOW
# define CCTS_OFLOW 0
#endif
#ifndef CCTS_IFLOW
# define CCTS_IFLOW 0
#endif
#ifndef MDMBUF
# define MDMBUF 0
#endif

  t.c_iflag &= ~(INPCK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IMAXBEL);
  t.c_iflag |= (IGNBRK | IGNPAR);

  t.c_oflag &= ~OPOST;

  t.c_cflag &= ~(CSIZE | PARENB | CRTSCTS | CCTS_OFLOW | CCTS_IFLOW | MDMBUF);
  t.c_cflag |= (CS8 | CREAD | CLOCAL);

  t.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);

  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;
  
  if ((tcsetattr(fd, TCSAFLUSH, &t)) == -1) {
    perror("tcsetattr()");
    exit(EXIT_FAILURE);
  }
  
  resettimeout();

  printf("Connected.\n");

  if ((fp = fdopen(fd, "r+b")) == NULL) {
    close(fd);
    perror("fdopen()");
    exit(EXIT_FAILURE);
  }
  setbuf(fp, NULL);

  fprintf(fp, "\r\n");
  while ((line = wl2kgetline(fp)) != NULL) {
    printf("%s\n", line);
    if (strstr(line, "cmd:")) {
      fprintf(fp, "mycall %s\r", MYCALL);
      printf("mycall %s\n", MYCALL);
      fprintf(fp, "tones 4\r");
      printf("tones 4\n");
      fprintf(fp, "chobell 0\r");
      printf("chobell 0\n");
      fprintf(fp, "lfignore 1\r");
      printf("lfignore 1\n");
      break;
    }
    fprintf(fp, "\r\n");
  }
  if (line == NULL) {
    fprintf(stderr, "Connection closed by foreign host.\n");
    exit(EXIT_FAILURE);
  }

#if 0
  while ((line = wl2kgetline(fp)) != NULL) {
    printf("%s\n", line);
    if (strstr(line, "Mycall:")) {
      break;
    }
  }
  if (line == NULL) {
    fprintf(stderr, "Connection closed by foreign host.\n");
    exit(EXIT_FAILURE);
  }
#endif

  while ((line = wl2kgetline(fp)) != NULL) {
    printf("%s\n", line);
    if (strstr(line, "cmd:")) {
      fprintf(fp, "c %s\r", YOURCALL);
      printf("c %s\n", YOURCALL);
      break;
    }
  }
  if (line == NULL) {
    fprintf(stderr, "Connection closed by foreign host.\n");
    exit(EXIT_FAILURE);
  }

  wl2kexchange(MYCALL, YOURCALL, fp, EMAILADDRESS);

  fclose(fp);
  g_mime_shutdown();
  exit(EXIT_SUCCESS);
  return 1;
}
