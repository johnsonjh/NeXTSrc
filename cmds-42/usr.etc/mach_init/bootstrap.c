/*
 * bootstrap -- fundamental service initiator and port server
 * Mike DeMoney, NeXT, Inc.
 * Copyright, 1990.  All rights reserved.
 *
 * bootstrap.c -- implementation of bootstrap main service loop
 */

/*
 * Imports
 */
#import	"bootstrap.h"
#import "bootstrap_internal.h"
#import "lists.h"
#import "error_log.h"
#import "parser.h"

#import	<sys/boolean.h>
#import	<sys/message.h>
#import <sys/notify.h>
#import <sys/task_special_ports.h>
#import <sys/thread_special_ports.h>
#import <sys/ioctl.h>
#import <sys/mig_errors.h>
#import <kern/mach_param.h>
#import	<mach.h>
#import <mach_host.h>
#import	<string.h>
#import	<ctype.h>
#import	<cthreads.h>
#import	<stdio.h>
#import <libc.h>
#import <kern/sched.h>

/* Mig should produce a declaration for this,  but doesn't */
extern boolean_t bootstrap_server(msg_header_t *InHeadP, msg_header_t *OutHeadP);

/*
 * Exports
 */
const char *program_name;	/* our name for error messages */

#ifndef CONF_FILE
#define	CONF_FILE	"/etc/bootstrap.conf"	/* default config file */
#endif CONF_FILE

const char *conf_file = CONF_FILE;

port_all_t backup_port;

/*
 * Bootstrap message sizes (taken from bootstrapServer.c)
 */
union bootstrap_inmsg {
	struct {
		msg_header_t Head;
		msg_type_t service_desiredType;
		port_t service_desired;
	} service_checkin;
	struct {
		msg_header_t Head;
		msg_type_long_t service_nameType;
		name_t service_name;
	} bootstrap_check_in;
	struct {
		msg_header_t Head;
		msg_type_long_t service_nameType;
		name_t service_name;
		msg_type_t service_portType;
		port_t service_port;
	} bootstrap_register;
	struct {
		msg_header_t Head;
		msg_type_long_t service_nameType;
		name_t service_name;
	} bootstrap_look_up;
	struct {
		msg_header_t Head;
		msg_type_long_t service_namesType;
		name_array_t service_names;
	} bootstrap_look_up_array;
	struct {
		msg_header_t Head;
	} bootstrap_get_unpriv_port;
	struct {
		msg_header_t Head;
		msg_type_long_t service_nameType;
		name_t service_name;
	} bootstrap_status;
	struct {
		msg_header_t Head;
	} bootstrap_info;
	struct {
		msg_header_t Head;
		msg_type_t requestor_portType;
		port_t requestor_port;
	} bootstrap_subset;
	struct {
		msg_header_t Head;
		msg_type_long_t service_nameType;
		name_t service_name;
	} bootstrap_create_service;
};
#define BOOTSTRAP_INMSG_SIZE sizeof(union bootstrap_inmsg)

union bootstrap_outmsg {
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t service_grantedType;
		port_t service_granted;
	} service_checkin;
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t isrunningType;
		boolean_t isrunning;
	} service_status;
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t service_portType;
		port_all_t service_port;
	} bootstrap_check_in;
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
	} bootstrap_register;
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t service_portType;
		port_t service_port;
	} bootstrap_look_up;
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_long_t service_portsType;
		port_array_t service_ports;
		msg_type_t all_services_knownType;
		boolean_t all_services_known;
	} bootstrap_look_up_array;
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t unpriv_portType;
		port_t unpriv_port;
	} bootstrap_get_unpriv_port;
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t service_activeType;
		boolean_t service_active;
	} bootstrap_status;
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_long_t service_namesType;
		name_array_t service_names;
		msg_type_long_t server_namesType;
		name_array_t server_names;
		msg_type_long_t service_activeType;
		bool_array_t service_active;
	} bootstrap_info;
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t subset_portType;
		port_t subset_port;
	} bootstrap_subset;
	struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t service_portType;
		port_t service_port;
	} bootstrap_create_service;
};
#define BOOTSTRAP_OUTMSG_SIZE sizeof(union bootstrap_outmsg)

