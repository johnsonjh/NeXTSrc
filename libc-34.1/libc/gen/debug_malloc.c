
/* debug_malloc.c */

#import <mallocdebug/PortListen.h>
#import <mallocdebug/mallocProtocol.h>

extern boolean_t
mallocProtocol_server (msg_header_t *InHeadP, msg_header_t *OutHeadP);

#import <stddef.h>			/* For NULL */
#import <stdlib.h>
#import <stdio.h>
#import <string.h>
#import <libc.h>			/* For getpid () */
#import <mach.h>
#import <mach_error.h>
#import <servers/netname.h>		/* For netname_look_up () */
#import <servers/bootstrap.h>		/* For bootstrap_status () */
#import <sys/message.h>
#import <mig_errors.h>
#import <msg_type.h>

#ifdef SHLIB
#undef valloc
#undef calloc
#undef realloc
#undef malloc
#undef free
#undef mstats
#undef malloc_error
#undef malloc_size
#undef malloc_debug
#undef malloc_good_size
#endif SHLIB

static unsigned int HASHSIZE;
static unsigned int FREESIZE;
static unsigned int MALLOCSIZE;
static unsigned int MAX_PASSWD_SIZE;
static const unsigned int PASSWD1 = 0xdeadbeef;
static const unsigned int PASSWD2 = 0xbeefdead;
static const unsigned int FREEBYTE = 0x7f;

static unsigned int inited = 0;
static unsigned int mallocCount = 0;	/* Includes realloc and calloc calls */
static unsigned int freeCount = 0;	/* Includes realloc calls */
static unsigned int reallocCount = 0;
static unsigned int callocCount = 0;

static unsigned int markCount = 0;	/* mallocCount at time of mark */

typedef struct
{
  msg_header_t head;
  msg_type_t retcodetype;
  kern_return_t return_code;
  char buf[MSG_SIZE_MAX];
} message_t;

typedef struct _hashBucket
{
  char *addr;
  void *pc;			/* The requestor */
  void *pc2;			/* The requestor's caller */
  size_t size;			/* The requested size */
  size_t realsize;		/* The allocated size */
  int mark;
  struct _hashBucket *next;
} hashBucket;

typedef struct
{
  char *base;
  char *cur;
  char *last;
  int size;
} Arena;

static Arena space = {0};
static Arena hashList = {0};
static Arena hashTab = {0};
static Arena freeArena = {0};
static unsigned int malloc_debugging = 0;
static hashBucket **hashTable, **freeList;
static hashBucket *freeBuckets = NULL;
static vm_task_t mytask;

static void die (char *msg)
{
  int *crash = NULL;
  
  fprintf (stderr, msg);
  fflush (stderr);
  if (malloc_debugging)
    *crash = 232;
}


static void initarena (Arena *arena, unsigned size)
{
  arena->base = NULL;
  vm_allocate (mytask, (vm_address_t *) &arena->base, size, 1);
  arena->cur = arena->base;
  arena->last = arena->cur + size;
  arena->size = 0;
}

static void receive_message (msg_header_t *msg, void *userData)
{
  message_t reply;
  kern_return_t ret;
  boolean_t foundMsg;
  
  foundMsg = mallocProtocol_server (msg, &reply.head);
  if (foundMsg != TRUE)
    die ("mallocProtocol_server");
  else
    {
      reply.head.msg_local_port = msg->msg_local_port;
      reply.head.msg_remote_port = msg->msg_remote_port;
      ret = msg_send (&reply.head, MSG_OPTION_NONE, 0);
      if (ret != SEND_SUCCESS)
        {
	  mach_error ("", ret);
	  die ("msg_send");
	}
    }
}

static void initmallocthread (void)	/* May cause calls to malloc. */
{
  port_t server_port;
  kern_return_t ret;
  int pid = getpid ();
  char servername[100];
  boolean_t running;
  
  /* Make sure the name server is up! */
  ret = bootstrap_status (bootstrap_port, "NetMessage", &running);
  if (ret != KERN_SUCCESS || running == FALSE)
    return;
  
  ret = port_allocate (mytask, &server_port);
  if (ret != KERN_SUCCESS)
      die ("port_allocate");
  sprintf (servername, "MallocDebug-%d", pid);
  ret = netname_check_in (name_server_port, servername, PORT_NULL, server_port);
  if (ret != KERN_SUCCESS)
      die ("name_server_port");
  NXPortListen (server_port, &receive_message, NULL);
}

