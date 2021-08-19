#include <sys/types.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <nextdev/odvar.h>	/* XXX */
#include <stdio.h>
#include <mntent.h>
#include <pwd.h>
#include "automountmessage.h"
#import <mach.h>
#import <sys/message.h>
#import <ufs/fs.h>
#import <servers/netname.h>
#import <nextdev/disk.h>
#import	<nextdev/video.h>
#import	<mon/nvram.h>

extern	int errno;
char *err_list[] = {
	"disk initialization failed",
	"mount point must not already exist",
	"unable to fsck disk",
	"no file system on partition or super block corrupted",
	"message communication error",
	"illegal characters in disk label name",
	"no disk volumes available",
	"repair disk on a two drive system",
};

struct am_message am_message_prototype = {
	{MSG_TYPE_NORMAL, 0, 0, 0, sizeof(struct am_message), TRUE},
	{MSG_TYPE_INTEGER_32, sizeof(int)*8, 1, TRUE, FALSE, FALSE},
	0,
	{MSG_TYPE_PORT, sizeof(port_t)*8, 1, TRUE, FALSE, FALSE},
	0,
	{MSG_TYPE_INTEGER_32, sizeof(u_int)*8, 1, TRUE, FALSE, FALSE},
	0,
	{MSG_TYPE_INTEGER_32, sizeof(int)*8, 1, TRUE, FALSE, FALSE},
	0,
	{MSG_TYPE_INTEGER_32, sizeof(int)*8, 1, TRUE, FALSE, FALSE},
	0,
	{MSG_TYPE_CHAR, sizeof(char)*8, MAXLBLLEN, TRUE, FALSE, FALSE},
	{0},
	{MSG_TYPE_CHAR, sizeof(char)*8, MAXPATHLEN, TRUE, FALSE, FALSE},
	{0},
};

main(argc, argv)
{
	struct am_message am_m, *m = &am_m, am_r, *r = &am_r;
	port_t am_port, wm_port, sig_port, reply_port;
	int init, eject, not_mount;
	char cmd[8];
	
	if (errno = -netname_look_up(name_server_port, "",
	    WORKSPACE_AUTODISKMOUNT, &am_port))
		fatal("netname_look_up: %s", WORKSPACE_AUTODISKMOUNT);
	if (errno = -port_allocate(task_self(), &reply_port))
		fatal("reply_port");
	if (errno = -port_allocate(task_self(), &sig_port))
		fatal("sig_port");
	if (errno = -port_allocate(task_self(), &wm_port))
		fatal("wm_port");
	if (errno = -netname_check_in(name_server_port,
	    AUTODISKMOUNT_WORKSPACE, sig_port, wm_port))
		fatal("netname_check_in: %s", AUTODISKMOUNT_WORKSPACE);
	if (argc == 1)
rx:	while (1) {
		if (recv_msg(m, 0, wm_port) < 0)
			fatal("recv_msg");
		if (m->header.msg_id != AM_INSERT)
			continue;
		if (m->flags & REQUIRES_INIT) {
			printf ("REQUIRES_INIT\n");
			printf("init? ");
			scanf("%d", &init);
			if (init)
				r->flags |= DO_INIT;
		}
		if (m->flags & REQUIRES_FSCK)
			printf ("REQUIRES_FSCK\n");
		*r = am_message_prototype;
		printf("eject? ");
		scanf("%d", &eject);
		if (eject)
			r->flags |= DO_EJECT;
		printf("not mount? ");
		scanf("%d", &not_mount);
		if (not_mount)
			r->flags |= DO_NOT_MOUNT;
		printf("label? ");
		scanf("%s", r->label);
		printf("mntpt? ");
		scanf("%s", r->mntpt);
		r->uid = (uid_t) getuid();
		send_msg(r, m->header.msg_id | MSG_REPLY, m->header.msg_remote_port,
			PORT_NULL);
	}
	if (1) {
		printf("cmd? ");
		scanf("%s", cmd);
		*m = am_message_prototype;
		switch (cmd[0]) {
		
		case 'i':
			m->dev = makedev(2, 0);
			strcpy(m->label, "label-foo");
			send_msg(m, WM_INIT, am_port, reply_port);
		}
	}
	goto rx;
}

pmsg (s, m)
	char *s;
	struct am_message *m;
{
	printf("%s: 0x%x: dev 0x%x flags 0x%x uid %d errno %d label <%s> mntpt <%s>\n",
		s, m->header.msg_id, m->dev, m->flags,
		m->uid, m->errno,
		m->label, m->mntpt);
}

int send_msg (m, type, rport, lport)
	struct am_message *m;
{
	m->header.msg_local_port = lport;
	m->header.msg_remote_port = rport;
	m->header.msg_id = type;
	m->header.msg_size = sizeof(struct am_message);
pmsg("send", m);
	if (errno = -msg_send(m, SEND_TIMEOUT | SEND_SWITCH, 120000)) {
		panic("msg_send");
		return (-1);
	}
        return (0);
}

int recv_msg (m, type, lport)
        struct am_message *m;
{
	m->header.msg_local_port = lport;
	m->header.msg_size = sizeof(struct am_message);
	if (errno = -msg_receive(m, MSG_OPTION_NONE, 0)) {
		panic("msg_receive");
		return (-1);
	}
pmsg("recv", m);
	if (type && m->header.msg_id != type) {
		panic("recv_msg: got type 0x%x, expected 0x%x",
			m->header.msg_id, type);
		return (-1);
	}
	return (0);
}

extern int sys_nerr;
extern char *sys_errlist[];

panic (s, p1, p2, p3, p4)
	char *s;
{
	char *errs;
	char eb1[256], eb2[256];
	
	errs = "unknown error code";
	if (errno > 0 && errno <= sys_nerr)
		errs = sys_errlist[errno];
	else
	if (errno < 0) {
		errno = -errno;
		errs = (char*) mach_errormsg((kern_return_t) errno);
	} else
	if (errno >= E_START && errno < E_END)
		errs = err_list[errno - E_START];
	if (s)
		sprintf(eb1, s, p1, p2, p3, p4);
	if (errno)
		sprintf(eb2, "%s%s%s (%d)", s? eb1 : "", s? ": " : "", errs, errno);
	log("%s\n", eb2);
}

fatal (errno, s, p1, p2, p3, p4)
	char *s;
{
	panic(errno, s, p1, p2, p3, p4);
	exit (-1);
}

log (s, p1, p2, p3, p4)
	char *s;
{
	printf (s, p1, p2, p3, p4);
}

