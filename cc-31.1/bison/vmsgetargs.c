/* VMS version of getargs; Uses DCL command parsing.
   Copyright (C) 1989 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#include <ctype.h>

/*
 *	VMS version of getargs(): Uses DCL command parsing
 *		(argc and argv are ignored)
 */
int verboseflag;
int definesflag;
int debugflag;
int nolinesflag;
extern int fixed_outfiles;

getargs(argc,argv)
     int argc;
     char *argv[];
{
  register char *cp;
  static char Input_File[256];
  extern char *infile;

  verboseflag = 0;
  definesflag = 0;
  debugflag = 0;
  fixed_outfiles = 0;
  nolinesflag = 0;
  /*
   *	Check for /VERBOSE qualifier
   */
  if (cli_present("BISON$VERBOSE")) verboseflag = 1;
  /*
   *	Check for /DEFINES qualifier
   */
  if (cli_present("BISON$DEFINES")) definesflag = 1;
  /*
   *	Check for /FIXED_OUTFILES qualifier
   */
  if (cli_present("BISON$FIXED_OUTFILES")) fixed_outfiles = 1;
  /*
   *	Check for /NOLINES qualifier
   */
  if (cli_present("BISON$NOLINES")) nolinesflag = 1;
  /*
   *	Check for /DEBUG qualifier
   */
  if (cli_present("BISON$DEBUG")) debugflag = 1;
  /*
   *	Get the filename
   */
  cli_get_value("BISON$INFILE", Input_File, sizeof(Input_File));
  /*
   *	Lowercaseify the input filename
   */
  cp = Input_File;
  while(*cp) {
    if (isupper(*cp)) *cp = tolower(*cp);
    cp++;
  }
  infile = Input_File;
}

/************		DCL PARSING ROUTINES		**********/

/*
 *	See if "NAME" is present
 */
int
cli_present(Name)
     char *Name;
{
  struct {int Size; char *Ptr;} Descr;

  Descr.Ptr = Name;
  Descr.Size = strlen(Name);
  return((cli$present(&Descr) & 1) ? 1 : 0);
}

/*
 *	Get value of "NAME"
 */
int
cli_get_value(Name,Buffer,Size)
     char *Name;
     char *Buffer;
{
  struct {int Size; char *Ptr;} Descr1,Descr2;

  Descr1.Ptr = Name;
  Descr1.Size = strlen(Name);
  Descr2.Ptr = Buffer;
  Descr2.Size = Size-1;
  if (cli$get_value(&Descr1,&Descr2,&Descr2.Size) & 1) {
    Buffer[Descr2.Size] = 0;
    return(1);
  }
  return(0);
}