static void initmalloc (void)
{
  HASHSIZE = vm_page_size - 1;
  FREESIZE = vm_page_size / 4;
  MALLOCSIZE = vm_page_size;
  MAX_PASSWD_SIZE = vm_page_size - 8;

  mytask = task_self ();
  initarena (&space, MALLOCSIZE);
  initarena (&hashList, MALLOCSIZE);
  initarena (&hashTab, (HASHSIZE + 1) * sizeof (hashBucket *));
  hashTable = (hashBucket **) hashTab.base;
  bzero (hashTable, (HASHSIZE + 1) * sizeof (hashBucket *));
  initarena (&freeArena, (FREESIZE) * sizeof (hashBucket *));
  freeList = (hashBucket **) freeArena.base;
  bzero (freeList, (FREESIZE) * sizeof (hashBucket *));
  inited = 1;
}

static char *arena_alloc (Arena *arena, unsigned size)
{
  char *new = NULL;
  
  if (arena->cur + size > arena->last)
    initarena (arena, arena->last - arena->base);
  new = arena->cur;
  arena->cur += size;
  arena->size += size;
  return new;
}

static int extra (int size)
{
  int mod = size % 4;
  
  return mod ? 4 - mod: 0;
}

static int realsize (int size)
{
  return size + extra (size);
}

static int passwdSize (int size)
{
  return realsize (size) + 8;
}

static void setPasswd (char *ptr, unsigned size)
{
  unsigned newsize = passwdSize (size);
  
  ptr -= 4;
  *((int *)ptr) = PASSWD1;
  *((int *) (ptr + newsize - 4)) = PASSWD2;
}

static char *debug_arena_alloc (Arena *arena, unsigned size)
{
  char *new = NULL;
  unsigned newsize = passwdSize (size);
  
  if (arena->cur + newsize > arena->last)
    initarena (arena, arena->last - arena->base);
  new = arena->cur;
  arena->cur += newsize;
  arena->size += newsize;
  setPasswd (new + 4, size);
  return new + 4 + extra (size);
}

static void log (char *new, unsigned size, void *pc, void *pc2)
{
  hashBucket *bucket;
  int index;
  
  if (freeBuckets)
    {
      bucket = freeBuckets;
      freeBuckets = bucket->next;
    }
  else 
    bucket = (hashBucket *)arena_alloc (&hashList, sizeof (hashBucket));
  bucket->addr = new;
  bucket->size = size;
  bucket->realsize = realsize (size);
  bucket->mark = mallocCount;
  bucket->pc = pc;
  bucket->pc2 = pc2;
  index = ((unsigned)new) % HASHSIZE;
  bucket->next = hashTable[index];
  hashTable[index] = bucket;
}

static void checknode (char *ptr, unsigned size)
{
  char *new;
  int newsize = passwdSize (size);
  
  new = ptr - 4 - extra (size);
  if (*((int *) new) != PASSWD1)
    die ("node underflow");
  if (*((int *) (new + newsize - 4)) != PASSWD2)
    die ("node overflow");
}

static enum status checkBucket (hashBucket *bucket)
{
  char *new;
  int newsize = passwdSize (bucket->size);
  
  if (bucket->realsize >= MAX_PASSWD_SIZE)
    return status_ok;
  
  new = bucket->addr - 4 - extra (bucket->size);
  if (*((int *) new) != PASSWD1)
    return status_bad_left;
  if (*((int *) (new + newsize - 4)) != PASSWD2)
    return status_bad_right;
  return status_ok;
}

#if 0
static void check_malloc (void)
{
  int i;
  hashBucket *bucket;
  
  for (i = 0; i < HASHSIZE; i++)
    for (bucket = hashTable[i]; bucket; bucket = bucket->next)
      if (bucket->size > 0 && bucket->realsize < MAX_PASSWD_SIZE)
	  checknode (bucket->addr, bucket->size);
}
#endif

static hashBucket *findBucket (char *ptr)
{
  int index;
  hashBucket *bucket;
  
  index = ((unsigned)ptr) % HASHSIZE;
  for (bucket = hashTable[index]; bucket; bucket = bucket->next)
    {
      if (bucket->addr == ptr)
        {
	  if (bucket->size < 0)
	    {
	      die ("already freed");
	      return NULL;
	    }
	  return bucket;
	}
    }
  return NULL;
}