/*
 * Last resort configuration
 *
 * If we can't find a /etc/bootstrap.conf, we use this configuration.
 * The services defined here are compatable with the old mach port stuff,
 * and of course, the names for these services should not be modified without
 * modifying mach_init in libc.
 */
char *default_conf =
	"init \"/usr/etc/init\";"
	"services NetMessage;";
	
port_t inherited_bootstrap_port = PORT_NULL;
boolean_t forward_ok = FALSE;
boolean_t debugging = FALSE;
int init_priority = BASEPRI_USER;

/*
 * Private macros
 */
#define	NELEM(x)		(sizeof(x)/sizeof(x[0]))
#define	END_OF(x)		(&(x)[NELEM(x)])
#define	streq(a,b)		(strcmp(a,b) == 0)

/*
 * Private declarations
 */	
static void add_init_arg(const char *arg);
static void wait_for_go(port_t init_notify_port);
static void init_ports(void);
static void set_machports(task_t target_task);
static void start_server(server_t *serverp);
static void unblock_init(server_t *serverp, task_t init_task);
static void exec_server(server_t *serverp);
static char **argvize(const char *string);
static void server_loop(void);
static void msg_destroy(msg_header_t *m);

/*
 * Private ports we hold receive rights for.  We also hold receive rights
 * for all the privileged ports.  Those are maintained in the server
 * structs.
 */
port_set_name_t bootstrap_port_set;
static port_all_t notify_port;

static int init_pid;
static int running_servers;
static char init_args[BOOTSTRAP_MAX_CMD_LEN];

