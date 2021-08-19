/*
	msgwrap.c
	Copyright 1988, NeXT, Inc.
	Responsibility: Greg Cockroft
*/

#import <libc.h>
#import <sys/param.h>
#import <sys/types.h>
#import <sys/stat.h>
#import  "math.h"
#import <sys/time.h>
#import <sys/dir.h>
#import <errno.h>
#import <sys/signal.h>
#import <sys/fcntl.h>
#import <pwd.h>
#import <stdio.h>

#define MYSUFFIX "msg"
#define MYSUFFIXLEN 3

/* maximum number of remote method parameters allowed */
#define NX_MAXMSGPARAMS 20

#define MAXTOKEN 100
#define MAXTYPES 6
#define PTRTYPES 1

char *typeNames[] = {"char","byte","int","double","port_t_send","port_t_rcv"};
char *cNames[] = {"char","char","int","double","port_t","port_t"};
char *iNames[] = {"c","b","i","d","s","r"};
char *oNames[] = {"C","B","I","D","S","R"};
char *params[] = {"paramList[%d].bval.p",
		  "paramList[%d].bval.p",
		  "paramList[%d].ival",
		  "paramList[%d].dval",
		  "paramList[%d].pval",
		  "paramList[%d].pval"};
char *lenParam = "paramList[%d].bval.len"; 
 
#define CHARTYPE 0
#define BYTETYPE 1
#define INTTYPE 2
#define DOUBLETYPE 3
#define PORTSENDTYPE 4
#define PORTRCVTYPE 4

char *colon = ":";
char *semi = ";";
char *star = "*";
char *lparen = "(";
char *rparen = ")";
char *minus = "-";

typedef struct _param {
    struct _param *next;
    int type;
    int stars;
    char *methodPart;
    char *paramName;
} param;

typedef struct _method {
    struct _method *next;
    param *params;
} method;

#define ISPTR(p) ((p->type <= PTRTYPES && p->stars == 2) || (p->type > PTRTYPES && p->stars))

int lookAhead = 0;
char lookAheadChar;
int lineNumber = 1;
char token[MAXTOKEN+1];
int nMethods = 0;
method *firstMethod = NULL;


char *readToken(FILE *fp)
{
    register char c;
    register int i;
    
    if (!lookAhead)
        c = getc(fp);
    else {
        c = lookAheadChar;
	lookAhead = 0;
    }
    
    while (c == ' ' || c == '\t' || c == '\n') {
        if (c == '\n')
	    lineNumber++;
        c = getc(fp);
    }
    
    if (c <= 0)
        return NULL;
    if (c == '*')
        return star;
    if (c == ';')
        return semi;
    if (c == ':')
        return colon;
    if (c == '-')
        return minus;
    if (c == '(')
        return lparen;
    if (c == ')')
        return rparen;

    i = 0;
    while (1) {
        switch (c) {
	case '\n':
	    lineNumber++;
	case '\0':
	case ' ':
	case '\t':
	case ':':
	case ';':
	case '*':
	case '(':
	case ')':
	case '-':
	    lookAhead = 1;
	    lookAheadChar = c;
	    goto endToken;
	default:
	    if (i == MAXTOKEN)
	        fprintf(stderr,"(%d) token too long\n",lineNumber);
	    if (i < MAXTOKEN)
	        token[i++] = c;
	}
	c = getc(fp);
    }
endToken:
    token[i] = '\0';
    return &token[0];
}

