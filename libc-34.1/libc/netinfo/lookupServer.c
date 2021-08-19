/* Module lookup */

#define EXPORT_BOOLEAN
#include <sys/boolean.h>
#include <sys/message.h>
#include <sys/mig_errors.h>

#ifndef	mig_internal
#define	mig_internal	static
#endif

#ifndef	TypeCheck
#define	TypeCheck 1
#endif

#ifndef	UseExternRCSId
#ifdef	hc
#define	UseExternRCSId		1
#endif
#endif

#ifndef	UseStaticMsgType
#if	!defined(hc) || defined(__STDC__)
#define	UseStaticMsgType	1
#endif
#endif

/* Due to pcc compiler bug, cannot use void */
#if	(defined(__STDC__) || defined(c_plusplus)) || defined(hc)
#define novalue void
#else
#define novalue int
#endif

#define msg_request_port	msg_local_port
#define msg_reply_port		msg_remote_port
#include <netinfo/lookup_types.h>

/* Routine _lookup_link */
mig_internal novalue _X_lookup_link
#if	(defined(__STDC__) || defined(c_plusplus))
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
#else
	(InHeadP, OutHeadP)
	msg_header_t *InHeadP, *OutHeadP;
#endif
{
	typedef struct {
		msg_header_t Head;
		msg_type_long_t nameType;
		lookup_name name;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t procnoType;
		int procno;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t __lookup_link
#if	(defined(__STDC__) || defined(c_plusplus))
		(port_t server, lookup_name name, int *procno);
#else
		();
#endif

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t procnoType = {
		/* msg_type_name = */		MSG_TYPE_INTEGER_32,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
	};
#endif	UseStaticMsgType

#if	TypeCheck
	msg_size = In0P->Head.msg_size;
	msg_simple = In0P->Head.msg_simple;
	if ((msg_size != 292) || (msg_simple != TRUE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
	if ((In0P->nameType.msg_type_header.msg_type_inline != TRUE) ||
	    (In0P->nameType.msg_type_header.msg_type_longform != TRUE) ||
	    (In0P->nameType.msg_type_long_name != MSG_TYPE_STRING) ||
	    (In0P->nameType.msg_type_long_number != 1) ||
	    (In0P->nameType.msg_type_long_size != 2048))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->RetCode = __lookup_link(In0P->Head.msg_request_port, In0P->name, &OutP->procno);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 40;

#if	UseStaticMsgType
	OutP->procnoType = procnoType;
#else	UseStaticMsgType
	OutP->procnoType.msg_type_name = MSG_TYPE_INTEGER_32;
	OutP->procnoType.msg_type_size = 32;
	OutP->procnoType.msg_type_number = 1;
	OutP->procnoType.msg_type_inline = TRUE;
	OutP->procnoType.msg_type_longform = FALSE;
	OutP->procnoType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->Head.msg_simple = TRUE;
	OutP->Head.msg_size = msg_size;
}

/* Routine _lookup_all */
mig_internal novalue _X_lookup_all
#if	(defined(__STDC__) || defined(c_plusplus))
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
#else
	(InHeadP, OutHeadP)
	msg_header_t *InHeadP, *OutHeadP;
#endif
{
	typedef struct {
		msg_header_t Head;
		msg_type_t procType;
		int proc;
		msg_type_long_t indataType;
		unit indata[4096];
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_long_t outdataType;
		ooline_data outdata;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t __lookup_all
#if	(defined(__STDC__) || defined(c_plusplus))
		(port_t server, int proc, inline_data indata, unsigned int indataCnt, ooline_data *outdata, unsigned int *outdataCnt);
#else
		();
#endif

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;
	unsigned int msg_size_delta;

#if	UseStaticMsgType
	static const msg_type_t procCheck = {
		/* msg_type_name = */		MSG_TYPE_INTEGER_32,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
	};
#endif	UseStaticMsgType

#if	UseStaticMsgType
	static const msg_type_long_t outdataType = {
	{
		/* msg_type_name = */		0,
		/* msg_type_size = */		0,
		/* msg_type_number = */		0,
		/* msg_type_inline = */		FALSE,
		/* msg_type_longform = */	TRUE,
		/* msg_type_deallocate = */	FALSE,
	},
		/* msg_type_long_name = */	MSG_TYPE_UNSTRUCTURED,
		/* msg_type_long_size = */	32,
		/* msg_type_long_number = */	0,
	};
#endif	UseStaticMsgType

	unsigned int outdataCnt;

#if	TypeCheck
	msg_size = In0P->Head.msg_size;
	msg_simple = In0P->Head.msg_simple;
	if ((msg_size < 44) || (msg_simple != TRUE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
#if	UseStaticMsgType
	if (* (int *) &In0P->procType != * (int *) &procCheck)
#else	UseStaticMsgType
	if ((In0P->procType.msg_type_inline != TRUE) ||
	    (In0P->procType.msg_type_longform != FALSE) ||
	    (In0P->procType.msg_type_name != MSG_TYPE_INTEGER_32) ||
	    (In0P->procType.msg_type_number != 1) ||
	    (In0P->procType.msg_type_size != 32))
#endif	UseStaticMsgType
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

#if	TypeCheck
	if ((In0P->indataType.msg_type_header.msg_type_inline != TRUE) ||
	    (In0P->indataType.msg_type_header.msg_type_longform != TRUE) ||
	    (In0P->indataType.msg_type_long_name != MSG_TYPE_UNSTRUCTURED) ||
	    (In0P->indataType.msg_type_long_size != 32))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

#if	TypeCheck
	msg_size_delta = (4 * In0P->indataType.msg_type_long_number);
	if (msg_size != 44 + msg_size_delta)
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->RetCode = __lookup_all(In0P->Head.msg_request_port, In0P->proc, In0P->indata, In0P->indataType.msg_type_long_number, &OutP->outdata, &outdataCnt);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 48;

#if	UseStaticMsgType
	OutP->outdataType = outdataType;
#else	UseStaticMsgType
	OutP->outdataType.msg_type_long_name = MSG_TYPE_UNSTRUCTURED;
	OutP->outdataType.msg_type_long_size = 32;
	OutP->outdataType.msg_type_header.msg_type_inline = FALSE;
	OutP->outdataType.msg_type_header.msg_type_longform = TRUE;
	OutP->outdataType.msg_type_header.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->outdataType.msg_type_long_number /* outdataCnt */ = /* outdataType.msg_type_long_number */ outdataCnt;

	OutP->Head.msg_simple = FALSE;
	OutP->Head.msg_size = msg_size;
}

/* Routine _lookup_one */
mig_internal novalue _X_lookup_one
#if	(defined(__STDC__) || defined(c_plusplus))
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
#else
	(InHeadP, OutHeadP)
	msg_header_t *InHeadP, *OutHeadP;
#endif
{
	typedef struct {
		msg_header_t Head;
		msg_type_t procType;
		int proc;
		msg_type_long_t indataType;
		unit indata[4096];
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_long_t outdataType;
		unit outdata[4096];
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t __lookup_one
#if	(defined(__STDC__) || defined(c_plusplus))
		(port_t server, int proc, inline_data indata, unsigned int indataCnt, inline_data outdata, unsigned int *outdataCnt);
#else
		();
#endif

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;
	unsigned int msg_size_delta;

#if	UseStaticMsgType
	static const msg_type_t procCheck = {
		/* msg_type_name = */		MSG_TYPE_INTEGER_32,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
	};
#endif	UseStaticMsgType

#if	UseStaticMsgType
	static const msg_type_long_t outdataType = {
	{
		/* msg_type_name = */		0,
		/* msg_type_size = */		0,
		/* msg_type_number = */		0,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	TRUE,
		/* msg_type_deallocate = */	FALSE,
	},
		/* msg_type_long_name = */	MSG_TYPE_UNSTRUCTURED,
		/* msg_type_long_size = */	32,
		/* msg_type_long_number = */	4096,
	};
#endif	UseStaticMsgType

	unsigned int outdataCnt;

#if	TypeCheck
	msg_size = In0P->Head.msg_size;
	msg_simple = In0P->Head.msg_simple;
	if ((msg_size < 44) || (msg_simple != TRUE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
#if	UseStaticMsgType
	if (* (int *) &In0P->procType != * (int *) &procCheck)
#else	UseStaticMsgType
	if ((In0P->procType.msg_type_inline != TRUE) ||
	    (In0P->procType.msg_type_longform != FALSE) ||
	    (In0P->procType.msg_type_name != MSG_TYPE_INTEGER_32) ||
	    (In0P->procType.msg_type_number != 1) ||
	    (In0P->procType.msg_type_size != 32))
#endif	UseStaticMsgType
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

#if	TypeCheck
	if ((In0P->indataType.msg_type_header.msg_type_inline != TRUE) ||
	    (In0P->indataType.msg_type_header.msg_type_longform != TRUE) ||
	    (In0P->indataType.msg_type_long_name != MSG_TYPE_UNSTRUCTURED) ||
	    (In0P->indataType.msg_type_long_size != 32))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

#if	TypeCheck
	msg_size_delta = (4 * In0P->indataType.msg_type_long_number);
	if (msg_size != 44 + msg_size_delta)
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	outdataCnt = 4096;

	OutP->RetCode = __lookup_one(In0P->Head.msg_request_port, In0P->proc, In0P->indata, In0P->indataType.msg_type_long_number, OutP->outdata, &outdataCnt);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 44;

#if	UseStaticMsgType
	OutP->outdataType = outdataType;
#else	UseStaticMsgType
	OutP->outdataType.msg_type_long_name = MSG_TYPE_UNSTRUCTURED;
	OutP->outdataType.msg_type_long_size = 32;
	OutP->outdataType.msg_type_header.msg_type_inline = TRUE;
	OutP->outdataType.msg_type_header.msg_type_longform = TRUE;
	OutP->outdataType.msg_type_header.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->outdataType.msg_type_long_number /* outdataCnt */ = /* outdataType.msg_type_long_number */ outdataCnt;

	msg_size_delta = (4 * outdataCnt);
	msg_size += msg_size_delta;

	OutP->Head.msg_simple = TRUE;
	OutP->Head.msg_size = msg_size;
}

/* Routine _lookup_ooall */
mig_internal novalue _X_lookup_ooall
#if	(defined(__STDC__) || defined(c_plusplus))
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
#else
	(InHeadP, OutHeadP)
	msg_header_t *InHeadP, *OutHeadP;
#endif
{
	typedef struct {
		msg_header_t Head;
		msg_type_t procType;
		int proc;
		msg_type_long_t indataType;
		ooline_data indata;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_long_t outdataType;
		ooline_data outdata;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern kern_return_t __lookup_ooall
#if	(defined(__STDC__) || defined(c_plusplus))
		(port_t server, int proc, ooline_data indata, unsigned int indataCnt, ooline_data *outdata, unsigned int *outdataCnt);
#else
		();
#endif

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t procCheck = {
		/* msg_type_name = */		MSG_TYPE_INTEGER_32,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
	};
#endif	UseStaticMsgType

#if	UseStaticMsgType
	static const msg_type_long_t outdataType = {
	{
		/* msg_type_name = */		0,
		/* msg_type_size = */		0,
		/* msg_type_number = */		0,
		/* msg_type_inline = */		FALSE,
		/* msg_type_longform = */	TRUE,
		/* msg_type_deallocate = */	FALSE,
	},
		/* msg_type_long_name = */	MSG_TYPE_UNSTRUCTURED,
		/* msg_type_long_size = */	32,
		/* msg_type_long_number = */	0,
	};
#endif	UseStaticMsgType

	unsigned int outdataCnt;

#if	TypeCheck
	msg_size = In0P->Head.msg_size;
	msg_simple = In0P->Head.msg_simple;
	if ((msg_size != 48) || (msg_simple != FALSE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
#if	UseStaticMsgType
	if (* (int *) &In0P->procType != * (int *) &procCheck)
#else	UseStaticMsgType
	if ((In0P->procType.msg_type_inline != TRUE) ||
	    (In0P->procType.msg_type_longform != FALSE) ||
	    (In0P->procType.msg_type_name != MSG_TYPE_INTEGER_32) ||
	    (In0P->procType.msg_type_number != 1) ||
	    (In0P->procType.msg_type_size != 32))
#endif	UseStaticMsgType
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

#if	TypeCheck
	if ((In0P->indataType.msg_type_header.msg_type_inline != FALSE) ||
	    (In0P->indataType.msg_type_header.msg_type_longform != TRUE) ||
	    (In0P->indataType.msg_type_long_name != MSG_TYPE_UNSTRUCTURED) ||
	    (In0P->indataType.msg_type_long_size != 32))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->RetCode = __lookup_ooall(In0P->Head.msg_request_port, In0P->proc, In0P->indata, In0P->indataType.msg_type_long_number, &OutP->outdata, &outdataCnt);
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 48;

#if	UseStaticMsgType
	OutP->outdataType = outdataType;
#else	UseStaticMsgType
	OutP->outdataType.msg_type_long_name = MSG_TYPE_UNSTRUCTURED;
	OutP->outdataType.msg_type_long_size = 32;
	OutP->outdataType.msg_type_header.msg_type_inline = FALSE;
	OutP->outdataType.msg_type_header.msg_type_longform = TRUE;
	OutP->outdataType.msg_type_header.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->outdataType.msg_type_long_number /* outdataCnt */ = /* outdataType.msg_type_long_number */ outdataCnt;

	OutP->Head.msg_simple = FALSE;
	OutP->Head.msg_size = msg_size;
}

boolean_t lookup_server
#if	(defined(__STDC__) || defined(c_plusplus))
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
#else
	(InHeadP, OutHeadP)
	msg_header_t *InHeadP, *OutHeadP;
#endif
{
	register msg_header_t *InP =  InHeadP;
	register death_pill_t *OutP = (death_pill_t *) OutHeadP;

#if	UseStaticMsgType
	static const msg_type_t RetCodeType = {
		/* msg_type_name = */		MSG_TYPE_INTEGER_32,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
	};
#endif	UseStaticMsgType

	OutP->Head.msg_simple = TRUE;
	OutP->Head.msg_size = sizeof *OutP;
	OutP->Head.msg_type = InP->msg_type;
	OutP->Head.msg_local_port = PORT_NULL;
	OutP->Head.msg_remote_port = InP->msg_reply_port;
	OutP->Head.msg_id = InP->msg_id + 100;

#if	UseStaticMsgType
	OutP->RetCodeType = RetCodeType;
#else	UseStaticMsgType
	OutP->RetCodeType.msg_type_name = MSG_TYPE_INTEGER_32;
	OutP->RetCodeType.msg_type_size = 32;
	OutP->RetCodeType.msg_type_number = 1;
	OutP->RetCodeType.msg_type_inline = TRUE;
	OutP->RetCodeType.msg_type_longform = FALSE;
	OutP->RetCodeType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType
	OutP->RetCode = MIG_BAD_ID;

	if ((InP->msg_id > 4241778) || (InP->msg_id < 4241775))
		return FALSE;
	else {
		typedef novalue (*SERVER_STUB_PROC)
#if	(defined(__STDC__) || defined(c_plusplus))
			(msg_header_t *, msg_header_t *);
#else
			();
#endif
		static const SERVER_STUB_PROC routines[] = {
			_X_lookup_link,
			_X_lookup_all,
			_X_lookup_one,
			_X_lookup_ooall,
		};

		if (routines[InP->msg_id - 4241775])
			(routines[InP->msg_id - 4241775]) (InP, &OutP->Head);
		 else
			return FALSE;
	}
	return TRUE;
}
