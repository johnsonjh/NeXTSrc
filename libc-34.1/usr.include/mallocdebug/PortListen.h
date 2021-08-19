
#import <mach.h>

/* Listen on a Mach port asynchronously. */

/* newPort is added to the port set being waited on by a thread devoted to
   that purpose.  When a message arrives on newPort, handler() will be called
   with the message and with the blind userData pointer passed to NXPortListen().
   Note that handler() will be executing in the asynchronous thread, so you
   cannot make use many appkit or libc calls. */

extern void NXPortListen (port_t newPort,
		          void (*handler) (msg_header_t *msg, void *userData),
		          void *userData);
