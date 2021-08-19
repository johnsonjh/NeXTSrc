#ifndef lint
static char sccsid[] = 	"@(#)rpc_svcout.c	1.2 88/05/08 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * @(#) from SUN 1.9
 */

/*
 * rpc_svcout.c, Server-skeleton outputter for the RPC protocol compiler
 */
#include <stdio.h>
#include <strings.h>
#include "rpc_parse.h"
#include "rpc_util.h"

static char RQSTP[] = "rqstp";
static char TRANSP[] = "transp";
static char ARG[] = "argument";
static char RESULT[] = "result";
static char ROUTINE[] = "local";

#ifdef NeXT_MOD
static write_program();
static printerr();
static printif();
#endif NeXT_MOD

/*
 * write most of the service, that is, everything but the registrations. 
 */
void
write_most()
{
	list *l;
	definition *def;
	version_list *vp;

	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind == DEF_PROGRAM) {
			for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
				f_print(fout, "\nstatic void ");
				pvname(def->def_name, vp->vers_num);
				f_print(fout, "();");
			}
		}
	}
	f_print(fout, "\n\n");
	f_print(fout, "main()\n");
	f_print(fout, "{\n");
	f_print(fout, "\tSVCXPRT *%s;\n", TRANSP);
	f_print(fout, "\n");
	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind != DEF_PROGRAM) {
			continue;
		}
		for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
			f_print(fout, "\t(void)pmap_unset(%s, %s);\n", def->def_name, vp->vers_name);
		}
	}
}


/*
 * write a registration for the given transport 
 */
void
write_register(transp)
	char *transp;
{
	list *l;
	definition *def;
	version_list *vp;

	f_print(fout, "\n");
	f_print(fout, "\t%s = svc%s_create(RPC_ANYSOCK", TRANSP, transp);
	if (streq(transp, "tcp")) {
		f_print(fout, ", 0, 0");
	}
	f_print(fout, ");\n");
	f_print(fout, "\tif (%s == NULL) {\n", TRANSP);
	f_print(fout, "\t\t(void)fprintf(stderr, \"cannot create %s service.\\n\");\n", transp);
	f_print(fout, "\t\texit(1);\n");
	f_print(fout, "\t}\n");

	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind != DEF_PROGRAM) {
			continue;
		}
		for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
			f_print(fout,
				"\tif (!svc_register(%s, %s, %s, ",
				TRANSP, def->def_name, vp->vers_name);
			pvname(def->def_name, vp->vers_num);
			f_print(fout, ", IPPROTO_%s)) {\n",
				streq(transp, "udp") ? "UDP" : "TCP");
			f_print(fout,
				"\t\t(void)fprintf(stderr, \"unable to register (%s, %s, %s).\\n\");\n",
				def->def_name, vp->vers_name, transp);
			f_print(fout, "\t\texit(1);\n");
			f_print(fout, "\t}\n");
		}
	}
}


/*
 * write the rest of the service 
 */
void
write_rest()
{
	f_print(fout, "\tsvc_run();\n");
	f_print(fout, "\t(void)fprintf(stderr, \"svc_run returned\\n\");\n");
	f_print(fout, "\texit(1);\n");
	f_print(fout, "}\n");
}

void
write_programs(storage)
	char *storage;
{
	list *l;
	definition *def;

	for (l = defined; l != NULL; l = l->next) {
		def = (definition *) l->val;
		if (def->def_kind == DEF_PROGRAM) {
			write_program(def, storage);
		}
	}
}


