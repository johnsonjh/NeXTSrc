#include "bm.h"
#ifdef sun
#include <strings.h>
#else
#include <string.h>
#endif sun
/* scan a newline-separated string of patterns and set up the
* vector of descriptors, one pattern descriptor per pattern. 
* Return the number of patterns */
int
MkDescVec(DescVec, Pats)
struct PattDesc *DescVec[];
char *Pats;
{
	int NPats = 0;
	char *EndPat;
	extern struct PattDesc *MakeDesc();
/* some systems use "strchr" instead of "index" */
#ifdef sun
	while (*Pats && (EndPat = index(Pats,'\n')) && NPats < MAXPATS)
#else
	while (*Pats && (EndPat = strchr(Pats,'\n')) && NPats < MAXPATS)
#endif
	{
		*EndPat = '\0';
		DescVec[NPats] = MakeDesc(Pats);
		Pats = EndPat + 1;
		++NPats;
	} /* while */
	if (*Pats && NPats < MAXPATS) {
		DescVec[NPats] = MakeDesc(Pats);
		++NPats;
	} /* if */
	return(NPats);
} /* MkDescVec */
