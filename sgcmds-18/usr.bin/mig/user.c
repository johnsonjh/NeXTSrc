/********************************************************
 *  Abstract:
 *	routines to write pieces of the User module.
 *	exports WriteUser which directs the writing of
 *	the User module
 *
 *  $Header: user.c,v 1.1 89/06/08 14:20:33 mmeyer Locked $
 *
 * HISTORY
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 03-Jan-89  Trey Matteson (trey@next.com)
 *	Removed init_<sysname> function
 *
 * 21-Feb-89  David Golub (dbg) at Carnegie-Mellon University
 *	Get name for header file from HeaderFileName, since it can
 *	change.
 *
 *  8-Feb-89  David Golub (dbg) at Carnegie-Mellon University
 *	Added WriteUserIndividual to put each user-side routine in its
 *	own file.
 *
 *  8-Jul-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Declared routines to be mig_external instead of extern,
 *	where mig_external is conditionally defined in <subsystem>.h.
 *	The Avalon folks want to define mig_external to be static
 *	in their compilations because they inlcude the User.c code in
 *	their programs.
 *
 * 23-Feb-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed the include of camelot_types.h to cam/camelot_types.h
 *
 * 19-Feb-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added comments for each routine. Called WriteMsgError
 *	for MIG_ARRAY_TOO_LARGE errors.
 *
 * 19-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	Change variable-length inline array declarations to use
 *	maximum size specified to Mig.  Make message variable
 *	length if the last item in the message is variable-length
 *	and inline.  Use argMultiplier field to convert between
 *	argument and IPC element counts.
 *
 * 19-Jan-88  Mary Thompson (mrt) at Carnegie-Mellon University
 *	In WriteInitRoutine changed reference from reply_port; to reply_port++;
 *	for lint code.
 *
 * 17-Jan-88  David Detlefs (dld) at Carnegie-Mellon University
 *	Modified to produce C++ compatible code via #ifdefs.
 *	All changes have to do with argument declarations.
 *
 * 16-Nov-87  David Golub (dbg) at Carnegie-Mellon University
 *	Handle variable-length inline arrays.
 *
 * 22-Oct-87  Mary Thompson (mrt) at Carnegie-Mellon University
 * 	Added a reference to rep_port in the InitRoutine
 *	with an ifdef lint conditional.
 *
 * 22-Sep-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Fixed check for TransId to be a not equal test
 *	rather than an equal test.
 *
 *  2-Sep-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed WriteCheckIdentity to check TransId instead
 *	of msg_id for a returned camelot reply
 *
 * 24-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added a  LINTLIBRARY  line to keep lint
 *	from complaining about routines that are not used.
 *
 * 21-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added Flag parameter to WritePackMsgType.
 *
 * 12-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Made various camelot changes: include of camelot_types.h
 *	Check for death_pill before correct msg-id.
 *
 * 10-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Renamed get_reply_port and dealloc_reply_port to
 *	mig_get_reply_port and mig_dealloc_reply_port.
 *	Fixed WriteRequestHead to handle MsgType parameter.
 *
 *  3-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Fixed to generate code that is the same for multi-threaded and
 *	single threaded use. Gets reply port from library routine
 *	get_reply_port and deallocates with routine
 *	dealloc_reply_port. Removed all routines in mig interface code
 *	to keep track of the reply port. The init routine still exists
 *	but does nothing.
 * 
 * 29-Jul_87  Mary Thompson (mrt) at Carnegie-Mellon University
 * 	Fixed call to WriteVarDecl to use correspond to
 *	the changes that were made in that routine.
 *
 * 16-Jul-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added write of MsgType to WriteSetMsgTypeRoutine.
 *
 *  8-Jun-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Removed #include of sys/types.h from WriteIncludes.
 *	Changed the KERNEL include from ../h to sys/
 *	Removed extern from WriteUser to make hi-c happy
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
#include "error.h"
#include "utils.h"
#include "global.h"

/*************************************************************
 *	Writes the standard includes. The subsystem specific
 *	includes  are in <SubsystemName>.h and writen by
 *	header:WriteHeader. Called by WriteProlog.
 *************************************************************/
