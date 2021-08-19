/*********************************************************
 *   ABSTRACT:
 *	Defines the types and structures used to pass
 *	information between the parser and code generator.
 *
 *	$Header: routine.h,v 1.1 89/06/08 14:20:05 mmeyer Locked $	
 *
 * HISTORY
 * 07-Apr-89  Richard Draves (rpd) at Carnegie-Mellon University
 *	Extensive revamping.  Added polymorphic arguments.
 *	Allow multiple variable-sized inline arguments in messages.
 *
 * 20-Dec-87  David Golub (dbg) at Carnegie-Mellon University
 *	Added pointers to last Request and Reply arguments.
 *	Added flag to show that (only) last argument is variable-sized.
 *	Added argMultiplier field for count arguments, where parent
 *	argument is itself a multiple of an IPC type.
 *
 * 16-Nov-87  David Golub (dbg) at Carnegie-Mellon University
 *	Don't add akbVarNeeded attribute here - server.c can
 *	better determine whether it is needed.
 *
 * 21-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added argFlag field to argument_t
 *
 * 18-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Changed akTid to omit akServerArg
 *	Changed arg_kind_t to u_int to make code more obvious and
 *	to get rid of compiler warnings and to give hc a chance.
 *	Changed flags on akTid, akDummy.
 *
 * 10-Aug-87  Mary Thompson (mrt) at Carnegie-Mellon University
 *	Added defines need to make MsgType a legitimate argument type
 *
 * 28-May-87  Richard Draves (rpd) at Carnegie-Mellon University
 *	Created.
 *************************************************************/

#ifndef	_ROUTINE_H
#define	_ROUTINE_H

#define	EXPORT_BOOLEAN
#if	NeXT
#include <sys/boolean.h>
#else	NeXT
#include <mach/boolean.h>
#endif	NeXT
#include <sys/types.h>
#include "type.h"

/* base kind arg */
#define akeNone		(0)
#define akeNormal	(1)	/* a normal, user-defined argument */
#define akeRequestPort	(2)	/* pointed at by rtRequestPort */
#define akeWaitTime	(3)	/* pointed at by rtWaitTime */
#define akeReplyPort	(4)	/* pointed at by rtReplyPort */
#define akeMsgType	(5)	/* pointed at by rtMsgType */
#define akeRetCode	(6)	/* pointed at by rtRetCode/rtReturn */
#define akeReturn	(7)	/* pointed at by rtReturn */
#define akeCount	(8)	/* a count arg for argParent */
#define akePoly		(9)	/* a poly arg for argParent */

/* first two digits hold the base arg_kind_t */
#define	akbRequest	(0000000100)	/* has a msg_type in request */
#define	akbReply	(0000000200)	/* has a msg_type in reply */
#define	akbUserArg	(0000000400)	/* an arg on user-side */
#define	akbServerArg	(0000001000)	/* an arg on server-side  */
#define akbSend		(0000002000)	/* value carried in request */
#define akbSendBody	(0000004000)	/* value carried in request body */
#define akbSendSnd	(0000010000)	/* value stuffed into request */
#define akbSendRcv	(0000020000)	/* value grabbed from request */
#define akbReturn	(0000040000)	/* value carried in reply */
#define akbReturnBody	(0000100000)	/* value carried in reply body */
#define akbReturnSnd	(0000200000)	/* value stuffed into reply */
#define akbReturnRcv	(0000400000)	/* value grabbed from reply */
#define akbReplyInit	(0001000000)	/* reply msg_type_t must be init'ed */
#define akbQuickCheck	(0002000000)	/* msg_type_t can be checked quickly */
#define akbReplyCopy	(0004000000)	/* copy reply value from request */
#define akbVarNeeded	(0010000000)	/* may need local var in server */
#define akbDestroy	(0020000000)	/* call destructor function */
#define akbVariable	(0040000000)	/* variable size inline data */

typedef u_int  arg_kind_t;

