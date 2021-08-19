
#import "mallocdebug/PortListen.h"
#import <stddef.h>				/* For NULL */
#import <stdlib.h>				/* For malloc() */
#import <cthreads.h>

/* The set of ports being listened to by the asynchronous thread. */

static port_set_name_t portSet = 0;

/* Information recorded about a port. */

typedef struct portDataStruct
{
  port_t port;
  void (*handler) (msg_header_t *msg, void *userData);
  struct portDataStruct *next;
  void *userData;
} portData;

static portData *portDataList = NULL;


static void portSetListen ()
{
  while (1)
    {
      msg_header_t header;
      portData *data;
       
      header.msg_local_port = portSet;
      header.msg_size = MSG_SIZE_MAX;
      
      msg_receive (&header, MSG_OPTION_NONE, 0);
      
      for (data = portDataList; data != NULL; data = data->next)
        {
	  if (data->port == header.msg_local_port)
	    {
	      (*data->handler) (&header, data->userData);
	      break;
	    }
	}
    }
}


void NXPortListen (port_t newPort,
	           void (*handler) (msg_header_t *msg, void *userData),
	           void *userData)
{
  portData *newPortData = malloc (sizeof (portData));
  
  newPortData->port = newPort;
  newPortData->handler = handler;
  newPortData->userData = userData;
  newPortData->next = portDataList;
  portDataList = newPortData;			/* Assumed to be atomic. */
  
  /* Allocate the port set and fork the thread. */
  
  if (portSet == 0)
    {
      cthread_t thread;
      
      port_set_allocate (task_self (), &portSet);
      thread = cthread_fork (&portSetListen, NULL);
      cthread_set_name (thread, "NXPortListen");
      cthread_detach (thread);
    }
  
  /* Add the new port to the port set. */
  
  port_set_add (task_self (), portSet, newPort);
}
