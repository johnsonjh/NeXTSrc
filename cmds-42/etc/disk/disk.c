/*	@(#)disk.c	1.0	08/28/87	(c) 1987 NeXT	*/

/* 
 **********************************************************************
 * HISTORY
 * 13-Jul-90  John Seamons (jks) at NeXT
 *	Remove some debugging code that allowed operating beyond the legal
 *	end of the media.
 *
 * 12-Mar-90  Doug Mitchell at NeXT
 *	Added floppy support.
 *
 
 *  3-Mar-90  John Seamons (jks) at NeXT
 *	Added "scan" command that searches for superblocks and prints
 *	their locations.  Used to discover what superblock backup number
 *	to give to fsck (i.e. "fsck -bnnn /dev/rod0a".
 *
 *  1-Mar-90  John Seamons (jks) at NeXT
 *	Removed commands which were judged to be of marginal interest to
 *	customers or were only intended for use during internal development.
 *
 * 15-Aug-89  Gregg Kellogg (gk) at NeXT
 *	In main loop, if gets returns NULL disk is aborted.
 *
 * 27-Oct-88  Mike DeMoney (mike) at NeXT
 *	Added support for SCSI disk types that may be configured with
 *	different sector sizes.
 *
 * 16-Mar-88  John Seamons (jks) at NeXT
 *	Cleaned up to support standard disk label definitions.
 *
 *  5-Mar-88  Mike DeMoney (mike) at NeXT
 *	Completed SCSI support.
 *
 * 16-Nov-87  Mike DeMoney (mike) at NeXT
 *	Modified for SCSI support.
 *
 * 28-Aug-87  John Seamons (jks) at NeXT
 *	Created.
 *
 **********************************************************************
 */

#define	A_OUT_COMPAT	1

/*
 *	TODO
 *
 *	- remove hardwired constants -- always use label info
 *	- cmp mode in r/w
 */

#include <mach.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <math.h>
#include <syslog.h>
#include <sys/features.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <ufs/fs.h>
#include <sys/loader.h>
#if	A_OUT_COMPAT
#include "exec.h"
#endif	A_OUT_COMPAT
#include <nextdev/npio.h>
#include <nextdev/video.h>
#include <nextdev/disk.h>
#include "disk.h"

/*
 * Default place to find block 0 boot
 * FIXME: maybe this should go in disktab?
 */
#define	BOOT		"/usr/standalone/boot"
#define	DISKNAME	"Disk"

/*
 * Device types
 */
#define	TY_SCSI		0x0001
#define	TY_OPTICAL	0x0002
#define	TY_REMOVABLE	0x0004
#define TY_FLOPPY	0x0008
#define	TY_ALL		0xffff

#define	TY_OD		(TY_OPTICAL|TY_REMOVABLE)

int	od_conf(), od_init(), od_req(), od_geterr(), od_wlabel(), od_cyl(),
	od_pstats(), od_ecc(), od_glabel(), od_format(),
	sd_conf(), sd_init(), sd_req(), sd_geterr(), sd_wlabel(), sd_cyl(),
	sd_pstats(), sd_ecc(), sd_glabel(), sd_format(),
	fd_conf(), fd_init(), fd_req(), fd_geterr(), fd_wlabel(), fd_cyl(),
	fd_pstats(), fd_ecc(), fd_glabel(), fd_format();

struct dtype_sw dtypes[] = {
	{
		"removable_rw_optical",	TY_OPTICAL | TY_REMOVABLE,
		od_conf, od_init, od_req, od_geterr, od_wlabel, od_cyl,
		od_pstats, od_ecc, od_glabel, od_format,
		4149, 4149*16, 20000, 1000
	},
	{
		"fixed_rw_scsi",	TY_SCSI,
		sd_conf, sd_init, sd_req, sd_geterr, sd_wlabel, sd_cyl,
		sd_pstats, sd_ecc, sd_glabel, sd_format,
		0, 0, 2000, 100
	},
	{
		/* SCSI opticals */
		"removable_rw_scsi",	TY_SCSI | TY_REMOVABLE,						
		sd_conf, sd_init, sd_req, sd_geterr, sd_wlabel, sd_cyl,
		sd_pstats, sd_ecc, sd_glabel, sd_format,
		0, 0, 20000, 1000
	},
	{
		"removable_rw_floppy",	TY_FLOPPY  | TY_REMOVABLE,
		fd_conf, fd_init, fd_req, fd_geterr, fd_wlabel, fd_cyl,
		fd_pstats, fd_ecc, fd_glabel, fd_format,
		0, 0, 20000, 1000
	},
	{
		NULL,			0
	}
};

int	help(), quit(), init(), seek(), Read(), Write(), test(), all(), set(),
	label(), bad(), bitmap(), eject(), tdbug(), rw(), scan(),
	look(), pstats(), zstats(), rwr(), Format(),
	verify(), boot(), tabort(),
	tvers(), newhost(), newlbl();

struct cmds {
	int	(*c_func)();
	int	c_typemask;
	char	*c_name;
	char	*c_desc;
} cmds[] = {
	init,	TY_ALL,		"init",		"initialize disk",
	eject,	TY_REMOVABLE,	"eject",	"eject disk",
	boot,	TY_ALL,		"boot",		"write block 0 boot",
	label,	TY_ALL,		"label",	"edit label information",
	newhost,TY_ALL,		"host",		"change hostname on label",
	newlbl,	TY_ALL,		"name",		"change disk label name",
	Format, TY_SCSI | TY_FLOPPY, "Format",  "Format Disk",
	bitmap,	TY_OPTICAL,	"bitmap",	"edit status bitmap",
	bad,	TY_OPTICAL,	"bad",		"edit bad block table",
	scan,	TY_ALL,		"scan",		"scan for superblocks",
	Read,	TY_ALL,		"read",		"read from disk",
	Write,	TY_ALL,		"write",	"write to disk",
	verify,	TY_ALL,		"verify",	"verify data on disk",
	rw,	TY_ALL,		"rw",		"read-after-write",
	rwr,	TY_ALL,		"rwr",		"read-after-write random",
	look,	TY_ALL,		"look",		"look at read/write buffer",
	set,	TY_ALL,		"set",		"set read/write buffer",
	pstats,	TY_OPTICAL,	"stats",	"print drive statistics",
	zstats,	TY_OPTICAL,	"zero",		"zero drive statistics",
	tabort,	TY_ALL,		"abort",	"toggle abort on error mode",
	tvers,	TY_ALL,		"vers",		"toggle label version",
	help,	TY_ALL,		"help",		"print this list",
	help,	TY_ALL,		"?",		"print this list",
	quit,	TY_ALL,		"quit",		"quit program",
};
int ncmds = sizeof (cmds) / sizeof (cmds[0]);

