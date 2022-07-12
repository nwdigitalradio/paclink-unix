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
#include <sys/wait.h>
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
#if HAVE_CTYPE_H
# include <ctype.h>
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
#if HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifndef bool
#include <stdbool.h>
#endif /* bool */

#include <getopt.h>
#include <gmime/gmime.h>
#include <poll.h>
#include <signal.h>

#include "compat.h"
#include "conf.h"
#include "timeout.h"
#include "wl2k.h"
#include "strutil.h"
#include "buffer.h"

#define BUFMAXIN  (256)
#define BUFMAXOUT (512)

/*
 * Globals
 */
unsigned char axread[BUFMAXIN];
unsigned char axwrite[BUFMAXOUT];

#define DFLTPACLEN   255   /* default packet length */
size_t paclen = DFLTPACLEN;
int gverbose_flag=FALSE;
int gsendmsgonly_flag=FALSE;

/* Flag set in sigalrm */
extern sig_atomic_t timeout_flag;

static bool loadconfig(int argc, char **argv, cfg_t *cfg);
static void usage(void);
static void displayversion(void);
static void displayconfig(cfg_t *cfg);
static void exitcleanup(cfg_t *cfg);
bool isax25connected(int s);

/**
 * Function: main
 *
 * The calling convention for this function is:
 *
 * wl2kax25 -c targetcall -a ax25port -t timeoutsecs -e emailaddress
 *
 * The parameters are:
 * mycall :  my call sign, which MUST be set in wl2k.conf
 * targetcall: callsign for the RMS
 * ax25port: name of the ax25 port to use (e.g., sm0)
 * timeoutsecs: timeout in seconds
 * emailaddress: email address where the retrieved message will be sent via sendmail
 *
 * The targetcall parameter does not support a path yet.
 */
