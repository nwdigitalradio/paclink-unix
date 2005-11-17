#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

#include <db.h>
#include <time.h>

#if HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include "midseen.h"

/* XXX */
#define MID_DB "mid.db"

#define MID_DB_MODE 0600

#define EXPIREDAYS 30

/* returns 0 for success, non-zero for error */
int
record_mid(char *mid)
{
  DB *dbp;
  DBT key, data;
  time_t now;
  int ret;

  if (db_create(&dbp, NULL, 0) != 0) {
    return -1;
  }
  if (dbp->open(dbp, NULL, MID_DB, NULL, DB_HASH, DB_CREATE, MID_DB_MODE) != 0) {
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

  if (db_create(&dbp, NULL, 0) != 0) {
    return -1;
  }
  if (dbp->open(dbp, NULL, MID_DB, NULL, DB_HASH, DB_CREATE, MID_DB_MODE) != 0) {
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
      if (difftime(now, stored) > (EXPIREDAYS * 24 * 60 * 60)) {
	fprintf(stderr, "Deleting expired mid entry %s\n", (char *) key.data);
	dbp->del(dbp, NULL, &key, 0);
	ret = NOTFOUND;
      }
    } else {
      fprintf(stderr, "MID database %s is corrupt\n", MID_DB);
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

  if (db_create(&dbp, NULL, 0) != 0) {
    return -1;
  }
  if (dbp->open(dbp, NULL, MID_DB, NULL, DB_HASH, DB_CREATE, MID_DB_MODE) != 0) {
    return -1;
  }
  dbp->cursor(dbp, NULL, &cursorp, 0);

  memset(&key, 0, sizeof(DBT));
  memset(&data, 0, sizeof(DBT));

  while ((ret = cursorp->c_get(cursorp, &key, &data, DB_NEXT)) == 0) {
    printf("checking mid %s\n", (char *) key.data);
    if (data.size == sizeof(stored)) {
      memcpy(&stored, data.data, sizeof(stored));
      now = time(NULL);
      if (difftime(now, stored) > (EXPIREDAYS * 24 * 60 * 60)) {
	fprintf(stderr, "Deleting expired mid entry %s\n", (char *) key.data);
	dbp->del(dbp, NULL, &key, 0);
      }
    } else {
      fprintf(stderr, "MID database %s is corrupt\n", MID_DB);
      exit(EXIT_FAILURE);
    }
  }
  if (ret != DB_NOTFOUND) {
    fprintf(stderr, "MID database %s is corrupt\n", MID_DB);
    exit(EXIT_FAILURE);
  }
  if (cursorp != NULL) {
    cursorp->c_close(cursorp);
  }

  dbp->close(dbp, 0);
  return 0;
}
