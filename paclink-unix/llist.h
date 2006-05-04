/* $Id$ */

#ifndef LLIST_H
#define LLIST_H

struct llist {
  void *data;
  struct llist *next;
};

struct llist **llist_add(struct llist **headp, void *data);
void llist_free(struct llist *link, void (*datafreefunc)(void *));

#endif
