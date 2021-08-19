/*
 * Definitions needed for safe stdio. 
 * Copyright (C) 1989 by NeXT, Inc.
 *
 * Safe stdio insures that all fopens/fcloses are mutually exclusive.
 */
extern FILE *safe_fopen(char *, char *);
extern int safe_fclose(FILE *);
