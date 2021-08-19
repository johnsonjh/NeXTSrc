#ifndef	_mallocProtocol
#define	_mallocProtocol

/* Module mallocProtocol */

#include <sys/kern_return.h>
#if	(defined(__STDC__) || defined(c_plusplus)) || defined(LINTLIBRARY)
#include <sys/port.h>
#include <sys/message.h>
#endif

#ifndef	mig_external
#define mig_external extern
#endif

#include "mallocProtocol_types.h"

/* Function getMallocData */
mig_external retval_t getMallocData
#if	defined(LINTLIBRARY)
    (server, whichNodes, nodeInfo, nodeInfoCnt, numNodes)
	port_t server;
	NodeType whichNodes;
	data_t *nodeInfo;
	unsigned int *nodeInfoCnt;
	int *numNodes;
{ return getMallocData(server, whichNodes, nodeInfo, nodeInfoCnt, numNodes); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server,
	NodeType whichNodes,
	data_t *nodeInfo,
	unsigned int *nodeInfoCnt,
	int *numNodes
);
#else
    ();
#endif
#endif

/* Function resetMalloc */
mig_external retval_t resetMalloc
#if	defined(LINTLIBRARY)
    (server)
	port_t server;
{ return resetMalloc(server); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server
);
#else
    ();
#endif
#endif

#endif	_mallocProtocol
