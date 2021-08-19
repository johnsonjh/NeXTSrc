#ifndef lint
static char sccsid[] = 	"@(#)rpc_main.c	1.2 88/05/08 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * @(#) from SUN 1.11
 */

/*
 * rpc_main.c, Top level of the RPC protocol compiler. 
 */

#include <stdio.h>
#include <strings.h>
#include <sys/file.h>
#include "rpc_util.h"
#include "rpc_parse.h"
#include "rpc_scan.h"

#define EXTEND	1		/* alias for TRUE */

struct commandline {
	int cflag;
	int hflag;
	int lflag;
	int sflag;
	int mflag;
	char *infile;
	char *outfile;
};

static char *cmdname;
static char CPP[] = "/lib/cpp";
static char CPPFLAGS[] = "-C";
static char *allv[] = {
	"rpcgen", "-s", "udp", "-s", "tcp",
};
static int allc = sizeof(allv)/sizeof(allv[0]);

#ifdef NeXT_MOD
static char *extendfile();
static open_output();
static open_input();
static c_output();
static h_output();
static s_output();
static l_output();
static do_registers();
static parseargs();
#endif NeXT_MOD

main(argc, argv)
	int argc;
	char *argv[];
{
	struct commandline cmd;

	if (!parseargs(argc, argv, &cmd)) {
		f_print(stderr,
			"usage: %s infile\n", cmdname);
		f_print(stderr,
			"       %s [-c | -h | -l | -m] [-o outfile] [infile]\n",
			cmdname);
		f_print(stderr,
			"       %s [-s udp|tcp]* [-o outfile] [infile]\n",
			cmdname);
		exit(1);
	}
	if (cmd.cflag) {
		c_output(cmd.infile, "-DRPC_XDR", !EXTEND, cmd.outfile);
	} else if (cmd.hflag) {
		h_output(cmd.infile, "-DRPC_HDR", !EXTEND, cmd.outfile);
	} else if (cmd.lflag) {
		l_output(cmd.infile, "-DRPC_CLNT", !EXTEND, cmd.outfile);
	} else if (cmd.sflag || cmd.mflag) {
		s_output(argc, argv, cmd.infile, "-DRPC_SVC", !EXTEND, 
			 cmd.outfile, cmd.mflag);
	} else {
		c_output(cmd.infile, "-DRPC_XDR", EXTEND, "_xdr.c");
		reinitialize();
		h_output(cmd.infile, "-DRPC_HDR", EXTEND, ".h");
		reinitialize();
		l_output(cmd.infile, "-DRPC_CLNT", EXTEND, "_clnt.c");
		reinitialize();
		s_output(allc, allv, cmd.infile, "-DRPC_SVC", EXTEND, 
			 "_svc.c", cmd.mflag);
	}
	exit(0);
}

/*
 * add extension to filename 
 */
static char *
extendfile(file, ext)
	char *file;
	char *ext;
{
	char *res;
	char *p;

	res = alloc(strlen(file) + strlen(ext) + 1);
	if (res == NULL) {
		abort();
	}
	p = rindex(file, '.');
	if (p == NULL) {
		p = file + strlen(file);
	}
	(void) strcpy(res, file);
	(void) strcpy(res + (p - file), ext);
	return (res);
}

/*
 * Open output file with given extension 
 */
static
open_output(infile, outfile)
	char *infile;
	char *outfile;
{
	if (outfile == NULL) {
		fout = stdout;
		return;
	}
	if (infile != NULL && streq(outfile, infile)) {
		f_print(stderr, "%s: output would overwrite %s\n", cmdname,
			infile);
		crash();
	}
	fout = fopen(outfile, "w");
	if (fout == NULL) {
		f_print(stderr, "%s: unable to open ", cmdname);
		perror(outfile);
		crash();
	}
	record_open(outfile);
}

/*
 * Open input file with given define for C-preprocessor 
 */
static
open_input(infile, define)
	char *infile;
	char *define;
{
	int pd[2];

	infilename = (infile == NULL) ? "<stdin>" : infile;
	(void) pipe(pd);
	switch (fork()) {
	case 0:
		(void) close(1);
		(void) dup2(pd[1], 1);
		(void) close(pd[0]);
		execl(CPP, CPP, CPPFLAGS, define, infile, NULL);
		perror("execl");
		exit(1);
	case -1:
		perror("fork");
		exit(1);
	}
	(void) close(pd[1]);
	fin = fdopen(pd[0], "r");
	if (fin == NULL) {
		f_print(stderr, "%s: ", cmdname);
		perror(infilename);
		crash();
	}
}

/*
 * Compile into an XDR routine output file
 */
static
c_output(infile, define, extend, outfile)
	char *infile;
	char *define;
	int extend;
	char *outfile;
{
	definition *def;
	char *include;
	char *outfilename;
	long tell;

	open_input(infile, define);	
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	f_print(fout, "#include <rpc/rpc.h>\n");
	if (infile && (include = extendfile(infile, ".h"))) {
		f_print(fout, "#include \"%s\"\n", include);
		free(include);
	}
	tell = ftell(fout);
	while (def = get_definition()) {
		emit(def);
	}
	if (extend && tell == ftell(fout)) {
		(void) unlink(outfilename);
	}
}

