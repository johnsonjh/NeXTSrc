/*
 * SUP Communication Module for 4.2 BSD
 *
 * SUP COMMUNICATION MODULE SPECIFICATIONS:
 *
 * IN THIS MODULE:
 *
 * DATA ENCRYPTION
 *	netcrypt (key)			turn on/off encryption of strings and files
 *	  char *key;			  encryption key
 *
 * OUTPUT TO NETWORK
 *
 *   MESSAGE START/END
 *	writemsg (msg)			start message
 *	  int msg;			  message type
 *	writemend ()			end message and flush data to network
 *
 *   MESSAGE DATA
 *	writeint (i)			write int
 *	  int i;			  integer to write
 *	writestring (p)			write string
 *	  char *p;			  string pointer
 *	writefile (f)			write open file
 *	  int f;			  open file descriptor
 *
 *   COMPLETE MESSAGE (start, one data block, end)
 *	writemnull (msg)		write message with no data
 *	  int msg;			  message type
 *	writemint (msg,i)		write int message
 *	  int msg;			  message type
 *	  int i;			  integer to write
 *	writemstr (msg,p)		write string message
 *	  int msg;			  message type
 *	  char *p;			  string pointer
 *
 * INPUT FROM NETWORK
 *   MESSAGE START/END
 *	readflush ()			flush any unread data (close)
 *	readmsg (msg)			read specified message type
 *	  int msg;			  message type
 *	readmend ()			read message end
 *
 *   MESSAGE DATA
 *	readskip ()			skip over one input data block
 *	readint (i)			read int
 *	  int *i;			  pointer to integer
 *	readstring (p)			read string
 *	  char **p;			  pointer to string pointer
 *	readfile (f)			read into open file
 *	  int f;			  open file descriptor
 *
 *   COMPLETE MESSAGE (start, one data block, end)
 *	readmnull (msg)			read message with no data
 *	  int msg;			  message type
 *	readmint (msg,i)		read int message
 *	  int msg;			  message type
 *	  int *i;			  pointer to integer
 *	readmstr (msg,p)		read string message
 *	  int msg;			  message type
 *	  char **p;			  pointer to string pointer
 *
 * RETURN CODES
 *	All routines normally return SCMOK.  SCMERR may be returned
 *	by any routine on abnormal (usually fatal) errors.  An
 *	unexpected MSGGOAWAY will result in SCMEOF being returned.
 *
 * COMMUNICATION PROTOCOL
 *	Messages always alternate, with the first message after
 *	connecting being sent by the client process.
 *
 *	At the end of the conversation, the client process will
 *	send a message to the server telling it to go away.  Then,
 *	both processes will close the network connection.
 *
 *	Any time a process receives a message it does not expect,
 *	the "readmsg" routine will send a MSGGOAWAY message and
 *	return SCMEOF.
 *	
 *	Each message has this format:
 *	    ----------    ------------    ------------         ----------
 *	    |msg type|    |count|data|    |count|data|   ...   |ENDCOUNT|
 *	    ----------    ------------    ------------         ----------
 *	size:  int	    int  var.	    int  var.	   	  int
 *
 *	All ints are assumed to be 32-bit quantities.  A message
 *	with no data simply has a message type followed by ENDCOUNT.
 *
 **********************************************************************
 * HISTORY
 * 02-Oct-86  Rudy Nedved (ern) at Carnegie-Mellon University
 *	Put a timeout on reading from the network.
 *
 * 25-May-86  Jonathan J. Chew (jjc) at Carnegie-Mellon University
 *	Renamed "howmany" parameter to routines "encode" and "decode" from
 *	to "count" to avoid conflict with 4.3BSD macro.
 *
 * 15-Feb-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added readflush() to flush any unread data from the input
 *	buffer.  Called by requestend() in scm.c module.
 *
 * 19-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added register variables to decode() for speedup.  Added I/O
 *	buffering to reduce the number or read/write system calls.
 *	Removed readmfil/writemfil routines which were not used and were
 *	not compatable with the other similarly defined routines anyway.
 *
 * 19-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created from scm.c I/O and crypt routines.
 *
 **********************************************************************
 */

#include <libc.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include "sup.h"
#include "supmsg.h"

extern int errno;

/*************************
 ***    M A C R O S    ***
 *************************/