method *readMethod(FILE *fp)
{
    method *newMethod;
    char *t;
    method localMethod;
    param *p;
    param *lastP;
    int stars;
    int nparams;
    int type;
    
    newMethod = NULL;
    localMethod.params = NULL;
    localMethod.next = NULL;
    lastP = NULL;
    nparams = 0;
    	
    t = readToken(fp);
    while (1) {
	if (t != token) {
	     fprintf(stderr,"(%d) Expecting identifier, found \"%s\"\n",
		     lineNumber,
		     t ? t : "EOF");
	     goto exitPoint;
	}
	nparams++;
	p = (param *) malloc(sizeof(param));
	p->next = NULL;
	p->paramName = NULL;
	p->methodPart = malloc(strlen(t)+1);
	strcpy(p->methodPart,t);
	
	t = readToken(fp);
	if (t != colon && t != semi) {
	     fprintf(stderr,"(%d) Expecting \":\" or \";\", found \"%s\"\n",
		     lineNumber,
		     t ? t : "EOF");
	     goto exitPoint;
	}
	if (t == semi) {
	    if (localMethod.params) {
		 fprintf(stderr,"(%d) Expecting \":\", found \";\"\n",
			 lineNumber);
		 goto exitPoint;
	    }
	    stars = 0;
	    type = -1;
	} else {
	    t = readToken(fp);
	    if (t != lparen) {
		 fprintf(stderr,"(%d) Expecting \"(\", found \"%s\"\n",
			 lineNumber,
			 t ? t : "EOF");
		 goto exitPoint;
	    }
	    
	    t = readToken(fp);
	    type = 0;
	    while (type < MAXTYPES) {
		if (!strcmp(t,typeNames[type]))
		    break;
		type++;
	    }
	    if (type == MAXTYPES) {
		fprintf(stderr,
                "(%d) Expecting a type identifier, found \"%s\"\n",
			 lineNumber,
			 t ? t : "EOF");
		 goto exitPoint;
	    }
	    stars = 0;
	    t = readToken(fp);
	    while (t == star) {
		 stars++;
		 t = readToken(fp);
	    }
	    if (t != rparen) {
		fprintf(stderr,"(%d) Expecting \")\", found \"%s\"\n",
			lineNumber,
			t ? t : "EOF");
		goto exitPoint;
	    }
	    if (stars > 2 || 
		(stars == 0 && type <= PTRTYPES) ||
		(stars == 2 && type > PTRTYPES)) {
		fprintf(stderr,
		        "(%d) Too many or too few stars\n",lineNumber);
		goto exitPoint;
	    }
	    
	    if (lastP && (lastP->type == BYTETYPE)) {
		if (type != INTTYPE || (lastP->stars != (stars+1))) {
		    fprintf(stderr,
	       "(%d) A byte parameter must be followed by an int parameter\n",
			    lineNumber);
		    goto exitPoint;
		}
	    }
	    t = readToken(fp);
	    if (t != token) {
		 fprintf(stderr,
		         "(%d) Expecting identifier, found \"%s\"\n",
			 lineNumber,
			 t ? t : "EOF");
		 goto exitPoint;
	    }
	    p->paramName = malloc(strlen(t)+1);
	    strcpy(p->paramName,t);
	    t = readToken(fp);
	}
	p->type = type;
	p->stars = stars;
	if (!lastP)
	    localMethod.params = p;
	else
	    lastP->next = p;
	lastP = p;
	if (t == semi)
	    break;
    }
    if (lastP->type == BYTETYPE) {
    	fprintf(stderr,
                "(%d) A byte parameter must be followed by an int parameter\n",
               lineNumber);
	goto exitPoint;
    }
    
    if (nparams > NX_MAXMSGPARAMS) {
    	fprintf(stderr,
                "(%d) Too many parameters (maximum is %d).\n",
                lineNumber,
		NX_MAXMSGPARAMS);
	goto exitPoint;
    }
    
    newMethod = (method *) malloc(sizeof(method));
    *newMethod = localMethod;
    
exitPoint:
    return newMethod;
}

int readifile(char *fname)
{
    register FILE    *fp;
    char *t;
    method *lastMethod;
    method *newMethod;
    int retvalue;
    
    retvalue = -1;
    firstMethod = 0;
    lastMethod = 0;
    fp = NULL;
    
    if (!(fp = fopen(fname, "r"))) {
	fprintf(stderr, "si:can't open %s\n", fname);
	goto f_exit;
    }
    
    t = readToken(fp);
    while (t) {
	if (t != minus) {
	    fprintf(stderr,"(%) Expecting \"-\", found \"%s\"\n",lineNumber,t);
	    goto f_exit;
	}
	newMethod = readMethod(fp);
	if (!newMethod)
	    goto f_exit;
	if (!lastMethod)
	    firstMethod = newMethod;
	else
	    lastMethod->next = newMethod;
	lastMethod = newMethod;
	nMethods++;
        t = readToken(fp);
    }

    retvalue = 0;

f_exit:

    if (fp)
	fclose(fp);
    return retvalue;
}

void writeHeader(FILE *fout, method *m)
{
    param *p;
    param *lastP;
    
    fprintf(fout,"-(int)");
    p = m->params;
    lastP = NULL;
    while (p) {
        if (lastP)
	   fprintf(fout,"\n\t");
	fprintf(fout,"%s",p->methodPart);
	if (p->paramName)
	    fprintf(fout," : (%s%s) %s",
	            cNames[p->type],
		    (!p->stars) ? "" : (p->stars == 1) ? " *" : " **",
		    p->paramName);
        lastP = p;
        p = p->next;
    }

}

