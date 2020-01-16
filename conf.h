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

/* $Id$ */

#ifndef CONF_H
#define CONF_H

#include <termios.h>

#define DFLT_TIMEOUTSECS (120)  /* default time out seconds */
#define DFLT_TELNET_PORT 8772
#define DFLT_TELNET_CALL "wl2k"
/* This telnet server was deprecasted in 2017 in favor of the Amazon
 * Web Service (AWS) address
#define DFLT_TELNET_HOSTNAME "server.winlink.org"
*/
#define DFLT_TELNET_HOSTNAME "cms.winlink.org"
#define DFLT_TELNET_PASSWORD "CMSTelnet"

struct conf {
  char *var;
  char *value;
  struct conf *next;
};

struct baudrate {
        const char *asc;
        speed_t num;
};

/*
 * Config parameters struct
 */
typedef struct _plu_config {
        char *mycall;
        char *targetcall;
        struct buffer *pathbuf;
        char *hostname;
        unsigned short  hostport;
        char *password;
        char *ax25port;
        char *emailaddr;
        char *wl2k_password;
        char *gridsquare;
        char *welcomemsg;
        unsigned int timeoutsecs;
        int  bVerbose;
        int  bSendonly;
        char    *serialdevice;
        int     modem;
        speed_t baudrate;
        FILE    *modemfp;
        int     ax25sock;
}cfg_t;

struct conf *conf_read(void);
char *conf_get(struct conf *conf, const char *var);

#endif