/* end of message */
#define ENDCOUNT (-1)			/* end of message marker */
#define NULLCOUNT (-2)			/* used for sending NULL pointer */

#define RETRIES 15			/* # of times to retry io */
#define FILEXFER 2048			/* block transfer size */
#define XFERSIZE(count) ((count > FILEXFER) ? FILEXFER : count)

/*********************************************
 ***    G L O B A L   V A R I A B L E S    ***
 *********************************************/

extern int scmerr ();			/* error printing routine */
extern int netfile;			/* network file descriptor */

int scmdebug;				/* scm debug flag */

static int cryptflag = 0;		/* whether to encrypt/decrypt data */
static char *cryptbuf;			/* buffer for data encryption/decryption */

extern char *goawayreason;		/* reason for goaway message */

struct buf {
	char b_data[FILEXFER];		/* buffered data */
	char *b_ptr;			/* pointer to end of buffer */
	int b_cnt;			/* number of bytes in buffer */
} buffers[2];
struct buf *bufptr;			/* buffer pointer */

/***********************************************
 ***    O U T P U T   T O   N E T W O R K    ***
 ***********************************************/

static
writedata (count,data)		/* write raw data to network */
int count;
char *data;
{
	register int x,tries;
	register struct buf *bp;

	if (bufptr) {
		if (bufptr->b_cnt + count <= FILEXFER) {
			bcopy (data,bufptr->b_ptr,count);
			bufptr->b_cnt += count;
			bufptr->b_ptr += count;
			return (SCMOK);
		}
		bp = (bufptr == buffers) ? &buffers[1] : buffers;
		bcopy (data,bp->b_data,count);
		bp->b_cnt = count;
		bp->b_ptr = bp->b_data + count;
		data = bufptr->b_data;
		count = bufptr->b_cnt;
		bufptr->b_cnt = 0;
		bufptr->b_ptr = bufptr->b_data;
		bufptr = bp;
	}
	tries = 0;
	for (;;) {
		errno = 0;
		x = write (netfile,data,count);
		if (x > 0)  break;
		if (errno)  break;
		if (++tries > RETRIES)  break;
		if (scmdebug > 0) {
			fflush (stdout);
			fprintf (stderr,"SCM Retrying failed network write\n");
			fflush (stderr);
		}
	}
	if (x <= 0) {
		if (errno == EPIPE)
			return (scmerr (-1,"Network write timed out"));
		if (errno)
			return (scmerr (errno,"Write error on network"));
		return (scmerr (-1,"Write retries failed"));
	}
	if (x != count)
		return (scmerr (-1,"Write error on network returned %d on write of %d",x,count));
	return (SCMOK);
}

static
writeblock (count,data)		/* write data block */
int count;
char *data;
{
	register int x;
	int y = byteswap(count);

	x = writedata (sizeof(int),(char *)&y);
	if (x == SCMOK)  x = writedata (count,data);
	return (x);
}

writemsg (msg)		/* write start of message */
int msg;
{
	int x;

	if (scmdebug > 1) {
		fflush (stdout);
		fprintf (stderr,"SCM Writing message %d\n",msg);
		fflush (stderr);
	}
	if (bufptr)
		return (scmerr (-1,"Buffering already enabled"));
	bufptr = buffers;
	bufptr->b_ptr = bufptr->b_data;
	bufptr->b_cnt = 0;
	x = byteswap (msg);
	return (writedata(sizeof(int),(char *)&x));
}

writemend ()		/* write end of message */
{
	register int count;
	register char *data;
	int x;

	x = byteswap (ENDCOUNT);
	x = writedata (sizeof(int),(char *)&x);
	if (x != SCMOK)  return (x);
	if (bufptr == NULL)
		return (scmerr (-1,"Buffering already disabled"));
	if (bufptr->b_cnt == 0) {
		bufptr = NULL;
		return (SCMOK);
	}
	data = bufptr->b_data;
	count = bufptr->b_cnt;
	bufptr = NULL;
	return (writedata (count, data));
}

writeint (i)		/* write int as data block */
int i;
{
	int x;
	if (scmdebug > 2) {
		fflush (stdout);
		fprintf (stderr,"SCM Writing integer %d\n",i);
		fflush (stderr);
	}
	x = byteswap(i);
	return (writeblock(sizeof(int),(char *)&x));
}

