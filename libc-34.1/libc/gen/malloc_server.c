/* Module mallocProtocol */

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
#include <kern/std_types.h>
#include "mallocdebug/mallocProtocol_types.h"

/* Function getMallocData */
mig_internal novalue _XgetMallocData
#if	(defined(__STDC__) || defined(c_plusplus))
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
#else
	(InHeadP, OutHeadP)
	msg_header_t *InHeadP, *OutHeadP;
#endif
{
	typedef struct {
		msg_header_t Head;
		msg_type_t whichNodesType;
		NodeType whichNodes;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t getMallocDataType;
		retval_t getMallocData;
		msg_type_long_t nodeInfoType;
		data_t nodeInfo;
		msg_type_t numNodesType;
		int numNodes;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern retval_t getMallocData
#if	(defined(__STDC__) || defined(c_plusplus))
		(port_t server, NodeType whichNodes, data_t *nodeInfo, unsigned int *nodeInfoCnt, int *numNodes);
#else
		();
#endif

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t whichNodesCheck = {
		/* msg_type_name = */		MSG_TYPE_INTEGER_32,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
	};
#endif	UseStaticMsgType

#if	UseStaticMsgType
	static const msg_type_t getMallocDataType = {
		/* msg_type_name = */		MSG_TYPE_INTEGER_32,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
	};
#endif	UseStaticMsgType

#if	UseStaticMsgType
	static const msg_type_long_t nodeInfoType = {
	{
		/* msg_type_name = */		0,
		/* msg_type_size = */		0,
		/* msg_type_number = */		0,
		/* msg_type_inline = */		FALSE,
		/* msg_type_longform = */	TRUE,
		/* msg_type_deallocate = */	FALSE,
	},
		/* msg_type_long_name = */	MSG_TYPE_CHAR,
		/* msg_type_long_size = */	8,
		/* msg_type_long_number = */	0,
	};
#endif	UseStaticMsgType

#if	UseStaticMsgType
	static const msg_type_t numNodesType = {
		/* msg_type_name = */		MSG_TYPE_INTEGER_32,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
	};
#endif	UseStaticMsgType

	unsigned int nodeInfoCnt;

#if	TypeCheck
	msg_size = In0P->Head.msg_size;
	msg_simple = In0P->Head.msg_simple;
	if ((msg_size != 32) || (msg_simple != TRUE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
#if	UseStaticMsgType
	if (* (int *) &In0P->whichNodesType != * (int *) &whichNodesCheck)
#else	UseStaticMsgType
	if ((In0P->whichNodesType.msg_type_inline != TRUE) ||
	    (In0P->whichNodesType.msg_type_longform != FALSE) ||
	    (In0P->whichNodesType.msg_type_name != MSG_TYPE_INTEGER_32) ||
	    (In0P->whichNodesType.msg_type_number != 1) ||
	    (In0P->whichNodesType.msg_type_size != 32))
#endif	UseStaticMsgType
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->getMallocData = getMallocData(In0P->Head.msg_request_port, In0P->whichNodes, &OutP->nodeInfo, &nodeInfoCnt, &OutP->numNodes);
	OutP->RetCode = KERN_SUCCESS;
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 64;

#if	UseStaticMsgType
	OutP->getMallocDataType = getMallocDataType;
#else	UseStaticMsgType
	OutP->getMallocDataType.msg_type_name = MSG_TYPE_INTEGER_32;
	OutP->getMallocDataType.msg_type_size = 32;
	OutP->getMallocDataType.msg_type_number = 1;
	OutP->getMallocDataType.msg_type_inline = TRUE;
	OutP->getMallocDataType.msg_type_longform = FALSE;
	OutP->getMallocDataType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

#if	UseStaticMsgType
	OutP->nodeInfoType = nodeInfoType;
#else	UseStaticMsgType
	OutP->nodeInfoType.msg_type_long_name = MSG_TYPE_CHAR;
	OutP->nodeInfoType.msg_type_long_size = 8;
	OutP->nodeInfoType.msg_type_header.msg_type_inline = FALSE;
	OutP->nodeInfoType.msg_type_header.msg_type_longform = TRUE;
	OutP->nodeInfoType.msg_type_header.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->nodeInfoType.msg_type_long_number /* nodeInfoCnt */ = /* nodeInfoType.msg_type_long_number */ nodeInfoCnt;

#if	UseStaticMsgType
	OutP->numNodesType = numNodesType;
#else	UseStaticMsgType
	OutP->numNodesType.msg_type_name = MSG_TYPE_INTEGER_32;
	OutP->numNodesType.msg_type_size = 32;
	OutP->numNodesType.msg_type_number = 1;
	OutP->numNodesType.msg_type_inline = TRUE;
	OutP->numNodesType.msg_type_longform = FALSE;
	OutP->numNodesType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->Head.msg_simple = FALSE;
	OutP->Head.msg_size = msg_size;
}

/* Function resetMalloc */
mig_internal novalue _XresetMalloc
#if	(defined(__STDC__) || defined(c_plusplus))
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
#else
	(InHeadP, OutHeadP)
	msg_header_t *InHeadP, *OutHeadP;
#endif
{
	typedef struct {
		msg_header_t Head;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t resetMallocType;
		retval_t resetMalloc;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern retval_t resetMalloc
#if	(defined(__STDC__) || defined(c_plusplus))
		(port_t server);
#else
		();
#endif

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t resetMallocType = {
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
	if ((msg_size != 24) || (msg_simple != TRUE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

	OutP->resetMalloc = resetMalloc(In0P->Head.msg_request_port);
	OutP->RetCode = KERN_SUCCESS;
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 40;

#if	UseStaticMsgType
	OutP->resetMallocType = resetMallocType;
#else	UseStaticMsgType
	OutP->resetMallocType.msg_type_name = MSG_TYPE_INTEGER_32;
	OutP->resetMallocType.msg_type_size = 32;
	OutP->resetMallocType.msg_type_number = 1;
	OutP->resetMallocType.msg_type_inline = TRUE;
	OutP->resetMallocType.msg_type_longform = FALSE;
	OutP->resetMallocType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->Head.msg_simple = TRUE;
	OutP->Head.msg_size = msg_size;
}

/* Function setMallocMarkNumber */
mig_internal novalue _XsetMallocMarkNumber
#if	(defined(__STDC__) || defined(c_plusplus))
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
#else
	(InHeadP, OutHeadP)
	msg_header_t *InHeadP, *OutHeadP;
#endif
{
	typedef struct {
		msg_header_t Head;
		msg_type_t nodeNumberType;
		int nodeNumber;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t setMallocMarkNumberType;
		retval_t setMallocMarkNumber;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern retval_t setMallocMarkNumber
#if	(defined(__STDC__) || defined(c_plusplus))
		(port_t server, int nodeNumber);
#else
		();
#endif

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t nodeNumberCheck = {
		/* msg_type_name = */		MSG_TYPE_INTEGER_32,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
	};
#endif	UseStaticMsgType

#if	UseStaticMsgType
	static const msg_type_t setMallocMarkNumberType = {
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
	if ((msg_size != 32) || (msg_simple != TRUE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

#if	TypeCheck
#if	UseStaticMsgType
	if (* (int *) &In0P->nodeNumberType != * (int *) &nodeNumberCheck)
#else	UseStaticMsgType
	if ((In0P->nodeNumberType.msg_type_inline != TRUE) ||
	    (In0P->nodeNumberType.msg_type_longform != FALSE) ||
	    (In0P->nodeNumberType.msg_type_name != MSG_TYPE_INTEGER_32) ||
	    (In0P->nodeNumberType.msg_type_number != 1) ||
	    (In0P->nodeNumberType.msg_type_size != 32))
#endif	UseStaticMsgType
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; goto punt0; }
#define	label_punt0
#endif	TypeCheck

	OutP->setMallocMarkNumber = setMallocMarkNumber(In0P->Head.msg_request_port, In0P->nodeNumber);
	OutP->RetCode = KERN_SUCCESS;
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 40;

#if	UseStaticMsgType
	OutP->setMallocMarkNumberType = setMallocMarkNumberType;
#else	UseStaticMsgType
	OutP->setMallocMarkNumberType.msg_type_name = MSG_TYPE_INTEGER_32;
	OutP->setMallocMarkNumberType.msg_type_size = 32;
	OutP->setMallocMarkNumberType.msg_type_number = 1;
	OutP->setMallocMarkNumberType.msg_type_inline = TRUE;
	OutP->setMallocMarkNumberType.msg_type_longform = FALSE;
	OutP->setMallocMarkNumberType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->Head.msg_simple = TRUE;
	OutP->Head.msg_size = msg_size;
}

/* Function getCurrentMallocNodeNumber */
mig_internal novalue _XgetCurrentMallocNodeNumber
#if	(defined(__STDC__) || defined(c_plusplus))
	(msg_header_t *InHeadP, msg_header_t *OutHeadP)
#else
	(InHeadP, OutHeadP)
	msg_header_t *InHeadP, *OutHeadP;
#endif
{
	typedef struct {
		msg_header_t Head;
	} Request;

	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
		msg_type_t getCurrentMallocNodeNumberType;
		retval_t getCurrentMallocNodeNumber;
		msg_type_t nodeNumberType;
		int nodeNumber;
	} Reply;

	register Request *In0P = (Request *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	extern retval_t getCurrentMallocNodeNumber
#if	(defined(__STDC__) || defined(c_plusplus))
		(port_t server, int *nodeNumber);
#else
		();
#endif

#if	TypeCheck
	boolean_t msg_simple;
#endif	TypeCheck

	unsigned int msg_size;

#if	UseStaticMsgType
	static const msg_type_t getCurrentMallocNodeNumberType = {
		/* msg_type_name = */		MSG_TYPE_INTEGER_32,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
	};
#endif	UseStaticMsgType

#if	UseStaticMsgType
	static const msg_type_t nodeNumberType = {
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
	if ((msg_size != 24) || (msg_simple != TRUE))
		{ OutP->RetCode = MIG_BAD_ARGUMENTS; return; }
#endif	TypeCheck

	OutP->getCurrentMallocNodeNumber = getCurrentMallocNodeNumber(In0P->Head.msg_request_port, &OutP->nodeNumber);
	OutP->RetCode = KERN_SUCCESS;
#ifdef	label_punt0
#undef	label_punt0
punt0:
#endif	label_punt0
	if (OutP->RetCode != KERN_SUCCESS)
		return;

	msg_size = 48;

#if	UseStaticMsgType
	OutP->getCurrentMallocNodeNumberType = getCurrentMallocNodeNumberType;
#else	UseStaticMsgType
	OutP->getCurrentMallocNodeNumberType.msg_type_name = MSG_TYPE_INTEGER_32;
	OutP->getCurrentMallocNodeNumberType.msg_type_size = 32;
	OutP->getCurrentMallocNodeNumberType.msg_type_number = 1;
	OutP->getCurrentMallocNodeNumberType.msg_type_inline = TRUE;
	OutP->getCurrentMallocNodeNumberType.msg_type_longform = FALSE;
	OutP->getCurrentMallocNodeNumberType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

#if	UseStaticMsgType
	OutP->nodeNumberType = nodeNumberType;
#else	UseStaticMsgType
	OutP->nodeNumberType.msg_type_name = MSG_TYPE_INTEGER_32;
	OutP->nodeNumberType.msg_type_size = 32;
	OutP->nodeNumberType.msg_type_number = 1;
	OutP->nodeNumberType.msg_type_inline = TRUE;
	OutP->nodeNumberType.msg_type_longform = FALSE;
	OutP->nodeNumberType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	OutP->Head.msg_simple = TRUE;
	OutP->Head.msg_size = msg_size;
}

boolean_t mallocProtocol_server
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

	if ((InP->msg_id > 3) || (InP->msg_id < 0))
		return FALSE;
	else {
		typedef novalue (*SERVER_STUB_PROC)
#if	(defined(__STDC__) || defined(c_plusplus))
			(msg_header_t *, msg_header_t *);
#else
			();
#endif
		static const SERVER_STUB_PROC routines[] = {
			_XgetMallocData,
			_XresetMalloc,
			_XsetMallocMarkNumber,
			_XgetCurrentMallocNodeNumber,
		};

		if (routines[InP->msg_id - 0])
			(routines[InP->msg_id - 0]) (InP, &OutP->Head);
		 else
			return FALSE;
	}
	return TRUE;
}