static void
WriteIncludes(file)
    FILE *file;
{
    register char *cp;
    /*
     * Strip any leading path from HeaderFileName.
     */
    cp = rindex(HeaderFileName, '/');
    if (cp == 0)
	cp = HeaderFileName;
    else
	cp++;	/* skip '/' */
    fprintf(file, "#include \"%s\"\n", cp);
#if	NeXT
    fprintf(file, "#include <kern/mach_types.h>\n");
    fprintf(file, "#include <sys/message.h>\n");
    fprintf(file, "#include <sys/mig_errors.h>\n");
    fprintf(file, "#include <sys/msg_type.h>\n");
#else	NeXT
    fprintf(file, "#include <mach/mach_types.h>\n");
    fprintf(file, "#include <mach/message.h>\n");
    fprintf(file, "#include <mach/mig_errors.h>\n");
    fprintf(file, "#include <mach/msg_type.h>\n");
#endif	NeXT
    if (IsCamelot)
	fprintf(file, "#include <cam/camelot_types.h>\n");
    fprintf(file, "#if\t!defined(KERNEL) && !defined(MIG_NO_STRINGS)\n");
    fprintf(file, "#include <strings.h>\n");
    fprintf(file, "#endif\n");
    fprintf(file, "/* LINTLIBRARY */\n");
    fprintf(file, "\n");
    fprintf(file, "extern port_t mig_get_reply_port();\n");
    fprintf(file, "extern void mig_dealloc_reply_port();\n");
    fprintf(file, "\n");

}

static void
WriteGlobalDecls(file)
    FILE *file;
{
    if (RCSId != strNULL)
	WriteRCSDecl(file, strconcat(SubsystemName, "_user"), RCSId);

    fprintf(file, "#define msg_request_port\tmsg_remote_port\n");
    fprintf(file, "#define msg_reply_port\t\tmsg_local_port\n");
    fprintf(file, "\n");
}

/*************************************************************
 *	Writes the standard #includes, #defines, and
 *	RCS declaration. Called by WriteUser.
 *************************************************************/
static void
WriteProlog(file)
    FILE *file;
{
    WriteIncludes(file);
    WriteBogusDefines(file);
    WriteGlobalDecls(file);
}

/*ARGSUSED*/
static void
WriteEpilog(file)
    FILE *file;
{
}

static void
WriteRequestHead(file, rt)
    FILE *file;
    routine_t *rt;
{
    if (rt->rtMaxRequestPos > 0)
	fprintf(file, "\tInP = &Mess.In;\n");

    fprintf(file, "\tInP->Head.msg_simple = %s;\n",
	    strbool(rt->rtSimpleSendRequest));

    fprintf(file, "\tInP->Head.msg_size = msg_size;\n");

    fprintf(file, "\tInP->%s = %s;\n",
	    rt->rtMsgType->argMsgField,
	    rt->rtMsgType->argVarName);

    fprintf(file, "\tInP->%s = %s;\n",
	    rt->rtRequestPort->argMsgField,
	    rt->rtRequestPort->argVarName);

    fprintf(file, "\tInP->%s = %s;\n",
	    rt->rtReplyPort->argMsgField,
	    rt->rtReplyPort->argVarName);

    fprintf(file, "\tInP->Head.msg_id = %d;\n",
	    rt->rtNumber + SubsystemBase);
}

/*************************************************************
 *  Writes declarations for the message types, variables
 *  and return  variable if needed. Called by WriteRoutine.
 *************************************************************/
static void
WriteVarDecls(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "\tunion {\n");
    fprintf(file, "\t\tRequest In;\n");
    if (!rt->rtOneWay)
	fprintf(file, "\t\tReply Out;\n");
    fprintf(file, "\t} Mess;\n");
    fprintf(file, "\n");

    fprintf(file, "\tregister Request *InP = &Mess.In;\n");
    if (!rt->rtOneWay)
	fprintf(file, "\tregister Reply *OutP = &Mess.Out;\n");
    fprintf(file, "\n");

    /* if not SimpleRoutine, we will need a temp var for error code */
    if (rt->rtKind != rkSimpleRoutine)
    {
	fprintf(file, "\tmsg_return_t msg_result;\n");
	fprintf(file, "\n");
    }

    if (!rt->rtOneWay)
    {
	fprintf(file, "#if\tTypeCheck\n");
	fprintf(file, "\tboolean_t msg_simple;\n");
	fprintf(file, "#endif\tTypeCheck\n");
	fprintf(file, "\n");
    }

    fprintf(file, "\tunsigned int msg_size = %d;\n",
	    rt->rtRequestSize);

    /* if either request or reply is variable, we need msg_size_delta */
    if ((rt->rtNumRequestVar > 0) ||
	(rt->rtNumReplyVar > 0))
	fprintf(file, "\tunsigned int msg_size_delta;\n");

    fprintf(file, "\n");
}

