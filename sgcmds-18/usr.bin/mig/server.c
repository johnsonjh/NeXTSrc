/********************************************************
 *  Abstract:
 *	routines to write pieces of the Server module.
 *	exports WriteServer which directs the writing of
 *	the Server module
 *
 *
 *	$Header: server.c,v 1.1 89/06/08 14:20:08 mmeyer Locked $
 *
 * HISTORY
 * Revision 1.17  89/08/14  17:41:30  mrt
 * 	Only define "punt" labels when they're actually used.  The
 * 	solution isn't elegant, but it works.
 * 	[89/08/10            mwyoung]
 * 
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 18-Oct-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Set the local port in the server reply message to
 *	PORT_NULL for greater efficiency and to make Camelot
 *	happy.
 *
 * 18-Apr-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed call to WriteLocalVarDecl in WriteMsgVarDecl
 *	to write out the parameters for the C++ code to a call
 *	a new routine WriteServerVarDecl which includes the *
 *	for reference variable, but uses the transType if it
 *	exists.
 *
 * 27-Feb-88  Richard Draves (rpd) at Carnegie-Mellon University
 *	Changed reply message initialization for camelot interfaces.
 *	Now we assume camelot interfaces are all camelotroutines and
 *	always initialize the dummy field & tid field.  This fixes
 *	the wrapper-server-call bug in distributed transactions.
 *
 * 23-Feb-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed the include of camelot_types.h to cam/camelot_types.h
 *
 * 19-Feb-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Fixed WriteDestroyArg to not call the destructor
 *	function on any in/out args.
 *
 *  4-Feb-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Fixed dld's code to write out parameter list to
 *	use WriteLocalVarDecl to get transType or ServType if
 *	they exist.
 *
 * 19-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	Change variable-length inline array declarations to use
 *	maximum size specified to Mig.  Make message variable
 *	length if the last item in the message is variable-length
 *	and inline.  Use argMultipler field to convert between
 *	argument and IPC element counts.
 *
 * 18-Jan-88  David Detlefs (dld) at Carnegie-Mellon University
 *	Modified to produce C++ compatible code via #ifdefs.
 *	All changes have to do with argument declarations.
 *
 *  2-Dec-87  David Golub (dbg) at Carnegie-Mellon University
 *	Added destructor function for IN arguments to server.
 *
 * 18-Nov-87  Jeffrey Eppinger (jle) at Carnegie-Mellon University
 *	Changed to typedef "novalue" as "void" if we're using hc.
 *
 * 17-Sep-87  Bennet Yee (bsy) at Carnegie-Mellon University
 *	Added _<system>SymTab{Base|End} for use with security
 *	dispatch routine.  It is neccessary for the authorization
 *	system to know the operations by symbolic names.
 *	It is harmless to user code as it only means an extra
 *	array if it is accidentally turned on.
 *
 * 24-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Corrected the setting of retcode for CamelotRoutines.
 *
 * 21-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added deallocflag to call to WritePackArgType.
 *
 * 14-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Moved type declarations and assignments for DummyType 
 *	and tidType to server demux routine. Automatically
 *	include camelot_types.h and msg_types.h for interfaces 
 *	containing camelotRoutines.
 *
 *  8-Jun-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Removed #include of sys/types.h and strings.h from WriteIncludes.
 *	Changed the KERNEL include from ../h to sys/
 *	Removed extern from WriteServer to make hi-c happy
 *
 * 28-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 *******************************************************/

#if	NeXT
#include <sys/message.h>
#else	NeXT
#include <mach/message.h>
#endif	NeXT
#include "write.h"
#include "utils.h"
#include "global.h"

static void
WriteIncludes(file)
    FILE *file;
{
    fprintf(file, "#define EXPORT_BOOLEAN\n");
#if	NeXT
    fprintf(file, "#include <sys/boolean.h>\n");
    fprintf(file, "#include <sys/message.h>\n");
    fprintf(file, "#include <sys/mig_errors.h>\n");
    if (IsCamelot) {
	fprintf(file, "#include <cam/camelot_types.h>\n");
	fprintf(file, "#include <sys/msg_type.h>\n");
    }
#else	NeXT
    fprintf(file, "#include <mach/boolean.h>\n");
    fprintf(file, "#include <mach/message.h>\n");
    fprintf(file, "#include <mach/mig_errors.h>\n");
    if (IsCamelot) {
	fprintf(file, "#include <cam/camelot_types.h>\n");
	fprintf(file, "#include <mach/msg_type.h>\n");
    }
#endif	NeXT
    fprintf(file, "\n");
}

