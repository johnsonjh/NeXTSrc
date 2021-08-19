#include <stdio.h>
#include "tags.h"

extern int Status, Quiet;
extern char tagdir[];
char tagpath[MAX];
char tagdir[MAX];  /* will hold parent dir of matching tagfile */

/* flags tell what part of tag entry to print */
int pFile,pLocation,pType,pSize,pCaller,pCallee,pAll,Quote,Verbose,Number=1;
int pSource=0;

typedef struct {
	char *tagfile;
	char *name;
	char *file;
	char *location;
	char *type;
	char *size;
	char *caller, *callee;
} Tag;

Tag *
s2tag(tag,s,tagfile)
	Char *tag,*s;
/* parse entry 's' for 'tag' into a 'Tag' structure and return it */
{
	static Tag t;
	static Tag *tp = (Tag *)0;
	static char tfile[MAX], name[MAX], file[MAX], location[MAX],
		type[MAX/4], size[MAX/4], caller[MAX], callee[MAX];
	char *p;

    	if (!tp){
    		t.tagfile = tfile;
	    	t.name = name;
	    	t.file = file;
	    	t.location = location;
		t.type = type;
		t.size = size;
	    	t.callee = callee;
	    	t.caller = caller;
	    	tp = &t;
	}
	*tfile= *name= *file= *location= *type= *size= *caller= *callee= '\0';

	skipspace(s); if (!*s) return tp;
    	for (p=file; !sp(*s); *p++ = *s++) ;
	*p = '\0';

    	strcpy(name,tag);
	strcpy(tfile,tagfile);
	skipspace(s); if (!*s) return tp;

	p=location;
	if (isdigit(*s)) while (*s && isdigit(*s)) *p++ = *s++;
	else {
		char search = (*p++ = *s++);
		char c2 = search;
		while (*s && (*s != search || *(s-1) == '\\'))
			*p++ = *s++;
	    	if (*s == search)
	    		*p++ = *s++;
		while (*s && !sp(*s)) ++s;
	}
	*p = '\0';

	skipspace(s); if (!*s) return tp;
	/* skip vi/ex nonsense chars */
	if (*s == ';') while (*s && !sp(*s)) ++s;

    	skipspace(s); if (!*s) return tp;
   	for (p=type; *s && !sp(*s); *p++ = *s++) ;
    	*p = '\0';

    	skipspace(s); if (!*s) return tp;
	for (p=size; *s && !sp(*s); *p++ = *s++) ;
	*p = '\0';

   	skipspace(s); if (!*s) return tp;
	if (*s != '{') return tp;
	++s;
   	skipspace(s); if (!*s) return tp;
    	for (p=caller; *s && !(*s==' ' && *(s+1)=='}'); *p++ = *s++) ;
	*p = '\0';

   	skipspace(s); if (!*s) return tp;
	if (*s == '}'){ ++s; skipspace(s); if (!*s) return tp; }
	if (*s != '{') return tp;
	++s;
   	skipspace(s); if (!*s) return tp;
    	for (p=callee; *s && !(*s==' ' && *(s+1)=='}'); *p++ = *s++) ;
	*p = '\0';

	return tp;
}

printSource(t)
	Tag *t;
