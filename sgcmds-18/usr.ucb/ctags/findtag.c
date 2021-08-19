#include <stdio.h>
#include "tags.h"

FindTag(tag, entry, tagpath)
	char *tag, *entry, *tagpath;
/*
** Lookup 'tag' and put the first tag entry (if any) in 'entry'.
** Return true if tag was found.
** Each file in 'tagpath' is searched.
*/
{
	char path[MAX];
	FILE *f;
	char *name, *c;

	if null(tag) return 0;
	strcpy(path, tagpath);
	for (c=path-1; c;) {
		name = ++c;
		while (*c && *c != ' ' && *c != ':') c++;
		if (!*c) c = 0;
		else *c = '\0';
		if ((f=fopen(name, "r")) && SearchTagsFile(f,tag,entry)){
			fclose(f);
			return 1;
		}
		fclose(f);
	}
	return 0;
}