int
main(int argc, char *argv[])
{
  int s;
  FILE *fp;
  unsigned int addrlen = 0;
  union {
    struct full_sockaddr_ax25 ax25;
    struct sockaddr_rose rose;
  } sockaddr;
  char *dev;
  pid_t procID;
  int childstatus;
  int childexitstatus;
  int sv[2];
  int ready;
  struct pollfd fds[2];
  ssize_t len;
  unsigned char *pbuf;
  ssize_t byteswritten;
  static cfg_t cfg;

  loadconfig(argc, argv, &cfg);

#if (GMIME_VER > 2)
  g_mime_init();
#else
  g_mime_init(0);
#endif

  setlinebuf(stdout);

  if (socketpair(PF_UNIX, SOCK_STREAM, 0, sv))
  {
    perror("socketpair");
    exitcleanup(&cfg);
    exit(EXIT_FAILURE);
  }

  // Begin AX25 socket code
  if (ax25_config_load_ports() == 0)
    fprintf(stderr, "wl2kax25: no AX.25 port data configured\n");

  if (cfg.ax25port != NULL) {
    if ((dev = ax25_config_get_dev(cfg.ax25port)) == NULL) {
      fprintf(stderr, "wl2kax25: invalid port name - %s\n",
              cfg.ax25port);
      exitcleanup(&cfg);
      return(EXIT_FAILURE);
    }
  }

  if ((s = socket(AF_AX25, SOCK_SEQPACKET, 0)) == -1) {
    perror("socket");
    printf("%d\n", __LINE__);
    exitcleanup(&cfg);
    exit(EXIT_FAILURE);
  }
  ax25_aton(ax25_config_get_addr(cfg.ax25port), &sockaddr.ax25);
  ax25_aton(cfg.mycall, &sockaddr.ax25);
  if (sockaddr.ax25.fsa_ax25.sax25_ndigis == 0) {
    ax25_aton_entry(ax25_config_get_addr(cfg.ax25port),
                    sockaddr.ax25.fsa_digipeater[0].
                    ax25_call);
    sockaddr.ax25.fsa_ax25.sax25_ndigis = 1;
  }
  sockaddr.ax25.fsa_ax25.sax25_family = AF_AX25;
  addrlen = sizeof(struct full_sockaddr_ax25);

  if (bind(s, (struct sockaddr *) &sockaddr, addrlen) == -1) {
    perror("bind");
    close(s);
    exitcleanup(&cfg);
    exit(EXIT_FAILURE);
  }

  if (ax25_aton(cfg.targetcall, &sockaddr.ax25) < 0) {
    close(s);
    perror("ax25_aton()");
    exitcleanup(&cfg);
    exit(EXIT_FAILURE);
  }
  sockaddr.rose.srose_family = AF_AX25;
  addrlen = sizeof(struct full_sockaddr_ax25);

  settimeout(cfg.timeoutsecs);
  if (connect(s, (struct sockaddr *) &sockaddr, addrlen) != 0) {
    close(s);
    perror("connect()");
    exitcleanup(&cfg);
    exit(EXIT_FAILURE);
  }
  unsettimeout();

  printf("Connected to AX.25 stack\n"); fflush(stdout);
  cfg.ax25sock=s;
  // End AX25 socket code

  // Fork a process
  if ((procID = fork())) {
    // Parent processing
    if (-1 == procID) {
      fprintf(stderr, "fork\n");
      exitcleanup(&cfg);
      exit(EXIT_FAILURE);
    }

    close(sv[1]);

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
	close(s);
        perror("poll");
        exitcleanup(&cfg);
        exit(EXIT_FAILURE);
      }

      // Inbound
      if (fds[0].revents & POLLIN) {

#ifdef NOT_NOW
              resettimeout();
#endif
        len = read(fds[0].fd, axread, sizeof(axread));
#ifdef TIMEOUT_DEV
        printf("DEBUG: Inbound read %ld\n", len);
#endif
        if ( len > 0 ) {

          pbuf = axread;

          while(len > 0) {
#ifdef NOT_NOW
            resettimeout();
#endif
#ifdef TIMEOUT_DEV
            printf("DEBUG: Inbound write %d\n", len);
#endif
            byteswritten = write(fds[1].fd, pbuf, MIN(paclen, (size_t)len));

            if (byteswritten == 0 || (byteswritten < 0 && errno != EAGAIN)) {
              fprintf(stderr,"%s error on inbound write: %s)\n",
                getprogname(), strerror(errno));
              break;
            }
            pbuf += byteswritten;
            len -=    byteswritten;
          }

				} else if (len == 0) {
					close(s);
          printf("EOF on ax25 socket, exiting...\n");
          exitcleanup(&cfg);
          exit(EXIT_FAILURE);
        }
      }

      // Outbound
      if (fds[1].revents & POLLIN) {

#ifdef NOT_NOW
        resettimeout();
#endif
        len = read(fds[1].fd, axwrite, sizeof(axwrite));
#ifdef TIMEOUT_DEV
        printf("DEBUG: Outbound read %d\n", len);
#endif
        if (len > 0 ) {

          pbuf = axwrite;

          while(len > 0) {

#ifdef NOT_NOW
            resettimeout();
#endif
#ifdef TIMEOUT_DEV
            printf("DEBUG: Outbound write %d\n", len);
#endif
            byteswritten = write(fds[0].fd, pbuf, MIN(paclen, (size_t)len));
            if (byteswritten == 0 || (byteswritten < 0 && errno != EAGAIN)) {
              fprintf(stderr,"%s error on outbound write: %s)\n",
                getprogname(), strerror(errno));
              break;
            }
            pbuf += byteswritten;
            len -=    byteswritten;
          }

        }   else if (len == 0) {
          printf("EOF on child fd, terminating communications loop.\n");
          break;
        }
      }
      if (timeout_flag != 1) {
        printf("timeout flag set!!\n");
        break;
      }
    } /* End of forever loop */

    childstatus = -1;
    childexitstatus = EXIT_SUCCESS;
    printf("Closing ax25 connection\n");
    if ( waitpid(procID, &childstatus, 0) == -1 ) {
      perror("waitpid failed");
    } else {
      if ( WIFEXITED(childstatus) ) {
	childexitstatus = WEXITSTATUS(childstatus);
	printf("Child exit status: %d\n", childexitstatus);
      }
    }

    {
      time_t start_time, current_time;
      bool bax25conn;

      start_time = time(NULL); /* get current time in seconds */

      if ((bax25conn=isax25connected(s)) == TRUE) {
        printf("Waiting for AX25 peer ... ");
        while(isax25connected(s)){
          current_time = time(NULL);
          if (difftime(current_time, start_time) > 12) {
            break;
          }
        }
        if (isax25connected(s)) {
          printf("timeout\n");
        }else {
          printf("disconnected\n");
        }
      }
    }

    g_mime_shutdown();
    close(sv[0]);
    close(s);
    exitcleanup(&cfg);
    exit(childexitstatus); /* exit parent process */
    return 1;
  }
  else
  {
    // Child processing
    printf("Child process\n");
    close(s);
    close(sv[0]);

    if ((fp = fdopen(sv[1], "r+b")) == NULL) {
      close(sv[1]);
      perror("fdopen()");
      exitcleanup(&cfg);
      _exit(EXIT_FAILURE);
    }

    /* set buf size to paclen */
    setvbuf(fp, NULL, _IOFBF, paclen);

    /*
     * The messages are exchanged in this call
     *
     * TODO: The sid sent by the client should contain an NXX,
     *       where NXX represents N followed by two digits of SSID.
     *       This allows the RMS to find the correct registered
     *       user in case the SSID has been changed in the network.
     */

    if(cfg.bVerbose) {
            printf("Child process calling wl2kexchange()\n");
    }
    settimeout(cfg.timeoutsecs);
    wl2k_exchange(&cfg, fp, fp);

    fclose(fp);
    printf("Child process exiting\n");
    exitcleanup(&cfg);
    _exit(EXIT_SUCCESS);
  }
}

