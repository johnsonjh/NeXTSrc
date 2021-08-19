/*
 *	automountmessage.h
 *	Copyright 1988, 1990 NeXT, Inc.
 */

#import <sys/param.h>
#import <sys/types.h>
#import <sys/port.h>
#import <sys/message.h>
#import <sys/dir.h>
#import	<nextdev/disk.h>
#import <nextdev/insertmsg.h>

/*
 *  Format of messages between autodiskmount and Workspace Manager.
 */
#define AUTODISKMOUNT_WORKSPACE	"autodiskmount_workspace"
#define	WORKSPACE_AUTODISKMOUNT	"workspace_autodiskmount"

/* header.msg_id types */
#define	AM_INSERT	0x00025582
#define	AM_MOUNTED	0x00025583
#define	WM_INIT		0x00025584
#define	WM_UNMOUNT	0x00025585
#define	WM_EJECT	0x00025586
#define	WM_GETLABEL	0x00025587
#define	WM_SETLABEL	0x00025588
#define	WM_MOUNT	0x00025589
#define	WM_NOTIFY	0x0002558a
#define	WM_NEWMOUNT	0x0002558b

#define	MSG_REPLY	0x80000000

typedef struct am_message {
	msg_header_t	header;
	msg_type_t	dev_type;		/* required in all msgs */
	u_int		dev;
	msg_type_t	port_type;		/* AM_INSERT & FOREIGN_DEV */
	port_t		port;
	msg_type_t	flags_type;
	u_int		flags;
#define	REQUIRES_INIT	0x00000001		/* AM_INSERT */
#define	REQUIRES_FSCK	0x00000002		/* AM_INSERT */
#define	FOREIGN_DEV	0x00000004		/* AM_INSERT */
#define	DO_INIT		0x00000008		/* AM_INSERT REPLY */
#define	DO_EJECT	0x00000010		/* AM_INSERT REPLY */
#define	DO_NOT_MOUNT	0x00000020		/* AM_INSERT REPLY */
#define	NO_FORMAT	0x00000040		/* AM_INSERT */
#define REMOVABLE	0x00000080		/* AM_INSERT */
#define READ_ONLY	0x00000100		/* AM_INSERT */
	msg_type_t	uid_type;		/* AM_INSERT REPLY, WM_INIT */
	int		uid;
	msg_type_t	errno_type;
	int		errno;
	msg_type_t	label_type;		/* AM_INSERT (REPLY) */
	char		label[MAXLBLLEN];	/* WM_SETLABEL, WM_GETLABEL REPLY */
	msg_type_t	mntpt_type;		/* AM_INSERT REPLY, WM_INIT */
	char		mntpt[MAXPATHLEN];
#define	MAXARGS		64
	msg_type_t	args_type;		/* AM_INSERT (REPLY), WM_INIT */
	char		args[MAXARGS];
	msg_type_t	dev_name_type;		/* required in all msgs */
	char		dev_name[OID_DEVSTR_LEN];
} am_message_t;

/* error codes */
#define	E_START		1000
#define	E_INITFAIL	1000
#define	E_MNTPT		1001
#define	E_FSCK		1002
#define	E_BADFS		1003
#define	E_MSG		1004
#define	E_LABEL		1005
#define	E_NOVOL		1006
#define	E_2DRIVE	1007
#define	E_2INIT		1008
#define	E_END		1009

