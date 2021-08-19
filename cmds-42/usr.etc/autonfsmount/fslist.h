/*
 * getmntent() for automount entries with caching (60 second timeout)
 * 
 * Copyright (C) 1990 by NeXT, Inc.
 */
#include <sys/time.h>

void fslist_set(struct timeval *tv);
struct mntent *fslist_get(void);
void fslist_end(void);
