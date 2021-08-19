#include <stdio.h>
#include "bm.h"
#include "Extern.h"
Execute(DescVec, NPats, TextFile, Buffer)
register struct PattDesc *DescVec[];
/* pointers to status vectors for the different
	* patterns, including skip tables, position in buffer, etc. */
register int NPats; /* number of patterns */
register char Buffer[]; /* holds text from file */
register int TextFile; /* file to search */
{
	int NRead, /* number of chars read from file */
		NWanted, /* number of chars wanted */
		NAvail, /* number of chars actually read */
		BuffSize, /* number of chars in buffer */
		BuffPos, /* offset of first char in Buffer in TextFile */
		BuffEx, /* flag to indicate that buffer has been searched */
		ResSize,
		/* number of characters in the last, incomplete line */
		Found, /* flag indicates whether pattern found
		* completely and all matches printed */
		Valid; /* was the match "valid", i.e. if -x used,
		* did the whole line match? */
	char *BuffEnd; /* pointer to last char of last complete line */

	/* misc working variables */
	int i;

	/* initialize */
	ResSize = 0;
	Found = 0;
	BuffPos = 0;
	for (i=0; i < NPats; i++) {
		DescVec[i] -> Success = 0;
		DescVec[i] -> Start = Buffer;
	} /* for */
	/* now do the searching */
	do {
		/* first, read a bufferfull and set up the variables */
		NWanted = MAXBUFF - ResSize; NRead = 0;
		do {
			NAvail =
			   read(TextFile,Buffer + ResSize + NRead, NWanted);
			if (NAvail == -1) {
				fprintf(stderr,
				  "bm: error reading from input file\n");
				exit(2);
			} /* if */
			NRead += NAvail; NWanted -= NAvail;
		} while (NAvail && NWanted);
		BuffEx = 0;
		BuffSize = ResSize + NRead;
		BuffEnd = Buffer + BuffSize - 1;
		/* locate the end of the last complete line */
		while (*BuffEnd != '\n' && BuffEnd >= Buffer)
			--BuffEnd;
		if (BuffEnd < Buffer)
			BuffEnd = Buffer + BuffSize - 1;
		while (!BuffEx) { /* work through one buffer full */
			BuffEx = 1; /* set it provisionally, then clear
			* it if we find the buffer non-empty */
			for (i=0; i< NPats; i++) {
				if (!DescVec[i]->Success)
				/* if the pattern  has not been found */
					DescVec[i]-> Success =
					Search(DescVec[i]->Pattern,
					DescVec[i]->PatLen, BuffEnd,
					DescVec[i]->Skip1, DescVec[i]->Skip2,
					DescVec[i]);
				if (DescVec[i]->Success){
				/* if a match occurred */
					BuffEx = 0;
					Valid = MatchFound(DescVec[i],BuffPos,
					Buffer, BuffEnd);
					Found |= Valid;
					if ((sFlag || lFlag) && Found)
						return(0);
				} /* if */
			} /* for */
		} /* while */
		if(NRead) {
			ResSize = MoveResidue(DescVec,NPats,Buffer,
			Buffer+BuffSize-1);
			BuffPos += BuffSize - ResSize;
		} /* if */
	} while (NRead);
	return(!Found);
} /* Execute */
