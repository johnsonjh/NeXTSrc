/*
 * Memory-memory copy definitions
 * Copyright (C) 1989 by NeXT, Inc.
 */
#define LOOKUP_BUF_SIZE (16 * 1024)
char lookup_buf[LOOKUP_BUF_SIZE];

int copy_mem(char *src, char **dst, int dst_offset, int src_bytes);