/*************************************************************
 *  Writes code to call the user provided error procedure
 *  when a MIG error occurs. Called by WriteMsgSend, 
 *  WriteMsgCheckReceive, WriteMsgSendReceive, WriteCheckIdentity,
 *  WriteRetCodeCheck, WriteTypeCheck, WritePackArgValue.
 *************************************************************/
static void
WriteMsgError(file, rt, error)
    FILE *file;
    routine_t *rt;
    char *error;
{
    switch (rt->rtKind)
    {
      case rkRoutine:
      case rkCamelotRoutine:
      case rkSimpleRoutine:
	fprintf(file, "\t\treturn %s;\n", error);
	break;
      case rkProcedure:
      case rkSimpleProcedure:
	fprintf(file, "\t\t{ %s(%s); return; }\n", rt->rtErrorName, error);
	break;
      case rkFunction:
	fprintf(file, "\t\t{ %s(%s); ", rt->rtErrorName, error);
	if (rt->rtNumReplyVar > 0)
	    fprintf(file, "OutP = &Mess.Out; ");
	fprintf(file, "return OutP->%s; }\n", rt->rtReturn->argMsgField);
	break;
      default:
	fatal("WriteMsgError(%s): bad routine_kind_t (%d)",
	      rt->rtName, (int) rt->rtKind);
    }
}

/*************************************************************
 *   Writes the msg_send call when there is to be no subsequent
 *   msg_receive. Called by WriteRoutine for SimpleProcedures
 *   or SimpleRoutines
 *************************************************************/
static void
WriteMsgSend(file, rt)
    FILE *file;
    routine_t *rt;
{
    if (rt->rtProcedure)  /* no value is returned to user */
    {
	fprintf(file, "\tmsg_result = msg_send(&InP->Head, %s, 0);\n",
		"MSG_OPTION_NONE");
	fprintf(file, "\tif (msg_result != SEND_SUCCESS)\n");
	WriteMsgError(file, rt, "msg_result");
    }
    else
	fprintf(file, "\treturn msg_send(&InP->Head, %s, 0);\n",
		"MSG_OPTION_NONE");
}

/*************************************************************
 *  Writes to code to check for error returns from msg_receive.
 *  Called by WriteMsgSendReceive and WriteMsgRPC
 *************************************************************/
static void
WriteMsgCheckReceive(file, rt, success)
    FILE *file;
    routine_t *rt;
    char *success;
{
    fprintf(file, "\tif (msg_result != %s) {\n", success);
    if (!akCheck(rt->rtReplyPort->argKind, akbUserArg))
    {
	/* If we aren't using a user-supplied reply port, then
	   deallocate the reply port for INVALID_PORT and TIMED_OUT errors. */

	if (rt->rtWaitTime == argNULL)
	    fprintf(file, "\t\tif (msg_result == RCV_INVALID_PORT)\n");
	else
	{
	    fprintf(file, "\t\tif ((msg_result == RCV_INVALID_PORT) ||\n");
	    fprintf(file, "\t\t    (msg_result == RCV_TIMED_OUT))\n");
	}
	fprintf(file, "\t\t\tmig_dealloc_reply_port();\n");
    }
    WriteMsgError(file, rt, "msg_result");
    fprintf(file, "\t}\n");
}

/*************************************************************
 *  Writes the msg_send and msg_receive calls and code to check
 *  for errors. Normally the msg_rpc code is generated instead
 *  although, the subsytem can be compiled with the -R option
 *  which will cause this code to be generated. Called by
 *  WriteRoutine if UseMsgRPC option is false.
 *************************************************************/
