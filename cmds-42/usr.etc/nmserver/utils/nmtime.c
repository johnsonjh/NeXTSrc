/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * nmtime.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/utils/RCS/nmtime.c,v $
 *
 */
#ifndef	lint
static char     rcsid[] = "$Header: nmtime.c,v 1.1 88/09/30 15:47:11 osdev Exp $";
#endif not lint
/*
 * Program to measure the timing of various network server operations.
 */

/*
 * HISTORY: 
 * 21-Jul-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Renamed variable to "inline" to "is_inline" to avoid conflicts
 *	with compilers that think "inline" is a keyword.
 *
 *  8-Dec-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Made faster by reusing as much data as possible in the
 *	message buffer.
 *
 *  7-Dec-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Created.
 *
 */


#include	"debug.h"
#include	"trace.h"
#include	"nm_defs.h"

#include	<stdio.h>
#include	<mach.h>
#include	<servers/netname.h>
#include	<sys/message.h>
#include	<msg_type.h>
#include	<sys/time.h>

#define readln(p) while (((getc(p))!='\n') && (!feof(p)))
#define NETNAME_NAME	"NEW_NETWORK_NAME_SERVER"


/*
 * ID's for test messages.
 */
#define	NM_TIME_READ	10001
#define	NM_TIME_WRITE	10002


/*
 * Global variables
 */
static port_t		nn_port;		/* port to new network server */
static char		service_name[80];	/* nmtime service */
static char		echo_name[80];		/* name of the echo server */
static port_t		echo_port;		/* port for current echo server */
static port_t		reply_port;
static struct {
	boolean_t	dealloc;		/* dealloc ool sections */
} options;


/*
 * Message buffer.
 */
#define	TIME_DATA_MAX	\
		(MSG_SIZE_MAX - (sizeof(msg_header_t) + sizeof(msg_type_long_t)))

struct msg_buff {
	msg_header_t	header;
	msg_type_long_t	desc;
	union {
		int		size;
		char		inline_data[TIME_DATA_MAX];
		char		*ool_ptr;
	}		msg_data;
} msg_buff;

#define DEALLOC_OOL(buf) {						\
	(void) vm_deallocate(task_self(),buf.msg_data.ool_ptr,		\
				buf.desc.msg_type_long_number);		\
}


/*
 * Tracing values.
 */
int	trace_recursion_level = 0;
#undef	tracing_on
int	tracing_on = 0;


/*
 * input --
 *
 * Obtain user input, allowing multiple inputs on the same line.
 *
 * Parameters:
 *
 * prompt: string to use as a prompt.
 * format: specification for data required.
 * OUT var: address where to put the data read.
 *
 * Results:
 *
 * Side effects:
 *
 * May print a prompt on stdout, and start a read on stdin.
 *
 * Design:
 *
 * Note:
 *
 */
void input(prompt, format, var)
char *prompt;
char *format;
char *var;
BEGIN("input")
	char c;
	int fcnt;

	while ((stdin->_cnt > 0) &&
	    ((((c = *(stdin)->_ptr&0377) == ' ') || (c == '\n'))))
		getc(stdin);
	fcnt = stdin->_cnt;

	if (fcnt == 0) {
		fprintf(stdout, "%s", prompt);
		fflush(stdout);
	}
	fscanf(stdin, format, var);

	RET;
END


/*
 * printmenu --
 *
 * Parameters: none
 *
 * Results:
 *
 * Side effects:
 *
 * Print a command menu on stdout.
 *
 * Design:
 *
 * Note:
 *
 */