#define	MAXSECT		16
#define	MAXSSIZE	DEV_BSIZE
#define	TBSIZE		(MAXSECT * MAXSSIZE)
#define	ALIGN		64
#define ri(l, h) \
	((random()/(2147483647/((h)-(l)+1)))+(l))

u_char	test_rbuf[TBSIZE+ALIGN], test_wbuf[TBSIZE+ALIGN],
	cmp_rbuf[TBSIZE+ALIGN];
int	abort_flag = 1, version = DL_VERSION;
int	f_init, f_stat, f_eject, f_test, f_boot, f_query, f_bulk, f_newhost;
int	f_label, f_format;
int	interactive, bad_modified, resp;
char	*prog, *fn, *name, line[BUFSIZ], *hostname = 0, *labelname = 0;
char	namebuf[BUFSIZ];
jmp_buf	env;
extern	int errno;
struct	drive_info drive_info, *di = &drive_info;
struct	timeval start, stop;
struct	dtype_sw *drive_type();
extern struct disktab *sd_inferdisktab(int fd);
extern struct disktab *fd_inferdisktab(int fd, char *devname, int init_flag, 
	int density);
int 	density;		/* for formatting floppies */

main (argc, argv)
	char *argv[];
{
	char *fp, cmd[64];
	register struct cmds *cp;
	int sigint();

	prog = *argv++;
	while (--argc > 0) {
		if (*argv[0] == '-') {
			fp = *argv;
			while (*(++fp)) switch (*fp) {
				case 'i':
					f_init = 1;
					break;
				case 't':
					name = *(++argv);
					argc--;
					break;
				case 'F':
					f_format = 1;
					break;
				case 'd':
					density = atoi(*(++argv));
					argc--;
					break;
				case 's':
					f_stat = 1;
					break;
				case 'e':
					f_eject = 1;
					break;
				case 'T':
					f_test = 1;
					break;
				case 'b':
					f_boot = 1;
					break;
				case 'h':
					hostname = *(++argv);
					argc--;
					break;
				case 'l':
					labelname = *(++argv);
					argc--;
					break;
				case 'q':
					f_query = 1;
					break;
				case 'B':
					f_bulk = 1;
					break;
				case 'H':
					hostname = *(++argv);
					argc--;
					f_newhost = 1;
					break;
				case 'L':
					labelname = *(++argv);
					argc--;
					f_label = 1;
					break;
				default:
					printf ("bad flag: %c\n", *fp);
					goto usage;
			}
		} else
			fn = *argv;
		argv++;
	}
	if (fn == 0 || *fn == 0 || argc < 0) {
usage:
		printf ("usage: %s [option flags] [action flags] "
			"raw-device\n", prog);
		printf ("option flags:\n");
		printf ("\t-h hostname\tspecify host name\n");
		printf ("\t-l labelname\tspecify label name\n");
		printf ("\t-t disk_type\tspecify disk type name\n");
		printf ("\t-d density\tspecify format density (in KBytes)\n");
		printf ("action flags:\n");
		printf ("\t-b\t\twrite boot block\n");
		printf ("\t-e\t\teject disk\n");
		printf ("\t-i\t\tinitialize disk\n");
		printf ("\t-q\t\tquery disk name and print it\n");
		printf ("\t-s\t\tprint disk statistics\n");
		printf ("\t-B\t\tbulk erase optical media\n");
		printf ("\t-F\t\tFormat Disk\n");
		printf ("\t-H hostname\tchange host name on label\n");
		printf ("\t-L labelname\tchange disk label name\n");
		printf ("\t-T\t\trun test patterns\n");
		printf ("interactive mode if no action flags specified\n");
		exit (-1);
	}
	dt = &disktab;
	openlog ("disk", LOG_USER, LOG_NOWAIT);
	if ((fd = open (fn, O_RDWR)) < 0)
		if (errno == ENODEV)
			bomb (S_EMPTY, "no disk cartridge inserted in drive");
		else
			panic (S_NEVER, fn);
	fn[strlen(fn)-1] = 0;	/* delete partition letter */
	if (ioctl (fd, DKIOCINFO, di) < 0)
		panic (S_NEVER, "get info");
		devblklen = di->di_devblklen;
	if (name == 0) {
		if (devblklen) {
			sprintf(namebuf, "%s-%d", di->di_name, devblklen);
			name = namebuf;
		} else
			name = di->di_name;
	}
again:
	if (*name == 0)
		bomb (S_NEVER, "can't figure out disk name, use -t option\n");
	if ((dt = getdiskbyname(name)) == 0) {		
		if (name == namebuf) {
			name = di->di_name;
			goto again;
		}
		/*
		 * No entry in disktab; let's see if device-specific code
		 * can figure it out...
		 */
		if(strncmp(fn, "/dev/rsd", 8) == 0)
			dt = sd_inferdisktab(fd);
		else if(strncmp(fn, "/dev/rfd", 8) == 0)
			dt = fd_inferdisktab(fd, fn, (f_init || f_format),
				density);
		if(dt == 0)
			bomb (S_NEVER, "%s: unknown disk name", name);
	}
	if (f_query) {
		printf("%s\n", name);
		exit(0);
	}
	printf ("disk name: %s\n", name);
	if (dt->d_secsize != DEV_BSIZE)
		printf("WARNING: disktab sector size (%d)"
		    "!= DEV_BSIZE (%d)\n", dt->d_secsize, DEV_BSIZE);
	if (dt->d_secsize < devblklen || dt->d_secsize % devblklen)
		printf("WARNING: device sector size (%d)"
		    "incompatable with DEV_BSIZE %d\n", devblklen,
		    DEV_BSIZE);
	dsp = drive_type(dt->d_type);
	if (dsp == NULL)
		bomb (S_NEVER, "%s: unknown disk type", dt->d_type);
	printf ("disk type: %s\n", dt->d_type);

	if (f_init || f_stat || f_eject || f_test || f_boot || f_bulk ||
	    f_newhost || f_label || f_format) {
		if (f_init)
			do_cmd("init");
		if (f_boot)
			do_cmd("boot");
		if (f_test)
			do_cmd("test");
		if (f_stat)
			do_cmd("stats");
		if (f_eject)
			do_cmd("eject");
		if (f_bulk)
			do_cmd("bulk");
		if (f_newhost)
			newhost();
		if (f_label)
			newlbl();
		if (f_format)
			Format();
		closelog();
		exit(0);
	}

	/* interactive mode */
	printf ("Disk utility\n\n");
	interactive = 1;
	signal (SIGINT, sigint);
	while (1) {
		setjmp (env);
		printf ("disk> ");
		if (gets(cmd) == NULL)
			quit();
		if (cmd[0] == 0)
			continue;
		do_cmd(cmd);
	}
}