static void
WriteMsgSendReceive(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "\tmsg_result = msg_send(&InP->Head, %s, 0);\n",
	    "MSG_OPTION_NONE");
    fprintf(file, "\tif (msg_result != SEND_SUCCESS)\n");
    WriteMsgError(file, rt, "msg_result");
    fprintf(file, "\n");

    /* fill in reply message header */
    fprintf(file, "\tOutP->Head.msg_size = sizeof(Reply);\n");
    fprintf(file, "\t/* OutP->Head.msg_local_port already set */\n");
    fprintf(file, "\n");

    fprintf(file, "\tmsg_result = msg_receive(&OutP->Head, %s, %s);\n",
	    rt->rtWaitTime != argNULL ? "RCV_TIMEOUT" : "MSG_OPTION_NONE",
	    rt->rtWaitTime != argNULL ? rt->rtWaitTime->argVarName : "0");
    WriteMsgCheckReceive(file, rt, "RCV_SUCCESS");
    fprintf(file, "\n");
}

/*************************************************************
 *  Writes the msg_rpc call and the code to check for errors.
 *  This is the default code to be generated. Called by WriteRoutine
 *  for all routine types except SimpleProcedure and SimpleRoutine.
 *************************************************************/
static void
WriteMsgRPC(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "\tmsg_result = msg_rpc(&InP->Head, %s, %s, 0, %s);\n",
	    rt->rtWaitTime != argNULL ? "RCV_TIMEOUT" : "MSG_OPTION_NONE",
	    "sizeof(Reply)",
	    rt->rtWaitTime != argNULL? rt->rtWaitTime->argVarName : "0");
    WriteMsgCheckReceive(file, rt, "RPC_SUCCESS");
    fprintf(file, "\n");
}

/*************************************************************
 *   Sets the correct value of the dealloc flag and calls
 *   Utils:WritePackMsgType to fill in the ipc msg type word(s)
 *   in the request message. Called by WriteRoutine for each
 *   argument that is to be sent in the request message.
 *************************************************************/
static void
WritePackArgType(file, arg)
    FILE *file;
    argument_t *arg;
{
    WritePackMsgType(file, arg->argType, arg->argDeallocate, arg->argLongForm,
		     "InP->%s", "%s", arg->argTTName);
    fprintf(file, "\n");
}

/*************************************************************
 *  Writes code to copy an argument into the request message.  
 *  Called by WriteRoutine for each argument that is to placed
 *  in the request message.
 *************************************************************/
static void
WritePackArgValue(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *it = arg->argType;
    register char *ref = argByReferenceUser(arg) ? "*" : "";

    if (it->itInLine && it->itVarArray)
    {
	/*
	 *	Copy in variable-size inline array with bcopy,
	 *	after checking that number of elements doesn't
	 *	exceed declared maximum.
	 */
	register argument_t *count = arg->argCount;
	register char *countRef = argByReferenceUser(count) ? "*" : "";
	register ipc_type_t *btype = it->itElement;

	/* Note btype->itNumber == count->argMultiplier */

	fprintf(file, "\tif (%s%s > %d)\n",
		countRef, count->argVarName,
		it->itNumber/btype->itNumber);
	WriteMsgError(file, arg->argRoutine, "MIG_ARRAY_TOO_LARGE");

	fprintf(file, "\tbcopy((char *) %s%s, (char *) InP->%s, ",
		ref, arg->argVarName, arg->argMsgField);
	fprintf(file, "%d * %s%s);\n",
		btype->itTypeSize, countRef, count->argVarName);
    }
    else if (arg->argMultiplier > 1)
	WriteCopyType(file, it, "InP->%s /* %d %s%s */", "/* %s */ %d * %s%s",
		      arg->argMsgField, arg->argMultiplier,
		      ref, arg->argVarName);
    else
	WriteCopyType(file, it, "InP->%s /* %s%s */", "/* %s */ %s%s",
		      arg->argMsgField, ref, arg->argVarName);
    fprintf(file, "\n");
}

