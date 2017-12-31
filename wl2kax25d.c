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

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef __RCSID
__RCSID("$Id$");
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

#ifndef bool
#include <stdbool.h>
#endif /* bool */

#include <getopt.h>
#include <gmime/gmime.h>
#include <poll.h>

#include "compat.h"
#include "conf.h"
#include "timeout.h"
#include "wl2k.h"
#include "strutil.h"
#include "printlog.h"

/*
 * Globals
 */
int gverbose_flag=FALSE;
int gsendmsgonly_flag=FALSE;

#define DFLTPACLEN   255   /* default packet length */
size_t paclen = DFLTPACLEN;

/*
 * Config parameters struct
 */
typedef struct _rmsax25_config {
  char *mycall;
  char *targetcall;
  char *ax25port;
  char *emailaddr;
  char *gridsquare;
  char *welcomemsg;
  unsigned int timeoutsecs;
  int  bVerbose;
  int  bSendonly;
}cfg_t;

static bool loadconfig(int argc, char **argv, cfg_t *cfg);
static void usage(void);
static void displayversion(void);
static void displayconfig(cfg_t *cfg);
static void print_esc_string (const char *str, bool bXlate);
static int print_esc_char (char c, bool bXlate);

/**
 * Function: main
 *
 * The calling convention for this function from /etc/ax25/ax25d.conf:
 *
 * wl2kax25d -c %U -a %d -t timeoutsecs -e emailaddress
 *
 * The parameters are:
 * mycall : my call sign, which MUST be set in wl2k.conf
 * -c %U  : callsign of remote station in upper case without the SSID
 * -a %d  : ax25port: name of the ax25 port that the connection is on (e.g., sm0)
 * -t timeoutsecs: timeout in seconds
 * -e emailaddress: email address where the retrieved message will be sent via sendmail
 *
 * See man ax25d.conf
 *
 * The targetcall parameter does not support a path yet.
 */
int
main(int argc, char *argv[])
{
  static cfg_t cfg;
  char *pStr;

  loadconfig(argc, argv, &cfg);

  print_log(LOG_DEBUG,"*** Incoming call from %s", cfg.targetcall);

  g_mime_init(0);

#if 1
  {
    int opts;

    opts = fcntl(STDIN_FILENO, F_GETFL);
    if (opts < 0) {
      print_log(LOG_ERR, "can not open ax25 stack fcntl(F_GETFL)");
      exit(EXIT_FAILURE);
    }
    if (opts | O_NONBLOCK) {
      print_log(LOG_DEBUG, "Input already set to NON block");
    } else {
      print_log(LOG_DEBUG, "Input set to BLOCK");
    }
  }
#endif

  /* set buf size to paclen */
  setvbuf(stdout, NULL, _IOFBF, paclen);

  settimeout(cfg.timeoutsecs);

  /*
   * The messages are exchanged in this call
   *
   * TODO: The sid sent by the client should contain an NXX,
   *       where NXX represents N followed by two digits of SSID.
   *       This allows the RMS to find the correct registered
   *       user in case the SSID has been changed in the network.
   */

  print_log(LOG_INFO, "[%s-%s-B2FIHM$]", SID_NAME, PACKAGE_VERSION);
  fprintf(stdout, "[%s-%s-B2FIHM$]\r", SID_NAME, PACKAGE_VERSION);

  /*
   * Use defaults if gridsquare or welcome message are not defined in
   * config file.
   */
  if(cfg.gridsquare) {
    pStr = cfg.gridsquare;
  } else {
    pStr ="XXyyzz";
  }
  /* Wonder what this all is for/means?? */
  print_log(LOG_INFO, "(am|em:h1,g:%s)", pStr);
  fprintf(stdout, "(am|em:h1,g:%s)\r", pStr);

  if(cfg.welcomemsg) {
    pStr = cfg.welcomemsg;
  } else {
    pStr="Welcome\r";
  }
  print_log(LOG_INFO, "Banner: %s", pStr);
  /* Output the banner msg to stdout parsing any escape '\' chars */
  print_esc_string(pStr, FALSE);

  wl2kd_exchange(cfg.mycall, cfg.targetcall, stdin, stdout, cfg.emailaddr, NULL);

  unsettimeout();
  print_log(LOG_DEBUG, "Exiting\n");

  exit(EXIT_SUCCESS);
}

/*
 * Prints usage information and exits
 *  - does not return
 */