/* It was a bozo who made main return a value!  Civil disobedience, here. */
void
main(int argc, const char * const argv[])
{
	const char *argp;
	char c;
	server_t *init_serverp;
	server_t *serverp;
	port_t init_notify_port;
	kern_return_t result;
	int pid;
	int force_fork = FALSE;
	processor_set_t pset, pset_priv;
#ifdef	NeXT
	extern void exit(int);
	
	signal (SIGUSR2, exit);
#endif	NeXT

	/* Initialize error handling */
 	program_name = rindex(*argv, '/');
	if (program_name)
		program_name++;
	else
		program_name = *argv;
 	argv++; argc--;

	init_pid = getpid();
	init_errlog(init_pid == 1);
	
	/* Initialize globals */
	init_lists();

	/* Parse command line args */
	while (argc > 0 && **argv == '-') {
		boolean_t init_arg = FALSE;
		argp = *argv++ + 1; argc--;
		while (*argp) {
			switch (c = *argp++) {
			case 'd':
				debugging = TRUE;
				break;
			case 'D':
				debugging = FALSE;
				break;
			case 'F':
				force_fork = TRUE;
				break;
			case 'f':
				if (argc > 0) {
					conf_file = *argv++; argc--;
				} else
					fatal("-c requires config file name");
				break;
			case '-':
				add_init_arg(argp);
				goto copyargs;
			default:
				init_arg = TRUE;
				break;
			}
		}
		if (init_arg)
			add_init_arg(argv[-1]);
	}
    copyargs:
	while (argc != 0) {
		argc--;
		add_init_arg(*argv++);
	}

	log("Started");

	/* Parse the config file */
	init_config();

	/*
	 * Enable the interactive policy.
	 */
	result = processor_set_default(host_self(), &pset);
	if (result != KERN_SUCCESS)
		kern_error(result, "processor_set_default");

	result = host_processor_set_priv(host_priv_self(), pset, &pset_priv);
	if (result != KERN_SUCCESS)
		kern_error(result, "host_processor_set_priv");

	
	result = processor_set_policy_enable(pset_priv, POLICY_INTERACTIVE);
	if (result != KERN_SUCCESS)
		kern_error(result, "processor_set_policy_enable %d",
			POLICY_INTERACTIVE);

	/*
	 * If we have to run init as pid 1, use notify port to
	 * synchronize init and bootstrap
	 */
	if ((init_serverp = find_init_server()) != NULL) {
		if (init_pid != 1 && ! debugging)
			fatal("Can't start init server if not pid 1");
		result = port_allocate(task_self(), &init_notify_port);
		if (result != KERN_SUCCESS)
			kern_fatal(result, "port_allocate");
		result = task_set_notify_port(task_self(), init_notify_port);
		if (result != KERN_SUCCESS)
			kern_fatal(result, "task_set_notify_port");
		debug("Set port %d for parent proc notify port",
		      init_notify_port);
	} else if (init_args[0] != '\0')
		fatal("Extraneous command line arguments");
	
	/* Fork if either not debugging or running /etc/init */
	if (force_fork || !debugging || init_serverp != NULL) {
		pid = fork();
		if (pid < 0)
			unix_fatal("fork");
	} else
		pid = 0;
	
	/* Bootstrap service runs in child (if there is one) */
	if (pid == 0) {	/* CHILD */
		/*
		 * If we're initiated by pid 1, we shouldn't get ever get 
		 * killed; designate ourselves as an "init process".
		 *
		 * This should go away with new signal stuff!
		 */
		if (init_pid == 1)
			init_process();
		
		/* Create declared service ports, our service port, etc */	
		init_ports();

		/* Set up mach ports (if necessary) */
		set_machports(task_self());

		/* Kick off all server processes */
		for (  serverp = FIRST(servers)
		     ; !IS_END(serverp, servers)
		     ; serverp = NEXT(serverp))
			start_server(serverp);
		
		/*
		 * If our priority's to be changed, set it up here.
		 */
		if (init_priority >= 0) {
			result = task_priority(task_self(), init_priority,
				TRUE);
			if (result != KERN_SUCCESS)
				kern_error(result, "task_priority %d",
					init_priority);
		}

		/* Process bootstrap service requests */
		server_loop();	/* Should never return */
		exit(1);
	}
	
	/* PARENT */
	if (init_serverp != NULL) {
		int i;

		strncat(init_serverp->cmd,
			init_args,
			sizeof(init_serverp->cmd));
		/*
		 * Wait, then either exec /etc/init or exit
		 * (which panics the system)
		 */
		for (i = 3; i; i--)
			close(i);
		wait_for_go(init_notify_port);
		exec_server(init_serverp);
		exit(1);
	}
	exit(0);
}

static void
add_init_arg(const char *arg)
{
	strncat(init_args, " ", sizeof(init_args));
	strncat(init_args, arg, sizeof(init_args));
}

static void
wait_for_go(port_t init_notify_port)
{
	msg_header_t init_go_msg;
	kern_return_t result;
	
	/*
	 * For now, we just blindly wait until we receive a message or
	 * timeout.  We don't expect any notifications, and if we get one,
	 * it probably means something dire has happened; so we might as
	 * well give a shot at letting init run.
	 */	
	init_go_msg.msg_local_port = init_notify_port;
	init_go_msg.msg_size = sizeof(init_go_msg);
	result = msg_receive(&init_go_msg, RCV_TIMEOUT, 60 * 1000);
	if (result != KERN_SUCCESS)
		kern_error(result, "msg_receive: init started");
}

