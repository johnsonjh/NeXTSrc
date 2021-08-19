#ifndef lint
static char sccsid[] = "@(#)micro.c	1.1 (Berkeley) 1/13/86";
#endif !lint

#include "../condevs.h"

#ifdef MICROCOM
/*
 *	microopn(telno, flds, dev) connect to Microcom QX/V.32c
 *	char *flds[], *dev[];
 *
 *	return codes:
 *		>0  -  file number  -  ok
 *		CF_DIAL,CF_DEVICE  -  failed
 */

static	char dcname[20];

/* ARGSUSED */
microopn(telno, flds, dev)
char *telno, *flds[];
struct Devices *dev;
{
	int	dh = -1;
	char *ii;
	extern errno;

	sprintf(dcname, "/dev/%s", dev->D_line);
	DEBUG(4, "dc - %s\n", dcname);
	if (setjmp(Sjbuf)) {
		logent(dcname, "TIMEOUT");
		if (dh >= 0)
			microcls(dh);
		return CF_DIAL;
	}
	signal(SIGALRM, alarmtr);
	getnextfd();
	alarm(10);
	dh = open(dcname, 2); /* read/write */
	alarm(0);

	for (ii = telno; *ii; ii++)
		if (*ii == '=')
		    *ii = ',';

	/* modem is open */
	next_fd = -1;
	if (dh >= 0) {
		fixline(dh, dev->D_speed);
		write(dh, "\r\r", 2);
		sleep(1);
		write(dh, "ATZ\r", 4);
		sleep(1);
		write(dh, "ATQ0E1\r", 7);
		if (expect("OK\r\n", dh) != 0) {
			logent(dcname, "Micro not responding OK");
			microcls(dh);
			return CF_DIAL;
		}
		if (dochat(dev, flds, dh)) {
			logent(dcname, "CHAT FAILED");
			microcls(dh);
			return CF_DIAL;
		}
		write(dh, "\rATDT", 5);
		write(dh, telno, strlen(telno));
		write(dh, "\r", 1);

		if (expect("CONNECT", dh) != 0) {
			logent("Micro no carrier", _FAILED);
			strcpy(devSel, dev->D_line);
			microcls(dh);
			return CF_DIAL;
		}

	}
	if (dh < 0) {
		logent(dcname, "CAN'T OPEN");
		return dh;
	}
	DEBUG(4, "hayes ok\n", CNULL);
	return dh;
}

microcls(fd)
int fd;
{
	char dcname[20];

	if (fd > 0) {
		sprintf(dcname, "/dev/%s", devSel);
		DEBUG(4, "Hanging up fd = %d\n", fd);
		sleep(1);
/*
 * Since we have a getty sleeping on this line, when it wakes up it sends
 * all kinds of garbage to the modem.  Unfortunatly, the modem likes to
 * execute the previous command when it sees the garbage.  The previous
 * command was to dial the phone, so let's make the last command reset
 * the modem.
 */
		write(fd, "\r+++", 4);
		sleep(2);
		write(fd, "\rATH0\r", 6);
		sleep(1);
		write(fd, "\rATZ\r", 5);
		close(fd);
		sleep(1);
		fd = open(dcname, 2); /* read/write */
		sleep(2);
		close(fd); /* read/write */
		delock(devSel);
	}
}
#endif MICROCOM