static void checkFreeBytes (hashBucket *bucket)
{
  char *addr = bucket->addr;
  int realsize = bucket->realsize;
  char *last = addr + realsize;
  
  for (; addr < last; addr++)
    if (*addr != FREEBYTE)
      die ("free node trashed");
}

static hashBucket *checkFreeList (size_t realsize)
{
  int index = realsize/4;
  int last = index + 10;
  int i;

  if (last > (FREESIZE-1))
    last = FREESIZE - 1;
  for (i = index; i <= last; i++)
    {
      if (freeList[index])
	{
	  hashBucket *bucket = freeList[index];
	  
	  freeList[index] = bucket->next;
	  if (malloc_debugging) 
	    checkFreeBytes (bucket);
	  return bucket;
	}
    }
  return NULL;
}

static void getpc (void **pc, void **pc2)
{
  unsigned int *fp, *parentfp, *grandparentfp;
  
  asm ("movl a6, %0":"=a" (fp));
  parentfp = (unsigned int *) (fp[0]);
  grandparentfp = (unsigned int *) (parentfp[0]);
  *pc = (void *) (parentfp[1]);
  *pc2 = (void *) (grandparentfp[1]);
}

void *malloc (size_t bytes)
{
  char *new;
  int logged = 0;
  void *pc, *pc2;
  
  if (!inited) 
      initmalloc ();
  
  if (bytes == 0)
    return NULL;
  getpc (&pc, &pc2);
  if (bytes < MAX_PASSWD_SIZE)
    {
      hashBucket *bucket = checkFreeList (realsize (bytes));
      
      if (bucket)
        {
	  int index;
	  
	  bucket->addr += extra (bytes);
	  bucket->pc = pc;
	  bucket->pc2 = pc2;
	  bucket->size = bytes;
	  bucket->mark = mallocCount;
	  index = ((unsigned)bucket->addr) % HASHSIZE;
	  bucket->next = hashTable[index];
	  hashTable[index] = bucket;
	  new = bucket->addr;
	  logged = 1;
        }
      else 
	new = debug_arena_alloc (&space, bytes);
    }
  else
    vm_allocate (task_self (), (vm_address_t *) &new, bytes, 1);
  if (!logged)
    log (new, bytes, pc, pc2);
  mallocCount++;
  if (mallocCount == 3)		/* cthread_init calls malloc () twice. */
    initmallocthread ();	/* May cause calls to malloc (). */
  return new;
}

static void addToFreeList (hashBucket *bucket)
{
  int index = bucket->realsize / 4;
  
  bucket->next = freeList[index];
  freeList[index] = bucket;
  bucket->addr -= extra (bucket->size);
  if (malloc_debugging)
    memset (bucket->addr, FREEBYTE, bucket->realsize);
}

static void removeFromHash (hashBucket *bucket)
{
  int index = ((unsigned)bucket->addr) % HASHSIZE;
  hashBucket **item;
  
  for (item = & (hashTable[index]); *item; item = & ((*item)->next))
    {
      if (*item == bucket)
	{
	  *item = (*item)->next;
	  break;
	}
    }
}

void free (void *ptr)
{
  hashBucket *bucket;
  
  if (!ptr)
    return;
  if (!inited) 
    initmalloc ();
  bucket = findBucket (ptr);
  if (!bucket)
    {
      die ("freed unmalloced pointer");
      return;
    }
  removeFromHash (bucket);
  if (bucket->realsize < vm_page_size)
    {
      if (bucket->realsize < MAX_PASSWD_SIZE)
	checknode (bucket->addr, bucket->size);
      addToFreeList (bucket);
    }
  else
    {
      vm_deallocate (task_self (), (vm_address_t) bucket->addr, bucket->realsize);
      bucket->next = freeBuckets;
      freeBuckets = bucket;
    }
  freeCount++;
}

void *realloc (void *ptr, size_t bytes)
{
  hashBucket *bucket = NULL;
  char *new = NULL;
  void *pc, *pc2;
  unsigned newsize;
  
  getpc (&pc, &pc2);
  if (ptr)
    {
      bucket = findBucket (ptr);
      if (!bucket)
        {
	  die ("realloc unmalloced pointer");
	  return NULL;
	}
      new = malloc (bytes);
      newsize = (bytes < bucket->size) ? bytes : bucket->size;
      bcopy (bucket->addr, new, newsize);
      free (ptr);
    }
  else 
    new = malloc (bytes);
  bucket = findBucket (new);
  bucket->pc = pc;
  bucket->pc2 = pc2;
  reallocCount++;
  return new;
}

