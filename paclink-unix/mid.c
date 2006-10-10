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

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  if HAVE_TIME_H
#   include <time.h>
#  endif
# endif
#endif
#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include <db.h>

#include "compat.h"
#include "mid.h"

/* returns 0 for success, non-zero for error */
int
record_mid(char *mid)
{
  DB *dbp;
  DBT key, data;
  time_t now;
  int ret;
  int r;

  if (db_create(&dbp, NULL, 0) != 0) {
    fprintf(stderr, "%s: record_mid(): db_create() failed\n", getprogname());
    return -1;
  }
  if ((r = dbp->open(dbp, NULL, WL2K_MID_DB, NULL, DB_HASH, DB_CREATE, MID_DB_MODE) != 0)) {
    dbp->err(dbp, r, "%s: %s", getprogname(), WL2K_MID_DB);
    dbp->close(dbp, 0);
    return -1;
  }
  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  key.data = mid;
  key.size = strlen(mid) + 1;

  now = time(NULL);
  data.data = &now;
  data.size = sizeof(now);

  /* returns 0 for success, non-zero for error */
  ret = dbp->put(dbp, NULL, &key, &data, 0);

  dbp->close(dbp, 0);

  return ret;
}

#define FOUND 1
#define NOTFOUND 0

/* returns 1 if key found, 0 otherwise */
int
check_mid(char *mid)
{
  DB *dbp;
  DBT key, data;
  time_t stored;
  time_t now;
  int ret;
  int r;

  if (db_create(&dbp, NULL, 0) != 0) {
    fprintf(stderr, "%s: check_mid(): db_create() failed\n", getprogname());
    return -1;
  }
  if ((r = dbp->open(dbp, NULL, WL2K_MID_DB, NULL, DB_HASH, DB_CREATE, MID_DB_MODE)) != 0) {
    dbp->err(dbp, r, "%s: %s", getprogname(), WL2K_MID_DB);
    dbp->close(dbp, 0);
    return -1;
  }
  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  key.data = mid;
  key.size = strlen(mid) + 1;

  if (dbp->get(dbp, NULL, &key, &data, 0) == 0) {
    ret = FOUND;
    if (data.size == sizeof(stored)) {
      memcpy(&stored, data.data, sizeof(stored));
      now = time(NULL);
      if (difftime(now, stored) > (MID_EXPIREDAYS * 24 * 60 * 60)) {
	fprintf(stderr, "Deleting expired mid entry %s\n", (char *) key.data);
	dbp->del(dbp, NULL, &key, 0);
	ret = NOTFOUND;
      }
    } else {
      fprintf(stderr, "MID database %s is corrupt\n", WL2K_MID_DB);
      exit(EXIT_FAILURE);
    }
  } else {
    ret = NOTFOUND;
  }
  dbp->close(dbp, 0);
  return ret;
}

int
expire_mids(void)
{
  DB *dbp;
  DBT key, data;
  DBC *cursorp;
  time_t stored;
  time_t now;
  int ret;
  int r;

  if (db_create(&dbp, NULL, 0) != 0) {
    fprintf(stderr, "%s: expire_mids(): db_create() failed\n", getprogname());
    return -1;
  }
  if ((r = dbp->open(dbp, NULL, WL2K_MID_DB, NULL, DB_HASH, DB_CREATE, MID_DB_MODE)) != 0) {
    dbp->err(dbp, r, "%s: %s", getprogname(), WL2K_MID_DB);
    dbp->close(dbp, 0);
    return -1;
  }
  dbp->cursor(dbp, NULL, &cursorp, 0);

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  while ((ret = cursorp->c_get(cursorp, &key, &data, DB_NEXT)) == 0) {
#if 0
    printf("checking mid %s\n", (char *) key.data);
#endif
    if (data.size == sizeof(stored)) {
      memcpy(&stored, data.data, sizeof(stored));
      now = time(NULL);
      if (difftime(now, stored) > (MID_EXPIREDAYS * 24 * 60 * 60)) {
	fprintf(stderr, "Deleting expired mid entry %s\n", (char *) key.data);
	dbp->del(dbp, NULL, &key, 0);
      }
    } else {
      fprintf(stderr, "MID database %s is corrupt\n", WL2K_MID_DB);
      exit(EXIT_FAILURE);
    }
  }
  if (ret != DB_NOTFOUND) {
    fprintf(stderr, "MID database %s is corrupt\n", WL2K_MID_DB);
    exit(EXIT_FAILURE);
  }
  if (cursorp != NULL) {
    cursorp->c_close(cursorp);
  }

  dbp->close(dbp, 0);
  return 0;
}

char *
generate_mid(const char *callsign)
{
  size_t clen;
  size_t rlen;
  char mid[MID_MAXLEN + 1];
  char midchars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  size_t i;
  static int initialized = 0;
  int r;

#define CALLSIGN_MAXLEN 6 /* XXX ssid? */

  clen = strlen(callsign);
  if (clen > CALLSIGN_MAXLEN) {
    fprintf(stderr, "bad callsign %s\n", callsign);
    return NULL;
  }
  rlen = MID_MAXLEN - 1 - clen;
  if (rlen < 1) {
    fprintf(stderr, "No room to generate mid!?\n");
    return NULL;
  }
  strlcpy(mid + rlen + 1, callsign, CALLSIGN_MAXLEN + 1);
  mid[rlen] = '_';
  mid[MID_MAXLEN] = '\0';
  if (!initialized) {
    srandom((unsigned long) time(NULL));
    initialized = 1;
  }
  do {
    for (i = 0; i < rlen; i++) {
      mid[i] = midchars[random() % (sizeof(midchars) - 1)];
    }
  } while ((r = check_mid(mid)) > 0);
  if (r < 0) {
    fprintf(stderr, "failure in check_mid()\n");
    return NULL;
  }
  record_mid(mid);
  return strdup(mid);
}