static void
init_ports(void)
{
	kern_return_t result;
	service_t *servicep;
	port_name_t previous;

	/* get inherited bootstrap port */
	result = task_get_bootstrap_port(task_self(),
					 &inherited_bootstrap_port);
	if (result != KERN_SUCCESS)
		kern_fatal(result, "task_get_bootstrap_port");

	/* We set this explicitly as we start each child */
	task_set_bootstrap_port(task_self(), PORT_NULL);
	if (inherited_bootstrap_port == PORT_NULL)
		forward_ok = FALSE;
	
	/* Create port set that server loop listens to */
	result = port_set_allocate(task_self(), &bootstrap_port_set);
	if (result != KERN_SUCCESS)
		kern_fatal(result, "port_set_allocate");
	
	/* Create notify port and add to server port set */
	result = port_allocate(task_self(), &notify_port);
	if (result != KERN_SUCCESS)
		kern_fatal(result, "port_allocate");
	result = task_set_notify_port(task_self(), notify_port);
	if (result != KERN_SUCCESS)
		kern_fatal(result, "task_set_notify_port");
	result = port_set_add(task_self(), bootstrap_port_set, notify_port);
	if (result != KERN_SUCCESS)
		kern_fatal(result, "port_set_add");
	
	/* Create backup port and add to server port set */
	result = port_allocate(task_self(), &backup_port);
	if (result != KERN_SUCCESS)
		kern_fatal(result, "port_allocate");
	result = port_set_add(task_self(), bootstrap_port_set, backup_port);
	if (result != KERN_SUCCESS)
		kern_fatal(result, "port_set_add");
	
	/* Create "lookup only" port and add to server port set */
	result = port_allocate(task_self(), &bootstraps.unpriv_port);
	if (result != KERN_SUCCESS)
		kern_fatal(result, "port_allocate");
	result = port_set_add(task_self(),
			      bootstrap_port_set,
			      bootstraps.unpriv_port);
	if (result != KERN_SUCCESS)
		kern_fatal(result, "port_set_add");

	/* Create "self" port and add to server port set */
	result = port_allocate(task_self(), &bootstraps.bootstrap_port);
	if (result != KERN_SUCCESS)
		kern_fatal(result, "port_allocate");
	result = port_set_add(task_self(), bootstrap_port_set,
			      bootstraps.bootstrap_port);
	if (result != KERN_SUCCESS)
		kern_fatal(result, "port_set_add");
		
	/*
	 * Allocate service ports for declared services.
	 */
	for (  servicep = FIRST(services)
	     ; ! IS_END(servicep, services)
	     ; servicep = NEXT(servicep))
	{
	 	switch (servicep->servicetype) {
		case DECLARED:
	 		result = port_allocate(task_self(), &(servicep->port));
			info("Declared port %d for service %s",
			      servicep->port,
			      servicep->name);
			if (result != KERN_SUCCESS)
				kern_fatal(result, "port_allocate");
			result = port_set_backup(task_self(),
						 servicep->port,
						 backup_port,
						 &previous);
			if (result != KERN_SUCCESS)
				kern_fatal(result, "port_set_backup");
			break;
		case MACHPORT:
			/*
			 * Allocate a mach port.
			 */
			result = port_allocate(task_self(), &(servicep->port));
			info("Allocated port %d bound to service %s",
			      servicep->port, servicep->name);
			if (result != KERN_SUCCESS)
				kern_fatal(result, "port_allocate");
			result = port_set_backup(task_self(),
						    servicep->port,
						    backup_port,
						    &previous);
			if (result != KERN_SUCCESS)
				kern_fatal(result, "port_set_backup");
			break;
		case SELF:
			servicep->port = bootstraps.bootstrap_port;
			servicep->server = new_server(MACHINIT,
				program_name, init_priority);
			info("Set port %d for self port",
			      bootstraps.bootstrap_port);
			break;
		case REGISTERED:
			fatal("Can't allocate REGISTERED port!?!");
			break;
		}
	}
}

