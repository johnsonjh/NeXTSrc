/*
 * bootstrap -- fundamental service initiator and port server
 * Mike DeMoney, NeXT, Inc.
 * Copyright, 1990.  All rights reserved.
 *
 * parser.c -- configuration file parser
 */
#import "lists.h"
#import "bootstrap_internal.h"
#import "error_log.h"
#import "parser.h"

#import	<sys/boolean.h>
#import <sys/port.h>

#import	<string.h>
#import <libc.h>
#import	<ctype.h>
#import <stdio.h>
#import <kern/sched.h>

#define	MAX_TOKEN_LEN	128

#define	NELEM(x)			(sizeof(x)/sizeof((x)[0]))
#define	LAST_ELEMENT(x)		((x)[NELEM(x)-1])
#define	STREQ(a, b)			(strcmp(a, b) == 0)
#define	NEW(type, num)		((type *)ckmalloc(sizeof(type) * (num)))

typedef enum {
	ASSIGN_TKN, EOF_TKN, FORWARD_TKN, INIT_TKN, MACHPORTS_TKN, NUM_TKN,
	RESTARTABLE_TKN, SELF_TKN, SEMICOLON_TKN, SERVER_TKN, SERVICES_TKN,
	STRING_TKN, ERROR_TKN, PRI_TKN
} token_t;

typedef struct {
	char *string;
	token_t token;
} keyword_t;

static keyword_t keywords[] = {
	{ "forward",			FORWARD_TKN },
	{ "init",			INIT_TKN },
	{ "machports",			MACHPORTS_TKN },
	{ "priority",			PRI_TKN },
	{ "restartable",		RESTARTABLE_TKN },
	{ "self",			SELF_TKN },
	{ "server",			SERVER_TKN },
	{ "services",			SERVICES_TKN },
	{ NULL,				ERROR_TKN }
};

static FILE *conf;
static int (*charget)(void);
static const char *default_conf_ptr;

static char token_string[MAX_TOKEN_LEN];
static token_t current_token;
static int token_num;
static int token_priority;
static int peekc;

static int get_from_conf(void);
static int get_from_default(void);
static boolean_t parse_conf_file(void);
static boolean_t parse_self(void);
static boolean_t parse_server(void);
static boolean_t parse_service(server_t *serverp);
static boolean_t parse_pri(void);
static void advance_token(void);
static token_t keyword_lookup(void);

/*
 * init_config -- read configuration file and build-up initial server and
 * service lists
 *
 * If can't find a suitable bootstrap.conf, use configuration given in
 * bootstrap.c to get system booted so someone can correct bootstrap.conf
 */
void
init_config(void)
{
	boolean_t parse_ok;

	conf = fopen(conf_file, "r");
	if (conf != NULL)
		charget = get_from_conf;
	else {
		error("Can't open %s -- using default configuration", conf_file);
		charget = get_from_default;
	}
	
	parse_ok = parse_conf_file();
	if ( ! parse_ok && charget == get_from_conf) {
		error("Can't parse %s -- using default configuration", conf_file);
		charget = get_from_default;
		init_lists();
		peekc = 0;
		parse_ok = parse_conf_file();
	}
	if ( ! parse_ok )
		fatal("Can't parse default configuration file");
}

/*
 * Function pointer "charget" points at get_from_conf or get_from_default
 */
static int
get_from_conf(void)
{
	return getc(conf);
}

static int
get_from_default(void)
{
	int c;
	
	if (default_conf_ptr == NULL)
		default_conf_ptr = default_conf;
		
	if (c = *default_conf_ptr)
		default_conf_ptr++;
	if (c == '\0')
		c = EOF;
	return c;
}

/*
 * What follows is a simple recursive descent parser
 * ("we don't need no stinkin' yacc")
 */