/*
 * akbRequest means msg_type/data fields are allocated in the request
 * msg.  akbReply means msg_type/data fields are allocated in the
 * reply msg.  These bits (with akbReplyInit and akbQuickCheck)
 * control msg structure declarations packing, and checking of
 * msg_type_t fields.
 *
 * akbUserArg means this argument is an argument to the user-side stub.
 * akbServerArg means this argument is an argument to
 * the server procedure called by the server-side stub.
 *
 * The akbSend* and akbReturn* bits control packing/extracting values
 * in the request and reply messages.
 *
 * akbSend means the argument's value is carried in the request msg.
 * akbSendBody implies akbSend; the value is carried in the msg body.
 * akbSendSnd implies akbSend; the value is stuffed into the request.
 * akbSendRcv implies akbSend; the value is pulled out of the request.
 *
 * akbReturn, akbReturnBody, akbReturnSnd, akbReturnRcv are defined
 * similarly but apply to the reply message.
 *
 * User-side code generation (header.c, user.c) and associated code
 * should use akbSendSnd and akbReturnRcv, but not akbSendRcv and
 * akbReturnSnd.  Server-side code generation (server.c) is reversed.
 * Code generation should use the more specific akb{Send,Return}{Snd,Rcv}
 * bits when possible, instead of akb{Send,Return}.
 *
 * Note that akRetCode and akReturn lack any Return bits, although
 * there is a value in the msg.  These guys are packed/unpacked
 * with special code, unlike other arguments.
 *
 * akbReplyInit implies akbReply.  It means the server-side stub
 * should initialize the argument's msg_type field in the reply msg.
 * Some special arguments (RetCode, Dummy, Tid) have their msg_type
 * fields in the reply message initialized by the server demux
 * function; these arguments have akbReply but not akbReplyInit.
 *
 * akbQuickCheck implies akbReply|akbRequest.  If it's on, then
 * the msg_type_t value can be checked quickly (by casting to an
 * int and checking with a single comparison).
 *
 * akbVariable means the argument has variable-sized inline data.
 * It isn't currently used for code generation, but routine.c
 * does use it internally.  It is added in rtAugmentArgKind.
 *
 * akbReplyCopy and akbVarNeeded help control code generation in the
 * server-side stub.  The preferred method of handling data in the
 * server-side stub avoids copying into/out-of local variables.  In
 * arguments get passed directly to the server proc from the request msg.
 * Out arguments get stuffed directly into the reply msg by the server proc.
 * For InOut arguments, the server proc gets the address of the data in
 * the request msg, and the resulting data gets copied to the reply msg.
 * Some arguments need a local variable in the server-side stub.  The
 * code extracts the data from the request msg into the variable, and
 * stuff the reply msg from the variable.
 *
 * akbReplyCopy implies akbReply.  It means the data should get copied
 * from the request msg to the reply msg after the server proc is called.
 * It is only used by akInOut.  akTid doesn't need it because the tid
 * data in the reply msg is initialized in the server demux function.
 *
 * akbVarNeeded means the argument needs a local variable in the
 * server-side stub.  It is added in rtAugmentArgKind and
 * rtCheckVariable.  An argument shouldn't have all three of
 * akbReturnSnd, akbVarNeeded and akbReplyCopy, because this indicates
 * the reply msg should be stuffed both ways.
 *
 * akbDestroy helps control code generation in the server-side stub.
 * It means this argument has a destructor function which should be called.
 *
 * Header file generation (header.c) uses:
 *	akbUserArg
 *
 * User stub generation (user.c) uses:
 *	akbUserArg, akbRequest, akbReply, akbSendSnd,
 *	akbSendBody, akbReturnRcv
 *
 * Server stub generation (server.c) uses:
 *	akbServerArg, akbRequest, akbReply, akbSendRcv, akbReturnSnd,
 *	akbReplyInit, akbReplyCopy, akbVarNeeded
 *
 *
 * During code generation, the routine, argument, and type data structures
 * are read-only.  The code generation functions' output is their only
 * side-effect.
 *
 *
 * Style note:
 * Code can use logical operators (|, &, ~) on akb values.
 * ak values should be manipulated with the ak functions.
 */

/* various useful combinations */

#define akbNone		(0)
#define akbAll		(~077)

#define akbSendBits	(akbSend|akbSendBody|akbSendSnd|akbSendRcv)
#define akbReturnBits	(akbReturn|akbReturnBody|akbReturnSnd|akbReturnRcv)
#define akbSendReturnBits	(akbSendBits|akbReturnBits)

