/* 
 * Event handling definitions
 * Copyright (C) 1989 by NeXT, Inc.
 */
extern int event_pipe[];
extern void event_handle(void);
extern void event_post(void);
extern void event_init(void (*)());
