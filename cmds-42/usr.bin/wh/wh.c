/*
 *  wh -- a program for finding instances of files along paths
 *      Composed from pieces of jag's wh.c for path searching,
 *	uses expand(3) to grok wildcards,
 *	normally uses ls(1) to format output, but
 *	handles emacs(1) error messages (-X) locally.
 *
 *      Options known to wh(1):
 *        name    -- search along current path for instances of name
 *        -f name -- search etc., useful if name starts with '-'
 *        -q      -- quit search after finding first instance of file
 *        -p path -- set path to search along
 *        -C      -- set path to CPATH
 *        -E      -- set path to EPATH
 *        -L      -- set path to LPATH
 *        -M      -- set path to MPATH
 *        -P      -- set path to  PATH
 *        -R      -- recursive directory search
 *	  -X      -- list names in emacs(1) error format
 *        --      -- pass remainder of switch to ls(1)
 *	All other switches (arguments starting with '-') are passed
 *	through as formatting options to ls(1) (collisions on -fqC).
 *
 *	Exit codes:
 *	  0 - if at least 1 one file was found,
 *	  1 - if no files were found,
 *	  2 - if some error was encountered
 *
 *  HISTORY
 * 03-Mar-86  Bob Fitzgerald (rpf) at Carnegie-Mellon University
 *	Added lsargs to fix old bug caused by addition of lsarggrow.
 *	Considered lazy-evaluating calls on ls(1) to cut down on processes,
 *	but didn't because ls sorts its input.
 *
 * 06-Feb-86  Bob Fitzgerald (rpf) at Carnegie-Mellon University
 *	Converted for 4.2 directory structure.
 *	Sent diagnostic output to stderr.
 *
 * 05-May-82  Bob Fitzgerald (rpf) at Carnegie-Mellon University
 *	Replaced home-brew runls() that invoked ls(1) with a call on the
 *	runvp(3) utility.
 *
 * 29-Apr-82  Bob Fitzgerald (rpf) at Carnegie-Mellon University
 *	Beefed up parser to recognize blocks of switches, extract those
 *	meaningful here and pass the rest to ls(1).  Circumvented two
 *	bugs in expand(3).  First, check file names returned to ensure
 *	that they really exist.  Second, grow buffer when necessary
 *	(return from expand is -1, not bufsize+1 as is documented).
 *
 * 28-Mar-82  Bob Fitzgerald (rpf) at Carnegie-Mellon University
 *	Created.
 *
 */

static char sccsid[] = "@(#) wh.c 3.2  3-Mar-86";

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <stdio.h>

extern unsigned char errno;
extern char *getenv();
extern char **malloc();

char *wh;
char ls[] = "ls";

int    lsargc;			/* number of free slots in ls arglist */
char **lsargv;			/* pointer to front of entire ls arglist */
char **lsargf;			/* pointer to first file name in ls arglist */
char **lsargp;			/* pointer to free slots in ls arglist */
char **lsargs;			/* pointer to saved spot in ls arglist */

char *pathname;
char *path;
char *givenname;
char  namebuffer[256];

int   qflag;			/* quit after first find(s) when set */
int   Rflag;			/* recursive directory search when set */
int   Xflag;			/* emacs(1) format error messages when set */

int   namehits;
int   totalhits;


/*
 *  Management of argument list to ls(1).
 *  LSARGSLACK must be at least 1 to allow room for NULL terminator.
 *  Slots in use are at the beginning (lsargp-lsargv) and end (LSARGSLACK).
 *  List is doubled in size each time it is grown.
 */
#define LSARGSLACK  2
#define LSARGINIT   126
#define lsargused   ((lsargp-lsargv)+LSARGSLACK)
#define lsargnext(cur) (cur<<1)


/*
 *  newpath -- takes the string name of a path (e.g. "PATH"),
 *      looks it up in the environment and remembers pointers
 *      to both pathname and path in global variables.
 *      Make sure storage for pathname and path remains valid,
 *	as only pointers are preserved here.
 */
