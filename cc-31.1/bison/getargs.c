/* Parse command line arguments for bison,
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


#include <stdio.h>
#include "files.h"

int verboseflag;
int definesflag;
int debugflag;
int nolinesflag;
extern int fixed_outfiles;/* JF */

void
getargs(argc, argv)
int argc;
char *argv[];
{
  register int c;
  char *p = argv[0];
  char *lastcomponent;

  extern int optind;
  extern char *optarg;

  verboseflag = 0;
  definesflag = 0;
  debugflag = 0;
  fixed_outfiles = 0;

#if 0 /* Let's avoid dependence on what name invoked with.
	 The file `yacc' can be a shell script that runs `bison -y'.  */
  /* See if the program was invoked as "yacc".  */

  lastcomponent = p;
  while (*p)
    {
      if (*p == '/')
	lastcomponent = p + 1;
      p++;
    }
  if (! strcmp (lastcomponent, "yacc"))
    /* If so, pretend we have "-y" as argument.  */
    fixed_outfiles = 1;
#endif

  while ((c = getopt (argc, argv, "yvdlto:")) != EOF)
    switch (c)
      {
      case 'y':
	fixed_outfiles = 1;
	break;

      case 'v':
        if(optind && argv[optind] && !strcmp(argv[optind],"-version")) {
	  extern char *version_string;

	  printf("%s",version_string);
	  while(getopt(argc,argv,"ersion")!='n')
	   ;
	} else
	  verboseflag = 1;
	break;

      case 'd':
	definesflag = 1;
	break;

      case 'l':
	nolinesflag = 1;
	break;

      case 't':
	debugflag = 1;
	break;

      case 'o':
	spec_outfile = optarg;
      }

  if (optind == argc)
    fatal("grammar file not specified");
  else
    infile = argv[optind];

  if (optind < argc - 1)
    fprintf(stderr, "bison: warning: extra arguments ignored\n");
}
