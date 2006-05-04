#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/types.h>
#ifdef __RCSID
__RCSID("$Id$");
#endif

#include <stdio.h>
#include <stdlib.h>
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
llist_free(struct llist *link, void (*datafreefunc)(void *))
{
  if (!link) {
    return;
  }
  llist_free(link->next, datafreefunc);
  if (datafreefunc && link->data) {
    datafreefunc(link->data);
  }
  free(link);
}
