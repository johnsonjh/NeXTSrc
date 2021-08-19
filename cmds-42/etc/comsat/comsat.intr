# 1 "comsat.c"
 






char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n All rights reserved.\n";




static char sccsid[] = "@(#)comsat.c	5.5 (Berkeley) 10/24/85";


# 1 "/os/king/MK/mk/sys/types.h"

 






































































# 1 "/os/king/MK/mk/sys/features.h"
 




































 









# 1 "/os/king/MK/mk/machine/FEATURES.h"
















































































# 48 "/os/king/MK/mk/sys/features.h"














# 73 "/os/king/MK/mk/sys/types.h"



 











 



 


 


 






# 1 "/os/king/MK/mk/machine/vm_types.h"
 

 











 






typedef	unsigned long	vm_offset_t;
typedef	unsigned long	vm_size_t;



# 105 "/os/king/MK/mk/sys/types.h"



typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef	unsigned short	ushort;		 














typedef struct  _physadr { short r[1]; } *physadr;
typedef struct  label_t {
        int     val[13];
} label_t;




typedef	struct	_quad { long val[2]; } quad;

typedef	long	daddr_t;
typedef	char *	caddr_t;
typedef	u_long	ino_t;
typedef	long	swblk_t;
typedef	long	size_t;
typedef	long	time_t;
typedef	short	dev_t;



typedef	long	off_t;

typedef	u_short	uid_t;
typedef	u_short	gid_t;


 









typedef long	fd_mask;





typedef	struct fd_set {
	fd_mask	fds_bits[(((256 )+(( (sizeof(fd_mask) * 8		)	)-1))/( (sizeof(fd_mask) * 8		)	)) ];
} fd_set;









# 17 "comsat.c"

# 1 "/os/king/MK/mk/sys/socket.h"

 









































 




# 1 "/os/king/MK/mk/sys/features.h"
 




































 









# 1 "/os/king/MK/mk/machine/FEATURES.h"
















































































# 48 "/os/king/MK/mk/sys/features.h"














# 49 "/os/king/MK/mk/sys/socket.h"



 







 



 








 



















 











 


struct	linger {
	int	l_onoff;		 
	int	l_linger;		 
};

 




 






















 



struct sockaddr {
	u_short	sa_family;		 
	char	sa_data[14];		 
};

 



struct sockproto {
	u_short	sp_family;		 
	u_short	sp_protocol;		 
};

 






















 




 


struct msghdr {
	caddr_t	msg_name;		 
	int	msg_namelen;		 
	struct	iovec *msg_iov;		 
	int	msg_iovlen;		 
	caddr_t	msg_accrights;		 
	int	msg_accrightslen;
};






# 18 "comsat.c"

# 1 "/os/king/MK/mk/sys/stat.h"

 













































 




# 1 "/os/king/MK/mk/sys/features.h"
 




































 









# 1 "/os/king/MK/mk/machine/FEATURES.h"
















































































# 48 "/os/king/MK/mk/sys/features.h"














# 53 "/os/king/MK/mk/sys/stat.h"







 







struct	stat
{
	dev_t	st_dev;
	ino_t	st_ino;
	unsigned short st_mode;
	short	st_nlink;
	uid_t	st_uid;
	gid_t	st_gid;
	dev_t	st_rdev;
	off_t	st_size;
	time_t	st_atime;
	int	st_spare1;
	time_t	st_mtime;
	int	st_spare2;
	time_t	st_ctime;
	int	st_spare3;
	long	st_blksize;
	long	st_blocks;
	long	st_spare4[2];
};

# 115 "/os/king/MK/mk/sys/stat.h"






















# 19 "comsat.c"

# 1 "/os/king/MK/mk/sys/wait.h"

 












































 



# 1 "/os/king/MK/mk/sys/features.h"
 




































 









# 1 "/os/king/MK/mk/machine/FEATURES.h"
















































































# 48 "/os/king/MK/mk/sys/features.h"














# 51 "/os/king/MK/mk/sys/wait.h"



 







 








 




union wait	{
	int	w_status;		 
	 


	struct {

		unsigned short  w_PAD16;
		unsigned        w_Retcode:8;     
 		unsigned        w_Coredump:1;    
 		unsigned        w_Termsig:7;     





	} w_T;
	 




	struct {

		unsigned short  w_PAD16;
 		unsigned        w_Stopsig:8;     
 		unsigned        w_Stopval:8;     




	} w_S;
};









 

















# 20 "comsat.c"

# 1 "/os/king/MK/mk/sys/file.h"

 




































































 








# 1 "/os/king/MK/mk/sys/features.h"
 




































 









# 1 "/os/king/MK/mk/machine/FEATURES.h"
















































































# 48 "/os/king/MK/mk/sys/features.h"