void writeMethodHeaders(char *name, char *whichClass)
{
    method *m;
    char outpath[MAXPATHLEN+1];
    register FILE *fout;
    
    sprintf(outpath,"%s%s.h",name,whichClass);
    if (!(fout = fopen(outpath, "w+"))) {
	fprintf(stderr, "si:can't open %s\n", outpath);
	return;
    }
    fprintf(fout,"#import <appkit/%s.h>\n",whichClass);
    fprintf(fout,"@interface %s%s : %s\n",name,whichClass,whichClass);
    fprintf(fout,"{}\n");
    m = firstMethod;
    while (m) {
        writeHeader(fout,m);
	fprintf(fout,";\n");
	m = m->next;
    }
    fprintf(fout,"@end");
    fclose(fout);
}

void writeHeaderM(FILE *fout, char *name, char *whichClass)
{
    fprintf(fout,"#import <appkit/appkit.h>\n");
    fprintf(fout,"#import \"%s%s.h\"\n",name,whichClass);
    fprintf(fout,"#import <mach.h>\n");
    fprintf(fout,"#import <sys/message.h>\n");
    fprintf(fout,"#import <servers/netname.h>\n");
    fprintf(fout,"extern port_t name_server_port;\n");
    fprintf(fout,"extern id NXResponsibleDelegate();\n");
    fprintf(fout,"@implementation  %s%s :%s\n",name,whichClass,whichClass);
    fprintf(fout,"{}\n");
}

char *typeString(param *p)
{
    int type;
    
    type = p->type;
    if ((type <= PTRTYPES && p->stars == 1) ||
        (type > PTRTYPES && !p->stars))
        return iNames[type];
    return oNames[type];
}

void writeSpeaker(char *name)
{
    char outpath[MAXPATHLEN+1];
    method *m;
    param *p;
    param *lastP;
    FILE *fout;
    
    sprintf(outpath,"%sSpeaker.m",name);
    if (!(fout = fopen(outpath, "w+"))) {
	fprintf(stderr, "si:can't open %s\n", outpath);
	return;
    }
    writeHeaderM(fout,name,"Speaker");
    m = firstMethod;
    while (m) {
        writeHeader(fout,m);
	fprintf(fout,"\n/* */\n");
	fprintf(fout,"{\n");
	fprintf(fout,"return [self selectorRPC:\"");
	p = m->params;
	while (p) {
	    fprintf(fout,"%s%s",p->methodPart,p->paramName ? ":":"");
	    p = p->next;
	}
	fprintf(fout,"\"\n");
	fprintf(fout,"\tparamTypes:\"");
	p = m->params;
	if (p->paramName) {
	    lastP = NULL;
	    while (p) {
		if (!lastP || lastP->type != BYTETYPE)
		    fprintf(fout,"%s",typeString(p));
		lastP = p;
		p = p->next;
	    }
	    p = m->params;
	    fprintf(fout,"\",");
        } else
	    fprintf(fout,"\"");
	
	lastP = NULL;
	while (p) {
	    if (p->paramName)
	        fprintf(fout,
		        "%s\t\t%s",(lastP ? ",\n" : "\n"),
			p->paramName);
	    lastP = p;
	    p = p->next;
	}
	fprintf(fout,"];\n");
	
        fprintf(fout,"}\n");
        m = m->next;
    }
    
    fprintf(fout,"@end\n");
    fclose(fout);
    
    writeMethodHeaders(name,"Speaker");
}

void writeListenerMethods(FILE *fout)
{
    method *m;
    param *p;
    param *lastP;
    
    m = firstMethod;
    while (m) {
        writeHeader(fout,m);
	fprintf(fout,"\n/* */\n");
	fprintf(fout,"{\n");
	fprintf(fout,"    id _NXd;\n");
	fprintf(fout,"    if (_NXd = NXResponsibleDelegate(self,\n\t@selector(");
	p = m->params;
	while (p) {
	    fprintf(fout,"%s%s",p->methodPart,p->paramName ? ":":"");
	    p = p->next;
	}
	fprintf(fout,")))\n\treturn [_NXd ");
	p = m->params;
	lastP = NULL;
	while (p) {
	    if (lastP)
	       fprintf(fout,"\n\t\t");
	    fprintf(fout,"%s",p->methodPart);
	    if (p->paramName)
		fprintf(fout," : %s",p->paramName);
	    lastP = p;
	    p = p->next;
	}
	fprintf(fout,"];\n    return -1;\n}\n\n");
        m = m->next;
    }
}