static void
WriteGlobalDecls(file)
    FILE *file;
{
    fprintf(file, "/* Due to pcc compiler bug, cannot use void */\n");
    fprintf(file, "#if\t%s || defined(hc)\n", NewCDecl);
    fprintf(file, "#define novalue void\n");
    fprintf(file, "#else\n");
    fprintf(file, "#define novalue int\n");
    fprintf(file, "#endif\n");
    fprintf(file, "\n");

    if (RCSId != strNULL)
	WriteRCSDecl(file, strconcat(SubsystemName, "_server"), RCSId);

    /* Used for locations in the request message, *not* reply message.
       Reply message locations aren't dependent on IsKernel. */

    if (IsKernel)
    {
	fprintf(file, "#define msg_request_port\tmsg_remote_port\n");
	fprintf(file, "#define msg_reply_port\t\tmsg_local_port\n");
    }
    else
    {
	fprintf(file, "#define msg_request_port\tmsg_local_port\n");
	fprintf(file, "#define msg_reply_port\t\tmsg_remote_port\n");
    }
}

static void
WriteProlog(file)
    FILE *file;
{
    fprintf(file, "/* Module %s */\n", SubsystemName);
    fprintf(file, "\n");
    
    WriteIncludes(file);
    WriteBogusDefines(file);
    WriteGlobalDecls(file);
}


static void
WriteSymTabEntries(file, stats)
    FILE *file;
    statement_t *stats;
{
    register statement_t *stat;
    register u_int current = 0;

    for (stat = stats; stat != stNULL; stat = stat->stNext)
	if (stat->stKind == skRoutine) {
	    register	num = stat->stRoutine->rtNumber;
	    char	*name = stat->stRoutine->rtName;
	    while (++current <= num)
		fprintf(file,"\t\t\t{ \"\", 0, 0 },\n");
	    fprintf(file, "\t{ \"%s\", %d, _X%s },\n",
	    	name,
		SubsystemBase + current - 1,
		name);
	}
    while (++current <= rtNumber)
	fprintf(file,"\t{ \"\", 0, 0 },\n");
}

static void
WriteArrayEntries(file, stats)
    FILE *file;
    statement_t *stats;
{
    register u_int current = 0;
    register statement_t *stat;

    for (stat = stats; stat != stNULL; stat = stat->stNext)
	if (stat->stKind == skRoutine)
	{
	    register routine_t *rt = stat->stRoutine;

	    while (current++ < rt->rtNumber)
		fprintf(file, "\t\t\t0,\n");
	    fprintf(file, "\t\t\t_X%s,\n", rt->rtName);
	}
    while (current++ < rtNumber)
	fprintf(file, "\t\t\t0,\n");
}