static void
WriteAdjustMsgSize(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t *ptype = arg->argParent->argType;
    char *ref = argByReferenceUser(arg) ? "*" : "";
    register ipc_type_t *btype = ptype->itElement;

    /* calculate the actual size in bytes of the data field.
       note that this quantity must be a multiple of four.
       hence, if the base type size isn't a multiple of four,
       we have to round up. */

    fprintf(file, "\tmsg_size_delta = (%d * %s%s)",
	    btype->itTypeSize, ref, arg->argVarName);
    if (btype->itTypeSize % 4 != 0)
	fprintf(file, "+3 &~ 3");
    fprintf(file, ";\n");

    fprintf(file, "\tmsg_size += msg_size_delta;\n");

    /* Don't bother moving InP unless there are more In arguments. */
    if (arg->argRequestPos < arg->argRoutine->rtMaxRequestPos)
    {
	fprintf(file, "\tInP = (Request *) ((char *) InP + ");
	fprintf(file, "msg_size_delta - %d);\n",
		ptype->itTypeSize + ptype->itPadSize);
    }
    fprintf(file, "\n");
}

/*
 * Called for every argument.  Responsible for packing that
 * argument into the request message.
 */
static void
WritePackArg(file, arg)
    FILE *file;
    register argument_t *arg;
{
    if (akCheck(arg->argKind, akbRequest))
	WritePackArgType(file, arg);

    if (akCheckAll(arg->argKind, akbSendSnd|akbSendBody))
	WritePackArgValue(file, arg);

    if ((akIdent(arg->argKind) == akeCount) &&
	akCheck(arg->argKind, akbSendSnd))
    {
	register ipc_type_t *ptype = arg->argParent->argType;

	if (ptype->itInLine && ptype->itVarArray)
	    WriteAdjustMsgSize(file, arg);
    }
}

/*************************************************************
 *  Writes code to check that the return msg_id is correct,
 *  the return tid for camelotRoutines is correct and that
 *  the size of the return message is correct. Called by
 *  WriteRoutine.
 *************************************************************/
static void
WriteCheckIdentity(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "#if\tTypeCheck\n");
    fprintf(file, "\tmsg_size = OutP->Head.msg_size;\n");
    fprintf(file, "\tmsg_simple = OutP->Head.msg_simple;\n");
    fprintf(file, "#endif\tTypeCheck\n");
    fprintf(file, "\n");

    if (rt->rtKind == rkCamelotRoutine)
    	fprintf(file, "\tif (!TID_EQUAL(OutP->Tid, InP->Tid))\n");
    else
	fprintf(file, "\tif (OutP->Head.msg_id != %d)\n",
		rt->rtNumber + SubsystemBase + 100);
    WriteMsgError(file, rt, "MIG_REPLY_MISMATCH");
    fprintf(file, "\n");
    fprintf(file, "#if\tTypeCheck\n");

    if (rt->rtNumReplyVar > 0)
	fprintf(file, "\tif (((msg_size < %d)",
		rt->rtReplySize);
    else
	fprintf(file, "\tif (((msg_size != %d)",
		rt->rtReplySize);
    if (rt->rtSimpleCheckReply)
	fprintf(file, " || (msg_simple != %s)",
		strbool(rt->rtSimpleReceiveReply));
    fprintf(file, ") &&\n");

    fprintf(file, "\t    ((msg_size != sizeof(%s)) ||\n",
	    rt->rtKind == rkCamelotRoutine ?
	    	"camelot_death_pill_t" : "death_pill_t");
    fprintf(file, "\t     (msg_simple != TRUE) ||\n");
    fprintf(file, "\t     (OutP->RetCode == KERN_SUCCESS)))\n");
    WriteMsgError(file, rt, "MIG_TYPE_ERROR");
    fprintf(file, "#endif\tTypeCheck\n");
    fprintf(file, "\n");
}

/*************************************************************
 *  Write code to generate error handling code if the RetCode
 *  argument of a Routine is not KERN_SUCCESS.
 *************************************************************/
static void
WriteRetCodeCheck(file, rt)
    FILE *file;
    routine_t *rt;
{
    fprintf(file, "\tif (OutP->RetCode != KERN_SUCCESS)\n");
    WriteMsgError(file, rt, "OutP->RetCode");
    fprintf(file, "\n");
}