# 80 "/os/king/MK/mk/sys/file.h"







 





















# 147 "/os/king/MK/mk/sys/file.h"


 























 





 
















 










 





 












 









 








 






# 273 "/os/king/MK/mk/sys/file.h"






# 21 "comsat.c"


# 1 "/os/king/MK/mk/netinet/in.h"

 

































 



# 1 "/os/king/MK/mk/sys/features.h"
 




































 









# 1 "/os/king/MK/mk/machine/FEATURES.h"
















































































# 48 "/os/king/MK/mk/sys/features.h"














# 40 "/os/king/MK/mk/netinet/in.h"










 







 




 















 








 






 


struct in_addr {
	u_long s_addr;
};

 
























 


struct sockaddr_in {
	short	sin_family;
	u_short	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};







 





 





























# 23 "comsat.c"


# 1 "/release/NeXT_root/usr/include/stdio.h"
 









extern	struct	_iobuf {
	int	_cnt;
	char	*_ptr;		 
	char	*_base;		 
	int	_bufsiz;
	short	_flag;
	char	_file;		 
} _iob[];




































struct _iobuf 	*fopen();
struct _iobuf 	*fdopen();
struct _iobuf 	*freopen();
struct _iobuf 	*popen();
long	ftell();
char	*fgets();
char	*gets();




# 25 "comsat.c"

# 1 "/release/NeXT_root/usr/include/sgtty.h"
 


# 1 "/os/king/MK/mk/sys/ioctl.h"

 




























































 
# 72 "/os/king/MK/mk/sys/ioctl.h"

# 1 "/os/king/MK/mk/sys/features.h"
 




































 









# 1 "/os/king/MK/mk/machine/FEATURES.h"
















































































# 48 "/os/king/MK/mk/sys/features.h"














# 73 "/os/king/MK/mk/sys/ioctl.h"



 







 











# 1 "/os/king/MK/mk/sys/ttychars.h"

 

































 







 





struct ttychars {
	char	tc_erase;	 
	char	tc_kill;	 
	char	tc_intrc;	 
	char	tc_quitc;	 
	char	tc_startc;	 
	char	tc_stopc;	 
	char	tc_eofc;	 
	char	tc_brkc;	 
	char	tc_suspc;	 
	char	tc_dsuspc;	 
	char	tc_rprntc;	 
	char	tc_flushc;	 
	char	tc_werasc;	 
	char	tc_lnextc;	 
};



 




















# 96 "/os/king/MK/mk/sys/ioctl.h"

# 1 "/os/king/MK/mk/sys/ttydev.h"

 


























 

 







 





 



















# 80 "/os/king/MK/mk/sys/ttydev.h"


# 97 "/os/king/MK/mk/sys/ioctl.h"


# 1 "/os/king/MK/mk/sys/ttyloc.h"
 










































 


struct ttyloc
{
    long tlc_hostid;		 
    long tlc_ttyid;		 
};

 





 





 

















# 99 "/os/king/MK/mk/sys/ioctl.h"




struct tchars {
	char	t_intrc;	 
	char	t_quitc;	 
	char	t_startc;	 
	char	t_stopc;	 
	char	t_eofc;		 
	char	t_brkc;		 
};
struct ltchars {
	char	t_suspc;	 
	char	t_dsuspc;	 
	char	t_rprntc;	 
	char	t_flushc;	 
	char	t_werasc;	 
	char	t_lnextc;	 
};

 





struct sgttyb {
	char	sg_ispeed;		 
	char	sg_ospeed;		 
	char	sg_erase;		 
	char	sg_kill;		 
	short	sg_flags;		 
};


 







struct winsize {
	unsigned short	ws_row;			 
	unsigned short	ws_col;			 
	unsigned short	ws_xpixel;		 
	unsigned short	ws_ypixel;		 
};

 


struct ttysize {
	unsigned short	ts_lines;
	unsigned short	ts_cols;
	unsigned short	ts_xxx;
	unsigned short	ts_yyy;
};




 











 



 



 









































































 






















































































# 362 "/os/king/MK/mk/sys/ioctl.h"


struct fsparam
{
    long fsp_free;		 
    long fsp_ifree;		 
    long fsp_size;		 
    long fsp_isize;		 
    long fsp_minfree;		 
    long fsp_unused[3];		 
};



 





 






 

































# 4 "/release/NeXT_root/usr/include/sgtty.h"


# 26 "comsat.c"

# 1 "/release/NeXT_root/usr/include/utmp.h"
 







 




struct utmp {
	char	ut_line[8];		 
	char	ut_name[8];		 
	char	ut_host[16];		 
	long	ut_time;		 
};
# 27 "comsat.c"

# 1 "/release/NeXT_root/usr/include/signal.h"
 







# 1 "/os/king/MK/mk/sys/signal.h"

 
































