static void
WriteEpilog(file, stats)
    FILE *file;
    statement_t *stats;
{
    fprintf(file, "\n");

    fprintf(file, "boolean_t %s\n", ServerProcName);
    fprintf(file, "#if\t%s\n", NewCDecl);
    fprintf(file, "\t(msg_header_t *InHeadP, msg_header_t *OutHeadP)\n");
    fprintf(file, "#else\n");
    fprintf(file, "\t(InHeadP, OutHeadP)\n");
    fprintf(file, "\tmsg_header_t *InHeadP, *OutHeadP;\n");
    fprintf(file, "#endif\n");

    fprintf(file, "{\n");
    fprintf(file, "\tregister msg_header_t *InP =  InHeadP;\n");

    if (IsCamelot)
	fprintf(file, "\tregister camelot_death_pill_t *OutP = (camelot_death_pill_t *) OutHeadP;\n");
    else
	fprintf(file, "\tregister death_pill_t *OutP = (death_pill_t *) OutHeadP;\n");

    fprintf(file, "\n");

    WriteStaticDecl(file, itRetCodeType,
		    itRetCodeType->itDeallocate, itRetCodeType->itLongForm,
		    "RetCodeType");
    fprintf(file, "\n");

    if (IsCamelot)
    {
	WriteStaticDecl(file, itDummyType,
			itDummyType->itDeallocate, itDummyType->itLongForm,
			"DummyType");
	fprintf(file, "\n");
	WriteStaticDecl(file, itTidType,
			itTidType->itDeallocate, itTidType->itLongForm,
			"TidType");
	fprintf(file, "\n");
    }

    fprintf(file, "\tOutP->Head.msg_simple = TRUE;\n");
    fprintf(file, "\tOutP->Head.msg_size = sizeof *OutP;\n");
    fprintf(file, "\tOutP->Head.msg_type = InP->msg_type;\n");
    fprintf(file, "\tOutP->Head.msg_local_port = PORT_NULL;\n"); 
    fprintf(file, "\tOutP->Head.msg_remote_port = InP->msg_reply_port;\n");
    fprintf(file, "\tOutP->Head.msg_id = InP->msg_id + 100;\n");
    fprintf(file, "\n");
    WritePackMsgType(file, itRetCodeType,
		     itRetCodeType->itDeallocate, itRetCodeType->itLongForm,
		     "OutP->RetCodeType", "RetCodeType");
    fprintf(file, "\tOutP->RetCode = MIG_BAD_ID;\n");
    fprintf(file, "\n");

    if (IsCamelot)
    {
	WritePackMsgType(file, itDummyType,
			 itDummyType->itDeallocate, itDummyType->itLongForm,
			 "OutP->DummyType", "DummyType");
	fprintf(file, "\t/* dummy doesn't need a value */\n");
	fprintf(file, "\n");
	WritePackMsgType(file, itTidType,
			 itTidType->itDeallocate, itTidType->itLongForm,
			 "OutP->TidType", "TidType");
	fprintf(file, "\tOutP->Tid = ((camelot_death_pill_t *)InP)->Tid;\n");
	fprintf(file, "\n");
    }

    fprintf(file, "\tif ((InP->msg_id > %d) || (InP->msg_id < %d))\n",
	    SubsystemBase + rtNumber - 1, SubsystemBase);
    fprintf(file, "\t\treturn FALSE;\n");
    fprintf(file, "\telse {\n");
    fprintf(file, "\t\ttypedef novalue (*SERVER_STUB_PROC)\n");
    fprintf(file, "#if\t%s\n", NewCDecl);
    fprintf(file, "\t\t\t(msg_header_t *, msg_header_t *);\n");
    fprintf(file, "#else\n");
    fprintf(file, "\t\t\t();\n");
    fprintf(file, "#endif\n");
#if	__STDC__
    fprintf(file, "\t\tstatic const SERVER_STUB_PROC routines[] = {\n");
#else	__STDC__
    fprintf(file, "\t\tstatic SERVER_STUB_PROC routines[] = {\n");
#endif	__STDC__

    WriteArrayEntries(file, stats);

    fprintf(file, "\t\t};\n");
    fprintf(file, "\n");

    /* Call appropriate routine */
    fprintf(file, "\t\tif (routines[InP->msg_id - %d])\n", SubsystemBase);
    fprintf(file, "\t\t\t(routines[InP->msg_id - %d]) (InP, &OutP->Head);\n",
	    SubsystemBase);
    fprintf(file, "\t\t else\n");
    fprintf(file, "\t\t\treturn FALSE;\n");
    
    fprintf(file, "\t}\n");
    
    fprintf(file, "\treturn TRUE;\n");
    fprintf(file, "}\n");

    /* symtab */

    if (GenSymTab) {
	fprintf(file,"\nmig_symtab_t _%sSymTab[] = {\n",SubsystemName);
	WriteSymTabEntries(file,stats);
	fprintf(file,"};\n");
	fprintf(file,"int _%sSymTabBase = %d;\n",SubsystemName,SubsystemBase);
	fprintf(file,"int _%sSymTabEnd = %d;\n",SubsystemName,SubsystemBase+rtNumber);
    }
}

/*
 *  Returns the return type of the server-side work function.
 *  Suitable for "extern %s serverfunc()".
 */
static char *
ServerSideType(rt)
    routine_t *rt;
{
    if (rt->rtServerReturn == argNULL)
	return "void";
    else
	return rt->rtServerReturn->argType->itTransType;
}