/*************************************************************
 *  Writes code to check that the type of each of the arguments
 *  in the reply message is what is expected. Called by 
 *  WriteRoutine for each argument in the reply message.
 *************************************************************/
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
	fprintf(file, "\tif (* (int *) &OutP->%s != * (int *) &%sCheck)\n",
		arg->argTTName, arg->argVarName);
	fprintf(file, "#else\tUseStaticMsgType\n");
    }
    fprintf(file, "\tif ((OutP->%s%s.msg_type_inline != %s) ||\n",
	    arg->argTTName,
	    arg->argLongForm ? ".msg_type_header" : "",
	    strbool(it->itInLine));
   fprintf(file, "\t    (OutP->%s%s.msg_type_longform != %s) ||\n",
	    arg->argTTName,
	    arg->argLongForm ? ".msg_type_header" : "",
	    strbool(arg->argLongForm));
    if (it->itOutName == MSG_TYPE_POLYMORPHIC)
    {
	if (!rt->rtSimpleCheckReply)
	    fprintf(file, "\t    (MSG_TYPE_PORT_ANY(OutP->%s.msg_type_%sname) && msg_simple) ||\n",
		    arg->argTTName,
		    arg->argLongForm ? "long_" : "");
    }
    else
	fprintf(file, "\t    (OutP->%s.msg_type_%sname != %s) ||\n",
		arg->argTTName,
		arg->argLongForm ? "long_" : "",
		it->itOutNameStr);
    if (!it->itVarArray)
	fprintf(file, "\t    (OutP->%s.msg_type_%snumber != %d) ||\n",
		arg->argTTName,
		arg->argLongForm ? "long_" : "",
		it->itNumber);
    fprintf(file, "\t    (OutP->%s.msg_type_%ssize != %d))\n",
	    arg->argTTName,
	    arg->argLongForm ? "long_" : "",
	    it->itSize);
    if (akCheck(arg->argKind, akbQuickCheck))
	fprintf(file, "#endif\tUseStaticMsgType\n");
    WriteMsgError(file, rt, "MIG_TYPE_ERROR");
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

    /* If there aren't any more Out args after this, then
       msg_size_delta value will only get used by TypeCheck code,
       so put the assignment under the TypeCheck conditional. */

    NoMoreArgs = arg->argReplyPos == rt->rtMaxReplyPos;

    /* If there aren't any more variable-sized arguments after this,
       then we must check for exact msg-size and we don't need
       to update msg_size. */

    LastVarArg = arg->argReplyPos+1 == rt->rtNumReplyVar;

    if (NoMoreArgs)
	fprintf(file, "#if\tTypeCheck\n");

    /* calculate the actual size in bytes of the data field.  note
       that this quantity must be a multiple of four.  hence, if
       the base type size isn't a multiple of four, we have to
       round up.  note also that btype->itNumber must
       divide btype->itTypeSize (see itCalculateSizeInfo). */

    fprintf(file, "\tmsg_size_delta = (%d * OutP->%s)",
	    btype->itTypeSize/btype->itNumber,
	    count->argMsgField);
    if (btype->itTypeSize % 4 != 0)
	fprintf(file, "+3 &~ 3");
    fprintf(file, ";\n");

    if (!NoMoreArgs)
	fprintf(file, "#if\tTypeCheck\n");

    /* Don't decrement msg_size until we've checked it won't underflow. */

    if (LastVarArg)
	fprintf(file, "\tif (msg_size != %d + msg_size_delta)\n",
		rt->rtReplySize);
    else
	fprintf(file, "\tif (msg_size < %d + msg_size_delta)\n",
		rt->rtReplySize);
    WriteMsgError(file, rt, "MIG_TYPE_ERROR");

    if (!LastVarArg)
	fprintf(file, "\tmsg_size -= msg_size_delta;\n");

    fprintf(file, "#endif\tTypeCheck\n");
    fprintf(file, "\n");
}

/*************************************************************
 *  Write code to copy an argument from the reply message
 *  to the parameter. Called by WriteRoutine for each argument
 *  in the reply message.
 *************************************************************/