static void
set_machports(task_t target_task)
{
	service_t *servicep;
	kern_return_t result;
	port_t mach_ports[TASK_PORT_REGISTER_MAX];
	int i;
	unsigned port_cnt = 0;

	/*
	 * hack compatability routine, delete when complete system rebuilt
	 * using bootstrap port
	 */

	for (  servicep = FIRST(services)
	     ; !IS_END(servicep, services)
	     ; servicep = NEXT(servicep))
	{
		if (servicep->mach_port_num >= port_cnt)
			port_cnt = servicep->mach_port_num + 1;
		if (port_cnt > TASK_PORT_REGISTER_MAX) {
			/*
			 * This can't happen, it's checked when
			 * the config file parsed
			 */
			error("Service %s: Illegal mach port "
				"registration number: %d",
				servicep->name,
				servicep->mach_port_num);
			port_cnt = TASK_PORT_REGISTER_MAX;
		}
	}

	if (port_cnt == 0) {
		debug("No mach ports");
		return;
	}

	debug("Setting mach ports for task %d", target_task);
	for (  servicep = FIRST(services)
	     ; !IS_END(servicep, services)
	     ; servicep = NEXT(servicep))
	{
		if (servicep->mach_port_num != NO_MACH_PORT) {
			ASSERT(canSend(servicep->port));
			mach_ports[servicep->mach_port_num] = servicep->port;
		}
	}
	result = mach_ports_register(target_task, mach_ports, port_cnt);
	if (result != KERN_SUCCESS) {
		kern_error(result, "mach_ports_register");
		return;
	}
	if (debugging)
	    for (i = 0; i < port_cnt; i++)
		debug("New mach port %d is %d", i, mach_ports[i]);
}

static void
start_server(server_t *serverp)
{
	kern_return_t result;
	port_t old_port;
	task_t init_task;
	int pid;
	
	/* get rid of any old server port (this might be a restart) */
	old_port = serverp->port;
	serverp->port = PORT_NULL;
	if (old_port != PORT_NULL)
		msg_destroy_port(old_port);
		
	/* Allocate privileged port for requests from service */
	result = port_allocate(task_self(), &serverp->port);
	info("Allocating port %d for server %s", serverp->port, serverp->cmd);
	if (result != KERN_SUCCESS)	
		kern_fatal(result, "port_allocate");
	/* Add privileged server port to bootstrap port set */
	result = port_set_add(task_self(), bootstrap_port_set, serverp->port);
	if (result != KERN_SUCCESS)
		kern_fatal(result, "port_set_add");

	/*
	 * Do what's appropriate to get bootstrap port and machports setup
	 * in server task
	 */
	switch (serverp->servertype) {
	case ETCINIT:
		/*
		 * This is pid 1 init program -- need to punch stuff
		 * back into pid 1 task rather than create a new task
		 */
		result = task_by_unix_pid(task_self(), init_pid, &init_task);
		info("init_task pid %d task port = %d", init_pid, init_task);
		serverp->task_port = init_task;
		if (result != KERN_SUCCESS)
			kern_fatal(result, "task_by_unix_pid");
		result = task_set_bootstrap_port(init_task, serverp->port);
		if (result != KERN_SUCCESS)
			kern_fatal(result, "task_set_bootstrap_port");
		unblock_init(serverp, init_task);
		break;

	case MACHINIT:
		break;

	case SERVER:
	case RESTARTABLE:
		/* Give trusted service a unique bootstrap port */
		result = task_set_bootstrap_port(task_self(), serverp->port);
		if (result != KERN_SUCCESS)
			kern_fatal(result, "task_set_bootstrap_port");
		/*
		 * Machports are setup for this task,
		 * so new server will inherit them
		 */
		pid = fork();
		if (pid < 0)
			unix_error("fork");
		else if (pid == 0) {	/* CHILD */
			exec_server(serverp);
			exit(1);
		} else {		/* PARENT */
			result = task_set_bootstrap_port(task_self(),
							 PORT_NULL);
			if (result != KERN_SUCCESS)
				kern_fatal(result, "task_set_bootstrap_port");

			result = task_by_unix_pid(task_self(),
						  pid,
						  &serverp->task_port);
			if (result != KERN_SUCCESS)
				kern_fatal(result, "getting server task port");
			running_servers += 1;
		}
		break;
	}
}

