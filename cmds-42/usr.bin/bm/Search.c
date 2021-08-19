#include "bm.h"
#include "Extern.h"
int Search(Pattern,PatLen,EndBuff, Skip1, Skip2, Desc)
register char Pattern[];
int PatLen;
char *EndBuff;
unsigned short int Skip1[], Skip2[];
struct PattDesc *Desc;
{
	register char *k, /* indexes text */
		*j; /* indexes Pattern */
	register int Skip; /* skip distance */
	char *PatEnd,
	*BuffEnd; /* pointers to last char in Pattern and Buffer */
	BuffEnd = EndBuff;
	PatEnd = Pattern + PatLen - 1;

	k = Desc->Start;
	Skip = PatLen-1;
	while ( Skip <= (BuffEnd - k) ) {
		j = PatEnd;
		k = k + Skip;
		while (*j == *k) {
			if (j == Pattern) {
				/* found it. Start next search
				* just after the pattern */
				Desc -> Start = k + Desc->PatLen;
				return(1);
			} /* if */
			--j; --k;
		} /* while */
		Skip = max(Skip1[*(unsigned char *)k],Skip2[j-Pattern]);
	} /* while */
	Desc->Start = k+Skip-(PatLen-1);
	return(0);
} /* Search */
