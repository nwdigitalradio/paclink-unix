/* $Id$ */

#ifndef WL2K_H
#define WL2K_H

char *wl2kgetline(FILE *fp);
void wl2kexchange(char *mycall, char *yourcall, FILE *fp);

#define WL2K_COMPRESSED_GOOD 0
#define WL2K_COMPRESSED_BAD 1

#endif

