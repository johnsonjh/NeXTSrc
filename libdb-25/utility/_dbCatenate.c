#include <stdio.h>
#include "dbUtil.h"

void
_dbCatenate(FILE *fp, Data *d)
{
	register char *fmt;
	register int i;
	extern int hexFlag;

	fmt = (hexFlag) ? "%.2x " : "%c";
	for(i = 0; i < d->k.n; i++)
		fprintf(fp, fmt, (int) d->k.s[i]);

	fprintf(fp, (hexFlag) ? "| " : " | ");
	for(i = 0; i < d->c.n; i++)
		fprintf(fp, fmt, (int) d->c.s[i]);

	fprintf(fp, "\n");
}