static void
WriteExtractArgValue(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register ipc_type_t	*argType = arg->argType;
    register char *ref = argByReferenceUser(arg) ? "*" : "";

    if (argType->itInLine && argType->itVarArray)
    {
	/*
	 *	Copy out variable-size inline array with bcopy,
	 *	after checking that number of elements doesn't
	 *	exceed user's maximum.
	 */
	register argument_t *count = arg->argCount;
	register char *countRef = argByReferenceUser(count) ? "*" : "";
	register ipc_type_t *btype = argType->itElement;

	/* Note count->argMultiplier == btype->itNumber */

	fprintf(file, "\tif (OutP->%s / %d > %s%s) {\n",
		count->argMsgField, btype->itNumber,
		countRef, count->argVarName);
	/*
	 *	If number of elements is too many for user receiving area,
	 *	fill user's area as much as possible.  Return the correct
	 *	number of elements.
	 */
	fprintf(file, "\t\tbcopy((char *) OutP->%s, (char *) %s%s, ",
		arg->argMsgField, ref, arg->argVarName);
	fprintf(file, "%d * %s%s);\n\t",
		btype->itTypeSize, countRef, count->argVarName);

	WriteCopyType(file, count->argType,
		      "%s%s /* %s %d */", "/* %s%s */ OutP->%s / %d",
		      countRef, count->argVarName, count->argMsgField,
		      btype->itNumber);
	WriteMsgError(file,arg->argRoutine,"MIG_ARRAY_TOO_LARGE");
	fprintf(file, "\t}\n");

	fprintf(file, "\tbcopy((char *) OutP->%s, (char *) %s%s, ",
		arg->argMsgField, ref, arg->argVarName);
	fprintf(file, "%d * OutP->%s);\n",
		btype->itTypeSize/btype->itNumber, count->argMsgField);
    }
    else if (arg->argMultiplier > 1)
	WriteCopyType(file, argType,
		      "%s%s /* %s %d */", "/* %s%s */ OutP->%s / %d",
		      ref, arg->argVarName, arg->argMsgField,
		      arg->argMultiplier);
    else
	WriteCopyType(file, argType,
		      "%s%s /* %s */", "/* %s%s */ OutP->%s",
		      ref, arg->argVarName, arg->argMsgField);
    fprintf(file, "\n");
}

static void
WriteExtractArg(file, arg)
    FILE *file;
    register argument_t *arg;
{
    register routine_t *rt = arg->argRoutine;

    if (akCheck(arg->argKind, akbReply))
	WriteTypeCheck(file, arg);

    if (akCheckAll(arg->argKind, akbVariable|akbReply))
	WriteCheckMsgSize(file, arg);

    /* Now that the RetCode is type-checked, check its value.
       Must abort immediately if it isn't KERN_SUCCESS, because
       in that case the reply message is truncated. */

    if (arg == rt->rtRetCode)
	WriteRetCodeCheck(file, rt);

    if (akCheckAll(arg->argKind, akbReturnRcv))
	WriteExtractArgValue(file, arg);

    /* This assumes that the count argument directly follows the 
       associated variable-sized argument and any other implicit
       arguments it may have.
       Don't bother moving OutP unless there are more Out arguments. */

    if ((akIdent(arg->argKind) == akeCount) &&
	akCheck(arg->argKind, akbReturnRcv) &&
	(arg->argReplyPos < arg->argRoutine->rtMaxReplyPos))
    {
	register ipc_type_t *ptype = arg->argParent->argType;

	if (ptype->itInLine && ptype->itVarArray)
	{
	    fprintf(file, "\tOutP = (Reply *) ((char *) OutP + msg_size_delta - %d);\n",
		    ptype->itTypeSize + ptype->itPadSize);
	    fprintf(file, "\n");
	}
    }
}

/*************************************************************
 *  Writes code to return the return value. Called by WriteRoutine
 *  for routines and functions.
 *************************************************************/
static void
WriteReturnValue(file, rt)
    FILE *file;
    routine_t *rt;
{
    if (rt->rtNumReplyVar > 0)
	fprintf(file, "\tOutP = &Mess.Out;\n");

    fprintf(file, "\treturn OutP->%s;\n", rt->rtReturn->argMsgField);
}

/*************************************************************
 *  Writes the elements of the message type declaration: the
 *  msg_type structure, the argument itself and any padding 
 *  that is required to make the argument a multiple of 4 bytes.
 *  Called by WriteRoutine for all the arguments in the request
 *  message first and then the reply message.
 *************************************************************/
static void
WriteFieldDecl(file, arg)
    FILE *file;
    argument_t *arg;
{
    WriteFieldDeclPrim(file, arg, FetchUserType);
}

