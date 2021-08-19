/*	@(#)msg.c	1.6	*/
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */


#include	"defs.h"
#include	"sym.h"

/*
 * error messages
 */
char	badopt[]	= "bad option(s)";
char	mailmsg[]	= "you have mail\n";
char	nospace[]	= "no space";
char	nostack[]	= "no stack space";
char	synmsg[]	= "syntax error";

char	badnum[]	= "bad number";
char	badparam[]	= "parameter null or not set";
char	unset[]		= "parameter not set";
char	badsub[]	= "bad substitution";
char	badcreate[]	= "cannot create";
char	nofork[]	= "fork failed - too many processes";
char	noswap[]	= "cannot fork: no swap space";
char	restricted[]	= "restricted";
char	piperr[]	= "cannot make pipe";
char	badopen[]	= "cannot open";
char	coredump[]	= " - core dumped";
char	arglist[]	= "arg list too long";
char	txtbsy[]	= "text busy";
char	toobig[]	= "too big";
char	badexec[]	= "cannot execute";
char	notfound[]	= "not found";
char	badfile[]	= "bad file number";
char	badshift[]	= "cannot shift";
char	baddir[]	= "bad directory";
char	badtrap[]	= "bad trap";
char	wtfailed[]	= "is read only";
char	notid[]		= "is not an identifier";
char 	badulimit[]	= "bad ulimit";	/* DAG -- lower case */
char	badreturn[] = "cannot return when not in function";
char	badexport[] = "cannot export functions";
char	badunset[] 	= "cannot unset";
char	nohome[]	= "no home directory";
char 	badperm[]	= "execute permission denied";
char	longpwd[]	= "sh error: pwd too long";
char	no11trap[]	= "cannot trap 11";		/* DAG -- made sharable */
char	argcount[]	= "argument count";		/* DAG */
char	neworsemi[]	= "newline or ;";		/* DAG */
char	dotstat[]	= "pwd: cannot stat .";		/* DAG */
char	paropen[]	= "pwd: cannot open ..";	/* DAG */
char	parstat[]	= "pwd: cannot stat ..";	/* DAG */
char	parread[]	= "pwd: read error in ..";	/* DAG */
#if JOBS
char	cjpostr[]	= ": couldn't jpost\n";
char	jcoffstr[]	= "job control not enabled\n";
char	jpanstr[]	= "sh bug: j_print_ent--no number ";
char	jinvstr[]	= "invalid job number\n";
char	ncjstr[]	= "no current job\n";
char	nstpstr[]	= ": not stopped\n";
char	tasjstr[]	= "There are stopped jobs.\n";
#endif
#if SYMLINK
char	nolstat[]	= ": can't lstat component";
#endif
#if BRL
char	badtimeout[]	= "bad timeout value\n";
char	noeot[]		= "use \"exit\" or \"logout\"\n";
#if pdp11
char	quofail[]	= "quota system failed\n";
#endif
#endif
/*
 * messages for 'builtin' functions
 */
char	btest[]		= "test";
char	badop[]		= "unknown operator ";
char	argexp[]	= "argument expected";	/* DAG -- made sharable */
char	parexp[]	= ") expected";		/* DAG */
char	rbmiss[]	= "] missing";		/* DAG */
/*
 * built in names
 */
char	pathname[]	= "PATH";
char	cdpname[]	= "CDPATH";
char	homename[]	= "HOME";
char	mailname[]	= "MAIL";
char	ifsname[]	= "IFS";
char	ps1name[]	= "PS1";
char	ps2name[]	= "PS2";
char	mchkname[]	= "MAILCHECK";
char	acctname[]  	= "SHACCT";
char	mailpname[]	= "MAILPATH";
#if BRL
char	timename[]	= "TIMEOUT";
#endif
#ifdef pyr
char	univname[]	= "UNIVERSE";
#endif

/*
 * string constants
 */