writestring (p)		/* write string as data block */
char *p;
{
	register int len,x;
	if (p == NULL) {
		int y = byteswap(NULLCOUNT);
		if (scmdebug > 2) {
			fflush (stdout);
			fprintf (stderr,"SCM Writing string NULL\n");
			fflush (stderr);
		}
		return (writedata (sizeof(int),(char *)&y));
	}
	if (scmdebug > 2) {
		fflush (stdout);
		fprintf (stderr,"SCM Writing string %s\n",p);
		fflush (stderr);
	}
	len = strlen (p);
	if (cryptflag) {
		x = getcryptbuf (len+1);
		if (x != SCMOK)
			return (x);
		encode (p,cryptbuf,len);
		p = cryptbuf;
	}
	return (writeblock(len,p));
}

writefile (f)		/* write open file as a data block */
int f;
{
	char buf[FILEXFER];
	register int number,sum,filesize,x;
	int y;
	struct stat statbuf;

	if (fstat(f,&statbuf) < 0)
		return (scmerr (errno,"Can't access open file for message"));
	filesize = statbuf.st_size;
	y = byteswap(filesize);
	x = writedata (sizeof(int),(char *)&y);

	if (cryptflag)  x = getcryptbuf (FILEXFER);

	if (x == SCMOK) {
		sum = 0;
		do {
			number = read (f,buf,FILEXFER);
			if (number > 0) {
				if (cryptflag) {
					encode (buf,cryptbuf,number);
					x = writedata (number,cryptbuf);
				}
				else {
					x = writedata (number,buf);
				}
				sum += number;
			}
		} while (x == SCMOK && number > 0);
	}
	if (sum != filesize)
		return (scmerr (-1,"File size error on output message"));
	if (number < 0)
		return (scmerr (errno,"Read error on file output message"));
	return (x);
}

writemnull (msg)	/* write message with no data */
int msg;
{
	register int x;
	x = writemsg (msg);
	if (x == SCMOK)  x = writemend ();
	return (x);
}

writemint (msg,i)		/* write message of one int */
int msg,i;
{
	register int x;
	x = writemsg (msg);
	if (x == SCMOK)  x = writeint (i);
	if (x == SCMOK)  x = writemend ();
	return (x);
}

writemstr (msg,p)		/* write message of one string */
int msg;
char *p;
{
	register int x;
	x = writemsg (msg);
	if (x == SCMOK)  x = writestring (p);
	if (x == SCMOK)  x = writemend ();
	return (x);
}

/*************************************************
 ***    I N P U T   F R O M   N E T W O R K    ***
 *************************************************/

static
readdata (count,data)		/* read raw data from network */
int count;
char *data;
{
	register char *p;
	register int n,m,x;
	int tries;
	static int bufcnt = 0;
	static char *bufptr;
	static char buffer[FILEXFER];
	static int imask;
	static struct timeval timout;

	if (count == 0 && data == NULL) {
		bufcnt = 0;
		return (SCMOK);
	}
	if (count <= bufcnt) {
		bcopy (bufptr,data,count);
		bufptr += count;
		bufcnt -= count;
		return (SCMOK);
	}
	if (bufcnt > 0) {
		bcopy (bufptr,data,bufcnt);
		data += bufcnt;
		count -= bufcnt;
	}
	bufptr = buffer;
	bufcnt = 0;
	timout.tv_usec = 0;
	timout.tv_sec = 2*60*60;
	p = buffer;
	n = FILEXFER;
	m = count;
	while (m > 0) {
		tries = 0;
		for (;;) {
			imask = 1 << netfile;
			if (select(32,&imask,0,0,&timout) < 0)
				imask = 1;
			errno = 0;
			if (imask)
				x = read (netfile,p,n);
			else
				return (scmerr (-1,"Timeout on network input"));
			if (x > 0)  break;
			if (x == 0)
				return (scmerr (-1,"Premature EOF on network input"));
			if (errno)  break;
			if (++tries > RETRIES)  break;
			if (scmdebug > 0) {
				fflush (stdout);
				fprintf (stderr,"SCM Retrying failed network read\n");
				fflush (stderr);
			}
		}
		if (x < 0) {
			if (errno)
				return (scmerr (errno,"Read error on network"));
			return (scmerr (-1,"Read retries failed"));
		}
		p += x;
		n -= x;
		m -= x;
		bufcnt += x;
	}
	bcopy (bufptr,data,count);
	bufptr += count;
	bufcnt -= count;
	return (SCMOK);
}

