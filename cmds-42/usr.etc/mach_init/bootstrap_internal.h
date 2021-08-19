/*
 * bootstrap -- fundamental service initiator and port server
 * Mike DeMoney, NeXT, Inc.
 * Copyright, 1990.  All rights reserved.
 *
 * bootstrap_internal.h -- global internal data definitions
 */

#import <mach.h>
#import <sys/boolean.h>

#define	BITS_PER_BYTE	8		/* this SHOULD be a well defined constant */
#define	ANYWHERE		TRUE	/* For use with vm_allocate() */

extern const char *program_name;
extern const char *conf_file;
extern const char *default_conf;
extern port_t lookup_only_port;
extern port_t inherited_bootstrap_port;
extern port_t self_port;		/* Compatability hack */
extern boolean_t forward_ok;
extern boolean_t debugging;
extern port_set_name_t bootstrap_port_set;
extern int init_priority;
extern boolean_t canReceive(port_t port);
extern boolean_t canSend(port_t port);

extern boolean_t mach_port_inuse(int mach_port);

extern void msg_destroy_port(port_t p);

#define	ASSERT(condition)	\
    do { \
		if (!(condition)) { \
		    error("Assertion failed: " #condition ", file " __FILE__ ", \
			  line %d.\n", __LINE__); \
		} \
    } while (0)