char	nullstr[]	= "";
char	sptbnl[]	= " \t\n";
char	defpath[]	= ":/usr/ucb:/bin:/usr/bin";
char	colon[]		= ": ";
char	minus[]		= "-";
char	endoffile[]	= "end of file";
char	unexpected[] 	= " unexpected";
char	atline[]	= " at line ";
char	devnull[]	= "/dev/null";
char	execpmsg[]	= "+ ";
char	readmsg[]	= "> ";
char	stdprompt[]	= "$ ";
char	supprompt[]	= "# ";
char	profile[]	= ".profile";
#if !defined(BRL) || defined(pdp11) || defined(NeXT)
char	sysprofile[]	= "/etc/profile.std";
#else
char	sysprofile[]	= "/usr/5lib/profile";
#endif
char	setstr[]	= "set";			/* DAG -- made sharable */
char	sicrstr[]	= "sicr";			/* DAG */
char	bang[]		= "!";				/* DAG */
char	pdstr[]		= ".";				/* DAG */
char	dotdot[]	= "..";				/* DAG */
char	slstr[]		= "/";				/* DAG */
char	eqlstr[]	= "=";				/* DAG */
char	neqstr[]	= "!=";				/* DAG */
char	lbstr[]		= "[";				/* DAG */
char	lpstr[]		= "(";				/* DAG */
char	rbstr[]		= "]";				/* DAG */
char	rpstr[]		= ")";				/* DAG */
char	dasheq[]	= "-eq";			/* DAG */
char	dashne[]	= "-ne";			/* DAG */
char	dashgt[]	= "-gt";			/* DAG */
char	dashlt[]	= "-lt";			/* DAG */
char	dashge[]	= "-ge";			/* DAG */
char	dashle[]	= "-le";			/* DAG */
char	dasha[]		= "-a";				/* DAG */
char	dashb[]		= "-b";				/* DAG */
char	dashc[]		= "-c";				/* DAG */
char	dashd[]		= "-d";				/* DAG */
char	dashf[]		= "-f";				/* DAG */
char	dashg[]		= "-g";				/* DAG */
#ifdef NeXT_MOD
char	dashh[]		= "-h";				/* MIKE */
#endif NeXT_MOD
char	dashk[]		= "-k";				/* DAG */
char	dashn[]		= "-n";				/* DAG */
char	dasho[]		= "-o";				/* DAG */
char	dashp[]		= "-p";				/* DAG */
char	dashr[]		= "-r";				/* DAG */
char	dashs[]		= "-s";				/* DAG */
char	dasht[]		= "-t";				/* DAG */
char	dashu[]		= "-u";				/* DAG */
char	dashw[]		= "-w";				/* DAG */
char	dashx[]		= "-x";				/* DAG */
char	dashz[]		= "-z";				/* DAG */
char	ptrcolon[]	= "ptrace: ";			/* DAG */
char	shell[]		= "SHELL";			/* DAG */
char	sfuncstr[]	= "(){\n";			/* DAG */
char	efuncstr[]	= "\n}";			/* DAG */
char	amperstr[]	= " &";				/* DAG */
char	lparstr[]	= "( ";				/* DAG */
char	rparstr[]	= " )";				/* DAG */
char	pipestr[]	= " | ";			/* DAG */
char	andstr[]	= " && ";			/* DAG */
char	orstr[]		= " || ";			/* DAG */
char	forstr[]	= "for ";			/* DAG */
char	instr[]		= " in";			/* DAG */
char	dostr[]		= "\ndo\n";			/* DAG */
char	donestr[]	= "\ndone";			/* DAG */
char	whilestr[]	= "while ";			/* DAG */
char	untilstr[]	= "until ";			/* DAG */
char	ifstr[]		= "if ";			/* DAG */
char	thenstr[]	= "\nthen\n";			/* DAG */
char	elsestr[]	= "\nelse\n";			/* DAG */
char	fistr[]		= "\nfi";			/* DAG */
char	casestr[]	= "case ";			/* DAG */
char	dsemistr[]	= ";;";				/* DAG */
char	esacstr[]	= "\nesac";			/* DAG */
char	fromstr[]	= "<<";				/* DAG */
char	toastr[]	= ">&";				/* DAG */
char	fromastr[]	= "<&";				/* DAG */
char	ontostr[]	= ">>";				/* DAG */
char	hashhdr[]	= "hits\tcost\tcommand\n";	/* DAG */
char	isbuiltin[]	= " is a shell builtin\n";	/* DAG */
char	isfunct[]	= " is a function\n";		/* DAG */
char	efnlstr[]	= "\n}\n";			/* DAG */
char	nfstr[]		= " not found\n";		/* DAG */
char	ishashed[]	= " is hashed (";		/* DAG */
char	rpnlstr[]	= ")\n";			/* DAG */
char	isstr[]		= " is ";			/* DAG */
#if JOBS
char	jshstr[]	= "jsh";
char	rsqbrk[]	= "] ";
char	spspstr[]	= "  ";
char	fgdstr[]	= "foreground       ";
char	stpdstr[]	= "stopped";
char	lotspstr[]	= "          ";
char	psgpstr[]	= " (signal) ";
char	ptinstr[]	= " (tty in) ";
char	ptoustr[]	= " (tty out)";
char	bgdstr[]	= "background       ";
char	spcstr[]	= " ";
char	rdinstr[]	= "< ";
char	appdstr[]	= ">> ";
char	inlnstr[]	= "<< ";
char	sfnstr[]	= "(){ ";
char	efnstr[]	= " }";
char	semspstr[]	= "; ";
char	lpnstr[]	= "(";
char	rpnstr[]	= ")";
char	insstr[]	= " in ";
char	sdostr[]	= "; do ";
char	sdonstr[]	= "; done";
char	sthnstr[]	= "; then ";
char	selsstr[]	= "; else ";
char	sfistr[]	= "; fi";
char	iesacstr[]	= " in ... esac";
#endif
#if BRL
char	drshell[]	= "-rsh";
char	rshell[]	= "rsh";
#if pdp11
char	quota[]		= "/bin/quota";
char	bye[]		= "$";		/* for quota system */
#endif
char	hangup[]	= "hangup";
char	logout[]	= "logout";
char	terminate[]	= "terminate";
char	timout[]	= "timeout";
#if pdp11
char	dsupath[]	= "/bin:/usr/bin:/usr/sbin:.";	/* su default PATH */
#else
char	dsupath[]	= "/bin:/usr/bin:/usr/ucb:/etc:.";
#endif
#ifdef NeXT_MOD
char	deftimout[]	= "0";		/* normal timeout (minutes) */
char	dsutimout[]	= "0";		/* superuser timeout */
#else	!NeXT_MOD
char	deftimout[]	= "30";		/* normal timeout (minutes) */
char	dsutimout[]	= "3";		/* superuser timeout */
#endif	!NeXT_MOD
#endif