/*
 * Print usage information and exit
 *  - does not return
 */
static void
usage(void)
{
  printf("Usage:  %s -c target-call [options]\n", getprogname());
  printf("  -c  --target-call   Set callsign to call\n");
  printf("  -a  --ax25port      Set AX25 port to use\n");
  printf("  -P  --wl2k-passwd   Set password for WL2K secure login\n");
  printf("  -t  --timeout       Set timeout in seconds\n");
  printf("  -e  --email-address Set your e-mail address\n");
  printf("  -s  --send-only     Send messages only\n");
  printf("  -V  --verbose       Print verbose messages\n");
  printf("  -v  --version       Display program version only\n");
  printf("  -C  --configuration Display configuration only\n");
  printf("  -h  --help          Display this usage info\n");
  exit(EXIT_SUCCESS);
}

/*
 * Display package version of this program.
 */
static void
displayversion(void)
{

  printf("%s  %s", getprogname(), PACKAGE_VERSION);

  /* Check verbose flag for displaying gmime version */
  if(gverbose_flag) {
    printf(", Using gmime version %d.%d.%d",
           gmime_major_version, gmime_minor_version, gmime_micro_version);
  }
  printf("\n");
}

/*
 * Display configuration parameters
 *  parsed from defaults, config file & command line
 */
static void
displayconfig(cfg_t *cfg)
{

  printf("%s: Using this config:\n", getprogname());

  if(cfg->mycall) {
    printf("  My callsign: %s\n", cfg->mycall);
  }

  if(cfg->targetcall) {
    printf("  Target callsign: %s\n", cfg->targetcall);
  }

  if(cfg->ax25port) {
    printf("  Ax25 port: %s\n", cfg->ax25port);
  }

#ifdef DEBUG /* Do not display Winlink password */
  if(cfg->wl2k_password) {
          printf("  WL2K secure login password: %s\n", cfg->wl2k_password);
  }
#endif /* DEBUG end */

  printf("  Timeout: %d\n", cfg->timeoutsecs);

  if(cfg->emailaddr) {
    printf("  Email address: %s\n", cfg->emailaddr);
  }
  if(cfg->gridsquare) {
    printf("  Grid square: %s\n", cfg->gridsquare);
  }
  printf("  Flags: verbose = %s, send-only = %s\n",
         cfg->bVerbose  ? "On" : "Off",
         cfg->bSendonly ? "On" : "Off");
}

