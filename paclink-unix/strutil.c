#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

#include <ctype.h>
#include "strutil.h"

char *
strupper(unsigned char *s)
{
  unsigned char *cp;

  for (cp = s; *cp; cp++) {
    if (islower(*cp)) {
      *cp = toupper(*cp);
    }
  }
  return s;
}
