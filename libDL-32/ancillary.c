#import <stdio.h>
#import <libc.h>
#import <string.h>
#import "ancillary.h"

/*
 * ancillaryPath(argv0, apath, apathsize)
 *
 * When called:
 * 	ancillaryPath(".../MyApp.app/MyApp", apath, sizeof(apath))
 * Returns apath filled with:
 * 	".../MyApp.app/
 * Errors:
 * 	returns NULL if no slash in path or ancillary_path too long
 * 	to fit in apath
 */
char *
ancillaryPath(char *argv0, char *apath, unsigned apathsize)
{
	char           *sp0;

	if (strlen(argv0) > apathsize)
		return (NULL);
	strcpy(apath, argv0);
	sp0 = rindex(apath, '/');
	if (sp0 == NULL)	/* no / in path */
		return (NULL);

	*(sp0++) = '\0';
	if (!strcmp(apath, ".")) apath = getwd(apath);
	return apath;
}