/*
 * Compile into an XDR header file
 */
static
h_output(infile, define, extend, outfile)
	char *infile;
	char *define;
	int extend;
	char *outfile;
{
	definition *def;
	char *outfilename;
	long tell;

	open_input(infile, define);
	outfilename =  extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	tell = ftell(fout);
	while (def = get_definition()) {
		print_datadef(def);
	}
	if (extend && tell == ftell(fout)) {
		(void) unlink(outfilename);
	}
}

/*
 * Compile into an RPC service
 */
static
s_output(argc, argv, infile, define, extend, outfile, nomain)
	int argc;
	char *argv[];
	char *infile;
	char *define;
	int extend;
	char *outfile;
	int nomain;
{
	char *include;
	definition *def;
	int foundprogram;
	char *outfilename;

	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	f_print(fout, "#include <stdio.h>\n");
	f_print(fout, "#include <rpc/rpc.h>\n");
	if (infile && (include = extendfile(infile, ".h"))) {
		f_print(fout, "#include \"%s\"\n", include);
		free(include);
	}
	foundprogram = 0;
	while (def = get_definition()) {
		foundprogram |= (def->def_kind == DEF_PROGRAM);
	}
	if (extend && !foundprogram) {
		(void) unlink(outfilename);
		return;
	}
	if (nomain) {
		write_programs((char *)NULL);
	} else {
		write_most();
		do_registers(argc, argv);
		write_rest();
		write_programs("static");
	}
}

static
l_output(infile, define, extend, outfile)
	char *infile;
	char *define;
	int extend;
	char *outfile;
{
	char *include;
	definition *def;
	int foundprogram;
	char *outfilename;

	open_input(infile, define);
	outfilename = extend ? extendfile(infile, outfile) : outfile;
	open_output(infile, outfilename);
	f_print(fout, "#include <rpc/rpc.h>\n");
	f_print(fout, "#include <sys/time.h>\n");
	if (infile && (include = extendfile(infile, ".h"))) {
		f_print(fout, "#include \"%s\"\n", include);
		free(include);
	}
	foundprogram = 0;
	while (def = get_definition()) {
		foundprogram |= (def->def_kind == DEF_PROGRAM);
	}
	if (extend && !foundprogram) {
		(void) unlink(outfilename);
		return;
	}
	write_stubs();
}

/*
 * Perform registrations for service output 
 */
static
do_registers(argc, argv)
	int argc;
	char *argv[];

{
	int i;

	for (i = 1; i < argc; i++) {
		if (streq(argv[i], "-s")) {
			write_register(argv[i + 1]);
			i++;
		}
	}
}

/*
 * Parse command line arguments 
 */
static
parseargs(argc, argv, cmd)
	int argc;
	char *argv[];
	struct commandline *cmd;

{
	int i;
	int j;
	char c;
	char flag[(1 << 8 * sizeof(char))];
	int nflags;

	cmdname = argv[0];
	cmd->infile = cmd->outfile = NULL;
	if (argc < 2) {
		return (0);
	}
	flag['c'] = 0;
	flag['h'] = 0;
	flag['s'] = 0;
	flag['o'] = 0;
	flag['l'] = 0;
	flag['m'] = 0;
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			if (cmd->infile) {
				return (0);
			}
			cmd->infile = argv[i];
		} else {
			for (j = 1; argv[i][j] != 0; j++) {
				c = argv[i][j];
				switch (c) {
				case 'c':
				case 'h':
				case 'l':
				case 'm':
					if (flag[c]) {
						return (0);
					}
					flag[c] = 1;
					break;
				case 'o':
				case 's':
					if (argv[i][j - 1] != '-' || 
					    argv[i][j + 1] != 0) {
						return (0);
					}
					flag[c] = 1;
					if (++i == argc) {
						return (0);
					}
					if (c == 's') {
						if (!streq(argv[i], "udp") &&
						    !streq(argv[i], "tcp")) {
							return (0);
						}
					} else if (c == 'o') {
						if (cmd->outfile) {
							return (0);
						}
						cmd->outfile = argv[i];
					}
					goto nextarg;

				default:
					return (0);
				}
			}
	nextarg:
			;
		}
	}
	cmd->cflag = flag['c'];
	cmd->hflag = flag['h'];
	cmd->sflag = flag['s'];
	cmd->lflag = flag['l'];
	cmd->mflag = flag['m'];
	nflags = cmd->cflag + cmd->hflag + cmd->sflag + cmd->lflag + cmd->mflag;
	if (nflags == 0) {
		if (cmd->outfile != NULL || cmd->infile == NULL) {
			return (0);
		}
	} else if (nflags > 1) {
		return (0);
	}
	return (1);
}