do_cmd(cmd)
char *cmd;
{
	register struct cmds *cp;

	for (cp = cmds; cp < &cmds[ncmds]; cp++)
		if (strncmp (cmd, cp->c_name, strlen (cmd)) == 0) {
			if (cp->c_typemask & dsp->ds_typemask) {
				(*cp->c_func)();
				return;
			}
			bomb(S_NEVER, "%s: invalid request for type %s", cmd,
			    dsp->ds_type);
		}
	bomb (S_NEVER, "%s: unknown command -- 'help' lists them", cmd);
}

help() {
	register struct cmds *cp;

	printf ("commands are:\n");
	for (cp = cmds; cp < &cmds[ncmds]; cp++)
		if (cp->c_typemask & dsp->ds_typemask)
			printf ("\t%-8s%s\n", cp->c_name, cp->c_desc);
}

scan()
{
	int blk = 0;
	char buf[SBSIZE];
	struct fs *fs = (struct fs*) buf;
	
	printf ("Backup superblocks at:\n");
	lseek (fd, 0, L_SET);
	while (1) {
		if (read (fd, buf, sizeof buf) < 0 && errno != EIO)
			break;
		if (fs->fs_magic == FS_MAGIC) {
			printf ("%d ", blk*8);
			fflush (stdout);
		}
		blk++;
	}
}

make_new_label()
{
	struct disk_label *l = &disk_label;
	
	bzero (l, sizeof (struct disk_label));
	l->dl_version = version;
	l->dl_dt = *dt;
	if (hostname) {
		strncpy(l->dl_hostname, hostname, MAXHNLEN);
	} else {
		if (interactive) {
			getrmsg("enter host name: ");
			strncpy(l->dl_hostname, line, MAXHNLEN);
		} else
			strncpy(l->dl_hostname, "localhost", MAXHNLEN);
	}
	if (labelname) {
		strcpy(l->dl_label, labelname, MAXLBLLEN);
	} else {
		if (!interactive) {
			strcpy(l->dl_label, DISKNAME);
		} else {
			getrmsg("enter disk label: ");
			strncpy(l->dl_label, line, MAXLBLLEN);
		}
	}
}

int Format() {
	if((*dsp->ds_format)(fd, fn, TRUE)) {
		bomb(S_NEVER, "Disk Format Failed\n");
	}	
}

init() {
	register int i, spbe;
	int status, size, nbad, *bbt;
	register struct disk_label *l = &disk_label;
	register struct partition *pp;
	char cmd[256];

	if (!confirm ("DESTROYS ALL EXISTING DISK DATA -- really initialize? "))
		return;

	make_new_label();
	d_size = dt->d_ncylinders * dt->d_ntracks * dt->d_nsectors;
	if (l->dl_version == DL_V1)
		spbe = l->dl_nsect >> 1;
	else
		spbe = 1;
	if (l->dl_version == DL_V1 || l->dl_version == DL_V2) {
		bbt = l->dl_bad;
		nbad = NBAD;
	} else {
		bbt = bad_block.bad_blk;
		nbad = NBAD_BLK;
		bzero (&bad_block, sizeof (bad_block));
	}
	i = (d_size - l->dl_front - l->dl_back -
		l->dl_ngroups * l->dl_ag_size) / spbe +
		l->dl_ngroups * (l->dl_ag_alts / spbe);
	i = (i < (nbad - 1))? i : nbad - 1;
	bbt[i] = -1;

	/* format volume if necessary */
	if((*dsp->ds_format)(fd, fn, FALSE)) {
		bomb(S_NEVER, "Disk Format Failed\n");
	}

	/* do device mode selections */
	(*dsp->ds_config)();
	(*dsp->ds_devinit)();
	printf ("writing disk label\n");
	(*dsp->ds_wlabel) (bbt);
	boot();
	for (i = 0; i < NPART; i++) {
		pp = &dt->d_partitions[i];
		if (pp->p_newfs == 0)
			continue;
		sprintf (cmd, "/usr/etc/newfs -n -v %s%c", fn, 'a'+i);
		printf ("creating new filesystem on %s%c\n", fn, 'a'+i);
		printf("%s\n", cmd);
		if (status = system (cmd))
			bomb (S_NEVER, "/usr/etc/newfs %s%c failed (status %d)",
				fn, 'a'+i, status >> 8);
	}
	printf ("initialization complete\n");
}