static void
WriteLocalVarDecl(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;

    if (it->itInLine && it->itVarArray)
    {
	register ipc_type_t *btype = it->itElement;

	fprintf(file, "\t%s %s[%d]", btype->itTransType,
		arg->argVarName, it->itNumber/btype->itNumber);
    }
    else
	fprintf(file, "\t%s %s", it->itTransType, arg->argVarName);
}

static void
WriteServerVarDecl(file, arg)
    FILE *file;
    argument_t *arg;
{
    fprintf(file, "%s %s%s",
	    arg->argType->itTransType,
	    argByReferenceServer(arg) ? "*" : "",
	    arg->argVarName);
}

/*
 *  Writes the local variable declarations which are always
 *  present:  InP, OutP, the server-side work function.
 */
static void
WriteVarDecls(file, rt)
    FILE *file;
    routine_t *rt;
{
    int i;

    fprintf(file, "\tregister Request *In0P = (Request *) InHeadP;\n");
    for (i = 1; i <= rt->rtMaxRequestPos; i++)
	fprintf(file, "\tregister Request *In%dP;\n", i);
    fprintf(file, "\tregister Reply *OutP = (Reply *) OutHeadP;\n");
    fprintf(file, "\textern %s %s\n",
	    ServerSideType(rt), rt->rtServerName);

    fprintf(file, "#if\t%s\n", NewCDecl);
    fprintf(file, "\t\t(");
    WriteList(file, rt->rtArgs, WriteServerVarDecl, akbServerArg, ", ", "");
    fprintf(file, ");\n");
    fprintf(file, "#else\n");
    fprintf(file, "\t\t();\n");
    fprintf(file, "#endif\n");
    fprintf(file, "\n");

    fprintf(file, "#if\tTypeCheck\n");
    fprintf(file, "\tboolean_t msg_simple;\n");
    fprintf(file, "#endif\tTypeCheck\n");
    fprintf(file, "\n");

    fprintf(file, "\tunsigned int msg_size;\n");

    /* if either request or reply is variable, we need msg_size_delta */
    if ((rt->rtNumRequestVar > 0) ||
	(rt->rtNumReplyVar > 0))
	fprintf(file, "\tunsigned int msg_size_delta;\n");

    fprintf(file, "\n");
}

static void
WriteMsgError(file, arg, error)
    FILE *file;
    register argument_t *arg;
    char *error;
{
    if (arg == argNULL)
	fprintf(file, "\t\t{ OutP->RetCode = %s; return; }\n", error);
    else {
	fprintf(file, "\t\t{ OutP->RetCode = %s; goto punt%d; }\n",
		error, arg->argPuntNum);
	fprintf(file, "#define\tlabel_punt%d\n", arg->argPuntNum);
    }
}

static void
WriteReplyInit(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "\n");
    fprintf(file, "\tmsg_size = %d;\n", rt->rtReplySize);
}

static void
WriteReplyHead(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "\n");
    if (rt->rtMaxReplyPos > 0)
	fprintf(file, "\tOutP = (Reply *) OutHeadP;\n");

    fprintf(file, "\tOutP->Head.msg_simple = %s;\n",
	    strbool(rt->rtSimpleSendReply));
    fprintf(file, "\tOutP->Head.msg_size = msg_size;\n");
}

static void
WriteCheckHead(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "#if\tTypeCheck\n");
    fprintf(file, "\tmsg_size = In0P->Head.msg_size;\n");
    fprintf(file, "\tmsg_simple = In0P->Head.msg_simple;\n");

    if (rt->rtNumRequestVar > 0)
	fprintf(file, "\tif ((msg_size < %d)",
		rt->rtRequestSize);
    else
	fprintf(file, "\tif ((msg_size != %d)",
		rt->rtRequestSize);
    if (rt->rtSimpleCheckRequest)
	fprintf(file, " || (msg_simple != %s)",
		strbool(rt->rtSimpleReceiveRequest));
    fprintf(file, ")\n");
    WriteMsgError(file, argNULL, "MIG_BAD_ARGUMENTS");
    fprintf(file, "#endif\tTypeCheck\n");
    fprintf(file, "\n");
}

