/*
 * Lookup various things definitions
 * Copyright (C) 1989 by NeXT, Inc.
 */
unsigned long getaddress(void *, ni_name);
unsigned long getmasteraddr(void *, ni_name *);
int getmaster(void *, ni_name *, ni_name *);
int is_trusted_network(void *, struct sockaddr_in *);
