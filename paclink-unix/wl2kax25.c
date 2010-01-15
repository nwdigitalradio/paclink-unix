/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2; c-brace-offset: -2; c-argdecl-indent: 2 -*- */

/*  paclink-unix client for the Winlink 2000 ham radio email system.
 *
 *  Copyright 2009 Nicholas S. Castellano <n2qz@arrl.net> and others,
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

#if HAVE_NETAX25_AX25_H
#include <netax25/ax25.h>
#else
#include <netax25/kernel_ax25.h>
#endif

#include <netax25/axlib.h>
#include <netax25/axconfig.h>
#include <netax25/nrconfig.h>
#include <netax25/rsconfig.h>


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

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <gmime/gmime.h>
#include <poll.h>

#include "compat.h"
#include "timeout.h"
#include "wl2k.h"
#include "strutil.h"

#define BUFMAX 256

static void usage(void);

static void
usage(void)
{
  fprintf(stderr, "usage:  %s mycall yourcall ax25port timeoutsecs emailaddress\n", getprogname());
}

/**
 * Function: main
 *
 * Note that the calling convention for this function will most probably change
 * in the near future.
 *
 * For now, the parameters are:
 * mycall :  my call sign
 * yourcall: callsign for the RMS
 * ax25port: name of the ax25 port to use (e.g., sm0)
 * timeoutsecs: timeout in seconds
 * emailaddress: email address where the retrieved message will be send via sendmail
 *
 * The yourcall parameter does not support a path yet.
 */
int
main(int argc, char *argv[])
{
   char *endp;
   int s;
   FILE *fp;
   int timeoutsecs;
   unsigned int addrlen = 0;
   union {
      struct full_sockaddr_ax25 ax25;
      struct sockaddr_rose rose;
   } sockaddr;
   char *dev;
   pid_t procID;
   int sv[2];
   int ready;
   struct pollfd fds[2];
   unsigned char axread[BUFMAX], axwrite[BUFMAX];

#define MYCALL  argv[1]
#define YOURCALL argv[2]
#define PORTNAME argv[3]
#define TIMEOUTSECS argv[4]
#define EMAILADDRESS argv[5]

   char *portname = (char *) PORTNAME;

   g_mime_init(0);

   setlinebuf(stdout);

   if (argc != 6) {
      usage();
      exit(EXIT_FAILURE);
   }

   strupper((unsigned char *) MYCALL);
   strupper((unsigned char *) YOURCALL);

   timeoutsecs = (int) strtol(TIMEOUTSECS, &endp, 10);
   if (*endp != '\0') {
      usage();
      exit(EXIT_FAILURE);
   }

   if (socketpair(PF_UNIX, SOCK_STREAM, 0, sv))
   {
      perror("socketpair");
      exit(EXIT_FAILURE);
   }
   // Fork a process
   if ((procID = fork())) {
      // Parent processing
      if (-1 == procID) {
         fprintf(stderr, "fork\n");
         exit(EXIT_FAILURE);
      }

      close(sv[1]);

      // Begin AX25 socket code
      if (ax25_config_load_ports() == 0)
         fprintf(stderr, "wl2kax25: no AX.25 port data configured\n");

      if (portname != NULL) {
         if ((dev = ax25_config_get_dev(portname)) == NULL) {
            fprintf(stderr, "wl2kax25: invalid port name - %s\n",
                  portname);
            return(EXIT_FAILURE);
         }
      }

      if ((s = socket(AF_AX25, SOCK_SEQPACKET, 0)) == -1) {
         perror("socket");
         printf("%d\n", __LINE__);
         exit(EXIT_FAILURE);
      }
      ax25_aton(ax25_config_get_addr(portname), &sockaddr.ax25);
      ax25_aton(MYCALL, &sockaddr.ax25);
      if (sockaddr.ax25.fsa_ax25.sax25_ndigis == 0) {
         ax25_aton_entry(ax25_config_get_addr(portname),
               sockaddr.ax25.fsa_digipeater[0].
               ax25_call);
         sockaddr.ax25.fsa_ax25.sax25_ndigis = 1;
      }
      sockaddr.ax25.fsa_ax25.sax25_family = AF_AX25;
      addrlen = sizeof(struct full_sockaddr_ax25);

      if (bind(s, (struct sockaddr *) &sockaddr, addrlen) == -1) {
         perror("bind");
         close(s);
         exit(EXIT_FAILURE);
      }


      if (ax25_aton(YOURCALL, &sockaddr.ax25) < 0) {
         close(s);
         perror("ax25_aton()");
         exit(EXIT_FAILURE);
      }
      sockaddr.rose.srose_family = AF_AX25;
      addrlen = sizeof(struct full_sockaddr_ax25);


      if (connect(s, (struct sockaddr *) &sockaddr, addrlen) != 0) {
         close(s);
         perror("connect()");
         exit(EXIT_FAILURE);
      }
      // End AX25 socket code

      resettimeout();

      printf("Connected.\n");

      fds[0].fd = s;
      fds[0].events = POLLIN;
      fds[1].fd = sv[0];
      fds[1].events = POLLIN;

      // poll here and feed to the ax25 socket.
      // Data must be chunked to the appropriate size
      for (;;) {
         ready = poll(fds, sizeof(fds)/sizeof(struct pollfd), -1);

         if (-1 == ready) {
            if (EINTR == errno)
               break;
            perror("poll");
            exit(EXIT_FAILURE);
         }

         // Inbound
         if (fds[0].revents & POLLIN) {
            unsigned int len = 0;
            len = read(fds[0].fd, axread, sizeof(axread));
            if (len > 0 )
               write(fds[1].fd, axread, len);
	    elsif (len == 0) {
	       printf("EOF on ax25 socket, exiting...\n");
	       exit(EXIT_FAILURE);
	    }
         }

         // Outbound
         if (fds[1].revents & POLLIN) {
            unsigned int len = 0;
            len = read(fds[1].fd, axwrite, sizeof(axwrite));
            if (len > 0 )
               write(fds[0].fd, axwrite, len);
	    elsif (len == 0) {
	       printf("EOF on child fd, terminating communications loop.\n");
	       break;
	    }
         }
      }

      printf("Closing ax25 connection\n");

      g_mime_shutdown();
      close(sv[0]);
      close(s);
      exit(EXIT_SUCCESS);
      return 1;
   }
   else
   {
      // Child processing
      printf("Child process\n");
      close(sv[0]);

      if ((fp = fdopen(sv[1], "r+b")) == NULL) {
         close(sv[1]);
         perror("fdopen()");
         _exit(EXIT_FAILURE);
      }

      setbuf(fp, NULL);

      /*
       * The messages are exchanged in this call
       *
       * TODO: The sid sent by the client should contain an NXX,
       *       where NXX represents N followed by two digits of SSID.
       *       This allows the RMS to find the correct registered
       *       user in case the SSID has been changed in the network.
       */

      printf("Child process calling wl2kexchange()\n");
      wl2kexchange(MYCALL, YOURCALL, fp, EMAILADDRESS);
      fclose(fp);
      printf("Child process exiting\n");
      _exit(EXIT_SUCCESS);
   }
}