static void
WriteTypeCheck(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;
    register routine_t *rt = arg->argRoutine;

    fprintf(file, "#if\tTypeCheck\n");
    if (akCheck(arg->argKind, akbQuickCheck))
    {
	fprintf(file, "#if\tUseStaticMsgType\n");
	fprintf(file, "\tif (* (int *) &In%dP->%s != * (int *) &%sCheck)\n",
		arg->argRequestPos, arg->argTTName, arg->argVarName);
	fprintf(file, "#else\tUseStaticMsgType\n");
    }
    fprintf(file, "\tif ((In%dP->%s%s.msg_type_inline != %s) ||\n",
	    arg->argRequestPos, arg->argTTName,
	    arg->argLongForm ? ".msg_type_header" : "",
	    strbool(it->itInLine));
    fprintf(file, "\t    (In%dP->%s%s.msg_type_longform != %s) ||\n",
	    arg->argRequestPos, arg->argTTName,
	    arg->argLongForm ? ".msg_type_header" : "",
	    strbool(arg->argLongForm));
    if (it->itOutName == MSG_TYPE_POLYMORPHIC)
    {
	if (!rt->rtSimpleCheckRequest)
	    fprintf(file, "\t    (MSG_TYPE_PORT_ANY(In%dP->%s.msg_type_%sname) && msg_simple) ||\n",
		    arg->argRequestPos, arg->argTTName,
		    arg->argLongForm ? "long_" : "");
    }
    else
	fprintf(file, "\t    (In%dP->%s.msg_type_%sname != %s) ||\n",
		arg->argRequestPos, arg->argTTName,
		arg->argLongForm ? "long_" : "",
		it->itOutNameStr);
    if (!it->itVarArray)
	fprintf(file, "\t    (In%dP->%s.msg_type_%snumber != %d) ||\n",
		arg->argRequestPos, arg->argTTName,
		arg->argLongForm ? "long_" : "",
		it->itNumber);
    fprintf(file, "\t    (In%dP->%s.msg_type_%ssize != %d))\n",
	    arg->argRequestPos, arg->argTTName,
	    arg->argLongForm ? "long_" : "",
	    it->itSize);
    if (akCheck(arg->argKind, akbQuickCheck))
	fprintf(file, "#endif\tUseStaticMsgType\n");
    WriteMsgError(file, arg, "MIG_BAD_ARGUMENTS");
    fprintf(file, "#endif\tTypeCheck\n");
    fprintf(file, "\n");
}

static void
WriteCheckMsgSize(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register routine_t *rt = arg->argRoutine;
    register ipc_type_t *btype = arg->argType->itElement;
    argument_t *count = arg->argCount;
    boolean_t NoMoreArgs, LastVarArg;

    /* If there aren't any more In args after this, then
       msg_size_delta value will only get used by TypeCheck code,
       so put the assignment under the TypeCheck conditional. */

    NoMoreArgs = arg->argRequestPos == rt->rtMaxRequestPos;

    /* If there aren't any more variable-sized arguments after this,
       then we must check for exact msg-size and we don't need
       to update msg_size. */

    LastVarArg = arg->argRequestPos+1 == rt->rtNumRequestVar;

    if (NoMoreArgs)
	fprintf(file, "#if\tTypeCheck\n");

    /* calculate the actual size in bytes of the data field.  note
       that this quantity must be a multiple of four.  hence, if
       the base type size isn't a multiple of four, we have to
       round up.  note also that btype->itNumber must
       divide btype->itTypeSize (see itCalculateSizeInfo). */

    fprintf(file, "\tmsg_size_delta = (%d * In%dP->%s)",
	    btype->itTypeSize/btype->itNumber,
	    arg->argRequestPos,
	    count->argMsgField);
    if (btype->itTypeSize % 4 != 0)
	fprintf(file, "+3 &~ 3");
    fprintf(file, ";\n");

    if (!NoMoreArgs)
	fprintf(file, "#if\tTypeCheck\n");

    /* Don't decrement msg_size until we've checked it won't underflow. */

    if (LastVarArg)
	fprintf(file, "\tif (msg_size != %d + msg_size_delta)\n",
		rt->rtRequestSize);
    else
	fprintf(file, "\tif (msg_size < %d + msg_size_delta)\n",
		rt->rtRequestSize);
    WriteMsgError(file, arg, "MIG_BAD_ARGUMENTS");

    if (!LastVarArg)
	fprintf(file, "\tmsg_size -= msg_size_delta;\n");

    fprintf(file, "#endif\tTypeCheck\n");
    fprintf(file, "\n");
}