static
write_program(def, storage)
	definition *def;
	char *storage;
{
	version_list *vp;
	proc_list *proc;
	int filled;

	for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
		f_print(fout, "\n");
		if (storage != NULL) {
			f_print(fout, "%s ", storage);
		}
		f_print(fout, "void\n");
		pvname(def->def_name, vp->vers_num);
		f_print(fout, "(%s, %s)\n", RQSTP, TRANSP);
		f_print(fout, "	struct svc_req *%s;\n", RQSTP);
		f_print(fout, "	SVCXPRT *%s;\n", TRANSP);
		f_print(fout, "{\n");

		filled = 0;
		f_print(fout, "\tunion {\n");
		for (proc = vp->procs; proc != NULL; proc = proc->next) {
			if (streq(proc->arg_type, "void")) {
				continue;
			}
			filled = 1;
			f_print(fout, "\t\t");
			ptype(proc->arg_prefix, proc->arg_type, 0);
			pvname(proc->proc_name, vp->vers_num);
			f_print(fout, "_arg;\n");
		}
		if (!filled) {
			f_print(fout, "\t\tint fill;\n");
		}
		f_print(fout, "\t} %s;\n", ARG);
		f_print(fout, "\tchar *%s;\n", RESULT);
		f_print(fout, "\tbool_t (*xdr_%s)(), (*xdr_%s)();\n", ARG, RESULT);
		f_print(fout, "\tchar *(*%s)();\n", ROUTINE);
		f_print(fout, "\n");
		f_print(fout, "\tswitch (%s->rq_proc) {\n", RQSTP);

		if (!nullproc(vp->procs)) {
			f_print(fout, "\tcase NULLPROC:\n");
			f_print(fout, "\t\t(void)svc_sendreply(%s, xdr_void, (char *)NULL);\n", TRANSP);
			f_print(fout, "\t\treturn;\n\n");
		}
		for (proc = vp->procs; proc != NULL; proc = proc->next) {
			f_print(fout, "\tcase %s:\n", proc->proc_name);
			f_print(fout, "\t\txdr_%s = xdr_%s;\n", ARG, 
				stringfix(proc->arg_type));
			f_print(fout, "\t\txdr_%s = xdr_%s;\n", RESULT, 
				stringfix(proc->res_type));
			f_print(fout, "\t\t%s = (char *(*)()) ", ROUTINE);
			pvname(proc->proc_name, vp->vers_num);
			f_print(fout, ";\n");
			f_print(fout, "\t\tbreak;\n\n");
		}
		f_print(fout, "\tdefault:\n");
		printerr("noproc", TRANSP);
		f_print(fout, "\t\treturn;\n");
		f_print(fout, "\t}\n");

		f_print(fout, "\tbzero((char *)&%s, sizeof(%s));\n", ARG, ARG);
		printif("getargs", TRANSP, "&", ARG);
		printerr("decode", TRANSP);
		f_print(fout, "\t\treturn;\n");
		f_print(fout, "\t}\n");

		f_print(fout, "\t%s = (*%s)(&%s, %s);\n", RESULT, ROUTINE, ARG,
			RQSTP);
		f_print(fout, 
			"\tif (%s != NULL && !svc_sendreply(%s, xdr_%s, %s)) {\n",
			RESULT, TRANSP, RESULT, RESULT);
		printerr("systemerr", TRANSP);
		f_print(fout, "\t}\n");

		printif("freeargs", TRANSP, "&", ARG);
		f_print(fout, "\t\t(void)fprintf(stderr, \"unable to free arguments\\n\");\n");
		f_print(fout, "\t\texit(1);\n");
		f_print(fout, "\t}\n");

		f_print(fout, "}\n\n");
	}
}

static
printerr(err, transp)
	char *err;
	char *transp;
{
	f_print(fout, "\t\tsvcerr_%s(%s);\n", err, transp);
}

static
printif(proc, transp, prefix, arg)
	char *proc;
	char *transp;
	char *prefix;
	char *arg;
{
	f_print(fout, "\tif (!svc_%s(%s, xdr_%s, %s%s)) {\n",
		proc, transp, arg, prefix, arg);
}


nullproc(proc)
	proc_list *proc;
{
	for (; proc != NULL; proc = proc->next) {
		if (streq(proc->proc_num, "0")) {
			return (1);
		}
	}
	return (0);
}
