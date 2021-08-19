#import <stdio.h>
#import <libc.h>
#import <text/text.h>
#import <findFile.h>
#import <sys/file.h>
#import <libc.h>

char *
findFileFromPath(char *startPath, char *file)
{
	char path[500];

	if (!startPath || !*startPath || startPath[1] == '\0') return NULL;

	sprintf(path, "%s/%s", startPath, file);
	if (access(path, R_OK) == 0) return strsave(path);

	return findFileFromPath(parentname(startPath), file);
}
