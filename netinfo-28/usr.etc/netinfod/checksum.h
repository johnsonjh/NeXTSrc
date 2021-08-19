/*
 * Checksum definitions
 * Copyright (C) 1989 by NeXT, Inc.
 */
void checksum_compute(unsigned *, void *);
void checksum_inc(unsigned *, ni_id);
void checksum_add(unsigned *, ni_id);
void checksum_rem(unsigned *, ni_id);