boot()
{
	char *blk0 = BOOT;
	int ok, bfd, size;
	char buf[1024];
#if	A_OUT_COMPAT
	struct exec *a = (struct exec*) &buf;
#endif	A_OUT_COMPAT
	struct mach_header *mh = (struct mach_header*) &buf;
	char *blk0buf;
	struct disk_label *l = &disk_label;
	int i, nblk, success, lblblk;

	/* write out boot blocks */
	if (interactive) {
		do {
			printf("Block 0 boot is \"%s\", ", blk0);
			if (!(ok = confirm("ok? "))) {
				getrmsg("Block 0 boot: ");
				blk0 = line;
			}
		} while (!ok);
	}
	if ((bfd = open(blk0, 0)) < 0)
		panic(S_NEVER, blk0);
	if (read(bfd, &buf, sizeof(buf)) != sizeof(buf))
		panic (S_NEVER, blk0);
#if	A_OUT_COMPAT
	if (a->a_magic == OMAGIC) {
		size = sizeof(a) + a->a_text + a->a_data;
	} else
#endif	A_OUT_COMPAT
	if (mh->magic == MH_MAGIC && mh->filetype == MH_PRELOAD) {
		struct segment_command *sc;
		int first_seg, cmd;
		
		sc = (struct segment_command*)
			(buf + sizeof (struct mach_header));
		first_seg = 1;
		size = 0;
		for (cmd = 0; cmd < mh->ncmds; cmd++) {
			switch (sc->cmd) {
			
			case LC_SEGMENT:
				if (first_seg) {
					size += sc->fileoff;
					first_seg = 0;
				}
				size += sc->filesize;
				break;
			}
			sc = (struct segment_command*)
				((int)sc + sc->cmdsize);
		}
	} else
		bomb(S_NEVER, "%s: unknown binary format", blk0);
	if ((blk0buf = (char*) malloc(size + 16)) == NULL)
		bomb(S_NEVER, "Size (%d) of %s to big for memory",
			size, blk0);
	blk0buf = (char*)((((int)blk0buf + 15) >> 4) << 4);
	lseek(bfd, 0, 0);
	if ((i = read(bfd, blk0buf, size)) < 0)
		panic (S_NEVER, blk0);
	if (i != size)
		bomb(S_NEVER, "%s: corrupted image size (%d != %d)",
			blk0, i, size);
	close(bfd);
	(*dsp->ds_glabel) (l, &bad_block, 0);
	/*
	 * Make sure boot blocks don't:
	 *	- overwrite labels
	 *	- overwrite each other
	 *	- extend beyond front porch
	 */

	/* FIXME: od: account for sparse labels and bitmap! */
	lblblk = NLABELS * howmany(sizeof(struct disk_label), l->dl_secsize);
	nblk = howmany(size, l->dl_secsize);
	success = 0;
	for (i = 0; i < NBOOTS; i++) {
		if (l->dl_boot0_blkno[i] < 0)
			continue;
		if (l->dl_boot0_blkno[i] < lblblk)
			bomb(S_NEVER, "boot block overlays labels");
		if (l->dl_boot0_blkno[i] + nblk > l->dl_front)
			bomb(S_NEVER, "boot block extends beyond front porch");
		if (i < NBOOTS-1
		    && l->dl_boot0_blkno[i] != l->dl_boot0_blkno[i+1]) {
			if (l->dl_boot0_blkno[i] > l->dl_boot0_blkno[i+1])
				bomb(S_NEVER, "boot blocks out of order");
			if (l->dl_boot0_blkno[i]+nblk > l->dl_boot0_blkno[i+1])
				bomb(S_NEVER, "boot blocks overlay each other");
		}
		success++;
	}
	if (! success)
		bomb(S_NEVER, "No boot blocks specified in label");
	success = 0;
	for (i = 0; i < NBOOTS; i++) {
		if (l->dl_boot0_blkno[i] < 0)
			continue;
		if ((*dsp->ds_req) (CMD_WRITE, l->dl_boot0_blkno[i], blk0buf,
		    size) < 0)
			printf("Write of boot block %d failed\n", i);
		else
			success++;
	}
	if (! success)
		bomb(S_NEVER, "No boot blocks on disk");
	free(blk0buf);
}

label() {
	register int i, j;
	register char c;
	register struct disk_label *l = &disk_label;
	register struct partition *p;
	register struct fs_info *f;
	int spbe;

	c = getrmsg ("label information: print, write? ");
	if (c == 'p') {
	(*dsp->ds_glabel) (l, &bad_block, 0);
	printf ("current label information on disk:\n");
	printf ("disk label version #%d\n",
		l->dl_version == DL_V1? 1 : l->dl_version == DL_V2? 2 : 3);
	printf ("disk label: %s\ndisk name: %s\ndisk type: %s\n",
		l->dl_label, l->dl_name, l->dl_type);
	printf ("ncyls %d ntrack %d nsect %d rpm %d\n",
		l->dl_ncyl, l->dl_ntrack, l->dl_nsect, l->dl_rpm);
	printf ("sector_size %d front_porch %d back_porch %d\n",
		l->dl_secsize, l->dl_front, l->dl_back);
	printf ("ngroups %d ag_size %d ag_alts %d ag_off %d\n",
		l->dl_ngroups, l->dl_ag_size, l->dl_ag_alts, l->dl_ag_off);
	printf ("boot blocks: ");
	for (i = 0; i < NBOOTS; i++)
		printf ("#%d at %d ", i+1, l->dl_boot0_blkno[i]);
	printf ("\n");
	if (l->dl_bootfile[0])
		printf ("bootfile: %s\n", l->dl_bootfile);
	if (l->dl_hostname)
		printf ("host name: %s\n", l->dl_hostname);
	if (l->dl_rootpartition)
		printf ("root partition: %c\n", l->dl_rootpartition);
	if (l->dl_rwpartition)
		printf ("read/write partition: %c\n", l->dl_rwpartition);
printf (
"part   base   size bsize fsize cpg density minfree newfs optim automount type\n");
/*
 x    xxxxxx xxxxxx  xxxx  xxxx xxx   xxxxx     xx%   xxx xxxxx       xxx ...
*/
	for (i = 0; i < NPART; i++) {
		p = &l->dl_part[i];
		if (p->p_base == -1)
			continue;
printf ("%c    %6d %6d  %4d  %4d %3d   %5d     %2d%%   %s %s       %s %s\n",
			i+'a', p->p_base, p->p_size, p->p_bsize, p->p_fsize,
			p->p_cpg, p->p_density, p->p_minfree,
			p->p_newfs? "yes" : " no",
			p->p_opt == 's'? "space" : " time",
			p->p_automnt? "yes" : " no", p->p_type);
		if (p->p_mountpt[0])
			printf ("mount point: %s\n", p->p_mountpt);
	}
	} else
	if (c == 'w') {
		write_label();
	} else
		printf ("invalid label information command\n");
}

