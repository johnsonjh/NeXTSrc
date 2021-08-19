/*
 * Global errno interface for gethostent
 * Copyright (C) 1989 by NeXT, Inc.
 *
 * This file contains global data and the size of the global data can NOT
 * change or otherwise it would make the shared library incompatable.
 */
int h_errno = 0;
