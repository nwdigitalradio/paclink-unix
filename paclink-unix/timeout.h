/* $Id$ */

#ifndef TIMEOUT_H
#define TIMEOUT_H

void sigalrm(int sig ATTRIBUTE_UNUSED);
void settimeout(int secs);
void resettimeout(void);
void unsettimeout(void);

#endif
