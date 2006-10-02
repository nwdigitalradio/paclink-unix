#if HAVE_CONFIG_H
# include "config.h"
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef __RCSID
__RCSID("$Id$");
#endif

#include "compat.h"

#if HAVE___PROGNAME
extern char *__progname;
#endif

#if HAVE___ARGV
extern char **__Argv;
#endif

#if HAVE____ARGV
extern char **___Argv;
#endif

const char *
getprogname(void)
{

#if HAVE___PROGNAME
  return __progname;
#else
#if HAVE___ARGV
  return __Argv[0];
#else
#if HAVE____ARGV
  return ___Argv[0];
#else
  return "(unknown program)";
#endif
#endif
#endif

}