static void
unblock_init(server_t *serverp, task_t init_task)
{
	msg_header_t init_go_msg;
	kern_return_t result;
	port_t init_notify_port;

	/*
	 * Proc 1 is blocked in a msg_receive on it's notify port, this lets
	 * it continue
	 */
	result = task_get_notify_port(init_task, &init_notify_port);
	if (result != KERN_SUCCESS)
		kern_fatal(result, "task_get_notify_port");
	bzero(&init_go_msg, sizeof init_go_msg);	
	init_go_msg.msg_simple = TRUE;
	init_go_msg.msg_size = sizeof init_go_msg;
	init_go_msg.msg_type = MSG_TYPE_NORMAL;
	init_go_msg.msg_remote_port = init_notify_port;
	init_go_msg.msg_local_port = PORT_NULL;
	result = msg_send(&init_go_msg, MSG_OPTION_NONE, 0);
	if (result != KERN_SUCCESS)
		kern_fatal(result, "msg_send");
	debug("sent go message");
}

static void
exec_server(server_t *serverp)
{
	int nfds, fd;
	char **argv;
	
	/*
	 * Setup environment for server, someday this should be Mach stuff
	 * rather than Unix crud
	 */
	log("Initiating server %s [pid %d]", serverp->cmd, getpid());
	argv = argvize(serverp->cmd);
	
	nfds = getdtablesize();

	/*
	 * If our priority's to be changed, set it up here.
	 */
	if (serverp->priority >= 0) {
		kern_return_t result;
		result = task_priority(task_self(), serverp->priority,
					 TRUE);
		if (result != KERN_SUCCESS)
			kern_error(result, "task_priority %d",
				serverp->priority);
	}

	close_errlog();

	/*
	 * Mark this process as an "init process" so that it won't be
	 * blown away by init when it starts up (or changes states).
	 */
	if (init_pid == 1) {
		debug("marking process %s as init_process\n", argv[0]);
		init_process();
	}

	for (fd = debugging ? 3 : 0; fd < nfds; fd++)
		close(fd);
	fd = open("/dev/tty", O_RDONLY);
	if (fd >= 0) {
		ioctl(fd, TIOCNOTTY, 0);
		close(fd);
	}

	execvp(argv[0], argv);
	unix_error("Can't exec %s", argv[0]);
}	

static char **
argvize(const char *string)
{
	static char *argv[100], args[1000];
	const char *cp;
	char *argp, term;
	int nargs;
	
	/*
	 * Convert a command line into an argv for execv
	 */
	nargs = 0;
	argp = args;
	
	for (cp = string; *cp;) {
		while (isspace(*cp))
			cp++;
		term = (*cp == '"') ? *cp++ : '\0';
		if (nargs < NELEM(argv))
			argv[nargs++] = argp;
		while (*cp && (term ? *cp != term : !isspace(*cp))
		 && argp < END_OF(args)) {
			if (*cp == '\\')
				cp++;
			*argp++ = *cp;
			if (*cp)
				cp++;
		}
		*argp++ = '\0';
	}
	argv[nargs] = NULL;
	return argv;
}

/*
 * server_loop -- pick requests off our service port and process them
 * Also handles notifications
 */