/*************************************************************
 *  Writes all the code comprising a routine body. Called by
 *  WriteUser for each routine.
 *************************************************************/
static void
WriteRoutine(file, rt)
    FILE *file;
    routine_t *rt;
{
    /* write the stub's declaration */

    fprintf(file, "\n");
    fprintf(file, "/* %s %s */\n", rtRoutineKindToStr(rt->rtKind), rt->rtName);
    fprintf(file, "mig_external %s %s\n", ReturnTypeStr(rt), rt->rtUserName);
    fprintf(file, "#if\t%s\n", NewCDecl);
    fprintf(file, "(\n");
    WriteList(file, rt->rtArgs, WriteVarDecl, akbUserArg, ",\n", "\n");
    fprintf(file, ")\n");
    fprintf(file, "#else\n");
    fprintf(file, "\t(");
    WriteList(file, rt->rtArgs, WriteNameDecl, akbUserArg, ", ", "");
    fprintf(file, ")\n");
    WriteList(file, rt->rtArgs, WriteVarDecl, akbUserArg, ";\n", ";\n");
    fprintf(file, "#endif\n");
    fprintf(file, "{\n");

    /* typedef of structure for Request and Reply messages */

    WriteStructDecl(file, rt->rtArgs, WriteFieldDecl, akbRequest, "Request");
    if (!rt->rtOneWay)
	WriteStructDecl(file, rt->rtArgs, WriteFieldDecl, akbReply, "Reply");

    /* declarations for local vars: Union of Request and Reply messages,
       InP, OutP and return value */

    WriteVarDecls(file, rt);

    /* declarations and initializations of the msg_type_t variables
       for each argument */

    WriteList(file, rt->rtArgs, WriteTypeDecl, akbRequest, "\n", "\n");
    if (!rt->rtOneWay)
	WriteList(file, rt->rtArgs, WriteCheckDecl,
		  akbQuickCheck|akbReply, "\n", "\n");

    /* fill in all the request message types and then arguments */

    WriteList(file, rt->rtArgs, WritePackArg, akbNone, "", "");

    /* fill in request message head */

    WriteRequestHead(file, rt);
    fprintf(file, "\n");

    /* Write the msg_send/msg_receive or msg_rpc call */

    if (rt->rtOneWay)
	WriteMsgSend(file, rt);
    else
    {
	if (UseMsgRPC)
	    WriteMsgRPC(file, rt);
	else
	    WriteMsgSendReceive(file, rt);

	/* Check the values that are returned in the reply message */

	WriteCheckIdentity(file, rt);
	WriteList(file, rt->rtArgs, WriteExtractArg, akbNone, "", "");

	/* return the return value, if any */

	if (rt->rtProcedure)
	    fprintf(file, "\t/* Procedure - no return needed */\n");
	else
	    WriteReturnValue(file, rt);
    }

    fprintf(file, "}\n");
}

/*************************************************************
 *  Writes out the xxxUser.c file. Called by mig.c
 *************************************************************/
void
WriteUser(file, stats)
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
	  case skUImport:
	  case skSImport:
	    break;
	  default:
	    fatal("WriteUser(): bad statement_kind_t (%d)",
		  (int) stat->stKind);
	}
    WriteEpilog(file);
}

/*************************************************************
 *  Writes out individual .c user files for each routine.  Called by mig.c
 *************************************************************/
void
WriteUserIndividual(stats)
    statement_t *stats;
{
    register statement_t *stat;

    for (stat = stats; stat != stNULL; stat = stat->stNext)
	switch (stat->stKind)
	{
	  case skRoutine:
	    {
		FILE *file;
		register char *filename;

		filename = strconcat(stat->stRoutine->rtName, ".c");
		file = fopen(filename, "w");
		if (file == NULL)
		    fatal("fopen(%s): %s", filename,
			  unix_error_string(errno));
		WriteProlog(file);
		WriteRoutine(file, stat->stRoutine);
		WriteEpilog(file);
		fclose(file);
		strfree(filename);
	    }
	    break;
	  case skImport:
	  case skUImport:
	  case skSImport:
	    break;
	  default:
	    fatal("WriteUser(): bad statement_kind_t (%d)",
		  (int) stat->stKind);
	}
}
