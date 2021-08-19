#include <stdio.h>
#ifdef sun
#include <strings.h>
#else
#include <string.h>
#endif sun
#include "Extern.h"
PrintLine(OffSet,LineStart,LineEnd)
int OffSet; /* offset of LineStart from beginning of file */
char *LineStart,
	*LineEnd;
{
	char OffStr[80];
	if (lFlag) {
		if (strlen(FileName) > 76) {
			fprintf(stderr,"bm: filename too long\n");
			exit(2);
		} /* if */
		if (strlen(FileName)) {
			sprintf(OffStr,"%s\n",FileName);
			write(1,OffStr,strlen(OffStr));
		} /* if */
		return;
	} /* if */
	if (FileName && !hFlag) {
		if (strlen(FileName) > 76) {
			fprintf(stderr,"bm: filename too long\n");
			exit(2);
		} /* if */
		sprintf(OffStr,"%s:",FileName);
		write(1,OffStr,strlen(OffStr));
	} /* if */
	if (nFlag) {
		sprintf(OffStr,"%d: ",OffSet);
		write(1,OffStr,strlen(OffStr));
	} /* if */
	write(1,LineStart,LineEnd-LineStart+1); 
	if (*LineEnd != '\n') write (1,"\n",1);
 } /* PrintLine */