/*
 * tables
 */

struct sysnod reserved[] =
{
	{ "case",	CASYM	},
	{ "do",		DOSYM	},
	{ "done",	ODSYM	},
	{ "elif",	EFSYM	},
	{ "else",	ELSYM	},
	{ "esac",	ESSYM	},
	{ "fi",		FISYM	},
	{ "for",	FORSYM	},
	{ "if",		IFSYM	},
	{ "in",		INSYM	},
	{ "then",	THSYM	},
	{ "until",	UNSYM	},
	{ "while",	WHSYM	},
	{ "{",		BRSYM	},
	{ "}",		KTSYM	}
};

int no_reserved = 15;

char	*sysmsg[] =
{
	0,
	"Hangup",
	0,	/* Interrupt */
	"Quit",
	"Illegal instruction",
	"Trace/BPT trap",
	"abort",
	"EMT trap",
	"Floating exception",
	"Killed",
	"Bus error",
	"Memory fault",
	"Bad system call",
	0,	/* Broken pipe */
	"Alarm call",
	"Terminated",
	"Signal 16",
#if defined(BERKELEY) || defined(BRL) && !defined(pdp11)
	"Stop",
	"Stop from keyboard",
	"Continue",
	"Child status change",
	"Background read",
	"Background write",
	"I/O possible",
	"CPU time limit",
	"File size limit",
	"Virtual time alarm",
	"Profiling timer alarm",
#if gould
	"Stack overflow",
#else
	"Signal 28",
#endif
	"Signal 29",
	"Signal 30",
	"Signal 31",
	"Signal 32",
#else
	"Signal 17",
#if BRL
	"Signal 18",
	"Signal 19",
#else
	"Child death",
	"Power Fail"
#endif
#endif
};

char	export[] = "export";
char	duperr[] = "cannot dup";
char	readonly[] = "readonly";


struct sysnod commands[] =
{
	{ pdstr,	SYSDOT	},	/* DAG -- use shared string */
	{ ":",		SYSNULL	},

#ifndef RES
	{ lbstr,	SYSTST },	/* DAG -- use shared string */
#endif

#ifdef pyr
	{ "att",	SYSATT },
#endif

#if JOBS
	{ "bg",		SYSBG },
#endif

	{ "break",	SYSBREAK },
	{ "cd",		SYSCD	},
	{ "continue",	SYSCONT	},
	{ "echo",	SYSECHO },
	{ "eval",	SYSEVAL	},
	{ "exec",	SYSEXEC	},
	{ "exit",	SYSEXIT	},
	{ export,	SYSXPORT },	/* DAG -- use shared string */

#if JOBS
	{ "fg",		SYSFG },
#endif

	{ "hash",	SYSHASH	},

#if JOBS
	{ "jobs",	SYSJOBS },
#endif

#if RES || BERKELEY
#ifndef BRL
	{ "login",	SYSLOGIN },
#endif
#endif

#if BRL
	{ logout,	SYSLOGOUT },
#endif

#ifndef BERKELEY
#ifdef RES
	{ "newgrp",	SYSLOGIN },
#else
	{ "newgrp",	SYSNEWGRP },
#endif
#endif

	{ "pwd",	SYSPWD },
	{ "read",	SYSREAD	},
	{ readonly,	SYSRDONLY },	/* DAG -- use shared string */
	{ "return",	SYSRETURN },
	{ setstr,	SYSSET	},	/* DAG -- use shared string */
	{ "shift",	SYSSHFT	},
	{ "test",	SYSTST },
	{ "times",	SYSTIMES },
	{ "trap",	SYSTRAP	},
	{ "type",	SYSTYPE },

#ifdef pyr
	{ "ucb",	SYSUCB },
#endif

#ifndef RES		
	{ "ulimit",	SYSULIMIT },
	{ "umask",	SYSUMASK },
#endif

#ifdef pyr
	{ "universe",	SYSUNIVERSE },
#endif

	{ "unset", 	SYSUNS },
	{ "wait",	SYSWAIT	}
};

int	no_commands = sizeof commands / sizeof(struct sysnod);	/* DAG -- improved */

#ifdef pyr
#include	<sys/types.h>
#include	<sys/inode.h>
#include	<universe.h>	/* for univ_name[] and univ_longname[] */
#endif
