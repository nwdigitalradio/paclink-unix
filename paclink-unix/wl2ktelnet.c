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

#ifndef bool
#include <stdbool.h>
#endif /* bool */

#include <getopt.h>
#include <gmime/gmime.h>

#include "compat.h"
#include "conf.h"
#include "timeout.h"
#include "wl2k.h"
#include "strutil.h"

/*
 * Globals
 */
int gverbose_flag=FALSE;

/*
 * Config parameters struct
 */
typedef struct _wl2ktelnet_config {
  char *mycall;
  char *targetcall;
  char *hostname;
  unsigned short  hostport;
  char *password;
  char *emailaddr;
  unsigned int timeoutsecs;
  int  bVerbose;
}cfg_t;

static bool loadconfig(int argc, char **argv, cfg_t *cfg);
static void usage(void);
static void displayversion(void);
static void displayconfig(cfg_t *cfg);

/**
 * Function: main
 *
 * The calling convention for this function is:
 *
 * wl2ktelnet -c targetcall -t timeoutsecs -e emailaddress -p password HOSTNAME PORT
 *
 * The parameters are:
 * mycall :  my call sign, which MUST be set in wl2k.conf
 * targetcall: callsign for the telnet server (WL2K)
 * timeoutsecs: timeout in seconds
 * emailaddress: email address where the retrieved message will be sent via sendmail
 * password of the telnet host (CMSTelnet)
 * hostname of the telnet host (server.winlink.org)
 * port to be used for the telnet host (8772)
 *
 */
int
main(int argc, char *argv[])
{
  struct hostent *host;
  int s;
  struct sockaddr_in s_in;
  FILE *fp;
  char *line;
  static cfg_t cfg;

  loadconfig(argc, argv, &cfg);

  g_mime_init(0);

  setlinebuf(stdout);

  if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  memset(&s_in, '\0', sizeof(struct sockaddr_in));
#if HAVE_SOCKADDR_IN_SIN_LEN
  s_in.sin_len = sizeof(struct sockaddr_in);
#endif
  s_in.sin_family = AF_INET;
  s_in.sin_addr.s_addr = inet_addr(cfg.hostname);
  if ((int) s_in.sin_addr.s_addr == -1) {
    host = gethostbyname(cfg.hostname);
    if (host) {
      memcpy(&s_in.sin_addr.s_addr, host->h_addr, (unsigned) host->h_length);
    } else {
      herror(cfg.hostname);
      exit(EXIT_FAILURE);
    }
  }
  s_in.sin_port = htons((unsigned short)cfg.hostport);
  printf("Connecting to %s %d ...\n", cfg.hostname, cfg.hostport);

  settimeout(cfg.timeoutsecs);
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
      fprintf(fp, ".%s\r\n", cfg.mycall);
      printf(" %s\n", cfg.mycall);
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
      fprintf(fp, "%s\r\n", cfg.password);
      break;
    }
  }
  if (line == NULL) {
    fprintf(stderr, "Connection closed by foreign host.\n");
    exit(EXIT_FAILURE);
  }

  wl2kexchange(cfg.mycall, cfg.targetcall, fp, cfg.emailaddr);

  fclose(fp);
  g_mime_shutdown();
  exit(EXIT_SUCCESS);
  return 1;
}

/*
 * Print usage information and exit
 *  - does not return
 */
static void
usage(void)
{
  printf("Usage:  %s [options] [HOSTNAME PORT]\n", getprogname());
  printf("  -c  --target-call   Set callsign to call\n");
  printf("  -p  --password      Set password for login\n");
  printf("  -t  --timeout       Set timeout in seconds\n");
  printf("  -e  --email-address Set your e-mail address\n");
  printf("  -v  --version       Display program version only\n");
  printf("  -V  --verbose       Print verbose messages\n");
  printf("  -C  --configuration Display configuration only\n");
  printf("  -h  --help          Display this usage info\n");
  exit(EXIT_SUCCESS);
}

/*
 * Display package version & repository version of this program.
 */
static void
displayversion(void)
{
  char *verstr;

  printf("%s  %s ", getprogname(), PACKAGE_VERSION);

  /* Check verbose flag for displaying gmime version */
  if(gverbose_flag) {
    printf("Using gmime version %d.%d.%d\n",
           gmime_major_version, gmime_minor_version, gmime_micro_version);
  }
}

/*
 * Display configuration parameters
 *  parsed from defaults, config file & command line
 */
static void
displayconfig(cfg_t *cfg)
{

  printf("Using this config:\n");

  if(cfg->mycall) {
    printf("  My callsign: %s\n", cfg->mycall);
  }

  if(cfg->targetcall) {
    printf("  Target callsign: %s\n", cfg->targetcall);
  }

  if(cfg->hostname) {
    printf("  Host name: %s\n", cfg->hostname);
  }

  printf("  Host port: %d\n", cfg->hostport);

  if(cfg->password) {
    printf("  Login password: %s\n", cfg->password);
  }

  printf("  Timeout: %d\n", cfg->timeoutsecs);

  if(cfg->emailaddr) {
    printf("  Email address: %s\n", cfg->emailaddr);
  }
  printf("  Flags: verbose = %s\n", cfg->bVerbose ? "On" : "Off");
}