write_label()
{
	register struct disk_label *l = &disk_label;
	struct disk_label old_disk_label;
	register struct disk_label *ol = &old_disk_label;
	int spbe, nbad, *bbt, o_nbad, *o_bbt, d_size;
	register int i, j;
	struct bad_block o_bad_block;

	make_new_label();
	
	/*
	 *  Merge in bad block info from old label.
	 *  This is required if we're just changing a value of
	 *  the label that doesn't effect disk geometry and
	 *  want to preserve the old bad block structure.
	 */
	if (l->dl_version == DL_V1 || l->dl_version == DL_V2) {
		bbt = l->dl_bad;
		o_bbt = ol->dl_bad;
		nbad = NBAD;
	} else {
		bbt = bad_block.bad_blk;
		o_bbt = o_bad_block.bad_blk;
		nbad = NBAD_BLK;
	}
	if (l->dl_version == DL_V1)
		spbe = l->dl_nsect >> 1;
	else
		spbe = 1;
	d_size = l->dl_ncyl * l->dl_ntrack * l->dl_nsect;
	i = (d_size - l->dl_front - l->dl_back -
		l->dl_ngroups * l->dl_ag_size) / spbe +
		l->dl_ngroups * (l->dl_ag_alts / spbe);
	if ((*dsp->ds_glabel) (ol, &o_bad_block, 1) >= 0) {
		if (!confirm (
"WARNING: using information from /etc/disktab to construct new disk label.\n"
"If all you want to do is change the host name or label name, then use the\n"
"\"host\" or \"name\" commands.  If the information in /etc/disktab doesn't\n"
"match the disk geometry specified by the current disk label then the disk\n"
"contents may be unreadable.\n"
"OK to construct new disk label? "))
			return;
		if (ol->dl_version == DL_V1)
			spbe = ol->dl_nsect >> 1;
		else
			spbe = 1;
		d_size = ol->dl_ncyl * ol->dl_ntrack * ol->dl_nsect;
		j = (d_size - ol->dl_front - ol->dl_back -
			ol->dl_ngroups * ol->dl_ag_size) / spbe +
			ol->dl_ngroups * (ol->dl_ag_alts / spbe);
		if (l->dl_version == ol->dl_version && i == j) {
			printf ("merging in bad block info from old label\n");
			for (j = 0; j < nbad; j++)
				bbt[j] = o_bbt[j];
		}
	}

	/* mark end of bad block table */
	i = (i < (nbad - 1))? i : nbad - 1;
	bbt[i] = -1;
	printf ("writing disk label\n");
	(*dsp->ds_wlabel) (bbt);
}

newhost() {
	register struct disk_label *l = &disk_label;

	(*dsp->ds_glabel) (l, &bad_block, 0);
	if (interactive) {
		getrmsg("enter host name: ");
		strncpy(l->dl_hostname, line, MAXHNLEN);
	} else {
		strncpy(l->dl_hostname, hostname, MAXHNLEN);
		printf ("changing hostname to %s\n", hostname);
	}
	(*dsp->ds_wlabel) (&bad_block);
}

newlbl() {
	register struct disk_label *l = &disk_label;

	(*dsp->ds_glabel) (l, &bad_block, 0);
	if (interactive) {
		getrmsg("enter disk label name: ");
		strncpy(l->dl_label, line, MAXLBLLEN);
	} else {
		strncpy(l->dl_label, labelname, MAXLBLLEN);
		printf ("changing disk label name to \"%s\"\n", labelname);
	}
	(*dsp->ds_wlabel) (&bad_block);
}

bad() {
	register int i, spbe, apag, alt, ag, offset, shift;
	register char c;
	register struct disk_label *l = &disk_label;
	int total, numbad, nbad, *bbt;

	if (l->dl_version == DL_V1 || l->dl_version == DL_V2) {
		bbt = l->dl_bad;
		nbad = NBAD;
	} else {
		bbt = bad_block.bad_blk;
		nbad = NBAD_BLK;
	}
	c = getrmsg ("bad block table: print, edit, write, stats? ");
	if (c == 'p') {
		if (!bad_modified) {
			(*dsp->ds_glabel) (l, &bad_block, 0);
			if (l->dl_version == DL_V1 || l->dl_version == DL_V2) {
				bbt = l->dl_bad;
				nbad = NBAD;
			} else {
				bbt = bad_block.bad_blk;
				nbad = NBAD_BLK;
			}
		}
		if (l->dl_version == DL_V1)
			spbe = l->dl_nsect >> 1;
		else
			spbe = 1;
		apag = l->dl_ag_alts / spbe;
		printf ("entry(ag,#): bad_block->alternate\n");
		printf ("entries not listed are available\n");
		for (i = 0; i < nbad && bbt[i] != -1; i++) {
			if (bbt[i] == 0)
				continue;
			ag = i / apag;
			if (ag < l->dl_ngroups) {
				alt = l->dl_front + ag*l->dl_ag_size +
					l->dl_ag_off + (i % apag) * spbe;
				printf ("%d(%d,%d): %d->%d  ", i, ag,
					i % apag, bbt[i], alt);
			} else {
				alt = l->dl_front + (i - apag*l->dl_ngroups)
					* spbe + l->dl_ngroups*l->dl_ag_size;
				printf ("%d(ovfl): %d->%d  ", i,
					bbt[i], alt);
			}
			if ((i % 6) == 0)
				printf ("\n");
		}
		printf ("\n");
	} else
	if (c == 'e') {
		printf ("edit bad block table -- <return> when finished\n");
		while (1) {
			if ((i = getnrmsg ("entry? ", 0, nbad)) == -1)
				break;
			if ((alt = getnrmsg ("bad block? ", 0, d_size)) ==
			    -1)
				break;
			bbt[i] = alt;

			/* mark bitmap entry as bad */
			i = alt;
			if (l->dl_version == DL_V1) {
				i = (alt / l->dl_nsect) << 1;
				if ((alt % l->dl_nsect) >= (l->dl_nsect >> 1))
					i |= 1;
			}
			offset = i >> 4;
			shift = (i & 0xf) << 1;
			i = bit_map[offset];
			i &= ~(3 << shift);
			bit_map[offset] = i | (SB_BAD << shift);
			bad_modified = 1;
		}
	} else
	if (c == 'w') {
		if (!confirm ("really write bad block table? "))
			return;
		(*dsp->ds_wlabel) (bbt);
		write_bitmap();
		bad_modified = 0;
	} else
	if (c == 's') {
		bad_block_stats();
	} else
		printf ("invalid bad block command\n");
}

bad_block_stats()
{
	register int i;
	register struct disk_label *l = &disk_label;
	int total, numbad, nbad, *bbt;

	(*dsp->ds_glabel) (l, &bad_block, 0);
	if (l->dl_version == DL_V1 || l->dl_version == DL_V2) {
		bbt = l->dl_bad;
		nbad = NBAD;
	} else {
		bbt = bad_block.bad_blk;
		nbad = NBAD_BLK;
	}
	total = numbad = 0;
	for (i = 0; i < nbad && bbt[i] != -1; i++) {
		total++;
		if (bbt[i])
			numbad++;
	}
	printf ("%d/%d (%4.1f%%) alternate blocks used\n",
		numbad, total,
		(float) numbad * 100.0 / (float) total);
}