void *calloc (size_t numElems, size_t byteSize)
{
  void *new = malloc (numElems * byteSize);
  void *pc, *pc2;
  hashBucket *bucket = NULL;
  
  getpc (&pc, &pc2);
  bzero (new, (numElems * byteSize));
  bucket = findBucket (new);
  bucket->pc = pc;
  bucket->pc2 = pc2;
  callocCount++;
  return new;
}

size_t malloc_size (void *ptr)
{
  hashBucket *bucket = findBucket (ptr);
  
  if (!bucket)
    {
      die ("cant find size");
      return 0;
    }
  return bucket->size;
}

size_t malloc_good_size (size_t byteSize)
{
  return byteSize;
}

size_t mstats (void)
{
  return space.cur - space.base;
}


void *valloc (size_t byteSize)
{
  int mod = byteSize % vm_page_size;
  void *pc, *pc2;
  void *new;
  hashBucket *bucket = NULL;
  
  getpc (&pc, &pc2);
  if (mod)
    byteSize += vm_page_size - mod;
  new = malloc (byteSize);
  bucket = findBucket (new);
  bucket->pc = pc;
  bucket->pc2 = pc2;
  return new;
}

int malloc_debug (int level)
{
  return 0;
}


void (*malloc_error (void (*func) (int))) (int)
{
  return NULL;
}

static int doit (hashBucket *bucket, NodeType nodeType)
{
  int result = 0;
  
  switch (nodeType)
    {
    case marked_nodes:
      if (bucket->mark >= markCount)
	result = 1;
      break;
    case all_nodes:
      result = 1;
      break;
    case smashed_nodes:
      result = (checkBucket (bucket) != status_ok);
      break;
    }
  return result;
}

static int countNodes (NodeType nodeType)
{
  int count = 0;
  hashBucket **this, **last;
  
  this = hashTable;
  last = this + HASHSIZE;
  for (; this < last; this++)
    {
      hashBucket *bucket;
      
      for (bucket = *this; bucket; bucket = bucket->next)
	if (doit (bucket, nodeType)) 
	  count++;
    }
  return count;
}


static void copyNodes (MallocData *mallocData, NodeType nodeType)
{
  hashBucket **this, **last;
  
  this = hashTable;
  last = this + HASHSIZE;
  for (; this < last; this++)
    {
      hashBucket *bucket;
      
      for (bucket = *this; bucket; bucket = bucket->next)
        {
	  if (doit (bucket, nodeType))
	    {
	      mallocData->addr = bucket->addr;
	      mallocData->pc = bucket->pc;
	      mallocData->pc2 = bucket->pc2;
	      mallocData->size = bucket->size;
	      mallocData->mark = bucket->mark;
	      mallocData->status = checkBucket (bucket);
	      mallocData++;
	    }  
	}
    }
}

static MallocData *allocMallocData (unsigned nodeSize)
{
  static int globalMallocData = 0;
  static int globalMallocSize = 0;
  
  if (globalMallocSize && (globalMallocSize < nodeSize))
    {
      vm_deallocate (task_self (), globalMallocData, globalMallocSize);
      globalMallocData = 0;
      globalMallocSize = 0;
    }
  if (!globalMallocSize)
    {
      globalMallocSize = nodeSize + vm_page_size;
      vm_allocate (task_self (), (vm_address_t *) &globalMallocData, globalMallocSize, 1);
    }
  return (MallocData *) globalMallocData;
}


retval_t getMallocData (port_t server, NodeType nodeType, data_t *nodeInfo,
    unsigned int *nodeInfoCnt, int *numNodes)
{
  int nNodes = countNodes (nodeType);
  unsigned nodeSize = nNodes * sizeof (MallocData);
  int mod = nodeSize % vm_page_size;
  MallocData *mallocData;
  
  if (mod)
    nodeSize += vm_page_size - mod;
  mallocData = allocMallocData (nodeSize);
  copyNodes (mallocData, nodeType);
  *nodeInfo = (data_t) mallocData;
  *nodeInfoCnt = nodeSize;
  *numNodes = nNodes;
  return 0;
}

retval_t resetMalloc (port_t server)
{
  markCount = mallocCount;
  return 0;
}

retval_t setMallocMarkNumber (port_t server, int nodeNumber)
{
  markCount = nodeNumber;
  return 0;
}

retval_t getCurrentMallocNodeNumber (port_t server, int *nodeNumber)
{
  *nodeNumber = markCount;
  return 0;
}
