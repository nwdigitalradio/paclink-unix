/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2; c-brace-offset: -2; c-argdecl-indent: 2 -*- */

/*  paclink-unix client for the Winlink 2000 ham radio email system.
 *
 *  Copyright 2006 Nicholas S. Castellano <n2qz@arrl.net> and others,
 *                 See the file AUTHORS for a list.
 *
 *  Code for the SCS P4dragon was copied from SCS simple terminal
 *    file: term.c
 *    Copyright 2005-2012 SCS GmbH & Co. KG, Hanau, Germany
 *    written by Peter Mack (peter.mack@scs-ptc.com)
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
#if HAVE_CTYPE_H
# include <ctype.h>
#endif
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <fcntl.h>
#include <termios.h>

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

#if defined(EXTA) && !defined(B19200)
# define B19200 EXTA
#endif

#if defined(EXTB) && !defined(B38400)
# define B38400 EXTB
#endif

/*
 * Globals
 */
int gverbose_flag=FALSE;
int gsendmsgonly_flag=FALSE;

#define SCSMODEM_UNDEFINED 0
#define SCSMODEM_PTCIIPRO 1
#define SCSMODEM_P4DRAGON 2

/* Names of SCS pactor modems */
const struct modemtype {
        const char *modem_name;
        int modem;
} modems[] = {
        {"PTC-IIpro", SCSMODEM_PTCIIPRO}, /* default */
        {"P4dragon",  SCSMODEM_P4DRAGON},
        {NULL, 0}
};