void writePerform(FILE *fout)
{
    method *m;
    param *p;
    param *lastP;
    int i;
    int j;
    char *format;
    
    fprintf(fout,"- (int) performRemoteMethod : (NXRemoteMethod *) method\n");
    fprintf(fout,"                  paramList : (NXParamValue *) paramList {\n");
    fprintf(fout,"/* */\n");
    fprintf(fout,"    switch (method - remoteMethods) {\n");
    
    m = firstMethod;
    i = 0;
    while (m) {
        fprintf(fout,"    case %d:\n",i);
	fprintf(fout,"\treturn [self ");
	p = m->params;
	lastP = NULL;
        j = 0;
	while (p) {
	    if (lastP)
	       fprintf(fout,"\n\t\t");
	    fprintf(fout,"%s",p->methodPart);
	    if (p->paramName) {
	        format = params[p->type];
		if (lastP && lastP->type == BYTETYPE) {
		    format = lenParam;
		    j--;
		}
		fprintf(fout," : %s",ISPTR(p) ? "&" : "");
		fprintf(fout,format,j);
	    }
	    lastP = p;
	    p = p->next;
	    j++;
	}
	fprintf(fout,"];\n");
        m = m->next;
	i++;
    }
    fprintf(fout,"    default:\n");
    fprintf(fout,
"\treturn [super performRemoteMethod : method paramList : paramList];\n");
    fprintf(fout,"    }\n}\n");
}

void writeListener(char *name)
{
    char outpath[MAXPATHLEN+1];
    method *m;
    param *p;
    param *lastP;
    int i;
    FILE *fout;
    
    sprintf(outpath,"%sListener.m",name);
    if (!(fout = fopen(outpath, "w+"))) {
	fprintf(stderr, "si:can't open %s\n", outpath);
	return;
    }
    
    writeHeaderM(fout,name,"Listener");
    fprintf(fout,"static NXRemoteMethod *remoteMethods = NULL;\n");
    fprintf(fout,"#define REMOTEMETHODS %d\n",nMethods);
    fprintf(fout,"+ (void)initialize \n/* */\n{\n");
    fprintf(fout,"    if (!remoteMethods) {\n");
    fprintf(fout,"\tremoteMethods =\n");
    fprintf(fout,
"\t(NXRemoteMethod *) malloc((REMOTEMETHODS+1)*sizeof(NXRemoteMethod));\n");

    m = firstMethod;
    i = 0;
    while (m) {
        fprintf(fout,"\tremoteMethods[%d].key = \n\t@selector(",i);
	p = m->params;
	while (p) {
	    fprintf(fout,"%s%s",p->methodPart,p->paramName ? ":":"");
	    p = p->next;
	}
	fprintf(fout,");\n");
        fprintf(fout,"\tremoteMethods[%d].types = \"",i);
	p = m->params;
	if (p->paramName) {
	    lastP = NULL;
	    while (p) {
		if (!lastP || lastP->type != BYTETYPE)
		    fprintf(fout,"%s",typeString(p));
		lastP = p;
		p = p->next;
	    }
	}
	fprintf(fout,"\";\n");
        m = m->next;
	i++;
    }

    fprintf(fout,"\tremoteMethods[REMOTEMETHODS].key = NULL;\n");
    fprintf(fout,"    }\n}\n");
    
    writeListenerMethods(fout);
    writePerform(fout);
    
    fprintf(fout,"- (NXRemoteMethod *) remoteMethodFor: (SEL) aSel {\n");
    fprintf(fout,"/* */\n");
    fprintf(fout,"    NXRemoteMethod *rm;\n");
    fprintf(fout,"    if (rm = NXRemoteMethodFromSel(aSel,remoteMethods))\n");
    fprintf(fout,"        return rm;\n");
    fprintf(fout,"    return [super remoteMethodFor : aSel];\n");
    fprintf(fout,"}\n");
    
    fprintf(fout,"@end\n");
    fclose(fout);
    
    writeMethodHeaders(name,"Listener");
}

void main(int argc, char *argv[])
{
    char rootName[MAXNAMLEN+1];
    int len;
    char *rpos;
    
    if (argc != 2) {
        fprintf(stderr,"usage: msgwrap <message spec file>\n");
	exit(1);
    }

    len = strlen(argv[1]);
    if (len > MAXNAMLEN) {
        fprintf(stderr,"file name too long\n");
	exit(1);
    }
    strcpy(rootName,argv[1]);
    if ((rpos = rindex(rootName,'.')) && 
        !(strcmp(rpos+1,MYSUFFIX))) {
        *rpos = '\0';
    }
    
    if (strlen(rootName) > (MAXNAMLEN-strlen("Listener.m"))) {
        fprintf(stderr,"file name too long\n");
	exit(1);
    }
    
    if (!readifile(argv[1])) {
	writeSpeaker(rootName);
	writeListener(rootName);
    }
}

/*

Modifications (starting at 0.8):

 1/05/89 trey	fixed "usage" message to use the correct app name
 4/13/90 trey	big lint cleanup, prototypes added

*/