/* attempt to find and print the source text for the given tag */
{
	FILE *fopen(), *f = fopen(t->file,"r");
	char s[1024],*p,*re_comp();
	int n = atoi(t->size);

	if (!f){
		f=fopen(sprintf(s,"%s%s",tagdir,t->file),"r");
		if (!f) return Error("couldn't find source for '%s' (%s)",t->name,t->file);
	}
	if (!t->location[0]) return fclose(f), Error("bad tag entry (%s)",t->name);
	if (isdigit(t->location[0])){
		int start = atoi(t->location), i=0;
		if (start==0) return fclose(f), Error("bad tag entry (%s)",t->name);
		while (i++ < start && (p=fgets(s,sizeof s,f))) ;
		if (p) printf("%s",p);
	} else { /* search for regex */
		char *b,buf[MAX],buf2[MAX];

		/* copy regex into 'buf', quoting special chars as nec. */
		for (b=buf,p=t->location+1;
			*p
	    		&& *p !='\n'
			&& !(*p=='$' && *(p+1)=='/')
			&& !(*p=='/' && *(p+1)=='\0')
			;p++){
			if (*p=='*' || *p=='[' || *p==']') *b++ = '\\';
	    		*b++ = *p;
		}
		*b='\0';
		if (b>buf && *(b-1)=='$') *(b-1)='\0';
		if (p=re_comp(buf))
			return fclose(f), Error("bad regex in tag entry for '%s' (%s)", t->name,t->location);
		*buf2='\0';
		while ((p=fgets(s,sizeof s,f)) && re_exec(s)==0)
			strcpy(buf2,s);
		if (p){
			if (strcmp("function",t->type)==0) /* catch type info, if any */
		    		printf("%s",buf2);
			printf("%s",p);
	    	}
	}
	if (p) while (--n>0 && fgets(s,sizeof s,f))
		printf("%s",s);
	else Error("couldn't find source for tag '%s'",t->name);
	fclose(f);
}

pTag(t) Tag *t; {
	Int pr=0;

#ifdef	NeXT_MOD
#define	p(c) putchar(c), pr=1
#define	ptab() if (pr) p('\t')
#define	pquote() if (Quote) p('\'')
#else	NeXT_MOD
#d p(c) putchar(c), pr=1
#d ptab() if (pr) p('\t')
#d pquote() if (Quote) p('\'')
#endif	NeXT_MOD
	if (pFile){
		ptab();
		printf("%s%s",(t->file[0]=='/'||!access(t->file))?
				"" : tagdir, t->file);
		pr++;
	}
	if (pLocation){
		ptab();
		if (!isdigit(t->location[0])) pquote();
		printf("%s", t->location);
		if (!isdigit(t->location[0])) pquote();
		pr++;
	}
	if (pType){
		ptab();
		printf("%s",t->type);
		pr++;
	}
	if (pSize){
		ptab();
		printf("%s",t->size);
		pr++;
	}
	if (pCaller){
		ptab();
		putchar(Quote? '\'' : '{');
		printf("%s",t->caller);
		putchar(Quote? '\'' : '}');
		pr++;
	}
	if (pCallee){
		ptab();
		putchar(Quote? '\'' : '{');
		printf("%s",t->callee);
		putchar(Quote? '\'' : '}');
		pr++;
	}
	if (pSource){
    		if (pr) printf("\n");
	    	printSource(t);
	} else
 		if (pr) printf("\n");
}

char *
strindex(s,t) 
        register char *s, *t;
{
        register int n=strlen(t);
        while (*s)
                if(!strncmp(s,t,n)) return s;
		else s++;
        return (char *)0;
}

char *
stripnl(s) Char *s; {
	Char *start=s;
	s += strlen(s)-1;
    	while (s>start && *s != '\n') --s;
	if (*s == '\n') *s = '\0';
	return start;
}

prTag(tagfilename, tag,entry)
	char *tagfilename,*tag,*entry;
{
	if (Verbose) printf("%s:\t%s:\t",tagfilename,tag);
	stripnl(entry);
	if (pAll){
    		skipspace(entry);
	    	printf("%s\n",entry);
	} else {
    		pTag(s2tag(tag,entry,tagfilename));
	}
}

char *Constraints[32];
int nConstraints=0;

setTag(s) Char *s; { /* set the 'Constraints' array, if nec. */
	nConstraints=0;
	for (;*s && !(*s==' '||*s=='/'); s++)
    		if (*s=='\n') *s = '\0';
    	while (*s){
		*s++ = '\0';
		while (*s ==' '|| *s=='/') ++s;
		if (*s) Constraints[nConstraints++] = s;
		for (;*s && !(*s==' '||*s=='/'); s++)
    			if (*s=='\n') *s = '\0';
	}
}

