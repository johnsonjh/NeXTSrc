/*	@(#)disk.h	1.0	08/28/87	(c) 1987 NeXT	*/

/* 
 * HISTORY
 * 17-Feb-89	Doug Mitchell at NeXT
 *	Added exec_time for scsi_req support.
 */
 
/* commands */
#define	CMD_SEEK	0
#define	CMD_READ	1
#define	CMD_WRITE	2
#define	CMD_VERIFY	3
#define	CMD_EJECT	4
#define	CMD_RESPIN	5
#define	CMD_ERASE	6

/* flags */
#define	SPEC_RETRY	1

/* errors */
#define	ERR_UNKNOWN	0
#define	ERR_ECC		1

/* return status */
#define	S_NEVER		1	/* should "never" happen */
#define	S_EMPTY		2	/* no media inserted */
#define	S_MEDIA		3	/* media is bad (can't write label) */

struct dtype_sw {
	char	*ds_type;	/* drive "type" from disktab "ty" entry */
	int	ds_typemask;	/* or of TY_* bits */
	int	(*ds_config)();	/* device configuration */
	int	(*ds_devinit)();/* complete device initialization */
	int	(*ds_req)();	/* request device specific cmd */
	int	(*ds_geterr)();	/* interpret command errors */
	int	(*ds_wlabel)();	/* write label */
	int	(*ds_cyl)();	/* compute max cylinder */
	int	(*ds_pstats)();	/* print device specific stats */
	int	(*ds_ecc)();	/* current ECC count */
	int	(*ds_glabel)();	/* get label */
	int 	(*ds_format)();	/* format volume if necessary */
	int	ds_basecyl;	/* base of active media area (cylinders) */
	int	ds_base;	/* base of active media area (sectors) */
	int	ds_dcyls;	/* display cylinders */
	int	ds_icyls;	/* increment cylinders */
	int	ds_maxcyl;	/* max cylinder */
};

struct	disk_label disk_label;
struct	bad_block bad_block;
struct	disk_req req;
struct	dtype_sw *dsp;
struct	disktab disktab, *dt;
struct	disk_stats stats;
struct 	timeval exec_time;
int	*bit_map, fd, d_size, devblklen;

