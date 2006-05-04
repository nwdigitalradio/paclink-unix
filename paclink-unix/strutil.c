#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

#include <ctype.h>
#include <string.h>
#include "strutil.h"

char *
strupper(unsigned const char *s)
{
  unsigned char *cp;

  for (cp = s; *cp; cp++) {
    if (islower(*cp)) {
      *cp = toupper(*cp);
    }
  }
  return s;
}

int
strbegins(const char *s, const char *prefix)
{
  size_t plen;

  plen = strlen(prefix);
  if (strncmp(s, prefix, plen) == 0) {
    return 1;
  }
  return 0;
}

int
strcasebegins(const char *s, const char *prefix)
{
  size_t plen;

  plen = strlen(prefix);
  if (strncasecmp(s, prefix, plen) == 0) {
    return 1;
  }
  return 0;
}