NoConstraints(s) Char *s; {
	int i;
    	for (i=0;i<nConstraints;i++)
		if (!strindex(s,Constraints[i])) return 0;
	return 1;
}

PrintTags(tag, tagpath)
	char *tag, *tagpath;
/*
** Lookup 'tag' and print its entry (if any).
** Each file in 'tagpath' is searched.
** Up to 'Number' tags are found and printed (default = 1).
** Global var 'tagdir' will hold parent dir of matching tag file, if any.
** Global flags determine what parts of entry are printed.
*/
{
	char path[MAX], entry[MAX];
	FILE *f;
	char *name, *c, *rindex();
	int n=0,found=0,l=strlen(tag);

	*tagdir = '\0';
	setTag(tag);
	if null(tag) return 0;
	strcpy(path, tagpath);
	for (c=path-1; c && (Number<0 || found<Number); ++n) {
		name = ++c;
		while (*c && *c != ' ' && *c != ':') c++;
		if (!*c) c = 0;
		else *c = '\0';
		f = fopen(name, "r");
		if (!f) { n--; continue; }
		if (SearchTagsFile(f,tag,entry)){
			char *s = rindex(strcpy(tagdir,name),'/');
			if (s) *(s+1) = '\0';
			else tagdir[0] = '\0';
			if (NoConstraints(entry)){
	    			prTag(name,tag,entry);
				++found;
			}
			while ((Number<0 || found<Number) &&
				fgets(entry,sizeof entry,f)){
				char *t = tag, *s;
				for (s=entry; *t && *s == *t; s++,t++);
				if (!*t && sp(*s)){
					skipspace(s);
					if (NoConstraints(s)){
		    				prTag(name,tag,s);
						++found;
					}
				} else goto Done;
			}
		}
	    Done:
		fclose(f);
	}
	if (!n) Error("No tags file");
	if (!found) Error("%s: No such tag", tag);
}

use(){
  Error("Use: %s [-aflstceqQSv] [-N#] [-T tagpath] [tags...]", av0);
  Error("  -a   print all tags found");
  Error("  -N#  print at most # entries (default 1)");
  Error("  -f   print the file");
  Error("  -l     ...     location (search expression)");
  Error("  -s     ...     size");
  Error("  -t     ...     type");
  Error("  -c     ...     list of callers");
  Error("  -e     ...     list of callees");
  Error("  -q   suppress error messages");
  Error("  -Q   surround locations and reference lists with quotes");
  Error("  -v   print \"tagfile: object: \" before each entry");
  Error("  -S   find and print the source text");
  Error("  -T path   set the path of indexes to 'path' (default $TAGS)");
  exit(1);
}

main(ac,av) Char *av[]; {
	char *s, *getenv(), n[MAX];
	Int i;

	*tagpath = '\0';
	for_each_argument {
	   Case 'f': pFile = 1;
	   Case 'l': pLocation = 1;
	   Case 's': pSize = 1;
	   Case 't': pType = 1;
	   Case 'c': pCaller = 1;
	   Case 'e': pCallee = 1;
	   Case 'q': Quiet = 1;
	   Case 'Q': Quote = 1;
	   Case 'v': Verbose = 1;
	   Case 'a': Number = -1;
	   Case 'N': Number = atoi(argument);
	   Case 'S': pSource = 1;
	   Case 'T': sprintf(tagpath, "%s %s", tagpath, argument);
	   Default : use();
	}
	if (!*tagpath) strcpy(tagpath, (s=getenv("TAGS"))? s : "tags");
	if (!(pFile||pLocation||pType||pSize||pCaller||pCallee||pSource))
		pAll = 1;
	if (i==ac)	while (scanf(" %s",n)==1) PrintTags(n, tagpath);
	else		for (;i<ac;++i)		  PrintTags(av[i], tagpath);
	exit(Status);
}