static void
WriteExtractArgValue(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;

    if (arg->argMultiplier > 1)
	WriteCopyType(file, it, "%s /* %d %s %d */", "/* %s */ In%dP->%s / %d",
		      arg->argVarName, arg->argRequestPos,
		      arg->argMsgField, arg->argMultiplier);
    else if (it->itInTrans != strNULL)
	WriteCopyType(file, it, "%s /* %s %d %s */", "/* %s */ %s(In%dP->%s)",
		      arg->argVarName, it->itInTrans,
		      arg->argRequestPos, arg->argMsgField);
    else
	WriteCopyType(file, it, "%s /* %d %s */", "/* %s */ In%dP->%s",
		      arg->argVarName, arg->argRequestPos, arg->argMsgField);
    fprintf(file, "\n");
}

static void
WriteInitializeCount(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *ptype = arg->argParent->argType;
    register ipc_type_t *btype = ptype->itElement;

    /*
     *	Initialize 'count' argument for variable-length inline OUT parameter
     *	with maximum allowed number of elements.
     */

    fprintf(file, "\t%s = %d;\n", arg->argVarName,
	    ptype->itNumber/btype->itNumber);
    fprintf(file, "\n");
}

static void
WriteExtractArg(file, arg)
    FILE *file;
    register argument_t *arg;
{
    if (akCheck(arg->argKind, akbRequest))
	WriteTypeCheck(file, arg);

    if (akCheckAll(arg->argKind, akbVariable|akbRequest))
	WriteCheckMsgSize(file, arg);

    if (akCheckAll(arg->argKind, akbSendRcv|akbVarNeeded))
	WriteExtractArgValue(file, arg);

    if ((akIdent(arg->argKind) == akeCount) &&
	akCheck(arg->argKind, akbReturnSnd))
    {
	register ipc_type_t *ptype = arg->argParent->argType;

	if (ptype->itInLine && ptype->itVarArray)
	    WriteInitializeCount(file, arg);
    }

    /* This assumes that the count argument directly follows the 
       associated variable-sized argument and any other implicit
       arguments it may have. */

    if ((akIdent(arg->argKind) == akeCount) &&
	akCheck(arg->argKind, akbSendRcv) &&
	(arg->argRequestPos < arg->argRoutine->rtMaxRequestPos))
    {
	register ipc_type_t *ptype = arg->argParent->argType;

	if (ptype->itInLine && ptype->itVarArray)
	{
	    fprintf(file, "\tIn%dP = (Request *) ((char *) In%dP + msg_size_delta - %d);\n",
		    arg->argRequestPos+1, arg->argRequestPos,
		    ptype->itTypeSize + ptype->itPadSize);
	    fprintf(file, "\n");
	}
    }
}

static void
WriteServerCallArg(file, arg)
    FILE *file;
    register argument_t *arg;
{
    ipc_type_t *it = arg->argType;
    boolean_t NeedClose = FALSE;

    if (argByReferenceServer(arg))
	fprintf(file, "&");

    if ((it->itInTrans != strNULL) &&
	akCheck(arg->argKind, akbSendRcv) &&
	!akCheck(arg->argKind, akbVarNeeded))
    {
	fprintf(file, "%s(", it->itInTrans);
	NeedClose = TRUE;
    }

    if (akCheck(arg->argKind, akbVarNeeded))
	fprintf(file, "%s", arg->argVarName);
    else if (akCheck(arg->argKind, akbSendRcv))
	fprintf(file, "In%dP->%s", arg->argRequestPos, arg->argMsgField);
    else
	fprintf(file, "OutP->%s", arg->argMsgField);

    if (NeedClose)
	fprintf(file, ")");

    if (!argByReferenceServer(arg) && (arg->argMultiplier > 1))
	fprintf(file, " / %d", arg->argMultiplier);
}

static void
WriteDestroyArg(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;

    fprintf(file, "#ifdef\tlabel_punt%d\n", arg->argPuntNum+1);
    fprintf(file, "#undef\tlabel_punt%d\n", arg->argPuntNum+1);
    fprintf(file, "punt%d:\n", arg->argPuntNum+1);
    fprintf(file, "#endif\tlabel_punt%d\n", arg->argPuntNum+1);

    if (akCheck(arg->argKind, akbVarNeeded))
	fprintf(file, "\t%s(%s);\n", it->itDestructor, arg->argVarName);
    else
	fprintf(file, "\t%s(In%dP->%s);\n", it->itDestructor,
		arg->argRequestPos, arg->argMsgField);
}