static void
server_loop(void)
{
    bootstrap_t *bootstrap;
    service_t *servicep;
    server_t *serverp;
    kern_return_t result;
    port_name_t previous;
    union {
    	msg_header_t hdr;
	char body[BOOTSTRAP_INMSG_SIZE];
    } msg;
    union {
    	msg_header_t hdr;
	death_pill_t death;
	char body[BOOTSTRAP_OUTMSG_SIZE];
    } reply;
	    
    for (;;) {
	msg.hdr.msg_local_port = bootstrap_port_set;
	msg.hdr.msg_size = sizeof(msg);
	result = msg_receive(&msg.hdr, MSG_OPTION_NONE, 0);
	if (result != KERN_SUCCESS) {
	    kern_error(result, "msg_receive");
	    continue;
	}

#if	DEBUG
	debug("received message on port %d\n", msg.hdr.msg_local_port);
#endif	DEBUG

	/*
	 * Pick off notification messages
	 */
	if (msg.hdr.msg_local_port == notify_port) {
	    notification_t *not = (notification_t *) &msg.hdr;
	    port_name_t np;

	    switch (not->notify_header.msg_id) {
	    case NOTIFY_PORT_DELETED:
		np = not->notify_port;
#if	DEBUG
		info("Notified port deleted %d", np);
#endif	DEBUG
		if (np == inherited_bootstrap_port)
		{
		    inherited_bootstrap_port = PORT_NULL;
		    forward_ok = FALSE;
		    break;
		}
		
		/*
		 * This may be notification that the task_port associated
		 * with a task we've launched has been deleted.  This
		 * happens only when the server dies.
		 */
		serverp = lookup_server_by_task_port(np);
		if (serverp != NULL) {
		    info("Notified that server %s died\n", serverp->cmd);
		    if (running_servers <= 0) {
			    /* This "can't" happen */
			    running_servers = 0;
			    error("task count error");
		    } else {
			    running_servers -= 1;
			    log("server %s died",
				serverp != NULL ? serverp->cmd : "Unknown");
			    if (serverp != NULL) {
				    /*
				     * FIXME: need to control execs
				     * when server fails immediately
				     */
				    if (   serverp->servertype == RESTARTABLE
					/*
					&& haven't started this recently */)
					    start_server(serverp);
			    }
		    }
		    break;
		}

		/*
		 * Check to see if a subset requestor port was deleted.
		 */
		while (bootstrap = lookup_bootstrap_req_by_port(np)) {
			info("notified that requestor of subset %d died",
				bootstrap->bootstrap_port);
			delete_bootstrap(bootstrap);
		}

		/*
		 * Check to see if a registered/defined service has gone
		 * away.
		 */
		while (servicep = lookup_service_by_port(np)) {
		    /*
		     * Port gone, server died.
		     */
		    info("Received destroyed notification for service %s "
			    "on bootstrap port %d\n",
			    servicep->name, servicep->bootstrap);
		    if (servicep->servicetype == REGISTERED) {
			log("Service %s failed - deallocate", servicep->name);
			msg_destroy_port(servicep->port);
			delete_service(servicep);
		    } else {
			/*
			 * Allocate a new, backed-up port for this service.
			 */
			log("Service %s failed - re-initialize",
			    servicep->name);
			msg_destroy_port(servicep->port);
			result = port_allocate(task_self(), &servicep->port);
			if (result != KERN_SUCCESS)
				kern_fatal(result, "port_allocate");
			result = port_set_backup(task_self(),
						    servicep->port,
						    backup_port,
						    &previous);
			if (result != KERN_SUCCESS)
				kern_fatal(result, "port_set_backup");
			servicep->isActive = FALSE;
		    }
		}
		break;
	    default:
		error("Unexpected notification: %d",
			not->notify_header.msg_id);
		break;
	    }
	} else if (msg.hdr.msg_local_port == backup_port) {
	    notification_t *not = (notification_t *) &msg.hdr;
	    port_name_t np = not->notify_port;

	    /*
	     * Port sent back to us, server died.
	     */
	    info("port %d returned via backup", np);
	    servicep = lookup_service_by_port(np);
	    if (servicep != NULL) {
		info("Received %s notification for service %s",
			not->notify_header.msg_id
		    == NOTIFY_PORT_DESTROYED
			? "destroyed" : "receive rights",
		    servicep->name);
		log("Service %s failed - port backed-up", servicep->name);
		ASSERT(canReceive(servicep->port));
		servicep->isActive = FALSE;
		result = port_set_backup(task_self(),
					 servicep->port,
					 backup_port,
					 &previous);
		if (result != KERN_SUCCESS)
			kern_fatal(result, "port_set_backup");
	    } else
	    	msg_destroy_port(np);
	} else {	/* must be a service request */
	    bootstrap_server(&msg.hdr, &reply.hdr);
#ifdef	DEBUG
	    debug("Handled request.");
#endif	DEBUG
	    if (reply.death.RetCode != MIG_NO_REPLY) {
		result = msg_send(&reply.hdr, SEND_TIMEOUT, 0);
#ifdef	DEBUG
		debug("Reply sent.");
#endif	DEBUG
		if (result != KERN_SUCCESS)
		    kern_error(result, "msg_send");
	    }
	}
	/* deallocate uninteresting ports received in message */
	msg_destroy(&msg.hdr);
    }
}