static void
usage(void)
{
  printf("Usage:  %s -c target-call [options]\n", getprogname());
  printf("  -c  --target-call   Set callsign to call\n");
  printf("  -a  --ax25port      Set AX25 port to use\n");
  printf("  -t  --timeout       Set timeout in seconds\n");
  printf("  -e  --email-address Set your e-mail address\n");
  printf("  -s  --send-only     Send messages only\n");
  printf("  -V  --verbose       Print verbose messages to log file\n");
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

  printf("%s  %s ", getprogname(), PACKAGE_VERSION);

  /* Check verbose flag for displaying gmime version */
  if(gverbose_flag) {
    printf("Using gmime version %d.%d.%d",
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

  printf("  Timeout: %d\n", cfg->timeoutsecs);

  if(cfg->emailaddr) {
    printf("  Email address: %s\n", cfg->emailaddr);
  }
  if(cfg->gridsquare) {
    printf("  Grid square: %s\n", cfg->gridsquare);
  }
  if(cfg->welcomemsg) {
    printf("  Welcome msg: ");
    /* Output the banner msg to stdout parsing any escape '\' chars */
    print_esc_string(cfg->welcomemsg, TRUE);
  }

  printf("  Flags: verbose = %s, send-only = %s\n",
    cfg->bVerbose ? "On" : "Off",
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
  static const char *short_options = "hVvsCc:t:e:a:";
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
    {NULL, no_argument, NULL, 0} /* array termination */
  };

  /* get a temporary buffer to build strings */
  cfgbuf = (char *)malloc(256);
  if(cfgbuf == NULL) {
    print_log(LOG_ERR,"loadconfig, out of memory");
    return(FALSE);
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
  config->welcomemsg = NULL;
  config->timeoutsecs = DFLT_TIMEOUTSECS;
  config->ax25port = NULL;
  config->bVerbose = FALSE;
  config->bSendonly = FALSE;

  /*
   * Get config from config file
   */

  fileconf = conf_read();
  if ((config->mycall = conf_get(fileconf, "mycall")) == NULL) {
    print_log(LOG_ERR, "failed to read mycall from configuration file\n");
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

  if ((cfgbuf = conf_get(fileconf, "gridsquare")) != NULL) {
    config->gridsquare = cfgbuf;
  }

  if ((cfgbuf = conf_get(fileconf, "welcomemsg")) != NULL) {
    config->welcomemsg = cfgbuf;
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
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0)
          break;
        print_log(LOG_ERR, "Debug: option parse %s ...", long_options[option_index].name);
        if (optarg)
          print_log(LOG_ERR, " with arg %s", optarg);
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
      case 'a':   /* set ax25 port */
        config->ax25port = optarg;
        break;
      case 'h':
        usage();  /* does not return */
        break;
      case '?':
        if (isprint (optopt)) {
          print_log(LOG_ERR, "Unknown option `-%c'", optopt);
        } else {
          print_log(LOG_ERR, "Unknown option character `\\x%x'", optopt);
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

  if(bDisplayVersion_flag) {
    displayversion();
    exit(EXIT_SUCCESS);
  }

  if(config->ax25port == NULL) {
    print_log(LOG_ERR, "Need to specify ax25 port");
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

  return(TRUE);
}

/*
 * Parse the char c that follows the escape char '\'
 * If bXlate is TRUE translate a '\r' to '\r\n' pair.
 */
static int
print_esc_char (char c, bool bXlate)
{

  switch (c) {
    case 'a':			/* Alert. */
      putchar ('\a');
      break;
    case 'b':			/* Backspace. */
      putchar ('\b');
      break;
    case 'c':			/* Cancel the rest of the output. */
      exit (EXIT_SUCCESS);
      break;
    case 'f':			/* Form feed. */
      putchar ('\f');
      break;
    case 'n':			/* New line. */
      putchar ('\n');
      break;
    case 'r':			/* Carriage return. */
      putchar ('\r');
      if(bXlate) {
        putchar('\n');
      }
      break;
    case 't':			/* Horizontal tab. */
      putchar ('\t');
      break;
    default:
      putchar (c);
      break;
  }
  return 1;
}

/*
 * Parse a string looking for the escape char '\'
 */
static void
print_esc_string (const char *str, bool bXlate)
{

  for (; *str; str++)
    if (*str == '\\') {
      str += print_esc_char (*(str+1), bXlate);
    }  else {
      putchar (*str);
    }
}
void disconnect(void) {
}