static void
WriteServerCall(file, rt)
    FILE *file;
    routine_t *rt;
{
    boolean_t NeedClose = FALSE;

    fprintf(file, "\t");
    if (rt->rtServerReturn != argNULL)
    {
	argument_t *arg = rt->rtServerReturn;
	ipc_type_t *it = arg->argType;

	if (rt->rtOneWay)
	    fprintf(file, "(void) ");
	else
	    fprintf(file, "OutP->%s = ", arg->argMsgField);
	if (it->itOutTrans != strNULL)
	{
	    fprintf(file, "%s(", it->itOutTrans);
	    NeedClose = TRUE;
	}
    }
    fprintf(file, "%s(", rt->rtServerName);
    WriteList(file, rt->rtArgs, WriteServerCallArg, akbServerArg, ", ", "");
    if (NeedClose)
	fprintf(file, ")");
    fprintf(file, ");\n");
}

static void
WriteGetReturnValue(file, rt)
    FILE *file;
    register routine_t *rt;
{
    if (rt->rtOneWay)
	fprintf(file, "\tOutP->%s = MIG_NO_REPLY;\n",
		rt->rtRetCode->argMsgField);
    else if (rt->rtServerReturn != rt->rtRetCode)
	fprintf(file, "\tOutP->%s = KERN_SUCCESS;\n",
		rt->rtRetCode->argMsgField);
}

static void
WriteCheckReturnValue(file, rt)
    FILE *file;
    register routine_t *rt;
{
    fprintf(file, "\tif (OutP->%s != KERN_SUCCESS)\n",
	    rt->rtRetCode->argMsgField);
    fprintf(file, "\t\treturn;\n");
}

static void
WritePackArgType(file, arg)
    FILE *file;
    register argument_t *arg;
{
    fprintf(file, "\n");

    WritePackMsgType(file, arg->argType, arg->argDeallocate, arg->argLongForm,
		     "OutP->%s", "%s", arg->argTTName);
}

static void
WritePackArgValue(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;

    fprintf(file, "\n");

    if (it->itInLine && it->itVarArray)
    {
	register argument_t *count = arg->argCount;
	register ipc_type_t *btype = it->itElement;

	/* Note btype->itNumber == count->argMultiplier */

	fprintf(file, "\tbcopy((char *) %s, (char *) OutP->%s, ",
		arg->argVarName, arg->argMsgField);
	fprintf(file, "%d * %s);\n",
		btype->itTypeSize, count->argVarName);
    }
    else if (arg->argMultiplier > 1)
	WriteCopyType(file, it, "OutP->%s /* %d %s */", "/* %s */ %d * %s",
		      arg->argMsgField,
		      arg->argMultiplier,
		      arg->argVarName);
    else if (it->itOutTrans != strNULL)
	WriteCopyType(file, it, "OutP->%s /* %s %s */", "/* %s */ %s(%s)",
		      arg->argMsgField, it->itOutTrans, arg->argVarName);
    else
	WriteCopyType(file, it, "OutP->%s /* %s */", "/* %s */ %s",
		      arg->argMsgField, arg->argVarName);
}

static void
WriteCopyArgValue(file, arg)
    FILE *file;
    argument_t *arg;
{
    fprintf(file, "\n");
    WriteCopyType(file, arg->argType, "/* %d */ OutP->%s", "In%dP->%s",
		  arg->argRequestPos, arg->argMsgField);
}

static void
WriteAdjustMsgSize(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *ptype = arg->argParent->argType;
    register ipc_type_t *btype = ptype->itElement;

    fprintf(file, "\n");

    /* calculate the actual size in bytes of the data field.
       note that this quantity must be a multiple of four.
       hence, if the base type size isn't a multiple of four,
       we have to round up. */

    fprintf(file, "\tmsg_size_delta = (%d * %s)",
	    btype->itTypeSize, arg->argVarName);
    if (btype->itTypeSize % 4 != 0)
	fprintf(file, "+3 &~ 3");
    fprintf(file, ";\n");

    fprintf(file, "\tmsg_size += msg_size_delta;\n");

    /* Don't bother moving OutP unless there are more Out arguments. */
    if (arg->argReplyPos < arg->argRoutine->rtMaxReplyPos)
    {
	fprintf(file, "\tOutP = (Reply *) ((char *) OutP + ");
	fprintf(file, "msg_size_delta - %d);\n",
		ptype->itTypeSize + ptype->itPadSize);
    }
}

