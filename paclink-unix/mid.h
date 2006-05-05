/* $Id$ */

#ifndef MID_H
#define MID_H

int record_mid(char *mid);
int check_mid(char *mid);
int expire_mids(void);
char *generate_mid(const char *callsign);

#endif
