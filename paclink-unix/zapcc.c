#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

#include <string.h>

char *
zapcc(char *s)
{
  char *p;

  if ((p = strchr(s, '\r')) != NULL) {
    *p = '\0';
  }
  if ((p = strchr(s, '\n')) != NULL) {
    *p = '\0';
  }
  return s;
}