bitmap()
{
	register int i, j, k, first, num, nht, offset, shift, from, to;
	register struct disk_label *l = &disk_label;
	register int *b;
	register char c;
	float U = 0, B = 0, E = 0, W = 0, T;

	c = getrmsg ("status bitmap: read, print, edit, change, write, stats? ");
	if (c == 'r') {
		(*dsp->ds_glabel) (l, &bad_block, 0);
		d_size = l->dl_ncyl * l->dl_ntrack * l->dl_nsect;
		if (bit_map)
			free (bit_map);
		if (l->dl_version == DL_V1)
			bit_map = (int*) malloc ((d_size / l->dl_nsect) >> 1);
		else
			bit_map = (int*) malloc (d_size >> 2);
		if (ioctl (fd, DKIOCGBITMAP, bit_map) < 0)
			panic (S_NEVER, "get bitmap");
	} else
	if (c == 'p') {
		if ((b = bit_map) == 0) {
			printf ("no bitmap read in yet\n");
			return;
		}
		if (l->dl_version == DL_V1) {
			nht = (d_size / l->dl_nsect) << 1;
			first = getnrmsg ("first half track? ", 0, nht - 1);
			if (first == -1)
				return;
			num = getnrmsg ("# of half tracks? ", 0, nht - first);
		} else {
			nht = d_size;
			first = getnrmsg ("first sector? ", 0, nht - 1);
			if (first == -1)
				return;
			num = getnrmsg ("# of sectors? ", 0, nht - first);
		}
		if (num == -1)
			return;
	printf ("#\tstatus: (U=untested, B=bad, e=erased, w=written)\n");
		for (i = first, j = 0; i < first + num; i++, j++) {
			if ((j % 64) == 0)
				printf ("\n%6d\t", i);
			switch ((b[i>>4] >> ((i&0xf) << 1)) & 3) {
				case SB_UNTESTED:	c = 'U';  break;
				case SB_BAD:		c = 'B';  break;
				case SB_ERASED:		c = 'e';  break;
				case SB_WRITTEN:	c = 'w';  break;
			}
			printf ("%c", c);
		}
		printf ("\n");
	} else
	if (c == 'e') {
		if (bit_map == 0) {
			printf ("no bitmap read in yet\n");
			return;
		}
		printf ("edit bitmap -- <return> when finished\n");
		if (l->dl_version == DL_V1)
			nht = (d_size / l->dl_nsect) << 1;
		else
			nht = d_size;
		while (1) {
			if ((num = getnrmsg ("entry? ", 0, nht)) == -1)
				break;
again:
			switch (getrmsg (
				"u=untested, b=bad, e=erased, w=written? ")) {
				case 'u':	j = SB_UNTESTED;  break;
				case 'b':	j = SB_BAD;  break;
				case 'e':	j = SB_ERASED;  break;
				case 'w':	j = SB_WRITTEN;  break;
				default:	printf ("huh?\n");  goto again;
			}

			offset = num >> 4;
			shift = (num & 0xf) << 1;
			i = bit_map[offset];
			i &= ~(3 << shift);
			bit_map[offset] = i | (j << shift);
			bad_modified = 1;
		}
	} else
	if (c == 'E') {
		if (bit_map == 0) {
			printf ("no bitmap read in yet\n");
			return;
		}
		printf ("edit bitmap range\n");
		if (l->dl_version == DL_V1)
			nht = (d_size / l->dl_nsect) << 1;
		else
			nht = d_size;
		if ((from = getnrmsg ("from? ", 0, nht)) == -1)
			return;
		if ((to = getnrmsg ("to? ", 0, nht)) == -1)
			return;
again2:
		switch (getrmsg (
			"u=untested, b=bad, e=erased, w=written? ")) {
			case 'u':	j = SB_UNTESTED;  break;
			case 'b':	j = SB_BAD;  break;
			case 'e':	j = SB_ERASED;  break;
			case 'w':	j = SB_WRITTEN;  break;
			default:	printf ("huh?\n");  goto again2;
		}

		for (num = from; num < to; num++) {
			offset = num >> 4;
			shift = (num & 0xf) << 1;
			i = bit_map[offset];
			i &= ~(3 << shift);
			bit_map[offset] = i | (j << shift);
		}
		bad_modified = 1;
	} else
	if (c == 'c') {
		if (bit_map == 0) {
			printf ("no bitmap read in yet\n");
			return;
		}
		printf ("change a range of bitmap entries\n");
		if (l->dl_version == DL_V1) {
			nht = (d_size / l->dl_nsect) << 1;
			first = getnrmsg ("first half track? ", 0, nht - 1);
			if (first == -1)
				return;
			num = getnrmsg ("# of half tracks? ", 0, nht - first);
		} else {
			nht = d_size;
			first = getnrmsg ("first sector? ", 0, nht - 1);
			if (first == -1)
				return;
			num = getnrmsg ("# of sectors? ", 0, nht - first);
		}
		if (num == -1)
			return;
huh:
		switch (getrmsg (
			"u=untested, b=bad, e=erased, w=written? ")) {
			case 'u':	k = SB_UNTESTED;  break;
			case 'b':	k = SB_BAD;  break;
			case 'e':	k = SB_ERASED;  break;
			case 'w':	k = SB_WRITTEN;  break;
			default:	printf ("huh?\n");  goto huh;
		}
		for (i = first; i < first + num; i++) {
			offset = i >> 4;
			shift = (i & 0xf) << 1;
			j = bit_map[offset];
			j &= ~(3 << shift);
			bit_map[offset] = j | (k << shift);
			bad_modified = 1;
		}
	} else
	if (c == 'w') {
		if (bit_map) {
			printf ("writing bitmap\n");
			write_bitmap();
		} else
			printf ("no bitmap read in yet\n");
	} else
	if (c == 's') {
		if ((b = bit_map) == 0) {
			printf ("no bitmap read in yet\n");
			return;
		}
		if (l->dl_version == DL_V1)
			nht = (d_size / l->dl_nsect) << 1;
		else
			nht = d_size;
		for (i = 0; i < nht; i++) {
			switch ((b[i>>4] >> ((i&0xf) << 1)) & 3) {
				case SB_UNTESTED:	U++;  break;
				case SB_BAD:		B++;  break;
				case SB_ERASED:		E++;  break;
				case SB_WRITTEN:	W++;  break;
			}
		}
		T = (U + B + E + W) / 100.0;
		printf ("%4.1f%% untested, %4.1f%% bad, %4.1f%% erased, "
			"%4.1f%% written\n", U/T, B/T, E/T, W/T);
	}
	else
		printf ("invalid status bitmap command\n");
}

write_bitmap() {
	register int e;

	if ((e = ioctl (fd, DKIOCSBITMAP, bit_map)) < 0)
		panic (S_NEVER, "set bitmap");
}

