/* sup.h -- declarations for sup, supnamesrv, supfilesrv
 *
 * VERSION NUMBER for any program is given by:  a.b (c)
 * where	a = PROTOVERSION	is the protocol version #
 *		b = PGMVERSION		is program # within protocol
 *		c = scmversion		is communication module version
 *			(i.e. operating system for which scm is configured)
 **********************************************************************
 * HISTORY
 * 19-Sep-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	Added FILESUPTDEFAULT definition.
 *
 * 07-Jun-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Removed FILESRVBUSYWAIT.  Now uses exponential backoff.
 *
 * 30-May-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added numeric port numbers to use when port names are not in the
 *	host table.
 *
 * 04-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Update protocol version to 5 for name server protocol change to
 *	allow multiple repositories per collection.  Added FILESRVBUSYWAIT
 *	of 5 minutes.  Added FILELOCK file to indicate collections that
 *	should be exclusively locked when upgraded.
 *
 * 22-Sep-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Merged 4.1 and 4.2 versions together.
 *
 * 04-Jun-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created for 4.2 BSD.
 *
 **********************************************************************
 */

/* PGMVERSION is defined separately in each program */
extern char scmversion[];		/* string version of scm */
#define PROTOVERSION 5			/* version of network protocol */
#define SCANVERSION  2			/* version of scan file format */

/* TCP servers for name server and file server */
#define NAMEPORT	"supnamesrv"
#define FILEPORT	"supfilesrv"
#define NAMEPORTNUM	869
#define FILEPORTNUM	871
#define DEBUGNPORT	"supnamedbg"
#define DEBUGFPORT	"supfiledbg"

/* Data files used in scan.c */
#define FILELIST	"sup/%s/list"
#define FILESCAN	"sup/%s/scan"
#define FILESCANTEMP	"sup/%s/scan.temp"

/* Data files used in sup.c */
#define FILEBASEDEFAULT	"/usr/%s" /* also supfilesrv and supscan */
#define FILESUPDEFAULT	"%s/lib/supfiles/coll.list"
#define FILESUPTDEFAULT	"%s/lib/supfiles/coll.what"
#define FILEHOSTS	"%s/lib/supfiles/name.host"
#define FILEBKDIR	"%s/BACKUP"
#define FILEBACKUP	"%s/BACKUP/%s"
#define FILELAST	"sup/%s/last"
#define FILELASTTEMP	"sup/%s/last.temp"
#define FILELOCK	"sup/%s/lock"	/* also supfilesrv */
#define FILEREFUSE	"sup/%s/refuse"
#define FILEWHEN	"sup/%s/when"

/* Data files used in supfilesrv.c */
#define FILEDIRS	"%s/lib/supservers/coll.dir" /* also supscan */
#define FILECRYPT	"sup/%s/crypt"
#define FILEFOREIGN	"sup/%s/host"
#define FILELOGFILE	"sup/%s/logfile"
#define FILEPREFIX	"sup/%s/prefix"	/* also supscan */

/* Data files used in supnamesrv.c */
#define FILENCOLLS	"%s/lib/supservers/coll.host" /* also supscan */

/* String length */
#define STRINGLENGTH	2000

/* Password transmission encryption key */
#define PSWDCRYPT	"SuperMan"
/* Test string for encryption */
#define CRYPTTEST	"Hello there, Sailor Boy!"

/* Default directory for system sup information */
#ifndef	DEFDIR
#define DEFDIR		"/usr/cs"
#endif	DEFDIR

/* Default login account for file server */
#ifndef	DEFUSER
#define DEFUSER		"anon"
#endif	DEFUSER

/* token for all local systems in hosts file */
#define LOCALSYSTEMS	"LOCAL"

/* subroutine return codes */
#define SCMOK		(1)		/* routine performed correctly */
#define SCMEOF		(0)		/* read EOF on network connection */
#define SCMERR		(-1)		/* error occurred during routine */

/* data structure for describing a file being upgraded */

struct treestruct {
/* fields for file information */
	char *Tname;			/* path component name */
	int Tflags;			/* flags of file */
	int Tmode;			/* st_mode of file */
	char *Tuser;			/* owner of file */
	int Tuid;			/* owner id of file */
	char *Tgroup;			/* group of file */
	int Tgid;			/* group id of file */
	int Tctime;			/* inode modification time */
	int Tmtime;			/* data modification time */
	struct treestruct *Tlink;	/* tree of link names */
	struct treestruct *Texec;	/* tree of execute commands */
/* fields for sibling AVL tree */
	int Tbf;			/* balance factor */
	struct treestruct *Tlo,*Thi;	/* ordered sibling tree */
};
typedef struct treestruct TREE;

/* bitfield not defined in stat.h */
#define S_IMODE		  07777		/* part of st_mode that chmod sets */

/* flag bits for files */
#define FNEW		     01		/* ctime of file has changed */
#define FBACKUP		     02		/* backup of file is allowed */
#define FNOACCT		     04		/* don't set file information */
#define FUPDATE		    010		/* only set file information */
#define FNEEDED		0100000		/* file needed for upgrade */

/* version 3 compatability */
#define	FCOMPAT		0010000		/* Added to detect execute commands to send */

/* message types now obsolete */
#define MSGFEXECQ	(115)
#define MSGFEXECNAMES	(116)

/* flag bits for files in list of all files */
#define ALLNEW		      01
#define ALLBACKUP	      02
#define ALLEND		      04
#define ALLDIR		     010
#define ALLNOACCT	     020
#define ALLSLINK	    0100

/* flag bits for file mode word */
#define MODELINK	  010000
#define MODEDIR		  040000
#define MODESYM		 0100000
#define MODENOACCT	 0200000
#define MODEUPDATE	01000000

/* blocking factor for filenames in list of all file names */
#define BLOCKALL	32

/* end version 3 compatability */

#define MAXCHILDREN 8			/* maximum number of children allowed
					   to sup at the same time */

/* scm and stree external declarations */
char *remotehost();
TREE *Tinsert(),*Tsearch(),*Tlookup();