static boolean_t
parse_conf_file(void)
{
	boolean_t parse_ok, good_parse;
	
	/*
	 * Configuration file syntax (and parsing routines).
	 *
	 * (parse_conf_file)
	 * CONF_FILE := STMT [ ; STMT ]* [ ; ]
	 * STMT := MACHPORTS | SERVER | SERVICE | SELF | forward | initpri
	 *
	 * (parse_server)
	 * SERVER := [ restartable ]  ( server | init ) SERVER_PATH_ARGS [ SERVICE ]
	 *
	 * (parse_service)
	 * SERVICE := services [ SERVICE_DECL ]+
	 * SERVICE_DECL := SERVICE_NAME [ = NUM ]
	 *
	 * (parse_self)
	 * SELF := self [ priority = NUM ] SERVICE_DECL
	 *
	 * Or more simply, just a list of:
	 *
	 * [[restartable] (server|init) SERVER_PATH_ARGS] [ priority = NUM ]
	 *   [services SERVICE_NAME [ = NUM ] [ SERVICE_NAME [ = NUM ]]*] ;
	 *
	 * self [ SERVICE_NAME [ = NUM ] ]+
	 *
	 * [ forward ]
	 *
	 */
	advance_token();
	if (current_token == EOF_TKN) {
		error("Empty configuration file: %s", conf_file);
		return FALSE;
	}
	
	good_parse = TRUE;
	while (current_token != EOF_TKN) {
		parse_ok = TRUE;
		switch (current_token) {
		case RESTARTABLE_TKN:
		case SERVER_TKN:
		case INIT_TKN:
			parse_ok = parse_server();
			break;
		case SERVICES_TKN:
			parse_ok = parse_service(NULL);
			break;
		case SELF_TKN:
			parse_ok = parse_self();
			break;
		case FORWARD_TKN:
			forward_ok = TRUE;
			advance_token();
			break;
		case SEMICOLON_TKN:
			advance_token();
			break;
		case EOF_TKN:
			break;
		default:
			parse_error(token_string, "start of new declaration");
			parse_ok = FALSE;
			break;
		}
		switch (current_token) {
		case SEMICOLON_TKN:
			advance_token();
			break;
		case EOF_TKN:
			break;
		default:
			if (parse_ok)
				parse_error(token_string, "expected ';'");
			/* Try to re-sync with input */
			while (current_token != SEMICOLON_TKN && current_token != EOF_TKN)
				advance_token();
			parse_ok = FALSE;
			break;
		}
		if (! parse_ok)
			good_parse = FALSE;
	}
	return good_parse;
}

static boolean_t
parse_self(void)
{
	name_t name;
	unsigned mach_port_num;
	
	ASSERT(current_token == SELF_TKN);
	advance_token();		/* Skip SELF_TKN */
	if (current_token == PRI_TKN) {
		boolean_t ok;
		ok = parse_pri();
		if (!ok)
			return FALSE;
		init_priority = token_priority;
	}
	while (current_token == STRING_TKN) {
		if (strlen(token_string) >= sizeof(name_t)) {
			parse_error(token_string, "Service name too long");
			return FALSE;
		}
		if (lookup_service_by_name(&bootstraps, token_string) != NULL)
		{
			parse_error(token_string, "Service name previously declared");
			return FALSE;
		}
		strcpy(name, token_string);
		mach_port_num = NO_MACH_PORT;
		advance_token();
		if (current_token == ASSIGN_TKN) {
			advance_token();
			if (current_token == NUM_TKN) {
				if (mach_port_inuse(token_num)) {
					parse_error(token_string, "illegal mach port number");
					return FALSE;
				}
				mach_port_num = token_num;
				advance_token();
			} else {
				parse_error(token_string, "expected number");
				return FALSE;
			}
		}
		(void) new_service(&bootstraps, name, PORT_NULL, ACTIVE, SELF,
				   NULL_SERVER, mach_port_num);
	}
	return TRUE;
}

static boolean_t
parse_server(void)
{
	server_t *serverp;
	servertype_t servertype = SERVER;
	
	if (current_token == RESTARTABLE_TKN) {
		advance_token();
		servertype = RESTARTABLE;
	}
	switch (current_token) {
	case SERVER_TKN:
		advance_token();
		break;
	case INIT_TKN:
		if (find_init_server() != NULL) {
			parse_error(token_string,
				"Can't specify multiple init servers");
			return FALSE;
		}
		if (servertype == RESTARTABLE) {
			parse_error(token_string,
				"Init server can not be restartable");
		 	return FALSE;
		}
		servertype = ETCINIT;
		advance_token();
		break;
	default:
		parse_error(token_string, "expected \"server\" or \"init\"");
		return FALSE;
	}
	if (current_token == PRI_TKN) {
		boolean_t ok;
		ok = parse_pri();
		if (!ok)
			return FALSE;
	} else
		token_priority = BASEPRI_USER;
	if (current_token != STRING_TKN) {
		parse_error(token_string,
			    "expected string giving server to exec");
		return FALSE;
	}
	serverp = new_server(servertype, token_string, token_priority);
	advance_token();
	if (current_token == SERVICES_TKN)
		return parse_service(serverp);
	return TRUE;
}

