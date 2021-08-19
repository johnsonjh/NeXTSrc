/*
 * socket definitions
 * Copyright (C) 1989 by NeXT, Inc.
 */
int socket_connect(struct sockaddr_in *, int, int);
int socket_open(struct sockaddr_in *, int, int);
int socket_close(int);
void socket_lock();
void socket_unlock();
