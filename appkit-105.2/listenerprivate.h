/*
	listenerprivate.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Greg Cockroft
*/

/*
 *    characters that represent parameter types in messages.
 */
 
#define NX_MSG_DOUBLE_IN 	'd'
#define NX_MSG_DOUBLE_OUT 	'D'
#define NX_MSG_INT_IN 		'i'
#define NX_MSG_INT_OUT 		'I'
#define NX_MSG_CHARS_IN 	'c'
#define NX_MSG_CHARS_OUT 	'C'
#define NX_MSG_BYTES_IN 	'b'
#define NX_MSG_BYTES_OUT 	'B'
#define NX_MSG_PORT_RCV_IN	'r'
#define NX_MSG_PORT_RCV_OUT	'R'
#define NX_MSG_PORT_SEND_IN	's'
#define NX_MSG_PORT_SEND_OUT	'S'

extern msg_return_t _NXSafeReceive(NXMessage *msg, int options, int timeout);
extern msg_return_t _NXSafeSend(NXMessage *msg, int options, int timeout);


#ifdef MSG_DEBUG
#define TRACE(msg)				NXLogError(msg)
#define TRACE1(msg, a1)				NXLogError(msg, a1)
#define TRACE2(msg, a1, a2)			NXLogError(msg, a1, a2)
#define TRACE3(msg, a1, a2, a3)			NXLogError(msg, a1, a2, a3)
#define TRACE4(msg, a1, a2, a3, a4)		NXLogError(msg, a1, a2, a3, a4)
#else
#define TRACE(msg)
#define TRACE1(msg, a1)
#define TRACE2(msg, a1, a2)
#define TRACE3(msg, a1, a2, a3)
#define TRACE4(msg, a1, a2, a3, a4)
#endif