static boolean_t
parse_service(server_t *serverp)
{
	name_t name;
	unsigned mach_port_num;
	
	ASSERT(current_token == SERVICES_TKN);
	advance_token();		/* Skip SERVICES_TKN */
	while (current_token == STRING_TKN) {
		if (strlen(token_string) >= sizeof(name_t)) {
			parse_error(token_string, "Service name too long");
			return FALSE;
		}
		if (lookup_service_by_name(&bootstraps, token_string) != NULL)
		{
			parse_error(token_string, "Service name previously declared");
			return FALSE;
		}
		strcpy(name, token_string);
		mach_port_num = NO_MACH_PORT;
		advance_token();
		if (current_token == ASSIGN_TKN) {
			advance_token();
			if (current_token == NUM_TKN) {
				if (mach_port_inuse(token_num)) {
					parse_error(token_string, "illegal mach port number");
					return FALSE;
				}
				mach_port_num = token_num;
				advance_token();
			} else {
				parse_error(token_string, "expected number");
				return FALSE;
			}
		}
		(void) new_service(&bootstraps, name, PORT_NULL, !ACTIVE,
				   DECLARED, serverp, mach_port_num);
	}
	return TRUE;
}

/*
 * Parse priority=NUM
 */
static boolean_t
parse_pri(void)
{
	ASSERT(current_token == PRI_TKN);
	advance_token();		/* Skip PRI_TKN */
	if (current_token != ASSIGN_TKN) {
		parse_error(token_string, "expected '='");
		return FALSE;
	}
	advance_token();		/* Skip = */
	if (current_token != NUM_TKN) {
		parse_error(token_string, "expected NUM");
		return FALSE;
	}
	advance_token();		/* Skip NUM */
	token_priority = token_num;
	return TRUE;
}
/*
 * advance_token -- advances input to next token
 *	Anything from a '#' on is comment and ignored
 *	
 * On return:
 *		current_token contains token_t of next token
 *		token_string contains string value of next token
 *		if token was number, token_num contains numeric value of token
 */
static void
advance_token(void)
{
	char *cp;

again:	
	while (peekc == '\0' || isspace(peekc))
		peekc = (*charget)();
	
	/* Skip comments */
	if (peekc == '#') {
		while (peekc != EOF && peekc != '\n')
			peekc = (*charget)();
		goto again;
	}
	
	cp = token_string;
	*cp = '\0';
	
	if (peekc == EOF) {
		current_token = EOF_TKN;
		return;
	}

	if (isalpha(peekc) || peekc == '\\') {
		/*
		 * this only allows names to be alphanumerics, '_', and
		 * backslash escaped characters.
		 * If you want something fancier, use "..."
		 */
		current_token = STRING_TKN;	/* guarantee it's not ERROR_TKN */
		for (; isalnum(peekc) || peekc == '_' || peekc == '\\';
		 peekc = (*charget)()) {
			if (cp >= &LAST_ELEMENT(token_string)) {
				cp = token_string;
				parse_error(token_string, "token too long");
				current_token = ERROR_TKN;
			}
			if (peekc == '\\')
				peekc = (*charget)();
			*cp++ = peekc;
		}
		*cp = '\0';
		if (current_token != ERROR_TKN)
			current_token = keyword_lookup();
		return;
	}
	
	/* Handle "-quoted strings */
	if (peekc == '"') {
		peekc = (*charget)();
		for (; peekc != EOF && peekc != '"'; peekc = (*charget)()) {
			if (cp >= &LAST_ELEMENT(token_string)) {
				cp = token_string;
				parse_error(token_string, "token too long");
				current_token = ERROR_TKN;
			}
			if (peekc == '\\')
				peekc = (*charget)();
			if (peekc == '\n') {
				cp = token_string;
				parse_error(token_string, "Missing \"");
				current_token = ERROR_TKN;
			}
			*cp++ = peekc;
		}
		if (peekc == EOF) {
			cp = token_string;
			parse_error(token_string, "Missing \"");
			current_token = ERROR_TKN;
		} else
			peekc = (*charget)();	/* skip closing " */
		*cp = '\0';
		if (current_token != ERROR_TKN)
			current_token = STRING_TKN;
		return;
	}

	if (isdigit(peekc)) {
		for (token_num = 0; isdigit(peekc); peekc = (*charget)())
			token_num = token_num * 10 + peekc - '0';
		current_token = NUM_TKN;
		return;
	}
	
	if (peekc == ';') {
		peekc = (*charget)();
		current_token = SEMICOLON_TKN;
		return;
	}
	
	if (peekc == '=') {
		peekc = (*charget)();
		current_token = ASSIGN_TKN;
		return;
	}
	
	current_token = ERROR_TKN;
	return;
}	

static token_t
keyword_lookup(void)
{
	keyword_t *kwp;
	
	for (kwp = keywords; kwp->string; kwp++)
		if (STREQ(kwp->string, token_string))
			return kwp->token;
	return STRING_TKN;
}