Read()
{
	register int blk, len, inc, i, incr;
	int ms, bytes, secnt, blkno, bcount;
	int size = dt->d_ntracks * dt->d_nsectors * dt->d_ncylinders;
	register struct disk_label *l = &disk_label;
	u_char *rb = (u_char*)(((int)test_rbuf+16)/16*16);

	blkno = blk = getnrmsg ("starting block? ", 0, size-1);
	secnt = getnrmsg ("# sectors per transfer? ", 1,
		TBSIZE / dt->d_secsize);
	len = getnrmsg ("number of transfers? ", 1, (size - blk) / secnt);
	incr = inc = getnrmsg ("sector increment? ", -size, size);
	bcount = dt->d_secsize * secnt;
	gettimeofday (&start, 0);
	bytes = 0;
	for (i = 0; i < len && setjmp (env) == 0; i++) {
		if ((*dsp->ds_req) (CMD_READ, blkno, rb, bcount, 0) < 0 &&
		    abort_flag)
			panic (S_NEVER, "read");
		blkno += incr;
		if (inc < 0)
			incr = -incr;
		bytes += bcount;
	}
	gettimeofday (&stop, 0);
	timevalsub (&stop, &start);
	ms = stop.tv_sec * 1000 + stop.tv_usec / 1000;
	if (ms == 0)
		ms = 1;
	printf ("%d bytes in %d ms = %u bytes/s\n",
		bytes, ms, (unsigned) ((bytes * 100) / (ms / 10)));
}

Write()
{
	register int blk, len, inc, i, j, incr;
	int size = dt->d_ntracks * dt->d_nsectors * dt->d_ncylinders;
	int ms, bytes, rand = 0, secnt, blkno, bcount;
	register struct disk_label *l = &disk_label;
	register u_char *wb = (u_char*)(((int)test_wbuf+16)/16*16);

	blkno = blk = getnrmsg ("starting block? ", 0, size-1);
	secnt = getnrmsg ("# sectors per transfer? ", 1,
		TBSIZE / dt->d_secsize);
	len = getnrmsg ("number of transfers? ", 1, (size - blk) / secnt);
	incr = inc = getnrmsg ("sector increment? ", -size, size);
	rand = confirm ("random data? ");
	bcount = dt->d_secsize * secnt;
	gettimeofday (&start, 0);
	bytes = 0;
	for (i = 0; i < len && setjmp (env) == 0; i++) {
		if (rand)
			for (j = 0; j < dt->d_secsize; j++)
				wb[i] = ri(0, 255);
		if ((*dsp->ds_req) (CMD_WRITE, blkno, wb, bcount, 0) < 0 &&
		    abort_flag)
			panic (S_NEVER, "write");
		blkno += incr;
		if (inc < 0)
			incr = -incr;
		bytes += bcount;
	}
	gettimeofday (&stop, 0);
	timevalsub (&stop, &start);
	ms = stop.tv_sec * 1000 + stop.tv_usec / 1000;
	if (ms == 0)
		ms = 1;
	printf ("%d bytes in %d ms = %u bytes/s\n",
		bytes, ms, (unsigned) ((bytes * 100) / (ms / 10)));
}

verify()
{
	register int blk, len, inc, incr;
	int size = dt->d_ntracks * dt->d_nsectors * dt->d_ncylinders;
	register struct disk_label *l = &disk_label;
	u_char *rb = (u_char*)(((int)test_rbuf+16)/16*16);
	int blkno, bcount, secnt;

	blkno = blk = getnrmsg ("starting block? ", 0, size-1);
	secnt = getnrmsg ("# sectors per transfer? ", 1,
		TBSIZE / dt->d_secsize);
	len = getnrmsg ("number of transfers? ", 1, (size - blk) / secnt);
	incr = inc = getnrmsg ("sector increment? ", -size, size);
	bcount = dt->d_secsize * secnt;
	while (blkno < blk + len) {
		if ((*dsp->ds_req) (CMD_VERIFY, blkno, rb, bcount, 0) < 0 &&
		    abort_flag)
			panic (S_NEVER, "verify");
		blkno += incr;
		if (inc < 0)
			incr = -incr;
	}
}

rw()
{
	register int blk, len, inc, i, incr;
	int size = dt->d_ntracks * dt->d_nsectors * dt->d_ncylinders;
	int rand = 0, blkno, bcount, cmp, secnt, err;
	register struct disk_label *l = &disk_label;
	register u_char *rb = (u_char*)(((int)test_rbuf+16)/16*16);
	register u_char *wb = (u_char*)(((int)test_wbuf+16)/16*16);
	register u_char *bp, *bp2;

	blkno = blk = getnrmsg ("starting block? ", 0, size-1);
	secnt = getnrmsg ("# sectors per transfer? ", 1,
		TBSIZE / dt->d_secsize);
	len = getnrmsg ("number of transfers? ", 1, (size - blk) / secnt);
	incr = inc = getnrmsg ("sector increment? ", -size, size);
	rand = confirm ("random data? ");
	cmp = confirm ("compare? ");
	bcount = dt->d_secsize * secnt;
	while (blkno < blk + len) {
		if (rand)
			for (i = 0; i < bcount; i++)
				wb[i] = ri(0, 255);
		if ((*dsp->ds_req) (CMD_WRITE, blkno, wb, bcount, 0) < 0 &&
		    abort_flag)
			panic (S_NEVER, "write");
		if ((*dsp->ds_req) (CMD_READ, blkno, rb, bcount, 0) < 0 &&
		    abort_flag)
			panic (S_NEVER, "read");
		if (cmp) {
			i = 0;  err = 0;
			for (bp = rb, bp2 = wb; bp < &rb[bcount];
			    bp++, bp2++, i++)
				if (*bp != *bp2) {
					log_msg ("1: R%02xW%02xX%02x@%d|%d ",
						*bp, *bp2, *bp ^ *bp2,
						blkno, i);
					err = 1;
				}
			if (err) {
				i = 0;
				for (bp = rb, bp2 = wb; bp < &rb[bcount];
				    bp++, bp2++, i++)
					if (*bp != *bp2) {
						log_msg ("2: R%02xW%02xX%02x@%d|%d ",
							*bp, *bp2, *bp ^ *bp2,
							blkno, i);
					}
			}
			if (err && abort_flag)
				return;
		}
		blkno += incr;
		if (inc < 0)
			incr = -incr;
	}
}

