/* $Id$ */

#ifndef TIMEOUT_H
#define TIMEOUT_H

void sigalrm(int sig __attribute__((__unused__)));
void settimeout(int secs);
void resettimeout(void);
void unsettimeout(void);

#endif
