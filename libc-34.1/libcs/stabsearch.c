/*  stabsearch --  search for best match within string table
 *
 *  int stabsearch (arg,table,quiet);
 *  char *arg;
 *  char **table;
 *  int quiet;
 *
 *  Just like stablk(3), but a match score is determined for each
 *  string in the table, and the best match is used.  If quiet=0,
 *  the user will be asked if he really meant the best matching
 *  string; if he says "no", a list of several other good matches
 *  will be printed.
 *  If there is exactly one perfect match, then its index will be
 *  returned, and the user will not be asked anything.  If there are
 *  several perfect matches, then up to 50 will be listed for the
 *  user to review and among which he can select one.
 *
 * HISTORY
 * 28-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Modified for 4.2 BSD.  Now puts output on std. error using fprintf
 *	and fprstab.
 *
 * 13-Mar-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Return NOMATCH if arg is zero.
 *
 * 26-Oct-83  Leonard Hamey (lgh) at Carnegie-Mellon University
 *	Included code to detect exact matches.
 *
 * 06-Feb-81  Steven Shafer (sas) at Carnegie-Mellon University
 *	Fixed bug in previous bug fixes.
 *
 * 27-Jan-81  Steven Shafer (sas) at Carnegie-Mellon University
 *	Added SCROLLSIZE, KEEPPERFECT, and associated code for better
 *	handling of long string tables.
 *
 * 16-Apr-80  Steven Shafer (sas) at Carnegie-Mellon University
 *	Changed option-printing code to use "prstab"; this will use
 *	multiple columns where appropriate.
 *
 * 12-Mar-80  Steven Shafer (sas) at Carnegie-Mellon University
 *	Added check for unique perfect match; will now return appropriate index
 *	without asking "Did you mean X?".
 *
 * 23-Jan-80  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created.  Almost identical to Dave McKeown's matching routine from
 *	the PDP-11, but the scores are normalized to lie in the range 0 to 100.
 *
 */

#include <stdio.h>

#define KEEPBEST 6		/* # of matches to keep around */
#define KEEPPERFECT 50		/* # of perfect matches to keep */
#define KEEPEXACT 50		/* # of exact matches to keep */
#define PERFECT 100		/* perfect match score */
#define EXACT 101		/* exact match pseudo-score */
#define THRESHOLD 35		/* minimum acceptable score */
#define NOMATCH -1		/* return value if no match */
#define MAXLENGTH 400		/* max length of arg and table entries */
#define SCROLLSIZE 20		/* size of each screenful for scrolling */

int srchscore();		/* score function */

int stabsearch (arg,table,quiet)
char *arg,**table;
int quiet;
{
	int bestentry[KEEPPERFECT], bestscore[KEEPPERFECT];
	int arglen;
	int maxscore;		/* best possible score */
	register int i,j,k;	/* temps */
	int nperfect;		/* # of perfect matches */
	int nexact; 		/* # of exact matches */
	int nentries;		/* # of entries in table */
	char line[MAXLENGTH];
	char a[MAXLENGTH],e[MAXLENGTH];

	if (arg == 0)
	    return (NOMATCH);
	if (strcmp (arg,"?") != 0) {

		for (i=0; i<KEEPPERFECT; i++) {
			bestentry[i] = -1;
			bestscore[i] = -1;
		}
		arglen = strlen (arg);
		maxscore = arglen * arglen;
		folddown (a,arg);

		nperfect = 0;
		nexact = 0;
		for (i=0; table[i]; i++) {
			folddown (e,table[i]);
			j = (srchscore (e,a) * PERFECT) / maxscore;
			if (nperfect == 0 && nexact == 0 && j != PERFECT) {
				if (j >= bestscore[KEEPBEST-1]) {
					k = KEEPBEST - 1;
					while ((k > 0) && (j > bestscore[k-1])) {
						bestscore[k] = bestscore[k-1];
						bestentry[k] = bestentry[k-1];
						--k;
					}
					bestscore[k] = j;
					bestentry[k] = i;
				}
			}
			else if (j == PERFECT && arglen == strlen (e))
			{	bestscore[0] = EXACT;
				if (nexact < KEEPEXACT)
					bestentry[nexact++] = i;
			}
			else if (j == PERFECT && nperfect < KEEPPERFECT && !nexact) {
				bestscore[0] = PERFECT;
				bestentry[nperfect] = i;
				nperfect++;
			}
		}
		nentries = i;

		if (bestscore[0] <= THRESHOLD) {
			if (!quiet) {
				fprintf (stderr,"Sorry, nothing matches \"%s\" reasonably.\n",arg);
				if (getbool("Do you want a list?",(nentries<=SCROLLSIZE))) goto makelist;
			}
			return (NOMATCH);
		}

		if (quiet)  return (bestentry[0]);
		if (nexact > 1)
		{	fprintf (stderr,"There are %d exact matches for \"%s\":\n",nexact,arg);
			j = getint ("Which one should be used?  (0 if none ok)",0,nexact,1);
			return ((j > 0) ? bestentry[j-1] : NOMATCH);
		}
		else if (nexact == 1) return (bestentry[0]);

		if (nperfect > 1) {	/* multiple max matches */
			fprintf (stderr,"There are %d perfect matches for \"%s\":\n",nperfect,arg);
			for (j=0; j<nperfect && ((j%SCROLLSIZE!=SCROLLSIZE-1) || getbool("Continue?",1)); j++) {
				fprintf (stderr,"(%d)\t%s\n",j+1,table[bestentry[j]]);
			}
			j = getint ("Which one should be used?  (0 if none ok)",0,nperfect,1);
			return ((j > 0) ? bestentry[j-1] : NOMATCH);
		}
		else if (nperfect == 1) {	/* unique perfect match */
			return (bestentry[0]);
		}

		sprintf (line,"Did you mean \"%s\" [%d] ?",
			table[bestentry[0]],bestscore[0]);
		if (getbool(line,1))  return (bestentry[0]);

		fprintf (stderr,"May I suggest the following?\n");
		for (i=0; i<KEEPBEST && bestscore[i] >= THRESHOLD; i++) {
			fprintf (stderr,"(%d)\t%-15s \t[%d]\n",i+1,table[bestentry[i]],bestscore[i]);
		}
		j = getint ("Which one should be used?  (0 if none ok)",0,i,1);
		if (j>0)  return (bestentry[j-1]);
		if (getbool("Do you want a list of all possibilities?",(nentries<=SCROLLSIZE)))  goto makelist;

	}
	else {
makelist:

		fprintf (stderr,"The choices are as follows:\n");
		fprstab (stderr,table);
	}

	return (NOMATCH);
}
