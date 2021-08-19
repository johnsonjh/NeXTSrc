#include <stdio.h>
#include "tags.h"
#ifdef	NeXT_MOD
#define loop(i,j) for(i=0;i<(j);i++)
#else	NeXT_MOD
#d loop(i,j) for(i=0;i<(j);i++)
#endif	NeXT_MOD

extern int Status, Quiet;
char tagpath[MAX];

	/* flags tell what part of tag entry to print */
int NumColumns = 80;	/* fold and indent lines at NumColumns */
int CurColumn = 1;	/* current column */
int CurLine = 0;	/* current line # */
char *UpArrow = "^";	/* print "^" after previously listed names */
int pNewline = 0;	/* print newline/indent after every object */
int Verbose = 1;	/* if true, print type string after object */
int Level = 0;		/* indentation level */
int MaxLevel = 100;	/* maximum recursion level */
char *Indent = "   ";	/* indentation string */
int NoMacros, NoTypedefs, NoFunctions, NoGlobals, NoStructs;

ParseTag(entry, type, callee)
	char *entry, *type, *callee;
/* Read from the tag entry the 'type' and the 'callee' list.  */
{
#ifdef	NeXT_MOD
#define notequal(a,b) strncmp(a,b,4)
#else	NeXT_MOD
#d notequal(a,b) strncmp(a,b,4)
#endif	NeXT_MOD
	while (*entry && notequal(entry, " ;\" ")) ++entry;
	if (*entry) entry += 4;
	while (*entry && !sp(*entry))
		*type++ = *entry++;
	*type = '\0';

	while (*entry && notequal(entry, "}	{ ")) ++entry;
	if (*entry) entry += 4;
	while (*entry && *entry != '}')
		*callee++ = *entry++;
	*callee = '\0';
}

char *sprintf();

newline()
/* Print a newline, and indent according to the current 'Level'.  */
{
	Int i;
	putchar('\n');
	loop(i,Level) printf("%s",Indent);
	CurColumn = Level*strlen(Indent)+1;
	++CurLine;
}

print(s,nl)
	Char *s;
/*
** Print 's', increment the 'CurColumn' (and wrap if necessary).
** If didn't wrap, print a space after 's'.
*/
{
	CurColumn += strlen(s)+1;	/* +1 for putchar, below */
	if (CurColumn > NumColumns)
		newline();
	printf("%s",s);
	if (pNewline) { if (nl) newline(); }
	else putchar(' ');
}

char *
Type(type)
	char *type;
/*
** If 'Verbose', return a string corresponding to 'type';
** e.g., if 'type' is `function`, return '()',
** if 'type' is `macro` return '#', etc.
*/
{
	if (Verbose) switch (*type) {
	Case 'f': return "()";
	Case 'm': return "#";
	}
	return "";
}

NotPrinting(type)
	char *type;
{
	switch (*type) {
	Case 'f': return NoFunctions;
	Case 'm': return NoMacros;
	Case 's': return NoStructs;
	Case 'g': return NoGlobals;
	Case 't': return NoTypedefs;
	}
	return 0;
}

#define BeginningOfLine	(CurColumn == 1+Level*strlen(Indent))

ptree(tag, path, nl)
	char *tag, *path;
	int nl;
/*
** Print the calling tree below the function 'tag'.
*/
{
	char entry[MAX], type[20], s[MAX], *callee = s;
	if (Level >= MaxLevel) return;
	if (lookup(tag)) return print(sprintf(s,"%s%s", tag, UpArrow),1);
	if (!FindTag(tag, entry, path)) return enter(strsave(tag));
	ParseTag(entry, type, callee);
	if (NotPrinting(type)) return;
	enter(strsave(tag));
	skipspace(callee);
	if (*callee){
		char name[100];
		int n;
		if (!BeginningOfLine) newline();
		n = CurLine;
		print(sprintf(entry, "%s%s{", tag, Type(type)),1);
		++Level;
		if (pNewline)
			printf("%s",Indent),
			CurColumn += strlen(Indent);
		while (sscanf(callee,"%s",name)==1) {
			callee += strlen(name); skipspace(callee);
			ptree(name,path,*callee);
		}
		if (n == CurLine && !pNewline) {
			print("}",0);
			--Level;
			newline();
		} else {
			--Level;
			if (!pNewline) newline();
			print("}",0);
			newline();
		}
	} else print(sprintf(entry, "%s%s", tag, Type(type)),nl);
}

main(ac,av) Char *av[]; {
	char *s, *getenv();
	Int i;

	if (s=getenv("TAGS")) strcpy(tagpath,s);
	for_each_argument {
	   Case 't': sprintf(tagpath, "%s %s", tagpath, argument);
	   Case 'c': NumColumns = atoi(argument);
	   Case 'n': pNewline = 1;
	   Case 'i': Indent = argument;
	   Case 'l': MaxLevel = atoi(argument);
	   Case 'F': NoFunctions = 1;
	   Case 'G': NoGlobals = 1;
	   Case 'M': NoMacros = 1;
	   Case 'S': NoStructs = 1;
	   Case 'T': NoTypedefs = 1;
	   Case 'q': Verbose = 0; UpArrow = "";
	   Default : Error("Use: %s [-nqFGMST] [-c cols] [-i indent] [-l level] [-t tagpath] [tags...]", av0);
		     exit(1);
	}
	if (!*tagpath) strcpy(tagpath,"tags");

	if (i==ac) {
		/* do it for all uncalled(==top level) tags */
		FILE *popen(), *f = popen("grep \"{ }.*{\" tags", "r");
		char s[MAX], tag[MAX];
		int n=1;
		if (!f) Error("%s: Couldn't grep in 'tags' file.", av0), exit(1);
		while (fgets(s,sizeof s,f) && sscanf(s,"%s",tag)==1) {
			if (strcmp(tag,"main"))
				ptree(tag, tagpath, 1),
				++n;
			newline();
		}
		if (n == 0) Error("%s: no top-level tags?", av0), exit(1);
	} else while(i<ac)
			ptree(av[i++],tagpath,1);
	exit(Status);
}