#define akNone		akeNone

#define akIn		akAddFeature(akeNormal,				\
	akbUserArg|akbServerArg|akbRequest|akbSendBits)

#define akOut		akAddFeature(akeNormal,				\
	akbUserArg|akbServerArg|akbReply|akbReturnBits|akbReplyInit)

#define akInOut		akAddFeature(akeNormal,				\
	akbUserArg|akbServerArg|akbRequest|akbReply|			\
	akbSendBits|akbReturnBits|akbReplyInit|akbReplyCopy)

#define akRequestPort	akAddFeature(akeRequestPort,			\
	akbUserArg|akbServerArg|akbSend|akbSendSnd|akbSendRcv)

#define akWaitTime	akAddFeature(akeWaitTime, akbUserArg)

#define akMsgType	akAddFeature(akeMsgType, akbUserArg)

#define akReplyPort	akAddFeature(akeReplyPort,			\
	akbUserArg|akbSend|akbSendSnd|akbSendRcv)

#define akRetCode	akAddFeature(akeRetCode, akbReply)

#define akReturn	akAddFeature(akeReturn,				\
	akbReply|akbReplyInit)

#define akCount		akAddFeature(akeCount,				\
	akbUserArg|akbServerArg)

#define akTid		akAddFeature(akeNormal,				\
	akbRequest|akbSend|akbSendBody|akbSendSnd|akbUserArg|akbReply)

#define akDummy		akAddFeature(akeNone, akbRequest|akbReply)

#define akPoly		akePoly

#define	akCheck(ak, bits)	((ak) & (bits))
#define akCheckAll(ak, bits)	(akCheck(ak, bits) == (bits))
#define akAddFeature(ak, bits)	((ak)|(bits))
#define akRemFeature(ak, bits)	((ak)&~(bits))
#define akIdent(ak)		((ak) & 077)

/*
 * The arguments to a routine/function are linked in left-to-right order.
 * argName is used for error messages and pretty-printing,
 * not code generation.  Code generation shouldn't make any assumptions
 * about the order of arguments, esp. count and poly arguments.
 * (Unfortunately, code generation for inline variable-sized arguments
 * does make such assumptions.)
 *
 * argVarName is the name used in generated code for function arguments
 * and local variable names.  argMsgField is the name used in generated
 * code for the field in msgs where the argument's value lives.
 * argTTName is the name used in generated code for msg-type fields and
 * static variables used to initialize those fields.  argPadName is the
 * name used in generated code for a padding field in msgs.
 *
 * argFlags can be used to override the deallocate and longform bits
 * in the argument's type.  rtProcessArgFlags sets argDeallocate and
 * argLongForm from it and the type.  Code generation shouldn't use
 * argFlags.
 *
 * argCount and argPoly get to the implicit count and poly arguments
 * associated with the argument; they should be used instead of argNext.
 * In these implicit arguments, argParent is a pointer to the "real" arg.
 *
 * In count arguments, argMultiplier is a scaling factor applied to
 * the count arg's value to get msg-type-number.  It is equal to
 *	argParent->argType->itElement->itNumber
 */

typedef struct argument
{
    /* if argKind == akReturn, then argName is name of the function */
    identifier_t argName;
    struct argument *argNext;

    arg_kind_t argKind;
    ipc_type_t *argType;

    string_t argVarName;	/* local variable and argument names */
    string_t argMsgField;	/* message field's name */
    string_t argTTName;		/* name for msg_type fields, static vars */
    string_t argPadName;	/* name for pad field in msg */

    ipc_flags_t argFlags;
    boolean_t argDeallocate;	/* overrides argType->itDeallocate */
    boolean_t argLongForm;	/* overrides argType->itLongForm */

    struct routine *argRoutine;	/* routine we are part of */

    struct argument *argCount;	/* our count arg, if present */
    struct argument *argPoly;	/* our poly arg, if present */
    struct argument *argParent;	/* in a count or poly arg, the base arg */
    int argMultiplier;		/* for Count argument: parent is a multiple
				   of a basic IPC type.  Argument must be
				   multiplied by Multiplier to get IPC
				   number-of-elements. */

    /* how variable/inline args precede this one, in request and reply */
    int argRequestPos;
    int argReplyPos;

    /* the label to jump to when server type-check fails */
    int argPuntNum;
} argument_t;

