/* 
 * Copyright (c) 1988 NeXT, Inc.
 *
 * HISTORY
 *  04-May-90 Created
 */

#ifndef __MONITOR_HEADER__
#define __MONITOR_HEADER__

extern void monstartup (char *lowpc, char *highpc);

extern void monitor (char *lowpc, char *highpc, char *buf, int bufsiz, int cntsiz);

extern void moncontrol (int mode);

extern void monoutput (const char *filename);

extern void moninit (void);

extern void monreset (void);

#endif	__MONITOR_HEADER__