const struct baudrate {
  const char *asc;
  speed_t num;
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

/*
 * Config parameters struct
 */
typedef struct _wl2kserial_config {
  char    *mycall;
  char    *targetcall;
  char    *serialdevice;
  char    *emailaddr;
  char    *wl2k_password;
  int     modem;
  speed_t baudrate;
  unsigned int timeoutsecs;
  int     bVerbose;
  int     bSendonly;
  FILE    *modemfp;
}cfg_t;

static cfg_t cfg;

static bool loadconfig(int argc, char **argv, cfg_t *cfg);
static void usage(void);
static void displayversion(void);
static void displayconfig(cfg_t *config);
void disconnect(void);

/**
 * Function: main
 *
 * The calling convention for this function is:
 *
 * wl2kserial -c targetcall -d device -b baudrate -t timeoutsecs -e emailaddress
 *
 * The parameters are:
 * mycall :  my call sign, which MUST be set in wl2k.conf
 * targetcall: callsign to contact
 * device: serial device to use
 * baudrate: baudrate to set for serial device
 * timeoutsecs: timeout in seconds
 * emailaddress: email address where the retrieved message will be sent via sendmail
 *
 */
int
main(int argc, char *argv[])
{
  int fd;
  FILE *fp;
  char *line;
  struct termios init;
  struct termios t;
  int flags;

  loadconfig(argc, argv, &cfg);

  g_mime_init(0);

  setlinebuf(stdout);

  settimeout(cfg.timeoutsecs);

  /* open device */
  printf("Opening %s ...\n", cfg.serialdevice);
  if ((fd = open(cfg.serialdevice, O_RDWR | O_NONBLOCK)) == -1) {
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
  if ((cfsetspeed(&t, cfg.baudrate)) == -1) {
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

  t.c_iflag &= (tcflag_t)~(INPCK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF | IMAXBEL);
  t.c_iflag |= (IGNBRK | IGNPAR);

  t.c_oflag &= (tcflag_t)~OPOST;

  t.c_cflag &= ~(CSIZE | PARENB | CRTSCTS | CCTS_OFLOW | CCTS_IFLOW | MDMBUF);
  t.c_cflag |= (CS8 | CREAD | CLOCAL);

  t.c_lflag &= (tcflag_t)~(ICANON | ISIG | IEXTEN | ECHO);

  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;

  /* set P4dragon serial port flags */
  if (cfg.modem == SCSMODEM_P4DRAGON) {
    t.c_oflag = 0;
    t.c_lflag = 0;
    t.c_cflag &= (unsigned int)~(CSTOPB | PARENB | PARODD | HUPCL);
    t.c_cflag |= (CS8 | CRTSCTS | CREAD | CLOCAL);
  }

  if ((tcsetattr(fd, TCSAFLUSH, &t)) == -1) {
    perror("tcsetattr()");
    exit(EXIT_FAILURE);
  }

  /* set P4dragon baud rate */
  if (cfg.modem == SCSMODEM_P4DRAGON) {

    struct serial_struct sstruct;

    /* get serial_struct */
    if (ioctl(fd, TIOCGSERIAL, &sstruct) < 0) {
      close (fd);
      perror("ioctl(): could not get serial ioctl");
      exit(EXIT_FAILURE);
    }

    /* set custom divisor to get 829440 baud */
    sstruct.custom_divisor = 29;
    sstruct.flags |= (int)ASYNC_SPD_CUST;

    /* set serial_struct */
    if (ioctl(fd, TIOCSSERIAL, &sstruct) < 0) {
      close (fd);
      perror("ioctl(): could not set custom comm baud divisor");
      exit(EXIT_FAILURE);
    }
  }

  resettimeout();

  printf("Connected.\n");

  if ((fp = fdopen(fd, "r+b")) == NULL) {
    close(fd);
    perror("fdopen()");
    exit(EXIT_FAILURE);
  }
  /* save for disconnect */
  cfg.modemfp = fp;
  setbuf(fp, NULL);

  fprintf(fp, "\r\n");
  while ((line = wl2kgetline(fp)) != NULL) {
    printf("%s\n", line);
    if (strstr(line, "cmd:")) {
      fprintf(fp, "mycall %s\r", cfg.mycall);
      printf("mycall %s\n", cfg.mycall);
      fprintf(fp, "tones 4\r");
      printf("tones 4\n");
      fprintf(fp, "chobell 0\r");     /* Changeover Bell off */
      printf("chobell 0\n");
      fprintf(fp, "lfignore 1\r");    /* xxx Insertion of Line feed */
      printf("lfignore 1\n");
      if (cfg.modem == SCSMODEM_P4DRAGON) {
        /* Taken from BPQ32 SCSPactor.c file
         * Copyright 2001-2015 John Wiseman G8BPQ
         * The SCSPactor.c file is part of LinBPQ/BPQ32.
         *
         * Set P4 Dragon with KISS over Hostmade
         */
        fprintf(fp, "MAXERR 30\r");   /* Max retries */
        printf("MAXERR 30\n");
        fprintf(fp, "MODE 0\r");      /* ASCII mode, no PTC II compression (Forwarding will use FBB Compression) */
        printf("MODE 0\n");
        fprintf(fp, "MAXSUM 20\r");   /* Max count for memory ARQ */
        printf("MAXSUM 20\n");
        fprintf(fp, "CWID 0\r");      /* CW ID disabled */
        printf("CWID 0\n");
        fprintf(fp, "PTCC 0\r");      /* Dragon out of PTC Compatibility Mode */
        printf("PTCC 0\n");
        fprintf(fp, "VER\r");         /* Try to determine Controller Type */
        printf("VER\n");
        fprintf(fp, "ADDLF 0\r");     /* Auto Line Feed disabled */
        printf("ADDLF 0\n");
        fprintf(fp, "ARX 0\r");       /* Amtor Phasing disabled */
        printf("ARX 0\n");
        fprintf(fp, "BC 0\r");        /* FEC reception is disabled */
        printf("BC 0\n");
        fprintf(fp, "BKCHR 2\r");     /* Breakin Char = 2*/
        printf("BKCHR 2\n");
        fprintf(fp, "CMSG 0\r");      /* Connect Message Off */
        printf("CMSG 0\n");
        fprintf(fp, "LISTEN 0\r");    /* Pactor Listen disabled */
        printf("LISTEN 0\n");
        fprintf(fp, "MAIL 0\r");      /* Disable internal mailbox reporting */
        printf("MAIL 0\n");
        fprintf(fp, "REMOTE 0\r");    /* Disable remote control */
        printf("REMOTE 0\n");

        fprintf(fp, "PAC CBELL 0\r");
        printf("PAC CBELL 0\n");
        fprintf(fp, "PAC CMSG 0\r");
        printf("PAC CMSG 0\n");
        fprintf(fp, "PAC PRBOX 0\r");
        printf("PAC PRBOX 0\n");

        /*  Automatic Status must be enabled for paclink-unix??
         *  Pactor must use Host Mode Channel 31
         *  PDuplex must be set. The Node code relies on automatic IRS/ISS changeover
         *	5 second duplex timer
         */
        fprintf(fp, "STATUS 0\r");
        printf("STATUS 0\n");
        fprintf(fp, "PTCHN 31\r");
        printf("PTCHN 31\n");
        fprintf(fp, "PDUPLEX 1\r");
        printf("PDUPLEX 1\n");
        fprintf(fp, "PDTIMER 5\r");
        printf("PDTIMER 5\n");
        fprintf(fp, "JHOST4\r");
        printf("JHOST4\n");
      }
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
      fprintf(fp, "c %s\r", cfg.targetcall);
      printf("c %s\n", cfg.targetcall);
      break;
    }
  }
  if (line == NULL) {
    fprintf(stderr, "Connection closed by foreign host.\n");
    exit(EXIT_FAILURE);
  }

  wl2k_exchange(cfg.mycall, cfg.targetcall, fp, fp, cfg.emailaddr, cfg.wl2k_password);

  disconnect();
  fclose(fp);
  g_mime_shutdown();
  exit(EXIT_SUCCESS);
  return 1;
}

void disconnect(void) {
  /* If D (Disconect) doesn't work try DD (Dirty Disconnect) */
  if (cfg.modem == SCSMODEM_P4DRAGON) {
    /* get out of host mode */
    fprintf(cfg.modemfp, "\x1bJHOST0\r");   /*  */
    fprintf(cfg.modemfp, "D\r");   /* Disconnect from Dragon modem */
    printf("Disconnect\n");
  }
}

/*
 * Print usage information and exit
 *  - does not return
 */
static void
usage(void)
{
  printf("Usage:  %s -c target_call -d serial_device -b baudrate -m modem_name options\n", getprogname());
  printf("  -c  --target-call   Set callsign to call\n");
  printf("  -d  --device        Set serial device name\n");
  printf("  -b  --baudrate      Set baud rate of serial device\n");
  printf("  -m  --modem         Set SCS modem type (PTC-IIpro, P4dragon)\n");
  printf("  -t  --timeout       Set timeout in seconds\n");
  printf("  -P  --wl2k-passwd   Set password for WL2K secure login\n");
  printf("  -e  --email-address Set your e-mail address\n");
  printf("  -s  --send-only     Send messages only\n");
  printf("  -V  --verbose       Print verbose messages\n");
  printf("  -v  --version       Display version of this program\n");
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
displayconfig(cfg_t *config)
{

  struct baudrate const *bp;
  struct modemtype const *mp;

  printf("%s: Using this config:\n", getprogname());

  if(config->mycall) {
    printf("  My callsign: %s\n", config->mycall);
  }
  if(config->targetcall) {
    printf("  Target callsign: %s\n", config->targetcall);
  }
  if(config->serialdevice) {
    printf("  Serial device: %s\n", config->serialdevice);
  }

  printf("  Baud rate: ");
  /* Loop through the baud rate table */
  for (bp = baudrates; bp->asc != NULL; bp++) {
    if (bp->num == config->baudrate) {
      break;
    }
  }
  if(bp->asc != NULL) {
    printf(" %s\n", bp->asc);
  } else {
    printf("Invalid\n");
  }

  printf("  Modem type: ");
  /* Loop through the modem name table */
  for (mp = modems; mp->modem_name != NULL; mp++) {
    if (mp->modem == config->modem) {
      break;
    }
  }
  if(mp->modem_name != NULL) {
    printf(" %s\n", mp->modem_name);
  } else {
    printf("Invalid\n");
  }

  if(config->wl2k_password) {
    printf("  WL2K secure login password: %s\n", config->wl2k_password);
  }

  printf("  Timeout: %d\n", config->timeoutsecs);

  if(config->emailaddr) {
    printf("  Email address: %s\n", config->emailaddr);
  }
  printf("  Flags: verbose = %s, send-only = %s\n",
         config->bVerbose  ? "On" : "Off",
         config->bSendonly ? "On" : "Off");
}

/* Load these 7 config parameters:
 * mycall targetcall device baudrate modem timeoutsecs emailaddress
 */
static bool
loadconfig(int argc, char **argv, cfg_t *config)
{
  struct conf *fileconf;
  char *endp;
  struct baudrate const *bp;
  speed_t baud;
  struct modemtype const *mp;
  int modemtype;
  int next_option;
  int option_index = 0; /* getopt_long stores the option index here. */
  char *cfgbuf;
  static int displayconfig_flag=FALSE;
  bool bRequireConfig_pass = TRUE;
  bool bDisplayVersion_flag = FALSE;
  char strDefaultBaudRate[]="9600";  /* String of default baudrate */
  char *pBaudRate = strDefaultBaudRate;
  char strDefaultModemType[]="PTC-IIpro";
  char *pModemType = strDefaultModemType;
  /* short options */
  static const char *short_options = "hVvsCc:t:e:d:b:P:m:";
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
    {"device",        required_argument, NULL, 'd'},
    {"baudrate",      required_argument, NULL, 'b'},
    {"wl2k-passwd",   required_argument, NULL, 'P'},
    {"modem",         required_argument, NULL, 'm'},
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

  free(cfgbuf);

  config->mycall = NULL;
  config->targetcall = NULL;
  config->timeoutsecs = DFLT_TIMEOUTSECS;
  config->wl2k_password = NULL;
  config->serialdevice = NULL;
  config->baudrate = B9600;
  config->bVerbose = FALSE;
  config->bSendonly = FALSE;
  config->modem = SCSMODEM_PTCIIPRO;

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

  if ((cfgbuf = conf_get(fileconf, "wl2k-password")) != NULL) {
    config->wl2k_password = cfgbuf;
  }

  if ((cfgbuf = conf_get(fileconf, "device")) != NULL) {
    config->serialdevice = cfgbuf;
  }

  /* Get pointer to an ASCII baud rate */
  if ((cfgbuf = conf_get(fileconf, "baud")) != NULL) {
    pBaudRate = cfgbuf;
  }

  /* Get pointer to an ASCII modem type */
  if ((cfgbuf = conf_get(fileconf, "modem")) != NULL) {
    pModemType = cfgbuf;
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
      case 'd':   /* set serial device name */
        config->serialdevice = optarg;
        break;
      case 'b':  /* Get pointer to an ASCII baud rate */
        pBaudRate = optarg;
        break;
      case 'm':  /* Get pointer to an ASCII modem type */
        pModemType = optarg;
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

  /* Set baud rate */
  if(pBaudRate != NULL) {
    /* Convert ASCII baud rate into some usable bits */
    baud = B0;
    for (bp = baudrates; bp->asc != NULL; bp++) {
      if (strcmp(bp->asc, pBaudRate) == 0) {
        baud = bp->num;
        break;
      }
    }

    /* Verify a valid baudrate */
    if (baud == B0) {
      fprintf(stderr, "%s: Invalid baud '%s' -- valid baud rates are: ",
              getprogname(), pBaudRate);
      for (bp = baudrates; bp->asc != NULL; bp++) {
        fprintf(stderr, "%s ", bp->asc);
      }
      fprintf(stderr, "\n");
      usage();  /* does not return */
    }

    /* Save the baudrate bits in the config struct */
    config->baudrate = baud;
  }

  /* Set modem type */
  if(pModemType != NULL) {
    /* Convert ASCII modem type into an int */
    modemtype = SCSMODEM_UNDEFINED;
    for (mp = modems; mp->modem_name != NULL; mp++) {
      if (strcasecmp(mp->modem_name, pModemType) == 0) {
        modemtype = mp->modem;
        break;
      }
    }

    /* Verify a valid modem type */
    if (modemtype == SCSMODEM_UNDEFINED) {
      fprintf(stderr, "%s: Invalid modem '%s' -- valid modem names are: ",
              getprogname(), pModemType);
      for (mp = modems; mp->modem_name != NULL; mp++) {
        fprintf(stderr, "%s ", mp->modem_name);
      }
      fprintf(stderr, "\n");
      usage();  /* does not return */
    }

    /* Save modem type in the config struct */
    config->modem = modemtype;
  }

  if (config->modem == SCSMODEM_P4DRAGON) {
    /* custom divisor only works if baud is set to 38400 */
    config->baudrate = B38400;
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
  if(config->serialdevice == NULL) {
    fprintf(stderr,  "%s: Need to specify serial device\n", getprogname() );
    bRequireConfig_pass = FALSE;
  }
  if(config->baudrate == 0) {
    fprintf(stderr,  "%s: Need to specify baud rate of serial device\n", getprogname() );
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