static
int readcount (count)			/* read count of data block */
int *count;
{
	register int x;
	int y;
	x = readdata (sizeof(int),(char *)&y);
	if (x != SCMOK)  return (x);
	*count = byteswap(y);
	return (SCMOK);
}

readflush ()
{
	return (readdata (0, NULL));
}

readmsg (msg)		/* read header for expected message */
int msg;		/* if message is unexpected, send back SCMHUH */
{
	register int x;
	int m;
	if (scmdebug > 1) {
		fflush (stdout);
		fprintf (stderr,"SCM Reading message %d\n",msg);
		fflush (stderr);
	}
	x = readdata (sizeof(int),(char *)&m);	/* msg type */
	if (x != SCMOK)  return (x);
	m = byteswap(m);
	if (m == msg)  return (x);

	/* check for MSGGOAWAY in case he noticed problems first */
	if (m != MSGGOAWAY)
		return (scmerr (-1,"Received unexpected message %d",m));
	(void) readstring (&goawayreason);
	(void) readmend ();
	if (goawayreason == NULL)
		return (SCMEOF);
	fflush (stdout);
	fprintf (stderr,"SCM GOAWAY %s\n",goawayreason);
	fflush (stderr);
	return (SCMEOF);
}

readmend ()
{
	register int x;
	int y;
	x = readdata (sizeof(int),(char *)&y);
	y = byteswap(y);
	if (x == SCMOK && y != ENDCOUNT)
		return (scmerr (-1,"Error reading end of message"));
	return (x);
}

readskip ()			/* skip over one input block */
{
	register int x;
	int n;
	char buf[FILEXFER];
	x = readcount (&n);
	if (x != SCMOK)  return (x);
	if (n < 0)
		return (scmerr (-1,"Invalid message count %d",n));
	while (x == SCMOK && n > 0) {
		x = readdata (XFERSIZE(n),buf);
		n -= XFERSIZE(n);
	}
	return (x);
}

int readint (buf)		/* read int data block */
int *buf;
{
	register int x;
	int y;
	x = readcount (&y);
	if (x != SCMOK)  return (x);
	if (y < 0)
		return (scmerr (-1,"Invalid message count %d",y));
	if (y != sizeof(int))
		return (scmerr (-1,"Size error for int message is %d",y));
	x = readdata (sizeof(int),(char *)&y);
	(*buf) = byteswap(y);
	if (scmdebug > 2) {
		fflush (stdout);
		fprintf (stderr,"SCM Reading integer %d\n",x);
		fflush (stdout);
	}
	return (x);
}

int readstring (buf)	/* read string data block */
register char **buf;
{
	register int x;
	int count;
	register char *p;
	extern int protver;	/* version 3 */

	x = readcount (&count);
	if (x != SCMOK)  return (x);
	if (count == NULLCOUNT) {
		if (scmdebug > 2) {
			fflush (stdout);
			fprintf (stderr,"SCM Reading string NULL\n");
			fflush (stderr);
		}
		*buf = NULL;
		return (SCMOK);
	}
	if (protver < 4 && count == ENDCOUNT) {
		*buf = (char *)ENDCOUNT;
		return (SCMOK);
	}
	if (count < 0)
		return (scmerr (-1,"Invalid message count %d",count));
	if ((p = malloc (count+1)) == NULL)
		return (scmerr (-1,"Can't malloc %d bytes for string",count));
	if (cryptflag) {
		x = getcryptbuf (count+1);
		if (x == SCMOK)  x = readdata (count,cryptbuf);
		if (x != SCMOK)  return (x);
		decode (cryptbuf,p,count);
	}
	else {
		x = readdata (count,p);
		if (x != SCMOK)  return (x);
	}
	p[count] = 0;		/* NULL at end of string */
	*buf = p;
	if (scmdebug > 2) {
		fflush (stdout);
		fprintf (stderr,"SCM Reading string %s\n",*buf);
		fflush (stderr);
	}
	return (SCMOK);
}

