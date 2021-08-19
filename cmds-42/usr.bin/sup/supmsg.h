/*
 * supmsg.h - global definitions/variables used in msg routines.
 *
 **********************************************************************
 * HISTORY
 * 04-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed hostname to hostT to support multiple repositories per
 *	collection.  Added FSETUPBUSY to tell clients that server is
 *	currently busy.
 *
 * 19-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */

/* Special messages reserved for SCM */
#define MSGGOAWAY	(-1)		/* see scm.c */

#ifdef	MSGNAME

/* Messages used with Name Server */
#define MSGNSIGNON	(1)		/* see msgsignon.c */
#define MSGNSIGNONACK	(2)		/* ... */
#define MSGNCOLL	(3)		/* see msgname.c */
#define MSGNHOST	(4)		/* ... */

#endif	MSGNAME

#ifdef	MSGFILE

/* Messages used with File Server */
#define MSGFSIGNON	(101)		/* see msgsignon.c */
#define MSGFSIGNONACK	(102)		/* ... */
#define MSGFSETUP	(103)		/* see msgsetup.c */
#define MSGFSETUPACK	(104)		/* ... */
#define MSGFLOGIN	(105)		/* see msglogin.c */
#define MSGFLOGACK	(106)		/* ... */
#define MSGFCRYPT	(107)		/* see msgcrypt.c */
#define MSGFCRYPTOK	(108)		/* ... */
#define MSGFREFUSE	(109)		/* see msgrefuse.c */
#define MSGFLIST	(110)		/* see msglist.c */
#define MSGFNEED	(111)		/* see msgneed.c */
#define MSGFDENY	(112)		/* see msgdeny.c */
#define MSGFSEND	(113)		/* see msgxfer.c */
#define MSGFRECV	(114)		/* see msgxfer.c */

/* MSGFSETUPACK data codes */
#define FSETUPOK	(999)		/* setupack set in msgsetup.c */
#define FSETUPHOST	(998)
#define FSETUPSAME	(997)
#define FSETUPOLD	(996)
#define FSETUPBUSY	(995)

/* MSGFLOGACK data codes */
#define FLOGOK		(989)		/* loginack set in msglogin.c */
#define FLOGNG		(988)

#endif	MSGFILE

#ifdef	MSGSUBR

/* used in all msg routines */
extern int	server;			/* true if we are the server */
extern char	*errstr;		/* error string from msg routine */
extern char	*collname;		/* collection name */
extern int	protver;		/* protocol version of partner */

#else	MSGSUBR

#if defined (MSGFILE) || defined (MSGNAME)

/* used in all msg routines */
int	server;				/* true if we are the server */
char	*errstr;			/* error string from msg routine */
char	*collname;			/* collection name */

/* msggoaway */
char	*goawayreason;			/* reason for goaway */

/* msgsignon */
int	pgmversion;			/* version of this program */
int	protver;			/* protocol version of partner */
int	pgmver;				/* program version of partner */
char	*scmver;			/* scm version of partner */
int	fspid;				/* process id of fileserver */
int	nspid;				/* process id of nameserver */

#endif	MSGFILE || MSGNAME

#ifdef	MSGNAME

/* msgname */
TREE	*hostT;				/* host names of repositories */

#endif	MSGNAME

#ifdef	MSGFILE

/* msgsetup */
char	*basedir;			/* base directory */
int	basedev;			/* base directory device */
int	baseino;			/* base directory inode */
int	lasttime;			/* time of last upgrade */
int	listonly;			/* only listing files, no data xfer */
int	newonly;			/* only send new files */
int	setupack;			/* ack return value for setup */

/* msgcrypt */
char	*crypttest;			/* encryption test string */

/* msglogin */
char	*logcrypt;			/* login encryption test */
char	*loguser;			/* login username */
char	*logpswd;			/* password for login */
int	logack;				/* login ack status */
char	*logerr;			/* error string from oklogin */

/* msgrefuse */
TREE	*refuseT;			/* tree of files to refuse */

/* msglist */
TREE	*listT;				/* tree of files to list */
int	scantime;			/* time that collection was scanned */

/* msgneed */
TREE	*needT;				/* tree of files to need */

/* msgdeny */
TREE	*denyT;				/* tree of files to deny */

/* msgrecv */
/* msgsend */
TREE	*upgradeT;			/* pointer to file being upgraded */

#endif	MSGFILE

#endif	MSGSUBR