# 77 "/os/king/MK/mk/sys/signal.h"

# 1 "/os/king/MK/mk/sys/features.h"
 




































 









# 1 "/os/king/MK/mk/machine/FEATURES.h"
















































































# 48 "/os/king/MK/mk/sys/features.h"














# 78 "/os/king/MK/mk/sys/signal.h"










 








































 



















# 160 "/os/king/MK/mk/sys/signal.h"













































int	(*signal())();


 



struct	sigvec {
	int	(*sv_handler)();	 
	int	sv_mask;		 
	int	sv_flags;		 
};





 



struct	sigstack {
	char	*ss_sp;			 
	int	ss_onstack;		 
};

 






struct	sigcontext {
	int	sc_onstack;		 
	int	sc_mask;		 

	int	sc_sp;			 
	int	sc_pc;			 
	int	sc_ps;			 
# 271 "/os/king/MK/mk/sys/signal.h"

};












 




# 9 "/release/NeXT_root/usr/include/signal.h"

# 28 "comsat.c"

# 1 "/release/NeXT_root/usr/include/errno.h"
 







# 1 "/os/king/MK/mk/sys/errno.h"

 

















































 







# 1 "/os/king/MK/mk/sys/features.h"
 




































 









# 1 "/os/king/MK/mk/machine/FEATURES.h"
















































































# 48 "/os/king/MK/mk/sys/features.h"














# 60 "/os/king/MK/mk/sys/errno.h"







 







 




































 



 








 

	 













	 













	 



 




 
















# 199 "/os/king/MK/mk/sys/errno.h"










# 9 "/release/NeXT_root/usr/include/errno.h"

# 29 "comsat.c"

# 1 "/release/NeXT_root/usr/include/netdb.h"
 







 






struct	hostent {
	char	*h_name;	 
	char	**h_aliases;	 
	int	h_addrtype;	 
	int	h_length;	 
	char	**h_addr_list;	 

};

 



struct	netent {
	char		*n_name;	 
	char		**n_aliases;	 
	int		n_addrtype;	 
	unsigned long	n_net;		 
};

struct	servent {
	char	*s_name;	 
	char	**s_aliases;	 
	int	s_port;		 
	char	*s_proto;	 
};

struct	protoent {
	char	*p_name;	 
	char	**p_aliases;	 
	int	p_proto;	 
};

struct hostent	*gethostbyname(), *gethostbyaddr(), *gethostent();
struct netent	*getnetbyname(), *getnetbyaddr(), *getnetent();
struct servent	*getservbyname(), *getservbyport(), *getservent();
struct protoent	*getprotobyname(), *getprotobynumber(), *getprotoent();

 



extern  int h_errno;	





# 30 "comsat.c"

# 1 "/release/NeXT_root/usr/include/syslog.h"
 







# 1 "/os/king/MK/mk/sys/syslog.h"

 


























 

 







 










	 












 














 





 










# 9 "/release/NeXT_root/usr/include/syslog.h"

# 31 "comsat.c"


 


int	debug = 0;


struct	sockaddr_in sin = { 	2		 };
extern	errno;

char	hostname[32];
struct	utmp *utmp = 0 ;
int	nutmp;
int	uf;
unsigned utmpmtime = 0;			 
unsigned utmpsize = 0;			 
int	onalrm();
int	reapchildren();
long	lastmsgtime;
char 	*malloc(), *realloc();




main(argc, argv)
	int argc;
	char *argv[];
{
	register int cc;
	char buf[1024 ];
	char msgbuf[100];
	struct sockaddr_in from;
	int fromlen;

	 
	fromlen = sizeof (from);
	if (getsockname(0, &from, &fromlen) < 0) {
		fprintf((&_iob[2]) , "%s: ", argv[0]);
		perror("getsockname");
		_exit(1);
	}
	chdir("/usr/spool/mail");
	if ((uf = open("/etc/utmp",0)) < 0) {
		openlog("comsat", 0, (3<<3)	);
		syslog(	3	, "/etc/utmp: %m");
		(void) recv(0, msgbuf, sizeof (msgbuf) - 1, 0);
		exit(1);
	}
	lastmsgtime = time(0);
	gethostname(hostname, sizeof (hostname));
	onalrm();
	signal(14	, onalrm);
	signal(22	, 	(int (*)())1 );
	signal(20	, reapchildren);
	for (;;) {
		cc = recv(0, msgbuf, sizeof (msgbuf) - 1, 0);
		if (cc <= 0) {
			if (errno != 	4		)
				sleep(1);
			errno = 0;
			continue;
		}
		sigblock((1 << ((14	)-1)) );
		msgbuf[cc] = 0;
		lastmsgtime = time(0);
		mailfor(msgbuf);
		sigsetmask(0);
	}
}