/* Load these 7 config parameters:
 * mycall targetcall hostname port timeoutsecs password emailaddress
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
  static const char *short_options = "hVvCc:t:e:p:";
  /* long options */
  static struct option long_options[] =
  {
    /* These options set a flag. */
    {"verbose",       no_argument,  &gverbose_flag, TRUE},
    {"config",        no_argument,  &displayconfig_flag, TRUE},
    /* These options don't set a flag.
    We distinguish them by their indices. */
    {"version",       no_argument,       NULL, 'v'},
    {"help",          no_argument,       NULL, 'h'},
    {"target-call",   required_argument, NULL, 'c'},
    {"timeout",       required_argument, NULL, 't'},
    {"email-address", required_argument, NULL, 'e'},
    {"password",      required_argument, NULL, 'p'},
    {NULL, no_argument, NULL, 0} /* array termination */
  };

  /* get a temporary buffer to build strings */
  cfgbuf = (char *)malloc(256);
  if(cfgbuf == NULL) {
    fprintf(stderr, "%s: loadconfig, out of memory\n", getprogname());
    return(FALSE);
  }

  /*
   * Initialize default config
   */

  /* use either cuserid(NULL) or  getenv("LOGNAME"),
   *  - getlogin() does NOT work */
  sprintf(cfgbuf, "%s@localhost", cuserid(NULL) );
  config->emailaddr = strdup(cfgbuf);

  strcpy(cfgbuf, DFLT_TELNET_PASSWORD);
  config->password = strdup(cfgbuf);

  strcpy(cfgbuf, DFLT_TELNET_CALL);
  config->targetcall = strdup(cfgbuf);

  strcpy(cfgbuf, DFLT_TELNET_HOSTNAME);
  config->hostname = strdup(cfgbuf);

  free(cfgbuf);

  config->mycall = NULL;
  config->timeoutsecs = DFLT_TIMEOUTSECS;
  config->hostport = DFLT_TELNET_PORT;
  config->bVerbose = FALSE;

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

  if ((cfgbuf = conf_get(fileconf, "password")) != NULL) {
    config->password = cfgbuf;
  }

  if ((cfgbuf = conf_get(fileconf, "hostname")) != NULL) {
    config->hostname = cfgbuf;
  }

  if ((cfgbuf = conf_get(fileconf, "hostport")) != NULL) {
    config->hostport = (unsigned short) strtol(cfgbuf, &endp, 10);
    if (*endp != '\0') {
      usage();  /* does not return */
    }
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
      case 'v':   /* set display version flag */
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
      case 't':   /* set time out in seconds */
        config->timeoutsecs = (unsigned int) strtol(optarg, &endp, 10);
        if (*endp != '\0') {
          usage(); /* does not return */
        }
        break;
      case 'e':   /* set email address */
        config->emailaddr = optarg;
        break;
      case 'p':   /* set password */
        config->password = optarg;
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

  /* set verbose flag here in case long option was used */
  config->bVerbose = gverbose_flag;

  /*
   * Get positional command line args hostname hostport
   */

  if(optind < argc) {
    config->hostname = argv[optind++];
  }

  if(optind < argc) {
    config->hostport = (unsigned short) strtol(argv[optind], &endp, 10);
    if (*endp != '\0') {
      usage();  /* does not return */
    }
  }

  if(bDisplayVersion_flag) {
    displayversion();
    exit(EXIT_SUCCESS);
  }

  /* test for required parameters */
  if(config->targetcall == NULL) {
    fprintf(stderr,  "%s: Need to specify target callsign\n", getprogname() );
    bRequireConfig_pass = FALSE;
  }
  if(config->hostname == NULL) {
    fprintf(stderr,  "%s: Need to specify hostname\n", getprogname() );
    bRequireConfig_pass = FALSE;
  }
  if(config->hostport == 0) {
    fprintf(stderr,  "%s: Need to specify hostport\n", getprogname() );
    bRequireConfig_pass = FALSE;
  }

  strupper((char *) config->mycall);
  strupper((char *) config->targetcall);

  /* If display config flag set just dump the configuration & exit */
  if(displayconfig_flag) {
    displayversion();
    displayconfig(config);
    exit(EXIT_SUCCESS);
  }

  /* Check configuration requirements */
  if(!bRequireConfig_pass) {
    usage();  /* does not return */
  }

  /* Be verbose */
  if(config->bVerbose) {
    displayversion();
    displayconfig(config);
  }

  return(TRUE);
}
