/*
 * telebit.c - A takeoff of hayes.c, but without the Hayes dependancies.
 *
 */

#include "../condevs.h"
#include <sys/file.h>

#ifdef TELEBIT

/*
 *	tbtpopn(telno, flds, dev) connect to Telebit (pulse call)
 *	tbttopn(telno, flds, dev) connect to Telebit (tone call)
 *	char *flds[], *dev[];
 *
 *	return codes:
 *		>0  -  file number  -  ok
 *		CF_DIAL,CF_DEVICE  -  failed
 */

tbtpopn(telno, flds, dev)
char *telno, *flds[];
struct Devices *dev;
{
	return tbtopn(telno, flds, dev, 0);
}

tbttopn(telno, flds, dev)
char *telno, *flds[];
struct Devices *dev;
{
	return tbtopn(telno, flds, dev, 1);
}

/* ARGSUSED */
tbtopn(telno, flds, dev, toneflag)
char *telno;
char *flds[];
struct Devices *dev;
int toneflag;
{
	extern errno;
	char dcname[20];
	char cbuf[MAXPH];
	register char *cp;
	register int i;
	int dh = -1, nrings = 0;
	int readflag = FREAD;

	sprintf(dcname, "/dev/%s", dev->D_line);
	DEBUG(4, "dc - %s\n", dcname);
	if (setjmp(Sjbuf)) {
		logent(dcname, "TIMEOUT");
		if (dh >= 0)
			tbtcls(dh);
		return CF_DIAL;
	}
	signal(SIGALRM, alarmtr);
	getnextfd();
	alarm(10);
	dh = open(dcname, 2); /* read/write */
	alarm(0);

	/* modem is open */
	next_fd = -1;
	if (dh >= 0) {
		fixline(dh, dev->D_speed);
		if (dochat(dev, flds, dh)) {
			logent(dcname, "CHAT FAILED");
			tbtcls(dh);
			return CF_DIAL;
		}
		/* Output one AT to let the modem autobaud */
		write(dh, "AT\r", 3);
		sleep(1);
		ioctl(dh, TIOCFLUSH, &readflag);
		/* Initialize the way the modem responds to us */
		write(dh, "ATV1E0H\r", 8);
		if (expect("OK\r\n", dh) != 0) {
			logent(dcname, "Trailblazer seems dead");
			tbtcls(dh);
			return CF_DIAL;
		}
		write(dh, "ATX1S7=44\r", 10);
		if (expect("OK\r\n", dh) != 0) {
			logent(dcname, "Trailblazer seems dead");
			tbtcls(dh);
			return CF_DIAL;
		}
		if (toneflag)
			write(dh, "\rATDT", 5);
		else
			write(dh, "\rATDP", 5);
		write(dh, telno, strlen(telno));
		write(dh, "\r", 1);

		if (setjmp(Sjbuf)) {
			logent(dcname, "TIMEOUT");
			strcpy(devSel, dev->D_line);
			tbtcls(dh);
			return CF_DIAL;
		}
		signal(SIGALRM, alarmtr);
		alarm(2*MAXMSGTIME);
		do {
			cp = cbuf;
			while (read(dh, cp ,1) == 1)
				if (*cp >= ' ')
					break;
			while (++cp < &cbuf[MAXPH] && read(dh, cp, 1) == 1 && *cp != '\n')
				;
			alarm(0);
			*cp-- = '\0';
			if (*cp == '\r')
				*cp = '\0';
			DEBUG(4,"\nGOT: %s", cbuf);
			alarm(MAXMSGTIME);
		} while (strncmp(cbuf, "RING", 4) == 0 && nrings++ < 5);
		if (strncmp(cbuf, "CONNECT", 7) != 0) {
			logent(cbuf, _FAILED);
			strcpy(devSel, dev->D_line);
			tbtcls(dh);
			return CF_DIAL;
		}
		if (strncmp(&cbuf[9], "FAST", 4) == 0) {
			/* Connected in PEP mode */
			if (dev->D_speed != 9600) {
				DEBUG(4,"Baudrate reset to 9600\n", 0);
				fixline(dh, 9600);
			}
		} else {
			i = atoi(&cbuf[8]);
			if (i > 0 && i != dev->D_speed) {	
				DEBUG(4,"Baudrate reset to %d\n", i);
				fixline(dh, i);
			}
		}

	}
	if (dh < 0) {
		logent(dcname, "CAN'T OPEN");
		return dh;
	}
	DEBUG(4, "telebit ok\n", CNULL);
	return dh;
}

tbtcls(fd)
int fd;
{
	char dcname[20];

	if (fd > 0) {
		sprintf(dcname, "/dev/%s", devSel);
		DEBUG(4, "Hanging up fd = %d\n", fd);
		sleep(3);
		write(fd, "+++", 3);
		sleep(3);
		write(fd, "ATZ\r", 4);
		if (expect("OK",fd) != 0)
			logent(devSel, "Trailblazer did not respond to ATZ");
		write(fd, "ATH\r", 4);
		sleep(1);
		close(fd);
		delock(devSel);
	}
}
#endif TELEBIT
