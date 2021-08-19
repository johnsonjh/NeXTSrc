/*
 * bootstrap -- fundamental service initiator and port server
 * Mike DeMoney, NeXT, Inc.
 * Copyright, 1990.  All rights reserved.
 *
 * lists.c -- implementation of list handling routines
 */

#import "bootstrap_internal.h"
#import "lists.h"
#import "error_log.h"

#import <sys/boolean.h>
#import <stdlib.h>
#import <string.h>

/*
 * Exports
 */
bootstrap_t bootstraps;		/* head of list of all bootstrap ports */
server_t servers;		/* head of list of all servers */
service_t services;		/* head of list of all services */
unsigned nservices;		/* number of services in list */

/*
 * Private macros
 */
#define	NEW(type, num)	((type *)ckmalloc(sizeof(type) * num))
#define	STREQ(a, b)		(strcmp(a, b) == 0)
#define	NELEM(x)			(sizeof(x)/sizeof((x)[0]))
#define	LAST_ELEMENT(x)		((x)[NELEM(x)-1])

void
init_lists(void)
{
	bootstraps.next = bootstraps.prev = &bootstraps;
	servers.next = servers.prev = &servers;
	services.next = services.prev = &services;
	nservices = 0;
}

server_t *
new_server(
	servertype_t	servertype,
	const char	*cmd,
	int		priority)
{
	server_t *serverp;

	debug("adding new server \"%s\" with priority %d\n", cmd, priority);	
	serverp = NEW(server_t, 1);
	if (serverp != NULL) {
		/* Doubly linked list */
		servers.prev->next = serverp;
		serverp->prev = servers.prev;
		serverp->next = &servers;
		servers.prev = serverp;

		serverp->port = PORT_NULL;
		serverp->servertype = servertype;
		serverp->priority = priority;
		strncpy(serverp->cmd, cmd, sizeof serverp->cmd);
		LAST_ELEMENT(serverp->cmd) = '\0';
	}
	return serverp;
}
			
service_t *
new_service(
	bootstrap_t	*bootstrap,
	const char	*name,
	port_t		service_port,
	boolean_t	isActive,
	servicetype_t	servicetype,
	server_t	*serverp,
	unsigned	mach_port_num)
{
	service_t *servicep;
	
	servicep = NEW(service_t, 1);
	if (servicep != NULL) {
		/* Doubly linked list */
		services.prev->next = servicep;
		servicep->prev = services.prev;
		servicep->next = &services;
		services.prev = servicep;
		
		nservices += 1;
		
		strncpy(servicep->name, name, sizeof servicep->name);
		LAST_ELEMENT(servicep->name) = '\0';
		servicep->bootstrap = bootstrap;
		servicep->server = serverp;
		servicep->port = service_port;
		servicep->isActive = isActive;
		servicep->servicetype = servicetype;
		servicep->mach_port_num = mach_port_num;
	}
	return servicep;
}

bootstrap_t *
new_bootstrap(
	bootstrap_t	*parent,
	port_name_t	bootstrap_port,
	port_name_t	unpriv_port,
	port_name_t	requestor_port)
{
	bootstrap_t *bootstrap;

	bootstrap = NEW(bootstrap_t, 1);
	if (bootstrap != NULL) {
		/* Doubly linked list */
		bootstraps.prev->next = bootstrap;
		bootstrap->prev = bootstraps.prev;
		bootstrap->next = &bootstraps;
		bootstraps.prev = bootstrap;
		
		bootstrap->bootstrap_port = bootstrap_port;
		bootstrap->unpriv_port = unpriv_port;
		bootstrap->requestor_port = requestor_port;
		bootstrap->parent = parent;
	}
	return bootstrap;
}

bootstrap_t *
lookup_bootstrap_by_port(port_t port)
{
	bootstrap_t *bootstrap;

	for (  bootstrap = FIRST(bootstraps)
	     ; !IS_END(bootstrap, bootstraps)
	     ; bootstrap = NEXT(bootstrap))
	{
		if (bootstrap->bootstrap_port == port)
			return bootstrap;
		if (bootstrap->unpriv_port == port)
			return bootstrap;
	}

	return &bootstraps;
}