#define	argByReferenceUser(arg)				\
	(akCheck((arg)->argKind, akbReturnRcv) &&	\
	 (arg)->argType->itStruct)

#define	argByReferenceServer(arg)			\
	(akCheck((arg)->argKind, akbReturnSnd) &&	\
	 (arg)->argType->itStruct)

typedef enum
{
    rkRoutine,
    rkSimpleRoutine,
    rkCamelotRoutine,
    rkSimpleProcedure,
    rkProcedure,
    rkFunction,
} routine_kind_t;

typedef struct routine
{
    identifier_t rtName;
    routine_kind_t rtKind;
    argument_t *rtArgs;
    u_int rtNumber;		/* used for making msg ids */

    identifier_t rtUserName;	/* user-visible name (UserPrefix + Name) */
    identifier_t rtServerName;	/* server-side name (ServerPrefix + Name) */

    /* rtErrorName is only used for Procs, SimpleProcs, & Functions */
    identifier_t rtErrorName;	/* error-handler name */

    boolean_t rtOneWay;		/* SimpleProcedure or SimpleRoutine */
    boolean_t rtProcedure;	/* Procedure or SimpleProcedure */
    boolean_t rtUseError;	/* Procedure or Function */

    boolean_t rtSimpleSendRequest;	/* is request a simple msg */
    boolean_t rtSimpleCheckRequest;	/* check msg-simple in request */
    boolean_t rtSimpleReceiveRequest;	/* if so, the expected value */

    boolean_t rtSimpleSendReply;	/* is reply a simple msg */
    boolean_t rtSimpleCheckReply;	/* check msg-simple in reply */
    boolean_t rtSimpleReceiveReply;	/* if so, the expected value */

    u_int rtRequestSize;	/* minimal size of a legal request msg */
    u_int rtReplySize;		/* minimal size of a legal reply msg */

    int rtNumRequestVar;	/* number of variable/inline args in request */
    int rtNumReplyVar;		/* number of variable/inline args in reply */

    int rtMaxRequestPos;	/* maximum of argRequestPos */
    int rtMaxReplyPos;		/* maximum of argReplyPos */

    /* distinguished arguments */
    argument_t *rtRequestPort;	/* always non-NULL, defaults to first arg */
    argument_t *rtReplyPort;	/* always non-NULL, defaults to Mig-supplied */
    argument_t *rtReturn;	/* non-NULL unless rtProcedure */
    argument_t *rtServerReturn;	/* NULL or rtReturn  */
    argument_t *rtRetCode;	/* always non-NULL */
    argument_t *rtWaitTime;	/* if non-NULL, will use RCV_TIMEOUT */
    argument_t *rtMsgType;	/* always non-NULL, defaults to NORMAL */
} routine_t;

#define rtNULL		((routine_t *) 0)
#define argNULL		((argument_t *) 0)

extern u_int rtNumber;
/* rt->rtNumber will be initialized */
extern routine_t *rtAlloc();
/* skip a number */
extern void rtSkip();

extern argument_t *argAlloc();

extern boolean_t
rtCheckMask(/* argument_t *args, u_int mask */);

extern routine_t *
rtMakeRoutine(/* identifier_t name, argument_t *args */);
extern routine_t *
rtMakeSimpleRoutine(/* identifier_t name, argument_t *args */);
extern routine_t *
rtMakeCamelotRoutine(/* identifier_t name, argument_t *args */);
extern routine_t *
rtMakeProcedure(/* identifier_t name, argument_t *args */);
extern routine_t *
rtMakeSimpleProcedure(/* identifier_t name, argument_t *args */);
extern routine_t *
rtMakeFunction(/* identifier_t name, argument_t *args, ipc_type_t *type */);

extern void rtPrintRoutine(/* routine_t *rt */);
extern void rtCheckRoutine(/* routine_t *rt */);

extern char *rtRoutineKindToStr(/* routine_kind_t rk */);

#endif	_ROUTINE_H
