#include <stdio.h>
#include "dbUtil.h"

void
_dbPrintD(dhead *d, FILE *fp, int verbose)
{
	fputc('\t', fp);
	if (verbose)
		fprintf(fp, "magic=%x, depth=%d, ", d->magic, d->depth);

	fprintf(fp, "leaf size=%d, leaves=%ld", d->llen, d->dlcnt);
	if (d->flag & dbFlagReadOnly)
		fprintf(fp, " -- read only");

	if (d->flag & dbFlagCompressed)
		fprintf(fp, " -- compressed");

	fputc('\n', fp);
}

