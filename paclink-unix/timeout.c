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