reapchildren()
{

	while (wait3((struct wait *)0, 	1	, (struct rusage *)0) > 0)
		;
}

onalrm()
{
	struct stat statbf;

	if (time(0) - lastmsgtime >= 120 )
		exit(0);
	if (debug) printf ("alarm\n");
	alarm(15);
	fstat(uf, &statbf);
	if (statbf.st_mtime > utmpmtime) {
		if (debug) printf (" changed\n");
		utmpmtime = statbf.st_mtime;
		if (statbf.st_size > utmpsize) {
			utmpsize = statbf.st_size + 10 * sizeof(struct utmp);
			if (utmp)
				utmp = (struct utmp *)realloc(utmp, utmpsize);
			else
				utmp = (struct utmp *)malloc(utmpsize);
			if (! utmp) {
				if (debug) printf ("malloc failed\n");
				exit(1);
			}
		}
		lseek(uf, 0, 0);
		nutmp = read(uf,utmp,statbf.st_size)/sizeof(struct utmp);
	} else
		if (debug) printf (" ok\n");
}

mailfor(name)
	char *name;
{
	register struct utmp *utp = &utmp[nutmp];
	register char *cp;
	char *rindex();
	int offset;

	if (debug) printf ("mailfor %s\n", name);
	cp = name;
	while (*cp && *cp != '@')
		cp++;
	if (*cp == 0) {
		if (debug) printf ("bad format\n");
		return;
	}
	*cp = 0;
	offset = atoi(cp+1);
	while (--utp >= utmp)
		if (!strncmp(utp->ut_name, name, sizeof(utmp[0].ut_name)))
			notify(utp, offset);
}

char	*cr;

notify(utp, offset)
	register struct utmp *utp;
{
	struct _iobuf  *tp;
	struct sgttyb gttybuf;
	char tty[20], name[sizeof (utmp[0].ut_name) + 1];
	struct stat stb;

	strcpy(tty, "/dev/");
	strncat(tty, utp->ut_line, sizeof(utp->ut_line));
	if (debug) printf ("notify %s on %s\n", utp->ut_name, tty);
	if (stat(tty, &stb) == 0 && (stb.st_mode & 0100) == 0) {
		if (debug) printf ("wrong mode\n");
		return;
	}
	if (fork())
		return;
	signal(14	, 	(int (*)())0 );
	alarm(30);
	if ((tp = fopen(tty,"w")) == 0) {
		if (debug) printf ("fopen failed\n");
		exit(-1);
	}
	ioctl(((tp)->_file) , (	0x40000000	|((sizeof(struct sgttyb)&0x7f		)<<16)|('t'<<8)| 8)  , &gttybuf);
	cr = (gttybuf.sg_flags&	0x00000010	) && !(gttybuf.sg_flags&	0x00000020	) ? "" : "\r";
	strncpy(name, utp->ut_name, sizeof (utp->ut_name));
	name[sizeof (name) - 1] = '\0';
	fprintf(tp,"%s\n\007New mail for %s@%.*s\007 has arrived:%s\n",
	    cr, name, sizeof (hostname), hostname, cr);
	fprintf(tp,"----%s\n", cr);
	jkfprintf(tp, name, offset);
	exit(0);
}

jkfprintf(tp, name, offset)
	register struct _iobuf  *tp;
{
	register struct _iobuf  *fi;
	register int linecnt, charcnt;
	char line[1024 ];
	int inheader;

	if (debug) printf ("HERE %s's mail starting at %d\n",
	    name, offset);
	if ((fi = fopen(name,"r")) == 0 ) {
		if (debug) printf ("Cant read the mail\n");
		return;
	}
	fseek(fi, offset, 	0	);
	 




	linecnt = 7;
	charcnt = 560;
	inheader = 1;
	while (fgets(line, sizeof (line), fi) != 0 ) {
		register char *cp;
		char *index();
		int cnt;

		if (linecnt <= 0 || charcnt <= 0) {  
			fprintf(tp,"...more...%s\n", cr);
			return;
		}
		if (strncmp(line, "From ", 5) == 0)
			continue;
		if (inheader && (line[0] == ' ' || line[0] == '\t'))
			continue;
		cp = index(line, ':');
		if (cp == 0 || (index(line, ' ') && index(line, ' ') < cp))
			inheader = 0;
		else
			cnt = cp - line;
		if (inheader &&
		    strncmp(line, "Date", cnt) &&
		    strncmp(line, "From", cnt) &&
		    strncmp(line, "Subject", cnt) &&
		    strncmp(line, "To", cnt))
			continue;
		cp = index(line, '\n');
		if (cp)
			*cp = '\0';
		fprintf(tp,"%s%s\n", line, cr);
		linecnt--, charcnt -= strlen(line);
	}
	fprintf(tp,"----%s\n", cr);
}
