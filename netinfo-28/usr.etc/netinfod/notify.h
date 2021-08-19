/*
 * Notification thread definitions
 * Copyright (C) 1989 by NeXT, Inc.
 *
 * The notification thread runs only on the master. It notifies
 * clone servers about changes to the database or resynchronization
 * requests from the master.
 */
int notify_start(void);
void notify_clients(unsigned, void *);
void notify_resync(void);
