/*
 * system routine definitions
 * Copyright (C) 1989 by NeXT, Inc.
 */
#ifndef NULL
#	define NULL ((void *)0)
#endif

const char *sys_hostname(void);
unsigned long sys_address(void);
int sys_ismyaddress(unsigned long);

void sys_errmsg(const char *, ...);
void sys_logmsg(const char *, ...);
void sys_panic(const char *, ...);
int sys_standalone(void);
int sys_spawn(const char *, ...);
long sys_time(void);
