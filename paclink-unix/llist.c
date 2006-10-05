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
#include "llist.h"

struct llist **
llist_add(struct llist **headp, void *data)
{
  struct llist **lp;

  if (!headp) {
    return NULL;
  }
  for (lp = headp; *lp; lp = &((*lp)->next))
    ;
  if ((*lp = (struct llist *) malloc(sizeof(struct llist))) == NULL) {
    perror("out of memory");
    exit(EXIT_FAILURE);
  }
  (*lp)->next = NULL;
  (*lp)->data = data;
  return headp;
}

void
llist_free(struct llist *llink, void (*datafreefunc)(void *))
{
  if (!llink) {
    return;
  }
  llist_free(llink->next, datafreefunc);
  if (datafreefunc && llink->data) {
    datafreefunc(llink->data);
  }
  free(llink);
}