newpath(fpathname)
    char *fpathname;
  {
    pathname = fpathname;
    path = getenv(fpathname);
    if (!path)
        quit(2, "%s:  path %s not found in environment\n", wh, pathname);
  }


/*
 *  lsarggrow -- increase size of arg list to expand(3) and ls(1).
 *	Copies front of old arglist into new arglist.
 *	Updates lsargc, lsargv, lsargf, lsargs, lsargp for new arg list.
 */
lsarggrow(fullname)
    char *fullname;
  {
    register char **oldp;
    register char **newp;

    lsargc = lsargnext(lsargc+lsargused);
    oldp = lsargv;
    newp = malloc(lsargc*sizeof(char *));
    if (newp == NULL)
        quit(2, "%s:  ran out of free space expanding %s\n", wh, fullname);
    lsargf = newp + (lsargf - lsargv);
    lsargs = newp + (lsargs - lsargv);
    lsargv = newp;
    while (oldp < lsargp)
        *newp++ = *oldp++;
    lsargp = newp;
    lsargc -= lsargused;
  }


/*
 *  runls -- prints the entries in lsargv
 *	either here (for -X) or by invoking ls(1)
 *	unwinds lsargv back to lsargf.
 */
runls()
  {
    if (Xflag)
      {
        char **lead;
        for (lead = lsargs; *lead; lead++)
            printf("\"%s\", line 1:\n", *lead);
      }
    else
      {
        int retcode;

        retcode = runvp(ls, lsargv);
        if (-1 == retcode)
            quit(2, "%s:  error %d in executing %s\n", wh, errno, ls);
        else if (0 < retcode)
            quit(2, "%s:  %s returned status %d\n", wh, ls, retcode);
      }
    lsargc += (lsargp - lsargf);
    lsargp = lsargf;
  }

/*
 *  lookup -- takes a full file name, looks it up and
 *	generates output for anything found.
 *      Records successful lookups by incrementing namehits and totalhits.
 *      Returns 0 on a hit with qflag set, otherwise returns non-zero
 */
int lookup(fullname)
    char *fullname;
  {
    lsargs = lsargp;
    if (*fullname != '\0')
	    strcat(fullname, "/");
    reclookup(fullname);
    if (lsargp == lsargs)
        return(1);

#ifndef LAZYLS
    runls();
#endif
    namehits++;
    totalhits++;
    return(!qflag);
  }

/*
 *  reclookup --
 */
reclookup(fullname)
    char *fullname;
  {
    int found;
    char **lead;

    if (Rflag)
      {
        int pathlen;
        DIR *dirp;

        pathlen = strlen(fullname);
        if (pathlen > 0)
          {
            fullname[pathlen-1] = NULL;
            if (NULL == (dirp = opendir(fullname)))
                fprintf(stderr, "couldn't open \"%s\"\n", fullname);
            strcat(fullname,"/");
          }
        else
          {
            if (NULL == (dirp = opendir(".")))
                fprintf(stderr, "couldn't open \".\"\n");
          }
        if (NULL != dirp)
          {
            struct direct *dp;

            while (NULL != (dp = readdir(dirp)))
              {
                if ( (0 != dp->d_ino) &&
                     (('.' != dp->d_name[0])) &&
                     (0 != strcmp(dp->d_name, ".")) &&
                     (0 != strcmp(dp->d_name, "..")) )
                  {
                    struct stat statbuf;
                    strcat(fullname, dp->d_name);
                    if (-1 != stat(fullname, &statbuf))
                      {
                        if (S_IFDIR & statbuf.st_mode)
                          {
                            strcat(fullname, "/");
                            reclookup(fullname);
                          }
                      }
                    else fprintf(stderr, "%s:  can't open directory %s\n", wh, fullname);
                    fullname[pathlen] = NULL;
                  }
              }
            closedir(dirp);
          }
      }

    /*
     *  expand wildcards
     *  return non-zero for nothing found
     *  check for expansion errors (expand apparently returns -1 for
     *    too-many-names, not lsargc+1 as is documented)
     *  NULL-terminate parameter list
     */
    strcat(fullname, givenname);
    found = expand(fullname,lsargp,lsargc);
    while ( (found < 0) || (lsargc < found) )
      {
        lsarggrow(fullname);
        found = expand(fullname,lsargp,lsargc);
      }
    *(lsargp+found) = 0;

    /*
     *  scan expanded list, making sure that the files really exist
     *  (since expand doesn't bother to check while expanding wildcards
     *  in directory names in the middle of paths).
     *  compress any bogus entries (before ls(1) gets them and prints
     *  some idiot message about file not existing).
     *  Check again for no acceptable files, returning non-zero if so
     */
    lead = lsargp;
    while (*lsargp = *lead)
      {
        static struct stat buf;
        if (!stat(*lead++, &buf))
          {
            lsargp++;
            lsargc--;
          }
      }
  }


