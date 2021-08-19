/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * nmxlog.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/utils/RCS/nmxlog.c,v $
 *
 */
#ifndef	lint
static char     rcsid[] = "$Header: nmxlog.c,v 1.1 88/09/30 15:47:14 osdev Exp $";
#endif not lint
/*
 * Program to translate a log from a network server.
 * Input = stdin or file argument.
 * Output = stdout.
 * Dictionary obtain (in order) from: 
 *	-d switch on the command line
 *	environment variable "NMLOGDICT"
 *	standard file specified by STDDICTFILE
 */

/*
 * HISTORY: 
 * 13-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Replaced net_recursion_level by trace_recursion_level
 *	and net_trace tracing_on.  Removed net_debug_level.
 *
 * 21-Mar-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Created.
 *
 */


#include	"trace.h"
#include	"nm_defs.h"
#include	"ls_defs.h"
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/file.h>


/*
 * Tracing values.
 */
int	trace_recursion_level = 0;
#undef	tracing_on
int	tracing_on = 0;


/*
 * Default path to the dictionary file.
 */
#define	STDDICTFILE	"/usr/mach/lib/net_dict.data"

/*
 * Number of possible dictionary entries.
 */
#define	DICTMAX		10000

/*
 * Usage string.
 */
#define	USAGE		"Usage: %s [-d dictfile] [<] infile > outfile\n"

/*
 * Table of translations.
 */
typedef struct {
    	int	string;		/* is this a string-type entry ? */
	char	*text;		/* format for the entry */
} translation_t;
translation_t	 translations[DICTMAX];
char	*textptr;		/* current free location to use for text */

/*
 * read_dict
 *
 * Read the dictionary file into the translations table.
 *
 * Parameters:
 *
 * dict_file: pathname of dictionary file.
 *
 * Results: none.
 *
 * Side effects:
 *
 * Opens the dictionary file. Fills the translations table. May abort the program
 * in case of error.
 *
 * Design:
 *
 * Note:
 *
 */
read_dict(dict_file)
char	*dict_file;
BEGIN("read_dict")
	int	i;
	FILE	*dict_str;	/* stream for dict_file */
	struct stat	stat;
	int	code;
	int	last_code = -1;

	/*
	 * First zero the translations.
	 */
	for (i = 0; i < DICTMAX; i++)
		translations[i].text = NULL;

	/*
	 * Try to open the dictionary file.
	 */
	if (!dict_file) {
	    	if ((dict_file = (char *)getenv("NMLOGDICT")) == NULL) {
		    	dict_file = STDDICTFILE;
		}
	}
    	if ((dict_str = fopen(dict_file, "r")) == NULL) {
	    	fprintf(stderr,"Cannot open \"%s\"\n", dict_file);
		exit(1);
	}

	/*
	 * Allocate space for the translation text.
	 */
	if (fstat(fileno(dict_str),&stat)) {
	    	perror("Cannot stat dictionary file");
		exit(1);
	}
	textptr = (char *)malloc(stat.st_size);
	if (textptr == 0) {
	    	fprintf(stderr,"Cannot allocate space for the translation text\n");
		exit(1);
	}

	/*
	 * Load all the translations.
	 */
	while(!feof(dict_str)) {
	    	if (fscanf(dict_str,"%d",&code) != 1) {
		    	if (feof(dict_str))
				break;
		    	fprintf(stderr,"Invalid entry in dictionary file - last valid code = %d\n",last_code);
			exit(1);
		}
		if ((code < 0) || (code >= DICTMAX)) {
		    	fprintf(stderr,"Invalid code in dictionary file: %d\n",code);
			exit(1);
		}
		if (translations[code].text) {
		    	fprintf(stderr,"Duplicate entry in dictionary for code %d\n",code);
			exit(1);
		}
		if ((translations[code].text = fgets(textptr,200,dict_str)) == NULL) {
		    	fprintf(stderr,"Invalid text in dictionary file - code = %\n",code);
			exit(1);
		}
		translations[code].string = sindex(textptr,"%s");
		last_code = code;
		textptr += strlen(textptr) + 1;
	}

	/*
	 * Close the dictionary file.
	 */
	fclose(dict_str);

	RET;
END



/*
 * main
 *
 * Main loop.
 *
 * Parameters:
 *
 * Results:
 *
 * Side effects:
 *
 * Design:
 *
 * Note:
 *
 */
void main(argc,argv)
int	argc;
char	*argv[];
BEGIN("main")
	char	*in_file = NULL;		/* input file */
	char	*dict_file = NULL;		/* path to dictionary file */
	char	*prog;				/* name under which invoked */
	FILE	*istr = NULL;			/* input stream */
	FILE	*ostr;				/* output stream */
	log_rec_t	log_rec[4];		/* space to read log records into */
	int	code;				/* current log code */
	translation_t	*tr_ptr;		/* current translation record */
	char	strtmp[1000];			/* temporary string for translation */

	/*
	 * Parse the command line.
	 */
	prog = *argv;
	for (argv++; --argc; argv++) {
	    	if ((*argv)[0] == '-') {
		    	switch((*argv)[1]) {
			    	case 'd':
					if (argc < 2) {
					    	fprintf(stderr, "No file name after \"-d\"\n");
					    	fprintf(stderr, USAGE, prog);
						exit(1);
					}
					argv++;
					argc--;
					dict_file = *argv;
					break;
				default:
					fprintf(stderr, "Unknown switch\n");
				    	fprintf(stderr, USAGE, prog);
					exit(1);
					break;
			}
		} else {
		    	if (in_file) {
			    	fprintf(stderr, "Multiple input specification\n");
			    	fprintf(stderr, USAGE, prog);
				exit(1);
			}
		    	in_file = *argv;
		}
	}

	/*
	 * Read the dictionary file.
	 */
	 read_dict(dict_file);

	/*
	 * Worry about input and output.
	 */
	if (in_file) {
	    	if ((istr = fopen(in_file, "r")) == NULL) {
		    	fprintf(stderr,"Cannot open \"%s\"\n", in_file);
			exit(1);
		}
	} else {
	    	istr = stdin;
	}
	ostr = stdout;

	/*
	 * Read and translate all log records.
	 */
	while(fread(log_rec,sizeof(log_rec_t),1,istr)) {
	    	code = log_rec[0].code;
		if ((code < 0) || (code >= DICTMAX) ||
			((tr_ptr = &translations[log_rec[0].code])->text == NULL)) {
		    	fprintf(stderr,"Invalid code: %d\n",log_rec[0].code);
		    	fprintf(ostr,"(%d) %d : --- Invalid code %x %x %x %x %x %x\n",
				log_rec[0].thread,
				log_rec[0].code,
				log_rec[0].a1,
				log_rec[0].a2,
				log_rec[0].a3,
				log_rec[0].a4,
				log_rec[0].a5,
				log_rec[0].a6);
			continue;
		}
	    	if (tr_ptr->string) {
		    	if (fread(&log_rec[1],sizeof(log_rec_t),3,istr) == 0) {
			    	fprintf(stderr,"Premature EOF on log string entry\n");
				break;
			}
			sprintf(strtmp,tr_ptr->text,&log_rec[0].a1);
		} else {
		    	sprintf(strtmp,tr_ptr->text,
				log_rec[0].a1,
				log_rec[0].a2,
				log_rec[0].a3,
				log_rec[0].a4,
				log_rec[0].a5,
				log_rec[0].a6);
		}
		fprintf(ostr,"(%6d) %4d : %s",log_rec[0].thread,log_rec[0].code,strtmp);
	}

	RET;
END
