s/result = (\*local)(&argument, rqstp);/socket_unlock(); & socket_lock();/
s/if (!svc_freeargs/if (result != NULL) { (*local)(\&argument, NULL); } \
	if (!svc_freeargs/
s/#include "clib.h"/&\
#include "socket_lock.h"\
\
extern int udp_sock;/
s/switch (rqstp->rq_proc)/\
	if (transp->xp_sock == udp_sock \&\& \
		!(rqstp->rq_proc == _NI_BIND || \
		  rqstp->rq_proc == _NI_PING)) { \
		svcerr_weakauth(transp); \
	} \
	&/
	