/*
 *  searchpath -- look for instances of filename on path recorded
 *      in global variable path.  Gripe if nothing found.
 *	Global givenname is used to pass filename into lookup.
 *	Global namehits is incremented by lookup when appropriate.
 */
searchpath(filename)
    char *filename;
  {
    namehits = 0;
    givenname = filename;
    searchp(path, "", namebuffer, lookup);
    if (!namehits)
        fprintf(stderr, "%s:  %s not found on %s\n", wh, filename, pathname);
  }


/*
 *  switchblock -- parse one switch block, being given a pointer to
 *	the first character after the '-'.
 */
switchblock(swp)
    char *swp;
  {
    char option;
    char *leadp  = swp;			/* next option to look at */
    char *trailp = swp;			/* next place to put ls(1) option */

    /*
     *  Scan over switches in block
     *  processing those we know (qCLMPRX) and eliminating them,
     *  compacting those we doesn't know about for later.
     */
    while (option = *leadp++)
      {
        switch (option)
          {
          case 'q': qflag++; break;
          case 'R': Rflag++; break;
          case 'X': Xflag++; break;
          case 'C': newpath("CPATH"); break;
          case 'E': newpath("EPATH"); break;
          case 'L': newpath("LPATH"); break;
          case 'M': newpath("MPATH"); break;
          case 'P': newpath( "PATH"); break;
          default : *trailp++ = option; break;
          }
      }

    /*
     *  If anything remains to be passed to ls(1),
     *  NULL terminate the switch block and back up over the '-'
     *  before appending block to the ls arg list.
     */
    if (trailp != swp)
      {
#ifdef LAZYLS
        if (lsargf != lsargp)
            runls();
#endif
        *trailp++ = 0;
        *lsargp++ = swp-1;
        lsargc--;
        lsargf = lsargp;
      }
  }


/*
 *  Main program -- parse command line
 */
main(argc, argv)
    int    argc;
    char **argv;
  {
    wh = argv[0];
    if (1 == argc)
        quit(2, "Usage:  %s { -qCELMPRX | -p path | -f file | file }\n", wh);

    totalhits = 0;
    newpath("PATH");
    lsargc = LSARGINIT;
    lsargp = lsargv = 0;
    lsarggrow(ls);
    *lsargp++ = ls;
    lsargc--;
    lsargf = lsargp;
    while (0 < --argc)
      {
        if ('-' == **++argv)
          {
            switch (*++*argv)
              {

              case 'p':
                if (0 >= --argc)
                    quit(2, "%s:  path name expected after -p\n", wh);
                newpath(*++argv);
              break;

              case 'f':
                if (0 >= --argc)
                    quit(2, "%s:  file name expected after -f\n", wh);
                searchpath(*++argv);
              break;

              case '-':
#ifdef LAZYLS
                if (lsargf != lsargp)
                    runls();
#endif
                *lsargp++ = *argv;
                lsargc--;
                lsargf = lsargp;
              break;

              default :
                switchblock(*argv);
              break;
              }
          }
         else
            searchpath(*argv);
      }
#ifdef LAZYLS
    if (lsargf != lsargp)
        runls();
#endif
    exit(totalhits == 0);
  }

