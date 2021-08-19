#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include "tags.h"

int Status, Quiet;
FILE *Errs = stdout;
Error(a,b,c,d){ if (!Quiet) fprintf(Errs,a,b,c,d),fputc('\n',Errs); Status=1; }
#define min(a,b) (a<b)? a : b

static
cmp(s,t)
	Char *s,*t;
{
	for(;*s==*t;s++,t++) if(!*s) return 0;
	return !*s? -1:
	       !*t?  1:
	       *s<*t? -2:
		     2;
}

static char *
binarySearch(f,word,s,s_len)
	FILE *f;
	Char *word, *s;
{
	Char c;
	long mid, top, bot=0, ftell();
    	char sb[512];
	
	fseek(f,0L,2);
	top = ftell(f);
	for(;;){
		mid = (top+bot)/2;
		fseek(f,mid,0);
		do mid++, c=getc(f); while(c!= EOF && c != '\n');
		if (!fgets(s,s_len,f)) break;
		switch(cmp(word,s)){
		case 1: case 2: bot=mid; continue;
		case -2: case -1: case 0:
			if (top<=mid) break;
			top = mid;
			continue;
		}
		break;
	}
	fseek(f,bot,0);
	while(ftell(f)<top){
		if (!fgets(s,s_len,f)) return (char *)0;
		switch (cmp(word,s)){
		case 1: case 2: continue;
		case 0: case -1: return s;
		case -2: return (char *)0;
		}
		break;
	}
	return fgets(s,s_len,f) && ((top=cmp(word,s))==0 || top== -1L)? s : 0;
}

SearchTagsFile(f,tag,l)
	FILE *f;
	char *tag, *l;
/*
 * Binary search for 'tag' in 'f'.
 * If found, return 1 and copy the entry (the line, not including the 'tag'
 * or the trailing '\\n') into 'l'; else return 0.
 * If there are multiple occurrences of 'tag' the first is returned.
 * The file 'f' is left pointing to the next tag.
 */
{
	char s[MAX];
	Char *p, *lp = l;
	*l = '\0';
	if (binarySearch(f,tag,s,sizeof s)){
		for (p=s; *p && !(*p=='\t'||*p==' '); ++p) ;
		while (*p == '\t' || *p == ' ') ++p;
		while ((*l = *p) && *l != '\n') ++l, ++p;
		*l = '\0';
	}
	return *lp? 1 : 0;
}
