/* Open and close files for bison,
   Copyright (C) 1984, 1986, 1989 Free Software Foundation, Inc.

This file is part of Bison, the GNU Compiler Compiler.

Bison is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

Bison is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Bison; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */


#ifdef VMS
#include <ssdef.h>
#define unlink delete
#define XPFILE "GNU_BISON:[000000]BISON.SIMPLE"
#define XPFILE1 "GNU_BISON:[000000]BISON.HAIRY"
#endif

#include <stdio.h>
#include "files.h"
#include "new.h"
#include "gram.h"

FILE *finput = NULL;
FILE *foutput = NULL;
FILE *fdefines = NULL;
FILE *ftable = NULL;
FILE *fattrs = NULL;
FILE *fguard = NULL;
FILE *faction = NULL;
FILE *fparser = NULL;

/* File name specified with -o for the output file, or 0 if no -o.  */
char *spec_outfile;

char *infile;
char *outfile;
char *defsfile;
char *tabfile;
char *attrsfile;
char *guardfile;
char *actfile;
char *tmpattrsfile;
char *tmptabfile;

extern char	*mktemp();	/* So the compiler won't complain */
extern char	*getenv();
FILE	*tryopen();	/* This might be a good idea */
void done();

extern int verboseflag;
extern int definesflag;
int fixed_outfiles = 0;


char*
stringappend(string1, end1, string2)
char *string1;
int end1;
char *string2;
{
  register char *ostring;
  register char *cp, *cp1;
  register int i;

  cp = string2;  i = 0;
  while (*cp++) i++;

  ostring = NEW2(i+end1+1, char);

  cp = ostring;
  cp1 = string1;
  for (i = 0; i < end1; i++)
    *cp++ = *cp1++;

  cp1 = string2;
  while (*cp++ = *cp1++) ;

  return ostring;
}


/* JF this has been hacked to death.  Nowaday it sets up the file names for
   the output files, and opens the tmp files and the parser */
void
openfiles()
{
  char *name_base;
  register char *cp;
  char *filename;
  int base_length;
  int short_base_length;

#ifdef VMS
  char *tmp_base = "sys$scratch:b_";
#else
  char *tmp_base = "/tmp/b.";
#endif
  int tmp_len = strlen (tmp_base);

  if (spec_outfile)
    {
      /* -o was specified.  The precise -o name will be used for ftable.
	 For other output files, remove the ".c" or ".tab.c" suffix.  */
      name_base = spec_outfile;
      /* BASE_LENGTH includes ".tab" but not ".c".  */
      base_length = strlen (name_base);
      if (!strcmp (name_base + base_length - 2, ".c"))
	base_length -= 2;
      /* SHORT_BASE_LENGTH includes neither ".tab" nor ".c".  */
      short_base_length = base_length;
      if (!strcmp (name_base + short_base_length - 4, ".tab"))
	short_base_length -= 4;
      else if (!strcmp (name_base + short_base_length - 4, "_tab"))
	short_base_length -= 4;
    }
  else
    {
      /* -o was not specified; compute output file name from input
	 or use y.tab.c, etc., if -y was specified.  */

      name_base = fixed_outfiles ? "y.y" : infile;

      /* Discard any directory names from the input file name
	 to make the base of the output.  */
      cp = name_base;
      while (*cp)
	{
	  if (*cp == '/')
	    name_base = cp+1;
	  cp++;
	}

      /* BASE_LENGTH gets length of NAME_BASE, sans ".y" suffix if any.  */

      base_length = strlen (name_base);
      if (!strcmp (name_base + base_length - 2, ".y"))
	base_length -= 2;
      short_base_length = base_length;

#ifdef VMS
      name_base = stringappend(name_base, short_base_length, "_tab");
#else
      name_base = stringappend(name_base, short_base_length, ".tab");
#endif
      base_length = short_base_length + 4;
    }

  finput = tryopen(infile, "r");

  filename = getenv ("BISON_SIMPLE");
  fparser = tryopen(filename ? filename : PFILE, "r");

  if (verboseflag)
    {
      outfile = stringappend(name_base, short_base_length, ".output");
      foutput = tryopen(outfile, "w");
    }

  if (definesflag)
    {
      defsfile = stringappend(name_base, base_length, ".h");
      fdefines = tryopen(defsfile, "w");
    }

  actfile = mktemp(stringappend(tmp_base, tmp_len, "act.XXXXXX"));
  faction = tryopen(actfile, "w+");
  unlink(actfile);

  tmpattrsfile = mktemp(stringappend(tmp_base, tmp_len, "attrs.XXXXXX"));
  fattrs = tryopen(tmpattrsfile,"w+");
  unlink(tmpattrsfile);

  tmptabfile = mktemp(stringappend(tmp_base, tmp_len, "tab.XXXXXX"));
  ftable = tryopen(tmptabfile, "w+");
  unlink(tmptabfile);

	/* These are opened by `done' or `open_extra_files', if at all */
  if (spec_outfile)
    tabfile = spec_outfile;
  else
    tabfile = stringappend(name_base, base_length, ".c");

#ifdef VMS
  attrsfile = stringappend(name_base, short_base_length, "_stype.h");
  guardfile = stringappend(name_base, short_base_length, "_guard.c");
#else
  attrsfile = stringappend(name_base, short_base_length, ".stype.h");
  guardfile = stringappend(name_base, short_base_length, ".guard.c");
#endif
}



/* open the output files needed only for the semantic parser.
This is done when %semantic_parser is seen in the declarations section.  */

void
open_extra_files()
{
  FILE *ftmp;
  int c;
  char *filename;
		/* JF change open parser file */
  fclose(fparser);
  filename = (char *) getenv ("BISON_HAIRY");
  fparser= tryopen(filename ? filename : PFILE1, "r");

		/* JF change from inline attrs file to separate one */
  ftmp = tryopen(attrsfile, "w");
  rewind(fattrs);
  while((c=getc(fattrs))!=EOF)	/* Thank god for buffering */
    putc(c,ftmp);
  fclose(fattrs);
  fattrs=ftmp;

  fguard = tryopen(guardfile, "w");

}

	/* JF to make file opening easier.  This func tries to open file
	   NAME with mode MODE, and prints an error message if it fails. */
FILE *
tryopen(name, mode)
char *name;
char *mode;
{
  FILE	*ptr;

  ptr = fopen(name, mode);
  if (ptr == NULL)
    {
      fprintf(stderr, "bison: ");
      perror(name);
      done(2);
    }
  return ptr;
}

void
done(k)
int k;
{
  if (faction)
    fclose(faction);

  if (fattrs)
    fclose(fattrs);

  if (fguard)
    fclose(fguard);

  if (finput)
    fclose(finput);

  if (fparser)
    fclose(fparser);

  if (foutput)
    fclose(foutput);

	/* JF write out the output file */
  if (k == 0 && ftable)
    {
      FILE *ftmp;
      register int c;

      ftmp=tryopen(tabfile, "w");
      rewind(ftable);
      while((c=getc(ftable)) != EOF)
        putc(c,ftmp);
      fclose(ftmp);
      fclose(ftable);
    }

#ifdef VMS
  delete(actfile);
  delete(tmpattrsfile);
  delete(tmptabfile);
  if (k==0) sys$exit(SS$_NORMAL);
  sys$exit(SS$_ABORT);
#else
  exit(k);
#endif
}