rwr()
{
	register int blk, i;
	int size = dt->d_ntracks * dt->d_nsectors * dt->d_ncylinders;
	int delta = 0, blkno, bcount, secnt, r = 0, w = 0;
	register struct disk_label *l = &disk_label;
	register u_char *rb = (u_char*)(((int)test_rbuf+16)/16*16);
	register u_char *wb = (u_char*)(((int)test_wbuf+16)/16*16);
	register u_char *bp, *bp2;

	blkno = blk = getnrmsg ("starting block? ", 0, size-1);
	secnt = getnrmsg ("# sectors per transfer? ", 1,
		TBSIZE / dt->d_secsize);
	delta = getnrmsg ("sector delta? ", 0, size);
	r = confirm ("read? ");
	w = confirm ("write? ");
	bcount = dt->d_secsize * secnt;
	while (1) {
		if (w && (*dsp->ds_req) (CMD_WRITE, blkno, wb, bcount, 0) < 0 &&
		    abort_flag)
			panic (S_NEVER, "write");
		if (r && (*dsp->ds_req) (CMD_READ, blkno, rb, bcount, 0) < 0 &&
		    abort_flag)
			panic (S_NEVER, "read");
		blkno += (ri (0, 2 * delta) - delta) * secnt;
		if (blkno > blk + delta * secnt ||
		    blkno < blk - delta * secnt)
			blkno = blk;
	}
}

#define	LOOK_W	24

look()
{
	register int i, j, off, len, stop;
	register u_char b;
	u_char *rb = (u_char*)(((int)test_rbuf+16)/16*16),
		*wb = (u_char*)(((int)test_wbuf+16)/16*16), *buf;

	buf = getrmsg ("read or write buffer? ") == 'r'? rb : wb;
	off = getnrmsg ("offset? ", 0, TBSIZE - 1);
	len = getnrmsg ("length? ", 1, TBSIZE - off);
	stop = (off + len + LOOK_W) / LOOK_W * LOOK_W;
	for (i = off; i < stop; i += LOOK_W) {
		printf ("\n%4d\t", i);
		for (j = i; j < i + LOOK_W; j++)
			printf ("%02x ", buf[j]);
		printf ("    ");
		for (j = i; j < i + LOOK_W; j++) {
			b = buf[j];
			printf ("%c", (b < ' ' || b > 0x7e)? '.' : b);
		}
	}
	printf ("\n");
}

set()
{
	register int i, off, len, rand = 0, val;
	u_char *rb = (u_char*)(((int)test_rbuf+16)/16*16),
		*wb = (u_char*)(((int)test_wbuf+16)/16*16), *buf;

	buf = getrmsg ("read or write buffer? ") == 'r'? rb : wb;
	off = getnrmsg ("offset? ", 0, TBSIZE - 1);
	len = getnrmsg ("length? ", 1, TBSIZE - off);
	if (getrmsg ("random, <constant>? ") == 'r')
		rand = 1;
	else
		val = atoi (line);
	for (i = off; i < off + len; i++)
		buf[i] = rand? ri(0, 255) : val;
}

eject() {
	if(ioctl(fd, DKIOCEJECT, 0))
		panic (S_NEVER, "eject");
}

tabort() {
	abort_flag ^= 1;
	printf ("abort on error mode %s\n", abort_flag? "on" : "off");
}

tvers() {
	version = (version == DL_V2)? DL_V3 : DL_V2;
	printf ("label version #%d\n", version == DL_V2? 2 : 3);
}

pstats() {
	register struct disk_stats *s = &stats;

	if (ioctl (fd, DKIOCGSTATS, s) < 0)
		panic (S_NEVER, "getstats");
	printf ("disk statistics:\n");
	printf ("\t%7d average ECC corrections per transfer\n", s->s_ecccnt);
	printf ("\t%7d maximum ECC corrections per transfer\n", s->s_maxecc);
	(*dsp->ds_pstats)();
	bad_block_stats();
}

zstats() {
	if (ioctl (fd, DKIOCZSTATS) < 0)
		panic (S_NEVER, "zerostats");
}

sigint() {
	longjmp (env, 1);
}

quit() {
	exit (0);
	closelog();
}

getn() {
	register int i;
	char resp[16];

	gets (resp);
	if (resp[0] == 0)
		return (-1);
	return (atoi (resp));
}

getnrmsg (msg, lo, hi)
	register char *msg;
	register int lo, hi;
{
	register int i;

retry:
	printf (msg);
 	if ((i = getn()) == -1)
		return (-1);
	if (i < lo || i > hi) {
		printf ("must be between %d and %d\n", lo, hi);
		goto retry;
	}
}

getrmsg (msg)
	register char *msg;
{
	register int i;

	printf (msg);
	gets (line);
	return ((int) line[0]);
}

confirm (msg)
	register char *msg;
{
	char resp[16];

	if (interactive == 0)
		return (1);
	printf (msg);
	if (strncmp (gets (resp), "y", 1) == 0)
		return (1);
	return (0);
}

panic (status, msg)
	char *msg;
{
	perror (msg);
	if (interactive)
		longjmp (env, 1);
	else {
		exit (status);
		closelog();
	}
}

bomb (status, msg, p1, p2, p3, p4)
	char *msg;
{
	printf (msg, p1, p2, p3, p4);
	printf ("\n");
	if (interactive)
		longjmp (env, 1);
	else {
		exit (status);
		closelog();
	}
}

log_msg (msg, p1, p2, p3, p4, p5, p6, p7, p8, p9)
	char *msg;
{
	printf (msg, p1, p2, p3, p4, p5, p6, p7, p8, p9);
	syslog (LOG_ERR, msg, p1, p2, p3, p4, p5, p6, p7, p8, p9);
}

timevalsub(t1, t2)
	struct timeval *t1, *t2;
{

	t1->tv_sec -= t2->tv_sec;
	t1->tv_usec -= t2->tv_usec;
	timevalfix(t1);
}

timevalfix(t1)
	struct timeval *t1;
{

	if (t1->tv_usec < 0) {
		t1->tv_sec--;
		t1->tv_usec += 1000000;
	}
	if (t1->tv_usec >= 1000000) {
		t1->tv_sec++;
		t1->tv_usec -= 1000000;
	}
}

struct dtype_sw *
drive_type(type)
char *type;
{
	struct dtype_sw *dsp;

	for (dsp = dtypes; dsp->ds_type; dsp++)
		if (strcmp(type, dsp->ds_type) == 0)
			return(dsp);
	return(NULL);
}