readfile (f)		/* read data block into open file */
int f;
{
	register int x;
	int count;
	char buf[FILEXFER];

	if (cryptflag) {
		x = getcryptbuf (FILEXFER);
		if (x != SCMOK)  return (x);
	}
	x = readcount (&count);
	if (x != SCMOK)  return (x);
	if (count < 0)
		return (scmerr (-1,"Invalid message count %d",count));
	while (x == SCMOK && count > 0) {
		if (cryptflag) {
			x = readdata (XFERSIZE(count),cryptbuf);
			if (x == SCMOK)  decode (cryptbuf,buf,XFERSIZE(count));
		}
		else
			x = readdata (XFERSIZE(count),buf);
		if (x == SCMOK) {
			write (f,buf,XFERSIZE(count));
			count -= XFERSIZE(count);
		}
	}
	return (x);
}

readmnull (msg)		/* read null message */
int msg;
{
	register int x;
	x = readmsg (msg);
	if (x == SCMOK)  x = readmend ();
	return (x);
}

readmint (msg,buf)		/* read int message */
int msg,*buf;
{
	register int x;
	x = readmsg (msg);
	if (x == SCMOK)  x = readint (buf);
	if (x == SCMOK)  x = readmend ();
	return (x);
}

int readmstr (msg,buf)		/* read string message */
int msg;
char **buf;
{
	register int x;
	x = readmsg (msg);
	if (x == SCMOK)  x = readstring (buf);
	if (x == SCMOK)  x = readmend ();
	return (x);
}

/*******************************************
 ***    D A T A   E N C R Y P T I O N    ***
 *******************************************/

/*
 * Subroutine version of "crypt" program
 */

#define ROTORSZ 256
#define MASK 0377
static char	t1[ROTORSZ];
static char	t2[ROTORSZ];
static char	t3[ROTORSZ];

netcrypt (pword)
char *pword;
{
	int ic, i, k, temp;
	unsigned random;
	char buf[13];
	long seed;

	if (pword == NULL) {
		cryptflag = 0;
		(void) getcryptbuf (0);
		return (SCMOK);
	}
	cryptflag = 1;
	for (i=0; i<ROTORSZ; i++)  t1[i] = t2[i] = t3[i] = 0;
	strncpy(buf, pword, 8);
	strncpy(buf, crypt(buf, buf), 13);
	seed = 123;
	for (i=0; i<13; i++)
		seed = seed*buf[i] + i;
	for(i=0;i<ROTORSZ;i++)
		t1[i] = i;
	for(i=0;i<ROTORSZ;i++) {
		seed = 5*seed + buf[i%13];
		random = seed % 65521;
		k = ROTORSZ-1 - i;
		ic = (random&MASK)%(k+1);
		random >>= 8;
		temp = t1[k];
		t1[k] = t1[ic];
		t1[ic] = temp;
		if(t3[k]!=0) continue;
		ic = (random&MASK) % k;
		while(t3[ic]!=0) ic = (ic+1) % k;
		t3[k] = ic;
		t3[ic] = k;
	}
	for(i=0;i<ROTORSZ;i++)
		t2[t1[i]&MASK] = i;
	return (SCMOK);
}

static getcryptbuf (x)
int x;
{
	static int cryptsize = 0;	/* size of current cryptbuf */

	if (cryptflag == 0) {
		if (cryptsize > 0)  free (cryptbuf);
		cryptsize = 0;
	} else if (x > cryptsize) {
		if (cryptsize > 0)  free (cryptbuf);
		cryptbuf = malloc (x+1);
		if (cryptbuf == NULL)
			return (scmerr (-1,"Can't allocate encryption buffer"));
		cryptsize = x;
	}
	return (SCMOK);
}

static encode (in,out,count)
char *in,*out;
int count;
{
	decode (in,out,count);
}

static decode (in,out,count)
char *in,*out;
register int count;
{
	register char *t1p,*t2p,*t3p;
	register int n1,n2;

	t1p = t1; t2p = t2; t3p = t3;
	n1 = n2 = 0;
	while (count-- > 0) {
		*out++ = t2p[(t3p[(t1p[((*in++)+n1)&MASK]+n2)&MASK]-n2)&MASK]-n1;
		if (((++n1)&MASK) == 0) {
			n1 = 0;
			if (((++n2)&MASK) == 0)
				n2 = 0;
		}
	}
}