/*
 * msg_destroy -- walk through a received message and deallocate any
 * useless ports or out-of-line memory
 */
static void
msg_destroy(msg_header_t *m)
{
	msg_type_t *mtp;
	msg_type_long_t *mtlp;
	void *dp;
	unsigned type, size, num;
	boolean_t inlne;
	port_t *pp;
	kern_return_t result;

	msg_destroy_port(m->msg_remote_port);
	for (  mtp = (msg_type_t *)(m + 1)
	     ; (unsigned)mtp - (unsigned)m < m->msg_size
	     ;)
	{
		inlne = mtp->msg_type_inline;
		if (mtp->msg_type_longform) {
			mtlp = (msg_type_long_t *)mtp;
			type = mtlp->msg_type_long_name;
			size = mtlp->msg_type_long_size;
			num = mtlp->msg_type_long_number;
			dp = (void *)(mtlp + 1);
		} else {
			type = mtp->msg_type_name;
			size = mtp->msg_type_size;
			num = mtp->msg_type_number;
			dp = (void *)(mtp + 1);
		}
		if (inlne)
			mtp = (msg_type_t *)(dp + num * (size/BITS_PER_BYTE));
		else {
			mtp = (msg_type_t *)(dp + sizeof(void *));
			dp = *(char **)dp;
		}
		if (MSG_TYPE_PORT_ANY(type)) {
			for (pp = (port_t *)dp; num-- > 0; pp++)
				msg_destroy_port(*pp);
		}
		if ( ! inlne ) {
			result = vm_deallocate(task_self(), (vm_address_t)dp,
			 num * (size/BITS_PER_BYTE));
			if (result != KERN_SUCCESS)
				kern_error(result,
					   "vm_deallocate: msg_destroy");
		}
	}
}

/*
 * msg_destroy_port -- deallocate port if it's not important to bootstrap
 * Bad name, this is used for things other than msg_destroy.
 */
void
msg_destroy_port(port_t p)
{
	if (   p == PORT_NULL
	    || p == task_self()
	    || p == thread_reply()
	    || p == bootstrap_port_set
	    || p == inherited_bootstrap_port
	    || lookup_service_by_port(p)
	    || lookup_server_by_port(p)
	    || lookup_bootstrap_by_port(p) != &bootstraps
	    || lookup_bootstrap_req_by_port(p) != &bootstraps
	    || p == bootstraps.bootstrap_port
	    || p == bootstraps.unpriv_port
	    || p == bootstraps.requestor_port)
		return;

#if	DEBUG
	debug("Deallocating port %d", p);
#endif	DEBUG
	(void) port_deallocate(task_self(), p);
}

boolean_t
mach_port_inuse(int mach_port_num)
{
	service_t *servicep;

	if (mach_port_num >= TASK_PORT_REGISTER_MAX)
		return TRUE;

	for (  servicep = FIRST(services)
	     ; !IS_END(servicep, services)
	     ; servicep = NEXT(servicep))
	{
	 	if (servicep->mach_port_num == mach_port_num)
			return TRUE;
	}
	return FALSE;
}

boolean_t
canReceive(port_t port)
{
	port_type_t p_type;
	kern_return_t result;
	
	result = port_type(task_self(), port, &p_type);
	if (result != KERN_SUCCESS) {
		kern_error(result, "port_type");
		return FALSE;
	}
	return (p_type == PORT_TYPE_RECEIVE_OWN);
}


boolean_t
canSend(port_t port)
{
	port_type_t p_type;
	kern_return_t result;
	
	result = port_type(task_self(), port, &p_type);
	if (result != KERN_SUCCESS) {
		kern_error(result, "port_type");
		return FALSE;
	}
	return (p_type == PORT_TYPE_SEND) || (p_type == PORT_TYPE_RECEIVE_OWN);
}
