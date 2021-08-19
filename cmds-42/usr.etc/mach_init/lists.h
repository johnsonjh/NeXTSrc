/*
 * bootstrap -- fundamental service initiator and port server
 * Mike DeMoney, NeXT, Inc.
 * Copyright, 1990.  All rights reserved.
 *
 * lists.h -- interface to list routines
 */

#import <mach.h>
#import <sys/boolean.h>
#import <servers/bootstrap_defs.h>

#ifndef NULL
#define	NULL	((void *)0)
#endif NULL

typedef struct bootstrap bootstrap_t;
typedef struct service service_t;
typedef struct server server_t;

/* Bootstrap info */
struct bootstrap {
	bootstrap_t		*next;		/* list of all bootstraps */
	bootstrap_t		*prev;
	bootstrap_t		*parent;
	port_name_t		bootstrap_port;
	port_name_t		unpriv_port;
	port_name_t		requestor_port;
};

/* Service types */
typedef enum {
	DECLARED,	/* Declared in config file */
	REGISTERED,	/* Registered dynamically */
	MACHPORT,	/* Name bound to existing mach port */
	SELF		/* Name bound bootstrap service itself */
} servicetype_t;

struct service {
	service_t	*next;		/* list of all services */
	service_t	*prev;
	name_t		name;		/* service name */
	port_name_t	port;		/* service port,
					   may have all rights if inactive */
	bootstrap_t	*bootstrap;	/* bootstrap port(s) used at this
					 * level. */
	boolean_t	isActive;	/* server is running */
	servicetype_t	servicetype;	/* Declared, Registered, or Machport */
	server_t	*server;	/* server, declared services only */
	unsigned	mach_port_num;	/* compatability hack */
};

/* Servere types */
typedef enum {
	SERVER,		/* Launchable server */
	RESTARTABLE,	/* Restartable server */
	ETCINIT,	/* Special processing for /etc/init */
	MACHINIT	/* mach_init doesn't get launched. */
} servertype_t;

#define	NULL_SERVER	NULL
#define	ACTIVE		TRUE
#define	NO_MACH_PORT	((unsigned)-1)

struct server {
	server_t	*next;		/* list of all servers */
	server_t	*prev;
	servertype_t	servertype;
	cmd_t		cmd;		/* server command to exec */
	int		priority;	/* priority to give server */
	port_all_t	port;		/* server's priv bootstrap port */
	port_name_t	task_port;	/* server's task port */
};

#define	NO_PID	(-1)

extern void init_lists(void);
extern server_t *new_server(
	servertype_t	servertype,
	const char	*cmd,
	int		priority);
extern service_t *new_service(
	bootstrap_t	*bootstrap,
	const char	*name,
	port_t		service_port,
	boolean_t	isActive,
	servicetype_t	servicetype,
	server_t	*serverp,
	unsigned	mach_port_num);
extern bootstrap_t *new_bootstrap(
	bootstrap_t	*parent,
	port_name_t	bootstrap_port,
	port_name_t	unpriv_port,
	port_name_t	requestor_port);
extern server_t *lookup_server_by_port(port_t port);
extern server_t *lookup_server_by_task_port(port_t port);
extern bootstrap_t *lookup_bootstrap_by_port(port_t port);
extern bootstrap_t *lookup_bootstrap_req_by_port(port_t port);
extern service_t *lookup_service_by_name(bootstrap_t *bootstrap, name_t name);
extern service_t *lookup_service_by_port(port_t port);
extern server_t *find_init_server(void);
extern void delete_service(service_t *servicep);
extern void delete_bootstrap(bootstrap_t *bootstrap);
extern void *ckmalloc(unsigned nbytes);

extern bootstrap_t bootstraps;		/* head of list of bootstrap ports */
extern server_t servers;		/* head of list of all servers */
extern service_t services;		/* head of list of all services */
extern unsigned nservices;		/* number of services in list */

#define	FIRST(q)		((q).next)
#define	NEXT(qe)		((qe)->next)
#define	PREV(qe)		((qe)->prev)
#define	IS_END(qe, q)		((qe) == &(q))