static void
WritePackArg(file, arg)
    FILE *file;
    argument_t *arg;
{
    if (akCheck(arg->argKind, akbReplyInit))
	WritePackArgType(file, arg);

    if (akCheckAll(arg->argKind, akbReturnSnd|akbVarNeeded))
	WritePackArgValue(file, arg);

    if (akCheck(arg->argKind, akbReplyCopy))
	WriteCopyArgValue(file, arg);

    if ((akIdent(arg->argKind) == akeCount) &&
	akCheck(arg->argKind, akbReturnSnd))
    {
	register ipc_type_t *ptype = arg->argParent->argType;

	if (ptype->itInLine && ptype->itVarArray)
	    WriteAdjustMsgSize(file, arg);
    }
}

static void
WriteFieldDecl(file, arg)
    FILE *file;
    argument_t *arg;
{
    WriteFieldDeclPrim(file, arg, FetchServerType);
}

static void
WriteRoutine(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "\n");

    fprintf(file, "/* %s %s */\n", rtRoutineKindToStr(rt->rtKind), rt->rtName);
    fprintf(file, "mig_internal novalue _X%s\n", rt->rtName);
    fprintf(file, "#if\t%s\n", NewCDecl);
    fprintf(file, "\t(msg_header_t *InHeadP, msg_header_t *OutHeadP)\n");
    fprintf(file, "#else\n");
    fprintf(file, "\t(InHeadP, OutHeadP)\n");
    fprintf(file, "\tmsg_header_t *InHeadP, *OutHeadP;\n");
    fprintf(file, "#endif\n");

    fprintf(file, "{\n");
    WriteStructDecl(file, rt->rtArgs, WriteFieldDecl, akbRequest, "Request");
    WriteStructDecl(file, rt->rtArgs, WriteFieldDecl, akbReply, "Reply");

    WriteVarDecls(file, rt);

    WriteList(file, rt->rtArgs, WriteCheckDecl,
	      akbQuickCheck|akbRequest, "\n", "\n");
    WriteList(file, rt->rtArgs, WriteTypeDecl, akbReplyInit, "\n", "\n");

    WriteList(file, rt->rtArgs, WriteLocalVarDecl,
	      akbVarNeeded, ";\n", ";\n\n");

    WriteCheckHead(file, rt);

    WriteList(file, rt->rtArgs, WriteExtractArg, akbNone, "", "");

    WriteServerCall(file, rt);
    WriteGetReturnValue(file, rt);

    /* In reverse order so we can jump into the middle. */

    WriteReverseList(file, rt->rtArgs, WriteDestroyArg, akbDestroy, "", "");
    fprintf(file, "#ifdef\tlabel_punt0\n");
    fprintf(file, "#undef\tlabel_punt0\n");
    fprintf(file, "punt0:\n");
    fprintf(file, "#endif\tlabel_punt0\n");

    if (rt->rtOneWay)
	fprintf(file, "\t;\n");
    else
    {
	WriteCheckReturnValue(file, rt);
	WriteReplyInit(file, rt);
	WriteList(file, rt->rtArgs, WritePackArg, akbNone, "", "");
	WriteReplyHead(file, rt);
    }

    fprintf(file, "}\n");
}

void
WriteServer(file, stats)
    FILE *file;
    statement_t *stats;
{
    register statement_t *stat;

    WriteProlog(file);
    for (stat = stats; stat != stNULL; stat = stat->stNext)
	switch (stat->stKind)
	{
	  case skRoutine:
	    WriteRoutine(file, stat->stRoutine);
	    break;
	  case skImport:
	  case skSImport:
	    WriteImport(file, stat->stFileName);
	    break;
	  case skUImport:
	    break;
	  default:
	    fatal("WriteServer(): bad statement_kind_t (%d)",
		  (int) stat->stKind);
	}
    WriteEpilog(file, stats);
}