/* Load these 5 config parameters:
 * mycall targetcall ax25port timeoutsecs emailaddress
 */
static bool
loadconfig(int argc, char **argv, cfg_t *config)
{
  struct conf *fileconf;
  char *endp;
  int next_option;
  int option_index = 0; /* getopt_long stores the option index here. */
  char *cfgbuf;
  static int displayconfig_flag=FALSE;
  bool bDisplayVersion_flag = FALSE;
  bool bRequireConfig_pass = TRUE;
  /* short options */
  static const char *short_options = "hVvsCc:t:e:a:P:";
  /* long options */
  static struct option long_options[] =
  {
    /* These options set a flag. */
    {"verbose",       no_argument,  &gverbose_flag, TRUE},
    {"config",        no_argument,  &displayconfig_flag, TRUE},
    {"send-only",     no_argument,  &gsendmsgonly_flag, TRUE},
    /* These options don't set a flag.
    We distinguish them by their indices. */
    {"version",       no_argument,       NULL, 'v'},
    {"help",          no_argument,       NULL, 'h'},
    {"target-call",   required_argument, NULL, 'c'},
    {"timeout",       required_argument, NULL, 't'},
    {"email-address", required_argument, NULL, 'e'},
    {"ax25port",      required_argument, NULL, 'a'},
    {"wl2k-passwd",   required_argument, NULL, 'P'},
    {NULL, no_argument, NULL, 0} /* array termination */
  };

  /* get a temporary buffer to build strings */
  cfgbuf = (char *)malloc(256);
  if(cfgbuf == NULL) {
    fprintf(stderr, "%s: loadconfig, out of memory\n", getprogname());
    exit(EXIT_FAILURE);
  }

  /*
   * Initialize default config
   */

  /* use either cuserid(NULL) or  getenv("LOGNAME"),
   *  - getlogin() does NOT work */
  sprintf(cfgbuf, "%s@localhost", cuserid(NULL) );
  config->emailaddr = strdup(cfgbuf);
  free(cfgbuf);

  config->mycall = NULL;
  config->targetcall = NULL;
  config->pathbuf = NULL;
  config->wl2k_password = NULL;
  config->timeoutsecs = DFLT_TIMEOUTSECS;
  config->ax25port = NULL;
  config->bVerbose = FALSE;
  config->bSendonly = FALSE;
  config->gridsquare = NULL;

  /*
   * Get config from config file
   */

  fileconf = conf_read();
  if ((config->mycall = conf_get(fileconf, "mycall")) == NULL) {
    fprintf(stderr, "%s: failed to read mycall from configuration file\n", getprogname());
    exit(EXIT_FAILURE);
  }

  if ((cfgbuf = conf_get(fileconf, "timeout")) != NULL) {
    config->timeoutsecs = (unsigned int) strtol(cfgbuf, &endp, 10);
    if (*endp != '\0') {
      usage();  /* does not return */
    }
  }

  if ((cfgbuf = conf_get(fileconf, "email")) != NULL) {
    config->emailaddr = cfgbuf;
  }

  if ((cfgbuf = conf_get(fileconf, "ax25port")) != NULL) {
    config->ax25port = cfgbuf;
  }

  if ((cfgbuf = conf_get(fileconf, "wl2k-password")) != NULL) {
    config->wl2k_password = cfgbuf;
  }

  if ((cfgbuf = conf_get(fileconf, "gridsquare")) != NULL) {
    config->gridsquare = cfgbuf;
  }

  /*
   * Get config from command line
   */

  opterr = 0;
  option_index = 0;
  next_option = getopt_long (argc, argv, short_options,
           long_options, &option_index);

  while( next_option != -1 ) {

    switch (next_option)
    {
      case 0:   /* long option without a short arg */
        /* If this option sets a flag, do nothing else now. */
        if (long_options[option_index].flag != 0)
          break;
        fprintf (stderr, "Debug: option %s", long_options[option_index].name);
        if (optarg)
          fprintf (stderr," with arg %s", optarg);
        fprintf (stderr,"\n");
        break;
      case 'v': /* set display version flag */
        bDisplayVersion_flag = TRUE;
        break;
      case 'V':   /* set verbose flag */
        gverbose_flag = TRUE;
        break;
      case 'c':   /* set callsign to contact */
        config->targetcall = optarg;
        break;
      case 'C':   /* set display config flag */
        displayconfig_flag = TRUE;
        break;
      case 's':   /* set send message only flag */
        gsendmsgonly_flag = TRUE;
        break;
      case 't':   /* set time out in seconds */
        config->timeoutsecs = (unsigned int) strtol(optarg, &endp, 10);
        if (*endp != '\0') {
          usage(); /* does not return */
        }
        break;
      case 'e':   /* set email address */
        config->emailaddr = optarg;
        break;
      case 'P':   /* set secure login password */
        config->wl2k_password = optarg;
        break;
      case 'a':   /* set ax25 port */
        config->ax25port = optarg;
        break;
      case 'h':
        usage();  /* does not return */
        break;
      case '?':
        if (isprint (optopt)) {
          fprintf (stderr, "%s: Unknown option `-%c'.\n",
            getprogname(), optopt);
        } else {
          fprintf (stderr,"%s: Unknown option character `\\x%x'.\n",
            getprogname(), optopt);
        }
        /* fall through */
      default:
        usage();  /* does not return */
        break;
    }

    next_option = getopt_long (argc, argv, short_options,
             long_options, &option_index);
  }

  /* set send only flag here in case long option was used */
  config->bSendonly = gsendmsgonly_flag;
  /* set verbose flag here in case long option was used */
  config->bVerbose = gverbose_flag;

  /* If there are more args left on command line parse them as radio path */
  if (optind < argc) {
    struct buffer *pathBuf;

    if( (pathBuf=buffer_new()) == NULL ) {
      fprintf(stderr, "%s: Failure getting new buffer\n", getprogname());
      exit(EXIT_FAILURE);
    }

    if (config->targetcall != NULL) {
      buffer_setstring(pathBuf, (unsigned char *)config->targetcall);
    }

    while(optind < argc) {
      buffer_addstring(pathBuf, (unsigned char *) " ");
      buffer_addstring(pathBuf, (unsigned char *)argv[optind]);
      optind++;
    }
    /* terminate the radio call path buffer */
    buffer_addchar(pathBuf, '\0');

    if(gverbose_flag) {
      printf("%s: Call Path: %s\n", getprogname(), pathBuf->data);
    }
    config->targetcall = (char *)pathBuf->data;
    config->pathbuf = pathBuf;
  }

  if(bDisplayVersion_flag) {
    displayversion();
    exitcleanup(config);
    exit(EXIT_SUCCESS);
  }

  /* test for required parameters */
  if(config->targetcall == NULL) {
    fprintf(stderr,  "%s: Need to specify target callsign\n", getprogname() );
    bRequireConfig_pass = FALSE;
  }
  if(config->ax25port == NULL) {
    fprintf(stderr,  "%s: Need to specify ax25 port\n", getprogname() );
    bRequireConfig_pass = FALSE;
  }

  /* display warning if gridsquare not set */
  if(config->gridsquare == NULL) {
    fprintf (stderr,"%s: WARNING: gridsquare not specified in config file\n", getprogname());
  }

  strupper((char *) config->mycall);
  strupper((char *) config->targetcall);

  /* If display config flag set just dump the configuration & exit */
  if(displayconfig_flag) {
    displayversion();
    displayconfig(config);
    exitcleanup(config);
    exit(EXIT_SUCCESS);
  }

  /* Check configuration requirements */
  if(!bRequireConfig_pass) {
    exitcleanup(config);
    usage();  /* does not return */
  }

  /* Be verbose */
  if(config->bVerbose) {
    displayversion();
    displayconfig(config);
  }

  return(TRUE);
}

/*
 * Clean up all the dead chickens before we bail
 */
static void exitcleanup(cfg_t *config)
{
  if(config->pathbuf != NULL) {
    buffer_free(config->pathbuf);
    config->pathbuf = NULL;
  }
}
void disconnect(void) {
}
