#ifndef lint
static char sccsid[] = 	"@(#)rpc_clntout.c	1.2 88/05/08 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * @(#) from SUN 1.5
 */

/*
 * rpc_clntout.c, Client-stub outputter for the RPC protocol compiler
 */
#include <stdio.h>
#include <strings.h>
#include "rpc_parse.h"
#include "rpc_util.h"

#define DEFAULT_TIMEOUT 25	/* in seconds */

#ifdef NeXT_MOD
static write_program();
static char *ampr();
static printbody();
#endif NeXT_MOD

void
write_stubs()
{
	list *l;
	definition *def;

	f_print(fout, 
		"\n/* Default timeout can be changed using clnt_control() */\n");
	f_print(fout, "static struct timeval TIMEOUT = { %d, 0 };\n",
		DEFAULT_TIMEOUT);
	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind == DEF_PROGRAM) {
			write_program(def);
		}
	}
}


static
write_program(def)
	definition *def;
{
	version_list *vp;
	proc_list *proc;

	for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
		for (proc = vp->procs; proc != NULL; proc = proc->next) {
			f_print(fout, "\n");
			ptype(proc->res_prefix, proc->res_type, 1);
			f_print(fout, "*\n");
			pvname(proc->proc_name, vp->vers_num);
			f_print(fout, "(argp, clnt)\n");
			f_print(fout, "\t");
			ptype(proc->arg_prefix, proc->arg_type, 1);
			f_print(fout, "*argp;\n");
			f_print(fout, "\tCLIENT *clnt;\n");
			f_print(fout, "{\n");
			printbody(proc);
			f_print(fout, "}\n\n");
		}
	}
}


static char *
ampr(type)
	char *type;
{
	if (isvectordef(type, REL_ALIAS)) {
		return ("");
	} else {
		return ("&");
	}
}

static
printbody(proc)
	proc_list *proc;
{
	f_print(fout, "\tstatic ");
	if (streq(proc->res_type, "void")) {
		f_print(fout, "char ");
	} else {
		ptype(proc->res_prefix, proc->res_type, 0);
	}
	f_print(fout, "res;\n");
	f_print(fout, "\n");
	f_print(fout, "\tbzero((char *)%sres, sizeof(res));\n", 
		ampr(proc->res_type));
	f_print(fout,
		"\tif (clnt_call(clnt, %s, xdr_%s, argp, xdr_%s, %sres, TIMEOUT) != RPC_SUCCESS) {\n",
		proc->proc_name, stringfix(proc->arg_type), 
		stringfix(proc->res_type), ampr(proc->res_type));
	f_print(fout, "\t\treturn (NULL);\n");
	f_print(fout, "\t}\n");
	if (streq(proc->res_type, "void")) {
		f_print(fout, "\treturn ((void *)%sres);\n", 
			ampr(proc->res_type));
	} else {
		f_print(fout, "\treturn (%sres);\n", ampr(proc->res_type));
	}
}
