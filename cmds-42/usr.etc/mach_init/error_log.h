/*
 * bootstrap -- fundamental service initiator and port server
 * Mike DeMoney, NeXT, Inc.
 * Copyright, 1990.  All rights reserved.
 *
 * error_log.h -- interface to logging routines
 */

#import <mach.h>

extern void init_errlog(boolean_t is_init);
extern void debug(const char *format, ...);
extern void info(const char *format, ...);
extern void log(const char *format, ...);
extern void error(const char *format, ...);
extern void kern_error(kern_return_t result, const char *format, ...);
extern void parse_error(const char *token_string, const char *format, ...);
extern void unix_error(const char *msg, ...);
extern void fatal(const char *msg, ...);
extern void kern_fatal(kern_return_t result, const char *msg, ...);
extern void unix_fatal(const char *msg, ...);
extern void close_errlog(void);