bootstrap_t *
lookup_bootstrap_req_by_port(port_t port)
{
	bootstrap_t *bootstrap;

	for (  bootstrap = FIRST(bootstraps)
	     ; !IS_END(bootstrap, bootstraps)
	     ; bootstrap = NEXT(bootstrap))
	{
		if (bootstrap->requestor_port == port)
			return bootstrap;
	}

	return NULL;
}

service_t *
lookup_service_by_name(bootstrap_t *bootstrap, name_t name)
{
	service_t *servicep;

	while (bootstrap) {
		for (  servicep = FIRST(services)
		     ; !IS_END(servicep, services)
		     ; servicep = NEXT(servicep))
		{
			if (!STREQ(name, servicep->name))
				continue;
			if (bootstrap && servicep->bootstrap != bootstrap)
				continue;
			return servicep;
		}
		bootstrap = bootstrap->parent;
	}

	return NULL;
}

service_t *
lookup_service_by_port(port_t port)
{
	service_t *servicep;
	
	for (  servicep = FIRST(services)
	     ; !IS_END(servicep, services)
	     ; servicep = NEXT(servicep))
	{
	  	if (port == servicep->port)
			return servicep;
	}
	return NULL;
}

server_t *
lookup_server_by_task_port(port_t port)
{
	server_t *serverp;
	
	for (  serverp = FIRST(servers)
	     ; !IS_END(serverp, servers)
	     ; serverp = NEXT(serverp))
	{
	  	if (port == serverp->task_port)
			return serverp;
	}
	return NULL;
}

void
delete_service(service_t *servicep)
{
	ASSERT(servicep->prev->next == servicep);
	ASSERT(servicep->next->prev == servicep);
	debug("deleting service %s", servicep->name);
	servicep->prev->next = servicep->next;
	servicep->next->prev = servicep->prev;
	free(servicep);
	nservices -= 1;
}

void
delete_bootstrap(bootstrap_t *bootstrap)
{
	bootstrap_t *child_bootstrap;

	ASSERT(bootstrap->prev->next == bootstrap);
	ASSERT(bootstrap->next->prev == bootstrap);

	for (  child_bootstrap = FIRST(bootstraps)
	     ; !IS_END(child_bootstrap, bootstraps)
	     ; child_bootstrap = NEXT(child_bootstrap))
	{
		if (child_bootstrap->parent == bootstrap)
			delete_bootstrap(child_bootstrap);
	}

	debug("deleting bootstrap %d, unpriv %d, requestor %d",
		bootstrap->bootstrap_port,
		bootstrap->unpriv_port,
		bootstrap->requestor_port);
	bootstrap->prev->next = bootstrap->next;
	bootstrap->next->prev = bootstrap->prev;
	port_deallocate(task_self(), bootstrap->bootstrap_port);
	port_deallocate(task_self(), bootstrap->unpriv_port);
	port_deallocate(task_self(), bootstrap->requestor_port);
	free(bootstrap);
}

server_t *
lookup_server_by_port(port_t port)
{
	server_t *serverp;
	
	for (  serverp = FIRST(servers)
	     ; !IS_END(serverp, servers)
	     ; serverp = NEXT(serverp))
	{
	  	if (port == serverp->port)
			return serverp;
	}
	return NULL;
}

server_t *
find_init_server(void)
{
	server_t *serverp;
	
	for (  serverp = FIRST(servers)
	     ; !IS_END(serverp, servers)
	     ; serverp = NEXT(serverp))
	{
	 	if (serverp->servertype == ETCINIT)
			return serverp;
	}
	return NULL;
}

void *
ckmalloc(unsigned nbytes)
{
	void *cp;
	
	if ((cp = malloc(nbytes)) == NULL)
		fatal("Out of memory");
	return cp;
}