void printmenu()
BEGIN("printmenu")
	fprintf(stdout, "\n");
	fprintf(stdout, "This programs allows to measure the performance of a network server.\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "Available commands:\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "    H,?.......Print this message\n");
	fprintf(stdout, "    C.........(Re)connect to network server\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "    L.........Look up echo server <host>\n");
	fprintf(stdout, "\n");
	fprintf(stdout, " Options:\n");
	fprintf(stdout, "       OD.........Deallocate OOL sections received <T/F>\n");
	fprintf(stdout, "       OE.........Set service name for echo server <name>\n");
	fprintf(stdout, "       OL.........List option settings\n");
	fprintf(stdout, "\n");
	fprintf(stdout, " Read transactions (data in the reply):\n");
	fprintf(stdout, "       RI.........IPC read <size> <count>\n");
	fprintf(stdout, "       RR.........RPC read <size> <count>\n");
	fprintf(stdout, "\n");
	fprintf(stdout, " Write transactions (data in the request):\n");
	fprintf(stdout, "       WI.........IPC write <size> <count>\n");
	fprintf(stdout, "       WR.........RPC write <size> <count>\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "    E.........Start echo service (does not return)\n");
	fprintf(stdout, "\n");
	fprintf(stdout, "    Q.........Quit\n");
	fprintf(stdout, "\n");
	RET;
END


/*
 * nmconnnect --
 *
 * Connect to the network name server.
 */
void nmconnect()
BEGIN("nmconnect")
	kern_return_t	kr;

	kr = netname_look_up(name_server_port, "", NETNAME_NAME, &nn_port);
	if (kr == NETNAME_SUCCESS) {
		fprintf(stdout,
			"Look up of new network server succeeds, port = %x.\n",
			nn_port);
	} else {
		fprintf(stdout,
	"Look up of new network server fails, using old network server, port = %x.\n",
			name_server_port);
		nn_port = name_server_port;
	}
	RET;
END


/*
 * lookupecho --
 *
 * Find a new echo server.
 *
 * Parameters:
 *
 * name: name of machine to look for, or NULL.
 *
 * Results:
 *
 * Side effects:
 *
 * May prompt for the name of the machine to control. Sets echo_port and 
 * echo_name to the appropriate value.
 *
 * Design:
 *
 * Note:
 *
 */
void lookupecho()
BEGIN("lookupecho")
	kern_return_t		kr;

	input("Echo server host: ", "%79s", echo_name);

	fprintf(stdout,"Looking up \"%s\" at %s ...\n",service_name,echo_name);
	kr = netname_look_up(nn_port,echo_name,service_name,&echo_port);
	if (kr == NETNAME_SUCCESS) {
		fprintf(stdout,"    [Successful]\n");
	} else {
		fprintf(stdout,"    [Can't find it!  kr = %1d]\n",kr);
		readln(stdin);
	}
	fflush(stdout);

	RET;
END


/*
 * genericopts --
 *
 * Execute any option setting or listing operation.
 */
void genericopts()
BEGIN("genericopts")
	char		subcmd[2];
	boolean_t	*subopt;
	char		newval[2];

#define	OPTVAL(opt)	(options.opt ? "TRUE" : "FALSE")

	subopt = NULL;

	input("Option subcommand: ", "%1s", subcmd);
	switch(subcmd[0]) {
		case 'D':
		case 'd':
			subopt = &options.dealloc;
			break;
		case 'E':
		case 'e':
			input("Echo service name: ", "%79s", service_name);
			break;
		case 'L':
		case 'l':
			fprintf(stdout,"  Deallocate OOL sections received: %s\n",
								OPTVAL(dealloc));
			fprintf(stdout,"  Echo service name: %s\n",service_name);
			break;
		default:
			fprintf(stdout, "Invalid subcommand. Type ? for help.\n");
			readln(stdin);
			RET;
	}

	if (subopt != NULL) {
		input("Option value (T/F): ", "%1s", newval);
		switch(newval[0]) {
			case 'T':
			case 't':
				*subopt = TRUE;
				break;
			case 'F':
			case 'f':
				*subopt = FALSE;
				break;
			default:
				fprintf(stdout, "Invalid option value.\n");
				readln(stdin);
				RET;
		}
	}

	RET;
END


/*
 * genericread --
 *
 * Execute any read transaction.
 */
void genericread()
BEGIN("genericread")
	char		subcmd[2];
	int		input_size;
	int		count;
	int		msg_type;
	int		msg_size;
	int		data_size;
	kern_return_t	kr;
	int		iteration;
	struct timeval	start_time, end_time;
	int		total_time;

	if (echo_port == PORT_NULL) {
		fprintf(stdout, "Must first look up an echo server\n");
		readln(stdin);
		RET;
	}

	input("Msg type: ", "%1s", subcmd);
	switch(subcmd[0]) {
		case 'I':
		case 'i':
			msg_type = MSG_TYPE_NORMAL;
			break;
		case 'R':
		case 'r':
			msg_type = MSG_TYPE_RPC;
			break;
		default:
			fprintf(stdout, "Invalid subcommand. Type ? for help.\n");
			readln(stdin);
			RET;
	}

	input("Data size: ", "%d", &input_size);
	if (input_size <= 0) {
		fprintf(stdout,"Invalid size.\n");
		readln(stdin);
		RET;
	}

	data_size = input_size;
	if (data_size > TIME_DATA_MAX) {
		fprintf(stdout, "Size too large for an inline message.\n");
		readln(stdin);
		RET;
	}

	input("Repeat count: ", "%d", &count);
	if (count <= 0) {
		fprintf(stdout,"Invalid count.\n");
		readln(stdin);
		RET;
	}
	
	/*
	 * Set up the fields in the message that never change.
	 * The echo server must preserve those fields.
	 */
	msg_buff.header.msg_simple = TRUE;
	msg_buff.header.msg_local_port = reply_port;
	msg_buff.header.msg_remote_port = echo_port;
	msg_buff.header.msg_id = NM_TIME_READ;

	msg_buff.desc.msg_type_long_name = MSG_TYPE_BYTE;
	msg_buff.desc.msg_type_long_size = 8;
	msg_buff.desc.msg_type_header.msg_type_inline = TRUE;
	msg_buff.desc.msg_type_header.msg_type_longform = TRUE;
	msg_buff.desc.msg_type_header.msg_type_deallocate = FALSE;

	msg_size = sizeof(msg_header_t) + sizeof(msg_type_long_t) + 4;

	(void) gettimeofday(&start_time, 0);

	for (iteration = 1; iteration <= count; iteration++) {
		msg_buff.header.msg_size = msg_size;
		msg_buff.header.msg_type = msg_type;
		msg_buff.desc.msg_type_long_number = 4;
		msg_buff.msg_data.size = htonl(data_size);

		kr = msg_rpc(&msg_buff, MSG_OPTION_NONE, MSG_SIZE_MAX, 0, 0);
		if (kr != SEND_SUCCESS) {
			fprintf(stdout,"msg_rpc fails on message %d, kr=%d.\n", 
								iteration, kr);
			readln(stdin);
			break;
		}
	}

	(void)gettimeofday(&end_time, 0);
	total_time = ((end_time.tv_sec - start_time.tv_sec) * 1000) +
				((end_time.tv_usec - start_time.tv_usec) / 1000);
	fprintf(stdout,"Sending %d messages took %d milliseconds.\n",
							count, total_time);

	RET;
END


/*
 * genericwrite --
 *
 * Execute any write transaction.
 */
void genericwrite()
BEGIN("genericwrite")
	char		subcmd[2];
	int		input_size;
	int		count;
	int		msg_type;
	int		msg_size;
	boolean_t	is_inline;
	int		data_size;
	char		*ool_ptr = NULL;
	kern_return_t	kr;
	int		iteration;
	struct timeval	start_time, end_time;
	int		total_time;

	if (echo_port == PORT_NULL) {
		fprintf(stdout, "Must first look up an echo server\n");
		readln(stdin);
		RET;
	}

	input("Msg type: ", "%1s", subcmd);
	switch(subcmd[0]) {
		case 'I':
		case 'i':
			msg_type = MSG_TYPE_NORMAL;
			break;
		case 'R':
		case 'r':
			msg_type = MSG_TYPE_RPC;
			break;
		default:
			fprintf(stdout, "Invalid subcommand. Type ? for help.\n");
			readln(stdin);
			RET;
	}

	input("Data size (negative size = OOL): ", "%d", &input_size);
	if (input_size < 0) {
		is_inline = FALSE;
		data_size = - input_size;
	} else {
		is_inline = TRUE;
		data_size = input_size;
		if (data_size > TIME_DATA_MAX) {
			fprintf(stdout, "Size too large for an inline message.\n");
			readln(stdin);
			RET;
		}
	}

	input("Repeat count: ", "%d", &count);
	if (count <= 0) {
		fprintf(stdout,"Invalid count.\n");
		readln(stdin);
		RET;
	}
	
	/*
	 * Set up the fields in the message that never change.
	 * The echo server must preserve those fields.
	 */
	msg_buff.header.msg_local_port = reply_port;
	msg_buff.header.msg_remote_port = echo_port;
	msg_buff.header.msg_id = NM_TIME_WRITE;

	msg_buff.desc.msg_type_long_name = MSG_TYPE_BYTE;
	msg_buff.desc.msg_type_long_size = 8;
	msg_buff.desc.msg_type_long_number = data_size;
	msg_buff.desc.msg_type_header.msg_type_longform = TRUE;
	msg_buff.desc.msg_type_header.msg_type_deallocate = FALSE;

	if (data_size == 0) {
		msg_size = sizeof(msg_header_t);
	} else {
		if (is_inline) {
			msg_size = sizeof(msg_header_t) + 
					sizeof(msg_type_long_t) + data_size;
			msg_buff.desc.msg_type_header.msg_type_inline = TRUE;
		} else {
			ool_ptr = NULL;
			kr = vm_allocate(task_self(),&ool_ptr,data_size,TRUE);
			if (kr != KERN_SUCCESS) {
				fprintf(stdout,"vm_allocate failed: kr=%d\n", kr);
			readln(stdin);
				RET;
			}
			msg_size = sizeof(msg_header_t) +
					sizeof(msg_type_long_t) + sizeof(char *);
			msg_buff.desc.msg_type_header.msg_type_inline = FALSE;
			msg_buff.msg_data.ool_ptr = ool_ptr;
		}
	}

	(void) gettimeofday(&start_time, 0);

	for (iteration = 1; iteration <= count; iteration++) {
		msg_buff.header.msg_simple = is_inline;
		msg_buff.header.msg_size = msg_size;
		msg_buff.header.msg_type = msg_type;

		kr = msg_rpc(&msg_buff, MSG_OPTION_NONE, sizeof(msg_header_t), 0, 0);
		if (kr != SEND_SUCCESS) {
			fprintf(stdout,"msg_rpc fails on message %d, kr=%d.\n", 
								iteration, kr);
			readln(stdin);
			break;
		}
	}

	(void)gettimeofday(&end_time, 0);
	total_time = ((end_time.tv_sec - start_time.tv_sec) * 1000) +
				((end_time.tv_usec - start_time.tv_usec) / 1000);
	fprintf(stdout,"Sending %d messages took %d milliseconds.\n",
							count, total_time);

	if (ool_ptr != NULL) {
		kr = vm_deallocate(task_self(),ool_ptr,data_size);
		if (kr != KERN_SUCCESS) {
			fprintf(stdout,"vm_deallocate failed: kr=%d\n", kr);
			readln(stdin);
		}
	}

	RET;
END


/*
 * echostart --
 *
 * Start echoing messages for another instance of the timer program.
 *
 * Note:
 *	This procedure never returns.
 */
void echostart()
BEGIN("echostart")
	kern_return_t		kr;
	port_t			server_port;
	port_t			dummy_port;
	int			size;

	kr = port_allocate(task_self(), &server_port);
	if (kr != KERN_SUCCESS) {
		fprintf(stdout,"port_allocate of server port failed, kr=%d.\n",kr);
		exit(1);
	}
	kr = port_disable(task_self(), server_port);
	if (kr != KERN_SUCCESS) {
		fprintf(stdout,"port_disable of server port failed, kr=%d.\n",kr);
		exit(1);
	}

	kr = netname_check_in(nn_port, service_name, 0, server_port);
	if (kr != KERN_SUCCESS) {
		fprintf(stdout,"checkin of server port failed, kr=%d.\n",kr);
		exit(1);
	}

	kr = port_allocate(task_self(), &dummy_port);
	kr = port_disable(task_self(), dummy_port);

	msg_buff.header.msg_size = sizeof(msg_header_t);
	msg_buff.header.msg_local_port = server_port;
	msg_buff.header.msg_remote_port = dummy_port;

	fprintf(stdout,"Starting echo service - hit ^C to abort.\n");
	fflush(stdout);

	while (1) {
		msg_buff.header.msg_simple = TRUE;
		msg_buff.header.msg_type = MSG_TYPE_NORMAL;
		kr = msg_rpc(&msg_buff, MSG_OPTION_NONE, MSG_SIZE_MAX, 0, 0);
		if (kr != SEND_SUCCESS) {
			fprintf(stdout,"msg_rpc failed, kr=%d.\n",kr); 
			RET;
		}

		switch (msg_buff.header.msg_id) {
			case NM_TIME_READ:
				/*
				 * We can reuse the msg_type from the request.
				 * Just fix the sizes.
				 */
				size = ntohl(msg_buff.msg_data.size);
				msg_buff.header.msg_size = sizeof(msg_header_t) + 
						sizeof(msg_type_long_t) + size;
				msg_buff.desc.msg_type_long_number = size;
				break;

			case NM_TIME_WRITE:
				if (msg_buff.header.msg_size !=
							sizeof(msg_header_t)) {
					/*
					 * We need to reset the data portion
					 * of the msg before sending it back.
					 */
					if ((options.dealloc) && 
					(msg_buff.header.msg_simple == FALSE)) {
						DEALLOC_OOL(msg_buff);
					}
					msg_buff.header.msg_size = 
							sizeof(msg_header_t);
				}
				break;

			default:
				fprintf(stdout," Received a message with unknown id: %d.\n", msg_buff.header.msg_id);
				fflush(stdout);
				break;
		}
	}

END


/*
 * initialize --
 *
 * Initialize all the test functions.
 */
int initialize()
BEGIN("initialize")
	kern_return_t		kr;

	/*
	 * Connect to name server.
	 */
#ifndef	NeXT
	init_netname(PORT_NULL);
#endif	NeXT
	nmconnect();

	/*
	 * Global variables.
	 */
	echo_port = PORT_NULL;
	kr = port_allocate(task_self(),&reply_port);
	if (kr != KERN_SUCCESS) {
		fprintf(stdout,"port_allocate of reply port failed, kr=%d\n",kr);
		exit(1);
	}
	kr = port_disable(task_self(),reply_port);
	if (kr != KERN_SUCCESS) {
		fprintf(stdout,"port_disable of reply port failed, kr=%d\n",kr);
		exit(1);
	}
	options.dealloc = FALSE;
	strcpy(service_name,"NM_TIME");

	RETURN(1);
END


/*
 * main --
 *
 * Main loop.
 *
 * Parameters:
 *
 * Results:
 *
 * Side effects:
 *
 * Design:
 *
 * Note:
 *
 */
main(argc, argv)
int argc;
char **argv;
BEGIN("main")
	int quit;
	char cmd[2];

	(void)initialize();

	if (argc > 1) {
		if ((argc == 2) && (strcmp(argv[1],"-e") == 0)) {
			echostart();
		} else {
			fprintf(stdout,"Usage: %s [-e]\n",argv[0]);
			exit(1);
		}
	}

	quit = FALSE;
	do {
		input("> ", "%1s", cmd);
		switch (cmd[0]) {
		case 'H':
		case 'h':
		case '?':
			printmenu();
			break;
		case 'C':
		case 'c':
			nmconnect();
			break;
		case 'L':
		case 'l':
			lookupecho();
			break;
		case 'O':
		case 'o':
			genericopts();
			break;
		case 'R':
		case 'r':
			genericread();
			break;
		case 'W':
		case 'w':
			genericwrite();
			break;
		case 'E':
		case 'e':
			echostart();
			break;
		case 'Q':
		case 'q':
			quit = TRUE;
			break;
		default:
			fprintf(stdout, "Invalid command. Type ? for list.\n");
			readln(stdin);
			break;
		}
	} 
	while (!quit);

	fprintf(stdout, "Terminated.\n");
	exit(0);
END


