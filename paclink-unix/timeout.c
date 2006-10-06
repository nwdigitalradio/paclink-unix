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

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif

#include "timeout.h"

unsigned int timeoutsecs;

void
sigalrm(int sig ATTRIBUTE_UNUSED)
{
  const char msg[] = "Timed out, exiting!\n";
  write(STDERR_FILENO, msg, sizeof(msg));
  _exit(EXIT_FAILURE);
}

void
settimeout(int secs)
{
  timeoutsecs = secs;
  resettimeout();
}

void
resettimeout(void)
{
  alarm(0);
  signal(SIGALRM, sigalrm);
  alarm(timeoutsecs);
}

void
unsettimeout(void)
{
  alarm(0);
  signal(SIGALRM, SIG_DFL);
}
