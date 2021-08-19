/*
 * This library is expected to be linked with termlib.
 *
 * System Library interface: 
 *	tenex_editmode(char *newmode);
 *		Sets the editmode to newmode (either vi or emacs or dumb).
 *	tenex_tenex_history();
 *		Lists current history list.
 *	tenex_set_breakchars(char *chars);
 *	tenex_set_macrofiles(char *files);
 *	tenex_set_savemacros(char *file);
 *	tenex_dobind(char *arglist);
 *	tenex_suspend();
 *	tenex_read(char *list, int size);
 *
 * The external function "Init_Bindings()" is called to initialize any bindings
 * before the first read. 
 */


/* This was derived by Steve Stone from the original "tenex.c" from the
 * CMU extended csh command.  It has been implemented as a general purpose
 * library routine.
 *
 * The editor was written by Duane Williams with assistance
 * from Doug Philips, who also wrote the interface to the history
 * list and the code to save and restore keyboard macros in files.
 * The tenex style filename recognition was written by Ken Greer.
 *
 * Copyright (C) 1983,1984 by Duane Williams
 * 
 **********************************************************************
 * HISTORY
 * 26-Oct-88  Steve Stone (steve@next.com)
 *	Reimplemented as a general purpose library routine.  Added local history
 *	and real history incremental search.
 * 14-Sep-87  Trey Matteson (trey@next.com)
 *	Made DUMB mode source .bindings file
 *	Made all modes set IgnoreEOF
 *	Moved hack test for ^d from filenamelist() to tenex()
 *
 * 22-May-86  Bradley White (bww) at Carnegie-Mellon University
 *	Added some missing default bindings for "vi" mode of the
 *	command line editor.  Added EnterViAppend().
 *
 * 07-May-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Replaced srccat with srccatdef and srccathome.
 *
 * 15-Sep-85  Duane Williams (dtw) at Carnegie-Mellon University
 *	Added dosetupterm procedure which simply resets the Initialized
 *	flag, causing tenex to be reinitialized.  Also substituted calls
 *	to the printprompt function in sh.c for our own, thereby
 *	eliminating a bug.
 *
 * 09-Sep-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Modified call to srccat() to allow standard bindings file.
 *
 * 27-Aug-84  Duane Williams (dtw) at Carnegie-Mellon University
 *	Added DeleteWord to the list of Functions!
 *
 * 25-Mar-84  Duane Williams (dtw) at Carnegie-Mellon University
 *	Changed DefineNamedMacro to always eliminate a previous macro
 *	with the specified name.  Changed the names of the old
 *	SearchForward and SearchReverse routines to
 *	IncrementalSearchForward and IncrementalSearchReverse and
 *	defined new routines with the old names.  Added InsertChar to
 *	the list of user accessible functions.  Fixed a bug in Return by
 *	adding a call to EndOfLine.  Fixed bug in InsertChar that caused
 * 	an addressing error when the repetition count was large; also put
 *	a limit on the size of the repetition count.
 *
 * 24-Mar-84  Duane Williams (dtw) at Carnegie-Mellon University
 *	Implemented macro binding to arbitrary keys, allowed macros to
 *	be interrupted, allowed nested macro execution, added "bind"
 *	command to the shell, and made tenex read ~/.bindings during
 *	initialization.
 *
 * 18-Mar-84  Duane Williams (dtw) at Carnegie-Mellon University
 *	History expansions were being erroneously processed by tenex
 *	as interactive input rather than simply being copied into the 
 *	input buffer.  This has been fixed by a change to GetChar.
 *
 * 11-Mar-84  Duane Williams (dtw) at Carnegie-Mellon University
 *	Modified FilenameList to exit on ^D if in DUMB mode and at
 *	beginning of line and ignoreeof is FALSE.
 *
 * 03-Mar-84  Duane Williams (dtw) at Carnegie-Mellon University
 *	Fixed bug in SaveMacroFile that allowed it to overwrite an
 *	existing file.  Declared WriteMacroFile and ReadMacroFile to be
 *	hidden.
 *
 * 13-Feb-84  Duane Williams (dtw) at Carnegie-Mellon University
 *	History expansions were being erroneously processed by tenex
 *	rather than simply being copied into the input buffer.  This has
 *	been fixed.  Implemented several new macro related features:
 *	(1) macros can be bound to arbitrary keys, (2) macros can call
 *	other macros, and (3) macros are interruptable.
 *
 * 16-Jan-84  Duane Williams (dtw) at Carnegie-Mellon University
 *	Fixed a bug in handling absent termcap info that prevented
 *	putting the editor into DUMB mode.  Improved error checking and
 *	error messages related to termcap failures.
 *
 * 14-Jan-84  Duane Williams (dtw) at Carnegie-Mellon University
 *	Added EndOfFile function so lusers can screw themselves more easily.
 *	Added ClearScreen, EraseLine, and selectable bell capability.
 *	Tty control mechanisms are now extracted from the termcap database.
 *
 * 23-Nov-83  Duane Williams (dtw) at Carnegie-Mellon University
 *	Rewrote InsertChar, believe it or not, to handle buffer overflow
 *	correctly.  Fixed action on return from Repetition.
 *
 * 06-Nov-83  Duane Williams (dtw) at Carnegie-Mellon University
 *	Added a "dumb" mode in which the shell behaves similar to the
 *	old cmu cshell.  The editing mode is now dependent on the shell
 *	variable "editmode".  Added the "breakchars" to allow the user
 *	to specify the word break chars.  CRMOD is now set each time the
 *	shell goes into CBREAK mode.  The quit key ^\ no longer
 *	generates a quit signal to the shell.
 *
 * 10-Oct-83  Doug Philips (dwp) at Carnegie-Mellon University
 *	Get input from yet another place ('ShellTypeAheadToTenex').
 *
 * 10-Oct-83  Doug Philips (dwp) at Carnegie-Mellon University
 *	Added macro file saving and restoring.  Only a minimal amount
 *	of error checking is done.  If something breaks, it will be because
 *	some turkey hand edited a macro file and got it wrong.  They don't
 *	deserve any more than they will get.  Note: You can place comments
 *	after the end of the macro and before the newline and they will be
 *	ignored.
 *
 * 05-Oct-83  Duane Williams (dtw) at Carnegie-Mellon University
 *	Added a bunch of new default "space" chars for forward and
 *	backward word commands.  Removed the default bindings for search
 *	functions.  Added the Tab function and default binding to ^I.
 *	Disallowed inserting chars into the middle of a line when the
 *	input buffer is full.  There's nothing else to do:  the shell
 *	just wasn't designed with this in mind.
 *
 * 21-Aug-83  Duane Williams (dtw) at Carnegie-Mellon University
 *	Converted to run in CBREAK rather than in RAW mode.  This seems
 *	to solve all typeahead problems and eliminates problems related
 *	to characters pushed back into the terminal input buffer.
 *
 * 14-Aug-83  Duane Williams (dtw) at Carnegie-Mellon University
 *	Typeahead is now handled correctly and control characters can
 *	now be displayed properly.  A new vi mode has been added; the
 *	EDITOR environment variable determines whether one gets emacs
 *	style or vi style command line editing.
 *
 * 30-Jul-83  Duane Williams (dtw) at Carnegie-Mellon University
 *	Doug and I added named keyboard macros, and I added a kill
 *      buffer.
 *
 * 30-Jul-83  Doug Philips (dwp) at Carnegie-Mellon University
 *	Added history scanning.
 *
 * 24-Jul-83  Duane Williams (dtw) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */

/*
 * The following functions exist and can be bound to keys using the
 *  bind-to-key command (see below):
 *
 *  Function				Default emacs		Default vi
 *  --------				-------------		----------
 *  appendtoeol                                                    ^XA
 *  backspace                               ^B                     ^X^H or ^Xh
 *  backwardword                            \eb                    ^XB or ^Xb
 *  beginningofhistory                      \e<
 *  beginningofline                         ^A                     ^X0 or ^X\^
 *  changechar                                                     ^Xs
 *  changefollowingobject                                          ^Xc
 *  changetoeol                                                    ^XC
 *  changewholeline                                                ^XS
 *  clearscreen                             ^L
 *  defaultbinding
 *  definenamedmacro                        \en
 *  deletecurrentchar                       ^D                     ^Xx
 *  deletefollowingobject
 *  deletepreviouschar                      ^H and DEL             ^XX and ^H
 *  deleteword                              \ed
 *  endoffile
 *  endofhistory                            \e>
 *  endofline                               ^E                     ^X$
 *  enterviappend                                                  ^Xa
 *  enterviinsert                                                  ^Xi
 *  eraseline
 *  eraseword                               \eh                    ^W
 *  executemacro
 *  executenamedmacro                       \ex
 *  executeunnamedmacro                     \ee                    ^X@
 *  exitviinsert                                                   \e
 *  forwardchar                             ^F                     ^Xl or ^Xspc
 *  forwardword                             \ef                    ^Xw or ^XW
                                                                   or X^e
 *  incrementalsearchforward                \e/                    ^X/
 *  incrementalsearchhistoryforward         \es                    ^Xs
 *  incrementalsearchhistoryreverse         \er                    ^Xr
 *  incrementalsearchreverse                \e?                    ^X?
 *  insertatbol                                                    ^XI
 *  insertchar
 *  insertliteralchar                       ^Q                     ^Q or ^V
 *  killregion                              ^W                     ^U
 *  killtoeol                               ^K                     ^XD
 *  loadmacrofile                           ^X^R
 *  nexthistentry                           ^N                     ^X+ or ^Xj
 *                                                                 ^X^N or ^N
 *  previoushistentry                       ^P                     ^X- or ^Xk
 *                                                                 ^X^P or ^P
 *  redisplay                               ^R                     ^X^L or ^X^R
 *                                                                 ^Xz or ^L
 *                                                                 ^R
 *  repetition                              ^U                     ^X1 ... ^X9
 *  replacechar
 *  return                                  \n or \r               ^X\n or ^X\r
 *                                                                 \n or \r
 *  savemacrofile                           ^X^S
 *  searchforward                                                  ^Xf
 *  searchreverse                                                  ^XF
 *  setmark                                 ^@                     ^Xm
 *  startremembering                        ^X(
 *  stopremembering                         ^X)
 *  suspend                                 ^Z                     ^Z
 *  tab                                     ^I                     ^I
 *  transposechars                          ^T
 *  viyankkillbuffer                                               ^Xp or ^XP
 *  yankkillbuffer                          ^Y
 *
 */

#include <sgtty.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>

#define CTRL_AT '\000'
#define CTRL_A '\001'
#define CTRL_B '\002'
#define CTRL_C '\003'
#define CTRL_D '\004'
#define CTRL_E '\005'
#define CTRL_F '\006'
#define CTRL_G '\007'
#define CTRL_H '\010'
#define CTRL_K '\013'
#define CTRL_L '\014'
#define CTRL_N '\016'
#define CTRL_P '\020'
#define CTRL_Q '\021'
#define CTRL_R '\022'
#define CTRL_S '\023'
#define CTRL_T '\024'
#define CTRL_U '\025'
#define CTRL_V '\026'
#define CTRL_W '\027'
#define CTRL_X '\030'
#define CTRL_Y '\031'
#define CTRL_Z '\032'
#define ESCAPE '\033'
#define LINEFEED '\n'
#define RETURN   '\r'
#define RUBOUT '\177'
#define SPACE	  ' '
#define TAB    '\t'
 
#define BINDINGS  128
#define MAXSPACES 128
#define MAXINT	  0x7FFF

#define DEF	       Editor
#define READ(buf,n)    read(SHIN,buf,n)
#define ISDIGIT(c)     ('0' <= (c) && (c) <= '9')
#define EQ(a,b)	       strcmp(a,b)==0

#define SETBINDING(keymap,function,key)	 keymap[key].Routine = function
#define REGBIND(function,key) SETBINDING(RegBindings,function,key)
#define ESCBIND(function,key) SETBINDING(EscBindings,function,key)
#define CNXBIND(function,key) SETBINDING(CnxBindings,function,key)

#define visible
#define hidden static
#define loop for(;;)

#undef TRUE
#undef FALSE
typedef enum { FALSE, TRUE } boolean;
typedef enum {EMACS,VI,ESC,CNX,RET,QUO,REP,INS,OVERFLOW,DUMB,EOF} code;
typedef enum { BACK, FORW } direction;
typedef enum { LIST, RECOGNIZE } COMMAND;

typedef struct {
    char * Head;
    char * Tail;
} String;

typedef code (*CodeFunction)();

typedef struct {
    CodeFunction Routine;
    char	 MacroName;
} KeyBinding;


/*---------------------------------------------------------------------------
 * Global variables, but not seen outside this package.
 *---------------------------------------------------------------------------
 */
hidden KeyBinding RegBindings [BINDINGS]; /* Regular keystroke bindings */
hidden KeyBinding EscBindings [BINDINGS]; /* Escape + keystroke bindings */
hidden KeyBinding CnxBindings [BINDINGS]; /* Control-X + keystroke bindings */
hidden KeyBinding * Bindings;		/* current keymap */
hidden KeyBinding * OldBindings;

#define BUFSIZ 1024
#define SHOUT 1
#define SHIN 0

hidden char CurrentChar;
hidden String KeyboardMacro [BINDINGS]; /* named keyboard macros */
hidden String UnNamedMacro;		/* unnamed keyboard macro */
hidden char MacroBuffer [BUFSIZ+2];	/* buffer for macro input */
hidden char SpaceBuffer [BUFSIZ];	/* used to clear line on tty */
hidden char BackSpaceBuffer [BUFSIZ];	/* used to backup cursor */
hidden char KillBuffer [BUFSIZ];
hidden char * KillTail = KillBuffer;
hidden char * InputBuffer;		/* beginning of input buffer */
hidden char * InputPos;		        /* current position (cursor) */
hidden char * InputTail;		/* after last char of input */
hidden char * Mark;		        /* remember position in input */
hidden char * MacroStart = 0;		/* executing macro pointer */
hidden char * MacroTail;		/* after last char of macro */
hidden char * MacroFinal = MacroBuffer + sizeof(MacroBuffer);
hidden boolean MacroDefined = FALSE;
hidden boolean DefiningMacro = FALSE;
hidden String * MacroStack = 0;	        /* macros can call macros */
hidden int MacroStackDepth = 0;	        /* current PushMacro position */
hidden int MacroStackLimit = 0;	        /* size of the macro stack */
hidden char SpaceChar [MAXSPACES];      /* chars to treat as spaces */
hidden int InputBufferSize;
hidden int RepetitionCount;	        /* repeat factor for functions */
hidden boolean IgnoreEOF = TRUE;
hidden boolean AudibleBell = TRUE;      /* whether to use audible bell */
hidden boolean VisibleBell = FALSE;	/* whether to use visible bell */
hidden boolean ShowControlChars = TRUE; /* reset from local mode word */
hidden boolean ControlCharsInInput = FALSE;
hidden code Editor;			/* set from editmode variable */
hidden boolean Initialized = FALSE;
hidden boolean MagicChar = FALSE;       /* is there a pushed back char? */
hidden char MagicPushBack;		/* single push back for repeat */
hidden struct sgttyb TtySgttyb;
hidden struct tchars TtyTchars, TenexTchars;
hidden struct ltchars TtyLocalChars, TenexLchars;
hidden int TtyLocalModeWord;
hidden char ReprintChar, WordErase, LiteralNext;
hidden char EraseChar, KillChar;

/* Termcap Storage */
hidden boolean TermKnown;
hidden char areabuf [256];		/* storage for tgetstr() */
hidden char * BackSpaceSequence;	/* pointers into areabuf */
hidden char * ClearScreenSequence;
hidden char * DeleteCharacterSequence;
hidden char * EndDeleteSequence;
hidden char * EndInsertSequence;
hidden char * InsertPrefixSequence;
hidden char * InsertSuffixSequence;
hidden char * StartDeleteSequence;
hidden char * StartInsertSequence;
hidden char * VisibleBellSequence;
hidden int LengthOfBackSpaceSequence;
hidden int LengthOfClearScreenSequence;
hidden int LengthOfDeleteCharacterSequence;
hidden int LengthOfEndDeleteSequence;
hidden int LengthOfEndInsertSequence;
hidden int LengthOfInsertPrefixSequence;
hidden int LengthOfInsertSuffixSequence;
hidden int LengthOfStartDeleteSequence;
hidden int LengthOfStartInsertSequence;
hidden int LengthOfVisibleBellSequence;

/* Command history */
hidden int maxhist = 100;		/* Should be settable */
hidden int wrapped = 0;
hidden int curhist, lasthist, histenter; /* History pointers */
hidden char **histlist;  /* Actual list of history entries */

/* Forward static decls */
hidden Crlf(), NewLine(), Beep(), CopyNChars(), InitBuffers();
hidden code BeginningOfLine(), EndOfLine();
hidden PopMacro(), Enter_Hist(), Prev_Hist(), Next_Hist();
hidden InsertCurrentHistEntry(), dobindings(), InitBindings();
hidden InitEmacsBindings(), InitViBindings(), InitDumbBindings();
hidden InitDefaultBindings(), keytran();
hidden printprompt(), stablk(), fprstab(), getbool();
hidden save_tty_state(), restore_tty_state(), MoveNChars();
hidden int min();
hidden char *folddown();

visible int tenex_mode = 0;

/*---------------------------------------------------------------------------
 * The following three functions control output to the tty and ensure the
 * proper display of control chars, if requested.
 *---------------------------------------------------------------------------
 */
hidden int
PrintingLength (string, length)
register char * string;
register int length;
{
    register char * end = string + length;
    
    if (! ControlCharsInInput) {
        return length;
    }
    
    if (ShowControlChars) {
	for (; string < end; string++) {
	    if (*string < SPACE) length++;
	}
    }
    else {
        for (; string < end; string++) {
	    if (*string < SPACE) length--;
	}
    }
    
    return length;
}

/* This function writes 'length' chars to the tty from a buffer starting
   at 'string'. */
hidden
WriteToScreen (string, length)
register char * string;
register int length;
{
    static char buf[2] = {'^',0};
    
    if (ControlCharsInInput) {
	register char * p, * end;
	
	end = string + length;

	while (string < end) {
	    for (p = string; p < end && *p >= SPACE && *p < RUBOUT; p++);
	    write (SHOUT, string, p - string);
	    
	    if (ShowControlChars && p < end) {
		buf[1] = (*p + 0100) & 0177; /* print RUBOUT as ^? */
		write (SHOUT, buf, 2);
	    }
	    
	    string = p + 1;
	}
    }
    else {
        write (SHOUT, string, length);
    }
}

/* This function backs up over 'length' chars on the tty determining the
   proper printing length from examination of a buffer starting at
   'string' */
hidden
Backup (string, length)
register char * string;
register int length;
{
    if (ControlCharsInInput) {
	length = PrintingLength (string, length);
    }

    /* Most, if not all, of our terminals backspace with ^H; so we
       optimize backspacing by using the backspace buffer.  I hope the
       else clause is never executed. */

    if (LengthOfBackSpaceSequence == 1) {
        write (SHOUT, BackSpaceBuffer, length);
    }
    else {
	register int i;
	
        for (i = 0; i < length; i++) {
	    write( SHOUT, BackSpaceSequence, LengthOfBackSpaceSequence);
	}
    }
}

/*
 * BlotOut is called to write spaces over lenth bytes to erase string.  If
 * string contains control characters, write spaces to cover '^' characters.
 */
hidden
BlotOut (string, length)
register char * string;
register int length;
{
    if (ControlCharsInInput) {
        length = PrintingLength (string, length);
	write (SHOUT, SpaceBuffer, length);
    }
    else {
        write (SHOUT, SpaceBuffer, length);
    }
}

/*---------------------------------------------------------------------------
 * The tenex routine runs with the user's terminal in cbreak mode.  Since
 * output processing will still be done by the tty driver in this mode, we
 * have to disable it explicitly; otherwise the affected keys can't be
 * rebound in the editor.
 *---------------------------------------------------------------------------
 */

/* Save the initial state of tty */
hidden
SaveTtyState()
{
    static boolean inited = FALSE;

    if (inited) return;			   /* only do this routine once */
    
    ioctl (SHIN, TIOCGETC, &TtyTchars);	   /* save current tchars */

    TenexTchars = TtyTchars;

    /* Leave t_intrc alone.  Interrupts are for real. */
    TenexTchars.t_quitc  = -1;
    TenexTchars.t_startc = -1;
    TenexTchars.t_stopc  = -1;

    ioctl (SHIN, TIOCGLTC, &TtyLocalChars);	/* save ltchars */

    TenexLchars = TtyLocalChars;
    ReprintChar = TtyLocalChars.t_rprntc;
    WordErase   = TtyLocalChars.t_werasc;
    LiteralNext = TtyLocalChars.t_lnextc;
    
    TenexLchars.t_suspc  = -1;
    TenexLchars.t_dsuspc = -1;
    TenexLchars.t_rprntc = -1;
    TenexLchars.t_flushc = -1;
    TenexLchars.t_werasc = -1;
    TenexLchars.t_lnextc = -1;

    ioctl (SHIN, TIOCLGET, &TtyLocalModeWord);
    ShowControlChars = (TtyLocalModeWord & LCTLECH) ? TRUE : FALSE;

    ioctl (SHIN, TIOCGETP, &TtySgttyb);
    EraseChar = TtySgttyb.sg_erase;
    KillChar  = TtySgttyb.sg_kill;

    inited = TRUE;
}

/* Get the termcap entry for the terminal in the environment variable TERM */

hidden
ReadTermcap()
{
    register char *termname;
    register int i;
    static boolean Disabled = FALSE;
    
    TermKnown = FALSE;

    for (i = 0; i < sizeof(areabuf); i++) {
        areabuf[i] = '\0';
    }

    LengthOfBackSpaceSequence = 0;
    LengthOfClearScreenSequence = 0;
    LengthOfDeleteCharacterSequence = 0;
    LengthOfEndDeleteSequence = 0;
    LengthOfEndInsertSequence = 0;
    LengthOfInsertPrefixSequence = 0;
    LengthOfInsertSuffixSequence = 0;
    LengthOfStartDeleteSequence = 0;
    LengthOfStartInsertSequence = 0;
    LengthOfVisibleBellSequence = 0;

    /* Get termcap description of tty capabilities that we are interested
       in. */
    termname = (char *)getenv("TERM");
    if (termname) {	/* What is the terminal name? */
        char termbuf [1024];	    /* size required by tgetent() */
	char * area = areabuf;
	
	if (tgetent(termbuf,termname) != 1) {    /* 1 == success */
	    printf("Warning: no termcap entry for %s.  Editing disabled.\n",
		    termname);
	    /* We are totally screwed if we don't know how to
	       backspace! */
	    BackSpaceSequence = area;
	    *area++ = CTRL_H;
	    LengthOfBackSpaceSequence = 1;
	    goto EndReadTermcap;
	}
	else {
	    TermKnown = TRUE;
	}
		
	BackSpaceSequence = area;
	if (tgetflag("bs")) {		/* backspace with ^H */
	    *area++ = CTRL_H;
	    LengthOfBackSpaceSequence = 1;
	}
	else {
	    tgetstr("bc",&area);	/* alternate backspace */
	    LengthOfBackSpaceSequence = strlen(BackSpaceSequence);
	    if (LengthOfBackSpaceSequence <= 0) {
		printf("Warning: incomplete termcap entry.  Editing disabled.\n");
		/* We are totally screwed if we don't know how to
		   backspace! */
	        TermKnown = FALSE;
		*area++ = CTRL_H;	/* use best guess */
		LengthOfBackSpaceSequence = 1;
	    }
	}
	
	ClearScreenSequence = area;
	tgetstr("cl",&area);		/* clear screen sequence */
	LengthOfClearScreenSequence = strlen(ClearScreenSequence);
	/* It doesn't matter if the clear screen sequence is null. */
	
	/* See how the user wants errors signaled.  The default action is
	   an audible bell.  The user can also choose to have either no
	   signal, a visible bell, or both a visible and an audible bell. */
	if (TermKnown == FALSE) {
	    AudibleBell = VisibleBell = FALSE;
	}
	else {
	    AudibleBell = TRUE;
	    if (VisibleBell = (/*adrof("visiblebell") ? TRUE :*/ FALSE)) {
		VisibleBellSequence = area;
		tgetstr("vb",&area);	/* get visible bell sequence */
		LengthOfVisibleBellSequence = strlen(VisibleBellSequence);
		if (LengthOfVisibleBellSequence == 0) {
		    VisibleBell = FALSE;	/* tty can't do visible bell */
		}
		else {
		    AudibleBell = (/*adrof("audiblebell") ?*/ TRUE /*: FALSE*/);
		}
	    }
	}
	
	StartInsertSequence = area;
	tgetstr("im",&area);		/* enter insert mode */
	LengthOfStartInsertSequence = strlen(StartInsertSequence);

	EndInsertSequence = area;
	tgetstr("ei",&area);		/* exit insert mode */
	LengthOfEndInsertSequence = strlen(EndInsertSequence);
	
	InsertPrefixSequence = area;
	tgetstr("ic",&area);		/* send before char insert */
	LengthOfInsertPrefixSequence = strlen(InsertPrefixSequence);
	
	InsertSuffixSequence = area;
	tgetstr("ip",&area);		/* send after char insert */
	LengthOfInsertSuffixSequence = strlen(InsertSuffixSequence);
	
	StartDeleteSequence = area;
	tgetstr("dm",&area);		/* enter delete mode */
	LengthOfStartDeleteSequence = strlen(StartDeleteSequence);
	
	EndDeleteSequence = area;
	tgetstr("ed",&area);		/* exit delete mode */
	LengthOfEndDeleteSequence = strlen(EndDeleteSequence);
	
	DeleteCharacterSequence = area;
	tgetstr("dc",&area);		/* delete one char in del mode */
	LengthOfDeleteCharacterSequence = strlen(DeleteCharacterSequence);
    }
    else {
        printf("Warning: terminal type unknown.  Editing disabled.\n");
	/* We are totally screwed if we don't know how to
	   backspace! */
	BackSpaceSequence = areabuf;
	*areabuf = CTRL_H;	/* use best guess */
	LengthOfBackSpaceSequence = 1;
    }

EndReadTermcap:;

    if (Disabled && TermKnown) {
        printf("Editing enabled.\n");
    }
    Disabled = TermKnown ? FALSE : TRUE;
}

/* 
 * Enter and exit cbreak mode, called while collecting a line of input.
 * While an input line is being collected we are in cbreak mode.  The TTY
 * state is restored to its previous value after the line is collected.
 */
hidden int in_cbreak_mode;
hidden int was_in_cbreak_mode;

hidden
EnterCbreakMode()
{
#if 0
    int (*osigttou) ();
    int (*osigttin) ();
#endif

    if (! in_cbreak_mode) {
	ioctl (SHIN, TIOCGETC, &TtyTchars);
	ioctl (SHIN, TIOCGLTC, &TtyLocalChars);
	in_cbreak_mode = 1;
    }
#if 0
    osigttou = signal (SIGTTOU, SIG_IGN);
    osigttin = signal (SIGTTIN, SIG_IGN);
#endif

    ioctl (SHIN, TIOCSETC, &TenexTchars);
    ioctl (SHIN, TIOCSLTC, &TenexLchars);

    ioctl (SHIN, TIOCGETP, &TtySgttyb);
    TtySgttyb.sg_flags |= (CBREAK | CRMOD);
    TtySgttyb.sg_flags &= ~(ECHO | RAW);
    ioctl (SHIN, TIOCSETN, &TtySgttyb);

#if 0
    signal (SIGTTOU, osigttou);
    signal (SIGTTIN, osigttin);
#endif
}

hidden
ExitCbreakMode()
{
    ioctl (SHIN, TIOCSETC, &TtyTchars);
    ioctl (SHIN, TIOCSLTC, &TtyLocalChars);

    TtySgttyb.sg_flags |= ECHO;
    TtySgttyb.sg_flags &= ~CBREAK;
    ioctl (SHIN, TIOCSETN, &TtySgttyb);
    in_cbreak_mode = 0;
}


/*---------------------------------------------------------------------------
 * GetChar -- Except for pending input typed after a \n, this function
 * does all the input for the tenex package
 *---------------------------------------------------------------------------
 */
hidden int
GetChar (buf)
char * buf;
{
beginGetChar:
#ifdef notdef
    if (ShellTypeAheadToTenex) {    /* from history expansion */
	InsertString( ShellTypeAheadToTenex, ShellTypeAheadToTenex +
		strlen( ShellTypeAheadToTenex));
	ShellTypeAheadToTenex = 0;
    }
#endif
    if (MagicChar) {		    /* pushed back from repeat function */
        *buf = MagicPushBack;
	MagicChar = FALSE;
    }
    else if (MacroStart) {	    /* reading from user defined macro */
	if (MacroStart < MacroTail) {
	    *buf = *MacroStart++;
	}
	else {
	    PopMacro();
	    goto beginGetChar;
	}
    }
    else {			    /* interactive */
	int count;
	
	if ((count = READ (buf, 1))  <= 0 ) {
	    if (count == 0) {
	        SaveMacros();
		return 0;
	    }
	    else {
	        return 0;
	    }
	}
    }

    *buf &= 0177;

    if (DefiningMacro && !MacroStart) {	  	/* exec macros called
						   while defining a macro,
						   but don't copy them */
	if (UnNamedMacro.Tail >= MacroFinal) {  /* MacroFinal is ptr to
						   end of unnamed macro
						   buffer */
	    MacroDefined = DefiningMacro = FALSE;
	    Beep();
	}
	*UnNamedMacro.Tail++ = *buf;
    }

    return 1;
}

hidden char *editmode; /* Contains terminal edit mode type: emacs, vi, dumb */

/*---------------------------------------------------------------------------
 * Tenex -- this is the main routine. This routine returns an input line to 
 * the caller.
 *---------------------------------------------------------------------------
 */
visible int
tenex_read (inputline, inputlinesize)
char * inputline;
int inputlinesize;
{
    static boolean interrupted = FALSE;	   /* macros can be interrupted */
    
 /* initialize */

    InputBufferSize = inputlinesize;
    Mark = InputPos = InputTail = InputBuffer = inputline;
    RepetitionCount = 1;
    Bindings = OldBindings = RegBindings;
    ControlCharsInInput = FALSE;

    if (! Initialized) {
	char *ed = editmode;
		
	SaveTtyState();
	ReadTermcap();
        InitBuffers();
	Initialized = TRUE;
	dobindings( ed);
    }
    
    if (interrupted) {
	MacroStart = 0, MacroStackDepth = 0;
    }
    else {
        interrupted = TRUE;
    }
    
    EnterCbreakMode();

    loop {
	if (GetChar (&CurrentChar) == 0) goto endloop;
	
	switch ( (*Bindings[CurrentChar].Routine)() ) {
	    case RET:
	        goto endloop;
	        break;
	    case CNX:
	    case ESC:
	        break;
	    case OVERFLOW:
		ExitCbreakMode();
		return inputlinesize;
	    case DUMB:
	    case INS:
		RepetitionCount = 1;
		break;
	    case REP:
		Bindings = RegBindings;
	        break;
	    case EMACS:
		Bindings = RegBindings;
		RepetitionCount = 1;
		break;
	    case VI:
		Bindings = OldBindings;
		RepetitionCount = 1;
	        break;
	    case EOF:
	        goto endloop;
	        break;
	    default:
	        break;
	}
    }

endloop:;
    ExitCbreakMode();

    NewLine();
    interrupted = FALSE;
    if (*InputTail == '\n');
	InputTail--;
    *InputTail = 0;
    Enter_Hist(InputBuffer, InputTail - InputBuffer);
    return (InputTail - InputBuffer);
}

/*---------------------------------------------------------------------------
 * The following functions simply switch to one of the alternate keymaps.
 *---------------------------------------------------------------------------
 */
hidden code
SetCtrlX ()
{
    Bindings = CnxBindings;
    return CNX;
}

/* Escape key bindings */
hidden code
SetEscape ()
{
    OldBindings = Bindings;
    Bindings = EscBindings;
    return ESC;
}

/* enter insert mode */
hidden code
EnterViInsert ()
{
    if (Editor == VI) {
	Bindings = OldBindings = RegBindings;
	return INS;
    }
    return DEF;
}

/* enter append mode */
hidden code
EnterViAppend ()
{
    if (Editor == VI) {
	if (InputPos < InputTail)
	    MoveNChars(1, FORW);
	Bindings = OldBindings = RegBindings;
	return INS;
    }
    return DEF;
}

/* exet vi insert mode */
hidden code
ExitViInsert ()
{
    if (Editor == VI) {
	Bindings = OldBindings = CnxBindings;
	return INS;
    }
    return DEF;
}

/* Put a newline into the input stream */
hidden code
Return ()
{
    extern code EndOfLine();
    
    EndOfLine();
    *InputTail++ = '\n';
    return RET;
}

/* Mark end of file */
hidden code
EndOfFile ()
{
    return EOF;
}

/*---------------------------------------------------------------------------
 * Reads an emacs like repetition factor or sets the default factor 
 * (4 * the previous repetition factor)
 *---------------------------------------------------------------------------
 */
hidden code
Repetition ()
{
    long n;
    char buf;

    if (Editor == VI) {
        buf = CurrentChar;
    }
    else {
	GetChar (&buf);
    
	if (! ISDIGIT(buf)) {
	    MagicPushBack = buf;	  /* push back char */
	    MagicChar = TRUE;
	    n = RepetitionCount * 4;
	    if (n > MAXINT) {
	        Beep();
		return DEF;
	    }
	    RepetitionCount = n;
	    return REP;
	}
    }

    n = buf - '0';

    loop {
	GetChar (&buf);
	
	if (ISDIGIT(buf)) {
	    n = n * 10 + (buf - '0');
	    if (n > MAXINT) {
	        Beep();
		return DEF;
	    }
	}
	else {
	    MagicPushBack = buf;	  /* push back char */
	    MagicChar = TRUE;
	    break;
	}
    }

    RepetitionCount = n;
    return REP;
}

/*---------------------------------------------------------------------------
 * InsertChar -- put a char into the input buffer and echo it on the
 * user's terminal.
 *
 * This function is complicated by the possibilities for overflow of the
 * input buffer.  No matter where the cursor is on the line, if doing the
 * full insertion would overflow the buffer, then we just scream at the
 * user and return.  If the cursor is at the end of the line and inserting
 * the chars will exactly fill the buffer, then we insert them and tell
 * the shell to give us a new buffer.  This can't be done if the cursor
 * isn't at the end of the line since it would make the position of the
 * cursor incorrect.  Most of the time there is plenty of room; so we just
 * insert the chars and return to the user for more input.
 *
 * Note: the buffer is returned to the shell when it fills, not when it
 * overflows; so the Return function can put a \n in the buffer without
 * worrying about overflow.
 *---------------------------------------------------------------------------
 */
hidden code
InsertChar ()
{
    register int num, repetition = RepetitionCount;
    register char * c, * d;
    register char * oldPos = InputPos;
    register char * final = InputBuffer + InputBufferSize;
    
    if (CurrentChar < SPACE) {
        ControlCharsInInput = TRUE;
    }
    
    /* In middle of line. */
    if (InputTail > InputPos) {

	/* Cannot allow input to either fill or overflow buffer! */
	if ( repetition >= final - InputTail ) {
	    Beep();
	    return DEF;
	}
    
	/* Shift tail of line to make room for insertion. */
	c = InputTail - 1;
	d = c + repetition;
	
	for (; c >= InputPos; c--, d--) {
	    *d = *c;
	}

	/* Put chars in input buffer. */
	InputTail += repetition;
	while (repetition-- > 0) {
	    *InputPos = CurrentChar;
	    InputPos++;
	}
	WriteToScreen (oldPos, InputTail - oldPos);
	Backup (InputPos, InputTail - InputPos);
    }
    else {
	/* Cannot allow input to suddenly overflow input buffer! */
	if ( repetition > final - InputTail ) {
	    Beep();
	    return DEF;
	}
    
	InputTail += repetition;
	while (repetition-- > 0) {
	    *InputPos = CurrentChar;
	    InputPos++;
	} 
	WriteToScreen (oldPos, InputTail - oldPos);
    }

    if (InputTail == final) {
        return OVERFLOW;
    }
    
    return INS;
}

/* Insert next character without translation */
hidden code
InsertLiteralChar ()
{
    GetChar (&CurrentChar);
    return InsertChar();
}

/* Insert character with repetition */
hidden int
DoInsertChar ()
{
    if (InputTail + RepetitionCount >= InputBuffer + InputBufferSize) {
        return 0;
    }
    InsertChar ();
    return 1;
}

hidden int
InsertString (head, tail)
char * head;
register char * tail;
{
    register char * start;
    register int    times;
    register int    count;
    
    times = count = RepetitionCount;
    
    if (head < tail) {
	/* Must not allow input buffer to overflow on InsertChar when
	   InsertChar won't have a chance to return a full buffer to the
	   shell! */
	if ((InputTail + (tail - head)) >= (InputBuffer + InputBufferSize)) {
	    return 0;
	}
	
	RepetitionCount = 1;

	for (; times--;) {
	    for (start = head; start < tail; start++) {
		CurrentChar = *start & 0177;
		InsertChar ();
	    }
	}

	RepetitionCount = count;
    }

    return 1;
}

hidden code
DefaultBinding ()
{
    Beep();
    return DEF;
}

#define TAB_LENGTH 8

hidden code
Tab ()
{
    register int n, repetition = RepetitionCount;
    register char * newPos;
    
    n = (InputPos - InputBuffer) / TAB_LENGTH;
    newPos = InputBuffer + (n + 1) * TAB_LENGTH;
    if (repetition > 1) 
	newPos += (repetition - 1) * TAB_LENGTH;

    CurrentChar = ' ';
    RepetitionCount = newPos - InputPos;
    if (! DoInsertChar ()) Beep();
    return DEF;
}

/*---------------------------------------------------------------------------
 * Redisplay -- redraw the current command line on the user's terminal
 *---------------------------------------------------------------------------
 */
hidden code
Redisplay ()
{
    Crlf();
    WriteToScreen (InputBuffer, InputTail - InputBuffer);
    Backup (InputPos, InputTail - InputPos);
    
    return DEF;
}

hidden
Crlf ()
{
    static char * crlf = "\r\n";
    write (SHOUT, crlf, 2);
}

hidden
NewLine ()
{
    char nl = LINEFEED;
    write (SHOUT, &nl, 1);
}

hidden code
ClearScreen ()
{
    if (LengthOfClearScreenSequence) {
	write (SHOUT, ClearScreenSequence, LengthOfClearScreenSequence);
	printprompt();
	WriteToScreen (InputBuffer, InputTail - InputBuffer);
	Backup (InputPos, InputTail - InputPos);
    }
    else {
        Beep();
    }
    return DEF;
}

hidden
Beep ()
{
    if (VisibleBell) {
	write (SHOUT, VisibleBellSequence, LengthOfVisibleBellSequence);
    }
    if (AudibleBell) {
	char beep = CTRL_G;
	write (SHOUT, &beep, 1);
    }
}

/*---------------------------------------------------------------------------
 * TransposeChars -- swap the two chars preceding the cursor
 *---------------------------------------------------------------------------
 */
hidden code
TransposeChars ()
{
    char temp;
    
    if (InputPos - InputBuffer >= 2) {
	temp = *(InputPos - 2);
        *(InputPos - 2) = *(InputPos - 1);
	*(InputPos - 1) = temp;
	Backup (InputPos - 2, 2);
	WriteToScreen (InputPos - 2, 2);
    }
    else {
        Beep();
    }

    return DEF;
}

hidden boolean
IsSpaceChar (pos)
char * pos;
{
    return (SpaceChar[*pos] ? TRUE : FALSE);
}

/*---------------------------------------------------------------------------
 * NextWordPos -- returns a pointer to the end of the word following the
 * cursor.  If the cursor is in the middle of a word a pointer to its end
 * is returned.
 *---------------------------------------------------------------------------
 */
hidden char *
NextWordPos (cur)
char * cur;
{
    register char * temp = cur;

    /* skip over spaces */
    for (;temp < InputTail && IsSpaceChar(temp); temp++);
    
    /* don't go past end of last word */
    if (temp == InputTail) {
        return cur;
    }
    
    /* skip over next word */
    for (; temp < InputTail && !IsSpaceChar (temp); temp++);

    return temp;
}

/*---------------------------------------------------------------------------
 * PrevWordPos -- returns a pointer to the beginning of the previous word.
 * If the cursor is in the middle of a word a pointer to the beginning of
 * it will be returned.
 *---------------------------------------------------------------------------
 */
hidden char *
PrevWordPos (cur)
char * cur;
{
    register char * temp = cur;
    
    /* skip over preceding spaces */
    for (;temp > InputBuffer && IsSpaceChar(temp-1); temp--);

    /* don't go past beginning of first word */
    if (temp == InputBuffer) {
        return cur;
    }
    
    /* skip over preceding word */
    for (; temp > InputBuffer && !IsSpaceChar(temp-1); temp--);

    return temp;
}

/*---------------------------------------------------------------------------
 * DeleteNChars -- deletes a specified number of chars from the input
 * buffer either backwards or forwards from the current position.
 *---------------------------------------------------------------------------
 */
hidden
DeleteNChars (num, dir)
int num;
direction dir;
{
    register char * newcurrent, * temp;
    
    if (dir == FORW) {
	newcurrent = InputPos + num;

        WriteToScreen (newcurrent, InputTail - newcurrent);
	BlotOut (InputPos, num);
	Backup (InputPos, InputTail - InputPos);
	
	for (temp = InputPos; newcurrent < InputTail; temp++, newcurrent++) {
	    *temp = *newcurrent;
	}
	InputTail -= num;
    }
    else if (dir == BACK) {
        newcurrent = InputPos - num;
	
	Backup (newcurrent, num);
	WriteToScreen (InputPos, InputTail - InputPos);
	BlotOut (newcurrent, num);
	Backup (newcurrent, InputTail - newcurrent);
	
	for (temp = newcurrent; InputPos < InputTail; temp++, InputPos++) {
	    *temp = *InputPos;
	}
	InputTail -= num;
	InputPos = newcurrent;
    }
}

/*---------------------------------------------------------------------------
 * DeleteWord -- delete from the cursor to the end of the following word
 *---------------------------------------------------------------------------
 */
hidden code
DeleteWord ()
{
    register char  *temp;
    register int    num;

    temp = InputPos;

    do {
	temp = NextWordPos (temp);
    } while (--RepetitionCount > 0);

    if (num = (temp - InputPos)) {
	if (Editor == VI) {
	    CopyNChars (KillBuffer, InputPos, num);
	    KillTail = KillBuffer + num;
	}
	DeleteNChars (num, FORW);
    }
    else {
	Beep ();
    }

    return DEF;
}

/*---------------------------------------------------------------------------
 * EraseWord -- delete from the cursor to the beginning of the previous
 * word
 *---------------------------------------------------------------------------
 */
hidden code
EraseWord ()
{
    register char  *temp;
    register int    num;

    temp = InputPos;

    do {
	temp = PrevWordPos (temp);
    } while (--RepetitionCount > 0);

    if (num = (InputPos - temp)) {
	if (Editor == VI) {
	    CopyNChars (KillBuffer, temp, num);
	    KillTail = KillBuffer + num;
	}
	DeleteNChars (num, BACK);
    }
    else {
	Beep ();
    }

    return DEF;
}

/*---------------------------------------------------------------------------
 * The following two functions delete a single char either following or
 * preceding the current position.
 *---------------------------------------------------------------------------
 */
hidden code
DeleteCurrentChar ()
{
    register int num;
    
    num = min (InputTail - InputPos, RepetitionCount);
    if (num < RepetitionCount) {
        Beep();
    }
        
    if (num) {
	if (Editor == VI) {
	    CopyNChars (KillBuffer, InputPos, num);
	    KillTail = KillBuffer + num;
	}
	DeleteNChars (num, FORW);
    }
    else {
        Beep();
    }

    return DEF;
}

hidden code
DeletePreviousChar ()
{
    register int num;
    
    num = min (InputPos - InputBuffer, RepetitionCount);
    if (num < RepetitionCount) {
        Beep();
    }
    
    if (num) {
	if (Editor == VI) {
	    CopyNChars (KillBuffer, InputPos - num, num);
	    KillTail = KillBuffer + num;
	}
	DeleteNChars (num, BACK);
    }
    else {
        Beep();
    }

    return DEF;
}

hidden
CopyNChars (dest, src, num)
char * dest, * src;
register int num;
{
    register int i;
    
    for (i = 0; i < num; i++) {
        *dest++ = *src++;
    }
}

/*---------------------------------------------------------------------------
 * Kill buffer code
 *---------------------------------------------------------------------------
 */
hidden code
SetMark ()
{
    Mark = InputPos;
    return DEF;
}

hidden code
KillRegion ()
{
    register int len;
    register char *pos;
    
    if (! Mark) {
        Beep();
    }
    else if (InputPos > Mark) {
	len = InputPos - Mark;
	CopyNChars (KillBuffer, Mark, len);
        DeleteNChars (len, BACK);
	KillTail = KillBuffer + len;
    }
    else if (InputPos < Mark) {
	len = Mark - InputPos;
	CopyNChars (KillBuffer, InputPos, len);
        DeleteNChars (len, FORW);
	Mark = InputPos;
	KillTail = KillBuffer + len;
    }
    else {
        Beep();
    }
    
    return DEF;
}

hidden code
KillToEOL ()
{
    register int temp;
    
    if (temp = InputTail - InputPos) {
	CopyNChars (KillBuffer, InputPos, temp);
	DeleteNChars (temp, FORW);
    	KillTail = KillBuffer + temp;
    }

    return DEF;
}

hidden code
EraseLine ()
{
    static code BeginningOfLine();
    static code KillToEOL();

    BeginningOfLine();
    return KillToEOL();
}

hidden code
YankKillBuffer ()
{
    if (KillBuffer >= KillTail || ! InsertString( KillBuffer, KillTail)) {
        Beep();
    }
    return DEF;
}

/*---------------------------------------------------------------------------
 * The following functions just move the cursor around in the input buffer.
 *---------------------------------------------------------------------------
 */
hidden
MoveNChars (num, dir)
int num;
direction dir;
{
    if (dir == FORW) {
        WriteToScreen (InputPos, num);
	InputPos += num;	
    }
    else if (dir == BACK) {
	InputPos -= num;
        Backup (InputPos, num);
    }
}

hidden code
ForwardChar ()
{
    register int num;

    num = min (InputTail - InputPos, RepetitionCount);
        
    if (num) {
	MoveNChars (num, FORW);
    }
    else {
        Beep();
    }

    return DEF;
}

hidden code
BeginningOfLine ()
{
    register int num;
    
    if (num = (InputPos - InputBuffer)) {
	MoveNChars (num, BACK);
    }

    return DEF;
}

hidden code
EndOfLine ()
{
    register int num;
    
    if (num = (InputTail - InputPos)) {
	MoveNChars (InputTail - InputPos, FORW);
    }

    return DEF;
}

hidden code
ForwardWord ()
{
    register char  *temp, *prev;
    register int    num;

    temp = InputPos;

    do {			/* locate new cursor position */
	prev = temp;
	temp = NextWordPos (prev);
	if (temp == prev) {
	    break;
	}
    } while (--RepetitionCount > 0);

    if (num = (temp - InputPos)) {
	MoveNChars (num, FORW);
    }

    return DEF;
}

hidden code
BackwardWord ()
{
    register char  *temp, *prev;
    register int    num;

    temp = InputPos;

    do {			/* locate new cursor position */
	prev = temp;
	temp = PrevWordPos (prev);
	if (temp == prev) {
	    break;
	}
    } while (--RepetitionCount > 0);

    if (num = (InputPos - temp)) {
	MoveNChars (num, BACK);
    }

    return DEF;
}

hidden code
BackSpace ()
{
    register int num;
        
    num = min (InputPos - InputBuffer, RepetitionCount);

    if (num) {
	MoveNChars (num, BACK);
    }
    else {
        Beep();
    }

    return DEF;
}

hidden char *breakchars;

visible
tenex_set_breakchars(chars)
char *chars;
{
	breakchars = (char *)xmalloc(strlen(chars)+1);
	strcpy(breakchars, chars);
}

hidden
InitBuffers()
{
    register char *p;
    register int    i;

    for (i = 0; i < MAXSPACES; i++) {
	SpaceChar[i] = 0;
    }

    if (breakchars == 0)
    {
	SpaceChar[SPACE] = 1;
	SpaceChar['\t'] = 1;
	SpaceChar['/'] = 1;
	SpaceChar['\\'] = 1;
	SpaceChar['('] = 1;
	SpaceChar[')'] = 1;
	SpaceChar['['] = 1;
	SpaceChar[']'] = 1;
	SpaceChar['{'] = 1;
	SpaceChar['}'] = 1;
	SpaceChar['.'] = 1;
	SpaceChar[','] = 1;
	SpaceChar[';'] = 1;
	SpaceChar['>'] = 1;
	SpaceChar['<'] = 1;
	SpaceChar['!'] = 1;
	SpaceChar['^'] = 1;
	SpaceChar['&'] = 1;
	SpaceChar['|'] = 1;
    }
    else {
	for (p = breakchars; *p; p++) {
	    SpaceChar[*p] = 1;
	}
    }

    for (i = 0; i < BUFSIZ; i++) {
	SpaceBuffer[i] = SPACE;
    }
    
    if (LengthOfBackSpaceSequence == 1) {
	for (i = 0; i < BUFSIZ; i++) {
	    BackSpaceBuffer[i] = *BackSpaceSequence;
	}
    }
}

/*---------------------------------------------------------------------------
 * Code for Keyboard Macros
 *---------------------------------------------------------------------------
 */
#define MACRO_STACK_INCREMENT 10
#define MACRO_ABSURDITY 128

hidden boolean
PushMacro (mac)
String mac;
{
    register int i;
    
    if (MacroStackDepth >= MacroStackLimit) {
	MacroStackLimit += MACRO_STACK_INCREMENT;
	if (MacroStack) {
	    MacroStack = (String *) xrealloc( MacroStack,
		    MacroStackLimit * sizeof( String));
	}
	else {
	    MacroStack = (String *) xmalloc( MacroStackLimit * sizeof( String));
	}
	if (! MacroStack) {
	    printf("\nout of memory\n");
	    if (MacroStackLimit > MACRO_STACK_INCREMENT) {
	        MacroStackLimit = MACRO_STACK_INCREMENT;
		MacroStack = (String *) xmalloc( MacroStackLimit * 
			sizeof( String));
		if (! MacroStack) {
		    MacroStackLimit = 0;
		}
	    }
	    MacroStart = 0, MacroStackDepth = 0;
	    return FALSE;
	}
    }
    if (MacroStart) {
	MacroStack[MacroStackDepth].Head = MacroStart;
	MacroStack[MacroStackDepth++].Tail = MacroTail;
    }

    MacroStart = mac.Head;
    MacroTail = mac.Tail;

    /* Don't start a macro that is already on the stack. */
    for (i = 0; i < MacroStackDepth; i++) {
        if (MacroStart <= MacroStack[i].Head && 
		MacroStack[i].Head < MacroTail) {
	    MacroStart = 0, MacroStackDepth = 0;
	    MacroStackLimit = MACRO_STACK_INCREMENT;
	    free( MacroStack);
	    MacroStack = (String *) xmalloc( MacroStackLimit * sizeof( String));
	    if (! MacroStack) {
	        printf("\nout of memory\n");
		MacroStackLimit = 0;
	    }
	    return FALSE;
	}
    }

    return TRUE;
}

hidden
PopMacro ()
{
    if (--MacroStackDepth < 0) {
	MacroStart = 0, MacroStackDepth = 0;
    }
    else {
        MacroStart = MacroStack[MacroStackDepth].Head;
	MacroTail = MacroStack[MacroStackDepth].Tail;
    }
}

hidden code
ExecuteMacro ()
{
    char buf = Bindings[CurrentChar].MacroName;
    
    if (! KeyboardMacro[buf].Head || ! PushMacro( KeyboardMacro[buf])) {
	Beep();
    }

    return DEF;
}

hidden code
ExecuteNamedMacro ()
{
    char buf;
    
    GetChar (&buf);
    
    if (! KeyboardMacro[buf].Head || ! PushMacro( KeyboardMacro[buf])) {
	Beep();
    }

    return DEF;
}

hidden code
ExecuteUnNamedMacro ()
{
    if (! MacroDefined || ! PushMacro( UnNamedMacro)) {
	Beep();
    }
    
    return DEF;
}

hidden code
StartRemembering ()
{
    if (DefiningMacro) {
	MacroDefined = DefiningMacro = FALSE;
        Beep();
    }
    else {
	DefiningMacro = TRUE;
	MacroDefined = FALSE;
    }
    UnNamedMacro.Head = MacroBuffer;
    UnNamedMacro.Tail = MacroBuffer;
    return DEF;
}

hidden code
StopRemembering ()
{
    if (DefiningMacro) {
	/* remove macro closing chars from tail of macro */
	UnNamedMacro.Tail--;
	if (Bindings != RegBindings) UnNamedMacro.Tail--;

	/* ignore null macros; otherwise DefineNamedMacro will fail with
	   xmalloc(0) */
        if (UnNamedMacro.Tail - UnNamedMacro.Head > 0) {
	    MacroDefined = TRUE;
	}
	DefiningMacro = FALSE;
    }
    else {
        Beep();
    }

    return DEF;
}

hidden code
DefineNamedMacro ()
{
    register int len;
    char buf;
    
    GetChar (&buf);
    
    if (KeyboardMacro[buf].Head) {
	free(KeyboardMacro[buf].Head);
    }

    if (DefiningMacro || !MacroDefined) {
	KeyboardMacro[buf].Head = 0;
        Beep();
    }
    else {
	if (!(KeyboardMacro[buf].Head = 
	    (char *)xmalloc(len = UnNamedMacro.Tail - UnNamedMacro.Head))) {
	    Beep();
	}
	else {
	    CopyNChars (KeyboardMacro[buf].Head, UnNamedMacro.Head, len);
	    KeyboardMacro[buf].Tail = KeyboardMacro[buf].Head + len;
	}
    }
    
    return DEF;
}

hidden
WriteMacroFile( fname)
char *fname;
{
  int fd;

  fd = open( fname, (O_WRONLY|O_CREAT|O_TRUNC), 0600);
  if (fd != -1) {
    int curmacro;
    char *iter;
    int len;

    for (curmacro = 0; curmacro < 128; curmacro++) {
      if (iter = KeyboardMacro[ curmacro].Head) {
	len = (KeyboardMacro[ curmacro].Tail-iter);
        write( fd, (char *)&curmacro, 1);
	write( fd, (char *)&len,      4);
        write( fd, iter, 	      len);
	write( fd, "\n", 	      1);	 /* just for readability */
      }
    }
    close(fd);
  }
  return fd;
}

hidden
ReadMacroFile( fname)
char *fname;
{
  int fd = open( fname, 0);
  if (fd != -1) {
    unsigned int size = lseek( fd, 0, 2); /* Seek to end and get size */
    lseek( fd, 0, 0);			  /* Back to the beginning to read */

    if (size) {
      char *filbuf = (char *)xmalloc( size);
      char *end = filbuf + size;
      char *curpos = filbuf;
      char curmacro;			/* the macro we are defining */
      int *macrosize;			/* and its size */

      if (!filbuf) return -1;		/* xmalloc failure */

      read( fd, filbuf, size);		/* vread is known not to work here! */

      while (curpos < end) {
        curmacro = *curpos++;		/* extract the macro name */
	macrosize = (int *)curpos;	/* Make a pointer to the size */
	curpos += 4;			/* And skip over it */

	if ((*macrosize<= 0) || (*macrosize > 1024)) break;

#define CurMac KeyboardMacro[ curmacro]
	if (CurMac.Head) free( CurMac.Head);  /* reclaim existing macro */

	if (!(CurMac.Tail = CurMac.Head = (char *)xmalloc( *macrosize))) break;

	if (curpos + *macrosize >= end) break; /* not past eof we don't */

	while ((*macrosize)-- > 0) *(CurMac.Tail++) = *curpos++;
	/* Copy macro, this leaves Tail in the 'right' place */
#undef CurMac

	while ( (curpos < end) && (*curpos++ != '\n') );
	/* Scan for trailing newline... */
      }
      free( filbuf);
    }
    close( fd);
  }
  return fd;
}

hidden code
SaveMacroFile()
{
  struct stat statbuf;

  *InputTail = '\0';
  if (stat( InputBuffer, &statbuf) == 0) { /* Don't overwrite existing files */
    char * errmsg = " [ File already exists ]";
    InsertString( errmsg, errmsg+strlen(errmsg));
    Beep();
    return DEF;
  }
  if (WriteMacroFile( InputBuffer) == -1)
    Beep();
  else {
    BeginningOfLine();
    KillToEOL();
  }
  return DEF;
}

hidden code
LoadMacroFile()
{
  *InputTail = '\0';
  if (ReadMacroFile( InputBuffer) == -1)
    Beep();
  else {
    BeginningOfLine();
    KillToEOL();
  }
  return DEF;
}


/*---------------------------------------------------------------------------
 * Incremental Search
 *---------------------------------------------------------------------------
 */

/* string_index returns a pointer to the first instance of 'chr' in 'str',
    or 0 if none is found */
hidden char *
string_index (str, chr)
String str;
char chr;
{
    register char *p;
    
    for (p = str.Head; p < str.Tail; p++) {
        if (*p == chr) return p;
    }
    return 0;
}

/* string_rindex returns a pointer to the last instance of 'chr' in 'str',
   or 0 if none is found */
hidden char *
string_rindex (str, chr)
String str;
char chr;
{
    register char *p;
    
    for (p = str.Tail - 1; p >= str.Head; p--) {
        if (*p == chr) return p;
    }
    return 0;
}
/* returns true iff the string defined by (s1,e1) is identical to the
   string defined by (s2,e2).  The 's' pointer points to the first char of
   the string, while the 'e' pointer points to the char after the last
   char of the string. */
hidden boolean
match (s1, e1, s2, e2)
register char *s1, *s2;
register char *e1, *e2;
{
    if ((e1 - s1) != (e2 - s2)) return FALSE;

    for (; s1 < e1 && *s1 == *s2; s1++, s2++);

    if (s1 < e1) return FALSE;
    else return TRUE;    
}

/* returns a pointer to the first char of the first instance of 'search'
   in 'input' */
hidden char *
Index (input, search)
String input, search;
{
    int searchlength;
    char lastchar;
    char * lastpos, * firstpos;
    String tempstring;
    
    searchlength = search.Tail - search.Head;
    if (searchlength == 0) return input.Head;
    lastchar = search.Tail[-1];

    tempstring.Tail = input.Tail;

    loop {    
	tempstring.Head = input.Head + searchlength - 1;
	if (tempstring.Head >= tempstring.Tail) return 0;

	if ((lastpos = string_index(tempstring,lastchar)) == 0) return 0;
        
	firstpos = lastpos - searchlength + 1;
    
	if (match(firstpos,lastpos,search.Head,search.Tail-1)) 
	    return firstpos;

	input.Head = firstpos + 1;
    }
}

/* returns a pointer to the first char of the last instance 'search' in
   'input' */
hidden char *
ReverseIndex (input, search)
String input, search;
{
    int searchlength;
    char lastchar;
    char *lastpos, *firstpos;
    
    searchlength = search.Tail - search.Head;
    if (searchlength == 0) return input.Head;
    lastchar = search.Tail[-1];

    loop {
	if ((lastpos = string_rindex(input,lastchar)) == 0) return 0;
	
	firstpos = lastpos - searchlength + 1;

	if (match(firstpos,lastpos,search.Head,search.Tail-1))
	    return firstpos;
	
	input.Tail = lastpos;
    }
}

hidden code
IncrementalSearchForward ()
{
    char searchbuffer [256];
    String searchstring;
    String inputstring, oldinputstring;
    int movement;
    char *ptr;

    inputstring.Head = InputPos;
    inputstring.Tail = InputTail;
    oldinputstring = inputstring;
    searchstring.Head = searchstring.Tail = searchbuffer;

    GetChar (searchstring.Tail);
    
    while (*(searchstring.Tail) != ESCAPE) {
	if (*(searchstring.Tail) != CTRL_N) {
	    searchstring.Tail++;
	}
	else {
	    oldinputstring = inputstring;
	    inputstring.Head = (char *)
		min (InputPos - (searchstring.Tail - searchstring.Head) + 1,
		     InputTail);
	}
	if (ptr = Index (inputstring, searchstring)) {
	    inputstring.Head = ptr;
	    movement = (ptr + (searchstring.Tail - searchstring.Head)) 
			- InputPos;
	    MoveNChars (movement, FORW);
	}
	else {
	    inputstring = oldinputstring;
	    searchstring.Tail--;
	    Beep();
	}

	if (searchstring.Tail > searchbuffer + 255) {
	    Beep();
	    return DEF;
	}
	
        GetChar (searchstring.Tail);
    }
    
    return DEF;
}

hidden int
max(a,b)
int a,b;
{
    if (a > b)
	return(a);
    return(b);
}

hidden int
min(a,b)
int a,b;
{
    if (a < b)
	return(a);
    return(b);
}

hidden code
IncrementalSearchReverse ()
{
    char searchbuffer [256];
    String searchstring;
    String inputstring, oldinputstring;
    int movement;
    char *ptr;

    inputstring.Head = InputBuffer;
    inputstring.Tail = InputPos;
    oldinputstring = inputstring;
    searchstring.Head = searchstring.Tail = searchbuffer;

    GetChar (searchstring.Tail);
    
    while (*(searchstring.Tail) != ESCAPE) {
	if (*(searchstring.Tail) != CTRL_N) {
	    searchstring.Tail++;
	}
	else {
	    oldinputstring = inputstring;
	    inputstring.Tail = (char *)
		max (InputPos + (searchstring.Tail - searchstring.Head) - 1,
		     InputBuffer);
	}
	if (ptr = ReverseIndex (inputstring, searchstring)) {
	    movement = InputPos - ptr;
	    MoveNChars (movement, BACK);
	}
	else {
	    inputstring = oldinputstring;
	    searchstring.Tail--;
	    Beep();
	}

	if (searchstring.Tail > searchbuffer + 255) {
	    Beep();
	    return DEF;
	}
	
        GetChar (searchstring.Tail);
    }
    
    return DEF;
}

hidden code
SearchForward()
{
    char lookfor, * found;
    
    *InputTail = '\0';
    GetChar( &lookfor);
    
    if (found = (char *)index( InputPos, lookfor)) {
        MoveNChars( found - InputPos + 1, FORW);
    }
    else {
        Beep();
    }
    return DEF;
}

hidden code
SearchReverse()
{
    extern char * rindex();
    char lookfor, save, * found;
    
    GetChar( &lookfor);

    save = *InputPos;
    *InputPos = '\0';
    
    if (found = rindex( InputBuffer, lookfor)) {
	*InputPos = save;
        MoveNChars( InputPos - found, BACK);
    }
    else {
        *InputPos = save;
	Beep();
    }
    return DEF;
}

hidden code
IncrementalSearchHistoryForward ()
{
    char searchbuffer [256];
    String searchstring;
    String inputstring, oldinputstring;
    int movement;
    char *ptr;
    int savecurhist;
    int nextoccur;

    inputstring.Head = InputPos;
    inputstring.Tail = InputTail;
    oldinputstring = inputstring;
    savecurhist = curhist;
    searchstring.Head = searchstring.Tail = searchbuffer;

    GetChar (searchstring.Tail);
    
    while (*(searchstring.Tail) != ESCAPE) {
	if (*(searchstring.Tail) != CTRL_N) {
	    searchstring.Tail++;
	    nextoccur = 0;
	}
	else {
	    nextoccur = 1;
	    oldinputstring = inputstring;
	    savecurhist = curhist;
	    inputstring.Head = (char *)
		min (InputPos - (searchstring.Tail - searchstring.Head) + 1,
		     InputTail);
	}
forward_hist:
	if (ptr = Index (inputstring, searchstring)) {
	    inputstring.Head = ptr;
	    movement = (ptr + (searchstring.Tail - searchstring.Head)) 
			- InputPos;
	    MoveNChars (movement, FORW);
	}
	else {
	    /* 
	     * Current string search failed.
	     * - Start at the begining of the next string in the history
	     *   list.
	     */
	    if (Next_Hist() < 0) {
failed_next:
		inputstring = oldinputstring;
		if (!nextoccur)
			searchstring.Tail--;
		curhist = savecurhist;
		InsertCurrentHistEntry();
	        BeginningOfLine();
		movement = oldinputstring.Head - oldinputstring.Tail;
		MoveNChars(movement, FORW);
    		inputstring.Head = InputPos = InputBuffer;
	        inputstring.Tail = InputTail;
		Beep();
	    } else {
		if (!InsertCurrentHistEntry())
		    goto failed_next;
	        BeginningOfLine();
    		inputstring.Head = InputPos = InputBuffer;
	        inputstring.Tail = InputTail;
		goto forward_hist;
	    }
	}

	if (searchstring.Tail > searchbuffer + 255) {
	    Beep();
	    return DEF;
	}
	
        GetChar (searchstring.Tail);
    }
    
    return DEF;
}

hidden code
BeginningOfHistory()
{
	while(Prev_Hist() >= 0) ;
	InsertCurrentHistEntry();
	BeginningOfLine();
}

hidden code
EndOfHistory()
{
	while(Next_Hist() >= 0) ;
	InsertCurrentHistEntry();
}

hidden code
IncrementalSearchHistoryReverse ()
{
    char searchbuffer [256];
    String searchstring;
    String inputstring, oldinputstring;
    int movement;
    char *ptr;
    int savecurhist;

    inputstring.Head = InputBuffer;
    inputstring.Tail = InputPos;
    oldinputstring = inputstring;
    savecurhist = curhist;
    searchstring.Head = searchstring.Tail = searchbuffer;

    GetChar (searchstring.Tail);
    
    while (*(searchstring.Tail) != ESCAPE) {
	if (*(searchstring.Tail) != CTRL_N) {
	    searchstring.Tail++;
	}
	else {
	    oldinputstring = inputstring;
	    savecurhist = curhist;
	    inputstring.Tail = (char *)
		max (InputPos + (searchstring.Tail - searchstring.Head) - 1,
		     InputBuffer);
	}
reverse_hist:
	if (ptr = ReverseIndex (inputstring, searchstring)) {
	    movement = InputPos - ptr;
	    MoveNChars (movement, BACK);
	}
	else {
	    /* 
	     * Current string search failed.
	     * - Start at the begining of the next string in the history
	     *   list.
	     */
	    if (Prev_Hist() < 0) {
failed_prev:
		inputstring = oldinputstring;
		searchstring.Tail--;
		curhist = savecurhist;
		InsertCurrentHistEntry();
		Beep();
	    } else {
		if (!InsertCurrentHistEntry())
		    goto failed_prev;
    		inputstring.Head = InputBuffer;
	        inputstring.Tail = InputPos = InputTail;
		goto reverse_hist;
	    }
	}

	if (searchstring.Tail > searchbuffer + 255) {
	    Beep();
	    return DEF;
	}
	
        GetChar (searchstring.Tail);
    }
    
    return DEF;
}

hidden
Enter_Hist(s, len)
char *s;
{
	if (*s == 0 || *s == '\n')
		return;
	if (histlist == 0) {
		histlist = (char **)xmalloc(sizeof(char *)*maxhist);
		bzero (histlist, sizeof(char *) * maxhist);
	}
	curhist = 0;
	if (histlist[histenter])
		free(histlist[histenter]);
	histlist[histenter] = (char *)xmalloc(len+1);
	strcpy(histlist[histenter], s);
	histlist[histenter][len] = 0;
	histenter++;
	if (histenter >= maxhist)
		histenter = 0;
	if (histenter == lasthist) {
		lasthist++;
		if (lasthist >= maxhist)
			wrapped = 1;
	}
	curhist = 0;
}

hidden char *
Cur_Hist()
{
	int indx = histenter+curhist;

	if (histlist == 0)
		return 0;
	if (indx < 0)
		indx += maxhist;
	if (indx >= maxhist)
		indx -= maxhist;
	return(histlist[indx]);
}

hidden int
Prev_Hist()
{
	if (curhist == -maxhist)
		return -1;
	curhist--;
	if (Cur_Hist())
		return 0;
	curhist++;
	return -1;
}

hidden int
Next_Hist()
{
	if (curhist == 0)
		return-1;
	curhist++;
	return 0;
}

hidden int
InsertCurrentHistEntry()
{
  register char *hist = Cur_Hist();
  int oldrep = RepetitionCount;
  if (hist == 0)
	return 0;
  RepetitionCount = 1;
  MoveNChars( InputPos-InputBuffer, BACK);
  DeleteNChars( InputTail-InputBuffer, FORW);
  if (! InsertString(hist, hist+strlen(hist))) return 0;
  RepetitionCount = oldrep;
  return 1;
}

hidden code
PreviousHistEntry()
{
  register i, r = RepetitionCount;

  for (i=0;i<r;i++)
    if (Prev_Hist() < 0) {
      Beep();
      break;
    }
  if (!InsertCurrentHistEntry())
    Beep();
  return DEF;
}

hidden code
NextHistEntry()
{
  register i;

  for (i=0;i<RepetitionCount;i++)
    if (Next_Hist() < 0) {
      Beep();
      break;
    }
  if (!InsertCurrentHistEntry())
    Beep();
  return DEF;
}

/*---------------------------------------------------------------------------
 * Special routines for VI mode
 *---------------------------------------------------------------------------
 */
hidden code
ChangeFollowingObject ()
{
    char buf;
    
    GetChar (&buf);
    
    switch (buf) {
	case SPACE:
	    DeleteCurrentChar();
	    break;
	case 'b':
	    EraseWord();
	    break;
	case 'd':
	    BeginningOfLine();
	    KillToEOL();
	    break;
	case 'w':
	    DeleteWord();
	    break;
	default:
	    Beep();
	    return DEF;
	    break;
    }
    
    return EnterViInsert();
}

hidden code
DeleteFollowingObject ()
{
    char buf;
    code result;
    
    GetChar (&buf);
    
    switch (buf) {
	case SPACE:
	    result = DeleteCurrentChar();
	    break;
	case 'b':
	    result = EraseWord();
	    break;
	case 'd':
	    BeginningOfLine();
	    result = KillToEOL();
	    break;
	case 'w':
	    result = DeleteWord();
	    break;
	default:
	    Beep();
	    result = DEF;
	    break;
    }
    
    return result;
}

hidden code
AppendToEOL ()
{
    EndOfLine ();
    return EnterViInsert();
}

hidden code
InsertAtBOL ()
{
    BeginningOfLine ();
    return EnterViInsert();
}

hidden code
ChangeChar ()
{
    DeleteCurrentChar();
    return EnterViInsert();
}

hidden code
ChangeToEOL ()
{
    KillToEOL ();
    return EnterViInsert();
}

hidden code
ChangeWholeLine ()
{
    BeginningOfLine();
    KillToEOL();
    return EnterViInsert();
}

hidden code
Suspend ()
{
    tenex_suspend();
    return(DEF);
}

hidden code
ReplaceChar ()
{
    DeleteCurrentChar();
    GetChar (&CurrentChar);
    if (! DoInsertChar()) Beep();
    return DEF;
}

hidden code
ViYankKillBuffer ()
{
    if (InputPos < InputTail) ForwardChar ();
    YankKillBuffer ();
    return DEF;
}

#define NMACROFILES 32

static char *macrofiles[NMACROFILES];
static int nmacrofiles;

visible
tenex_set_macrofiles(files)
char *files;
{
	register char *p;
	int lastfile = 0;

	/* multiple file names separated by either spaces or colons */
	while(*files && !lastfile) {
		/* Find termination of string */
		if (nmacrofiles >= NMACROFILES) {
			printf("too many macro files: limit=%d\n", NMACROFILES);
			return;
		}
		for(p=files;;p++) {
			if (*p == ':' || *p == ' ' || *p == '\t') {
				*p = 0;
				break;
			}
			if (*p == 0) {
				lastfile = 1;
				break;
			}
		}
		macrofiles[nmacrofiles] = (char *)xmalloc(strlen(files)+1);
		strcpy(macrofiles[nmacrofiles], files);
		nmacrofiles++;
		if (lastfile)
			return;
		/* skip leading/trailing shile space */
		do {
			p++;
		} while(*p && (*p == ' ' || *p == '\t' || *p == ':'));
		files = p;
	}
}

/*---------------------------------------------------------------------------
 * InitMacros -- read macro files during initialization
 *---------------------------------------------------------------------------
 */
hidden
InitMacros ()
{
	register i;

	if (!nmacrofiles)
		return;
	for(i=0;i<nmacrofiles;i++)
		ReadMacroFile(macrofiles[i]);
}

hidden char *savemacros;

visible
tenex_set_savemacros(file)
char *file;
{
	savemacros = (char *)xmalloc(strlen(file)+1);
	strcpy(savemacros, file);
}

visible	   /* so it can be called from goodbye() */
SaveMacros ()
{
    if (!savemacros) {
        return;
    }
    WriteMacroFile(savemacros);
}

/*---------------------------------------------------------------------------
 * Sets the bindings for keymaps.
 *---------------------------------------------------------------------------
 */

/* dobindings is called from doset (sh.set.c) and during tenex
   initialization */
hidden int
dobindings (val)
char * val;
{
    char *s;

    if (! Initialized) return;	  /* only works when tenex is running */
    
    InitBindings( val);
    InitMacros();
    Init_Bindings();
}

hidden
InitBindings (ed)
char * ed;
{

    if (ed == 0 || TermKnown == FALSE) {
        Editor = DUMB;
    }
    else {
        if (EQ(ed,"vi") || EQ(ed,"ex")) {
	    Editor = VI;
	}
	else if (EQ(ed,"emacs")) {
	    Editor = EMACS;
	}
	else
	    Editor = DUMB;
    }

    InitDefaultBindings();

    switch (Editor) {
	case EMACS:
	    InitEmacsBindings();
	    break;
	case VI:
	    InitViBindings();
	    break;
	default:
	    InitDumbBindings();
	    break;
    }
}    

hidden
InitEmacsBindings()
{
    CNXBIND (ExecuteUnNamedMacro, 'e');
    CNXBIND (SaveMacroFile, CTRL_S);
    CNXBIND (StartRemembering, '(');
    CNXBIND (StopRemembering, ')');
    CNXBIND (LoadMacroFile, CTRL_R);

    ESCBIND (BackwardWord, 'b');
    ESCBIND (DefineNamedMacro, 'n');
    ESCBIND (DeleteWord, 'd');
    ESCBIND (EraseWord, 'h');
    ESCBIND (ForwardWord, 'f');
    ESCBIND (ExecuteUnNamedMacro, 'e');
    ESCBIND (ExecuteNamedMacro, 'x');
    ESCBIND (IncrementalSearchForward, '/');
    ESCBIND (IncrementalSearchReverse, '?');
    ESCBIND (IncrementalSearchHistoryForward, 's');
    ESCBIND (IncrementalSearchHistoryReverse, 'r');
    ESCBIND (BeginningOfHistory, '<');
    ESCBIND (EndOfHistory, '>');

    REGBIND (BackSpace, CTRL_B);
    REGBIND (BeginningOfLine, CTRL_A);
    REGBIND (ClearScreen, CTRL_L);
    REGBIND (DeleteCurrentChar, CTRL_D);
    REGBIND (DeletePreviousChar, CTRL_H);
    REGBIND (DeletePreviousChar, RUBOUT);
    REGBIND (EndOfLine, CTRL_E);
    REGBIND (ForwardChar, CTRL_F);
    REGBIND (InsertLiteralChar, CTRL_Q);
    REGBIND (KillToEOL, CTRL_K);
    REGBIND (KillRegion, CTRL_W);
    REGBIND (NextHistEntry, CTRL_N);
    REGBIND (PreviousHistEntry, CTRL_P);
    REGBIND (Redisplay, CTRL_R);
    REGBIND (Repetition, CTRL_U);
    REGBIND (Return, LINEFEED);
    REGBIND (Return, RETURN);
    REGBIND (SetCtrlX, CTRL_X);
    REGBIND (SetEscape, ESCAPE);
    REGBIND (SetMark, CTRL_AT);
    REGBIND (Tab, TAB);
    REGBIND (TransposeChars, CTRL_T);
    REGBIND (YankKillBuffer, CTRL_Y);
    REGBIND (Suspend, CTRL_Z);
}

hidden
InitViBindings()
{
    CNXBIND (AppendToEOL, 'A');
    CNXBIND (BackSpace, CTRL_H);
    CNXBIND (BackSpace, 'h');
    CNXBIND (BackwardWord, 'B');
    CNXBIND (BackwardWord, 'b');
    CNXBIND (BeginningOfLine, '0');
    CNXBIND (BeginningOfLine, '^');
    CNXBIND (ChangeChar, 's');
    CNXBIND (ChangeFollowingObject, 'c');
    CNXBIND (ChangeToEOL, 'C');
    CNXBIND (ChangeWholeLine, 'S');
    CNXBIND (DeleteCurrentChar, 'x');
    CNXBIND (DeleteFollowingObject, 'd');
    CNXBIND (DeletePreviousChar, 'X');
    CNXBIND (EndOfLine, '$');
    CNXBIND (ForwardChar, 'l');
    CNXBIND (ForwardChar, SPACE);
    CNXBIND (ForwardWord, 'w');
    CNXBIND (ForwardWord, 'W');
    CNXBIND (ForwardWord, 'e');
    CNXBIND (InsertAtBOL, 'I');
    CNXBIND (KillToEOL, 'D');
    CNXBIND (ExecuteNamedMacro, '@');
    CNXBIND (NextHistEntry, '+');
    CNXBIND (NextHistEntry, 'j');
    CNXBIND (NextHistEntry, CTRL_N);
    CNXBIND (PreviousHistEntry, '-');
    CNXBIND (PreviousHistEntry, 'k');
    CNXBIND (PreviousHistEntry, CTRL_P);
    CNXBIND (Redisplay, CTRL_L);
    CNXBIND (Redisplay, CTRL_R);
    CNXBIND (Redisplay, 'z');
    CNXBIND (Repetition, '1');
    CNXBIND (Repetition, '2');
    CNXBIND (Repetition, '3');
    CNXBIND (Repetition, '4');
    CNXBIND (Repetition, '5');
    CNXBIND (Repetition, '6');
    CNXBIND (Repetition, '7');
    CNXBIND (Repetition, '8');
    CNXBIND (Repetition, '9');
    CNXBIND (ReplaceChar, 'r');
    CNXBIND (Return, LINEFEED);
    CNXBIND (Return, RETURN);
    CNXBIND (IncrementalSearchForward, '/');
    CNXBIND (IncrementalSearchReverse, '?');
    CNXBIND (IncrementalSearchHistoryForward, 's');
    CNXBIND (IncrementalSearchHistoryReverse, 'r');
    CNXBIND (SearchForward, 'f');
    CNXBIND (SearchReverse, 'F');
    CNXBIND (SetMark, 'm');
    CNXBIND (EnterViAppend, 'a');
    CNXBIND (EnterViInsert, 'i');
    CNXBIND (ViYankKillBuffer, 'p');
    CNXBIND (ViYankKillBuffer, 'P');
    
    REGBIND (DeletePreviousChar, CTRL_H);
    REGBIND (DeletePreviousChar, EraseChar);
    REGBIND (EraseWord, CTRL_W);
    REGBIND (ExitViInsert, ESCAPE);
    REGBIND (InsertLiteralChar, CTRL_Q);
    REGBIND (InsertLiteralChar, CTRL_V);
    REGBIND (KillRegion, CTRL_U);
    REGBIND (NextHistEntry, CTRL_N);
    REGBIND (PreviousHistEntry, CTRL_P);
    REGBIND (Redisplay, CTRL_L);
    REGBIND (Redisplay, CTRL_R);
    REGBIND (Suspend, CTRL_Z);
    REGBIND (Return, LINEFEED);
    REGBIND (Return, RETURN);
    REGBIND (Tab, TAB);
}    

hidden
InitDumbBindings()
{
    IgnoreEOF = (/*adrof("ignoreeof") ? TRUE : */FALSE);
    
    REGBIND (ClearScreen, CTRL_L);
    REGBIND (DeletePreviousChar, EraseChar);
    REGBIND (Redisplay, ReprintChar);
    REGBIND (KillRegion, KillChar);
    REGBIND (EraseWord, WordErase);
    REGBIND (InsertLiteralChar, LiteralNext);
    REGBIND (Return, LINEFEED);
    REGBIND (Return, RETURN);
}

hidden
InitDefaultBindings()
{
    register int i;
    
    for (i = 0; i < BINDINGS; i++) {
	REGBIND (DefaultBinding, i);
	ESCBIND (DefaultBinding, i);
	CNXBIND (DefaultBinding, i);
    }

    for (i = SPACE; i < RUBOUT; i++) {
	REGBIND (InsertChar, i);
    }
}

hidden char *
parsearg(vp)
char **vp;
{
	register char *endp, *p, *val;
	register len;

	p = *vp;
	/* Find the begining */
	loop {
		if (*p == 0)
			return(0);
		if (*p != ' ' && *p != '\t')
			break;
		p++;
	}
	/* we will return p, now modify *vp to point to the end of the string */
	endp = p;
	loop {
		if (*endp == ' ' || *endp == '\t' || *endp == 0)
			break;
		endp++;
	}
	*vp = endp;
	len = endp - p + 1;
	val = (char *)xmalloc(len);
	bcopy(p, val, len-1);
	val[len-1] = 0;
	return(val);
}

hidden int
keytran (keyname, keymap, key)
char * keyname;
KeyBinding ** keymap;
char * key;
{
    if (keyname[0] != '\\') {
        *keymap = RegBindings;
    }
    else if (keyname[1] == '\\') {
        *keymap = RegBindings;
	keyname += 1;
    }
    else if (keyname[1] == 'e') {
        *keymap = EscBindings;
	keyname += 2;
    }
    else if (keyname[1] == '^') {
        if (keyname[2] == 'X' || keyname[2] == 'x') {
	    *keymap = CnxBindings;
	    keyname += 3;
	}
	else {
	    *keymap = RegBindings;
	}
    }
    else {
        return 0;
    }
    
    if (keyname[0] != '\\')         *key = keyname[0];	/* regular char */
    else if (keyname[1] == '\\')    *key = '\\';	/* literal \ */
    else if (keyname[1] == 'e')	    *key = 033;	        /* escape */
    else if (keyname[1] == 'n')	    *key = '\n';	/* newline */
    else if (keyname[1] == 'r')	    *key = '\r';	/* return */
    else if (keyname[1] == 't')	    *key = '\t';	/* tab */
    else if (keyname[1] == '^') {
	if (keyname[2] == '?')	    *key = 0177;	/* rubout */
	else if (keyname[2] == '[') *key = 033;		/* escape */
	else {
	    if (keyname[2] > 0140 && keyname[2] < 0173) /* fold up */
		keyname[2] -= 040;
	    if (keyname[2] < 077 || keyname[2] > 0137) return 0;
	    *key = (keyname[2] + 0100) & 0177;
	}
    }
    else {
        return 0;
    }
    
    return 1;
}
    
hidden char * FunctionNames [] = {
    "appendtoeol",
    "backspace",
    "backwardword",
    "beginningofhistory",
    "beginningofline",
    "changechar",
    "changefollowingobject",
    "changetoeol",
    "changewholeline",
    "clearscreen",
    "defaultbinding",
    "definenamedmacro",
    "deletecurrentchar",
    "deletefollowingobject",
    "deletepreviouschar",
    "deleteword",
    "endoffile",
    "endofhistory",
    "endofline",
    "eraseline",
    "eraseword",
    "executemacro",
    "executenamedmacro",
    "executeunnamedmacro",
    "exitviinsert",
    "forwardchar",
    "forwardword",
    "incrementalsearchforward",
    "incrementalsearchhistoryforward",
    "incrementalsearchhistoryreverse",
    "incrementalsearchreverse",
    "insertatbol",
    "insertchar",
    "insertliteralchar",
    "killregion",
    "killtoeol",
    "loadmacrofile",
    "nexthistentry",
    "previoushistentry",
    "redisplay",
    "repetition",
    "replacechar",
    "return",
    "savemacrofile",
    "searchforward",
    "searchreverse",
    "setmark",
    "startremembering",
    "stopremembering",
    "suspend",
    "tab",
    "transposechars",
    "viyankkillbuffer",
    "yankkillbuffer",
    0    
};

hidden CodeFunction FunctionPointers [] = {
    AppendToEOL,
    BackSpace,
    BackwardWord,
    BeginningOfHistory,
    BeginningOfLine,
    ChangeChar,
    ChangeFollowingObject,
    ChangeToEOL,
    ChangeWholeLine,
    ClearScreen,
    DefaultBinding,
    DefineNamedMacro,
    DeleteCurrentChar,
    DeleteFollowingObject,
    DeletePreviousChar,
    DeleteWord,
    EndOfFile,
    EndOfHistory,
    EndOfLine,
    EraseLine,
    EraseWord,
    ExecuteMacro,
    ExecuteNamedMacro,
    ExecuteUnNamedMacro,
    ExitViInsert,
    ForwardChar,
    ForwardWord,
    IncrementalSearchForward,
    IncrementalSearchHistoryForward,
    IncrementalSearchHistoryReverse,
    IncrementalSearchReverse,
    InsertAtBOL,
    InsertChar,
    InsertLiteralChar,
    KillRegion,
    KillToEOL,
    LoadMacroFile,
    NextHistEntry,
    PreviousHistEntry,
    Redisplay,
    Repetition,
    ReplaceChar,
    Return,
    SaveMacroFile,
    SearchForward,
    SearchReverse,
    SetMark,
    StartRemembering,
    StopRemembering,
    Suspend,
    Tab,
    TransposeChars,
    ViYankKillBuffer,
    YankKillBuffer
};

/*---------------------------------------------------------------------------
 * dobind -- a new cshell command to allow the user to interactively bind
 * internal functions and macros to keys.  Its arguments should be a list
 * of function name and key name pairs.
 *---------------------------------------------------------------------------
 */
visible int
tenex_dobind (arglist)
char *arglist;
{
    register char * proc, * keyname;
    register int index, keylen, i;
    KeyBinding * keymap;
    char key;

    if (arglist == 0)
        return;
    
    loop {
	if ( ((proc = parsearg(&arglist)) == 0) ||
	     ((keyname = parsearg(&arglist)) == 0))
		return;
	/* Strip surrounding quotes if they exist */
	keylen = strlen( keyname);
	if (keyname[0] == '"' && keyname[keylen-1] == '"') {
		bcopy(keyname+1,keyname,keylen-2);
		keylen -= 2;
	}
	for (i=0; i < keylen; i++) keyname[i] &= 0177;
	
	if (proc[1] == 0 && KeyboardMacro[ proc[0] ].Head) {
	    index = stablk( "executemacro", FunctionNames, 1);
	}
	else {
	    folddown (proc, proc);
	    if ((index = stablk (proc, FunctionNames, 1)) < 0 ) {
	      /* dont print error message for the command we deleted */
		if( strcmp(proc, "filenameexpansion") &&
			strcmp(proc, "filenamelist"))
		    printf("bind: unknown function '%s'\n", proc);
		free(keyname); free(proc);
		return;
	    }
	}
	if (keytran (keyname, &keymap, &key) == 0) {
	    printf("bind: illegal key binding for %s\n", proc);
	    free(keyname); free(proc);
	    return;
	}
	keymap[key].Routine = FunctionPointers [index];
	keymap[key].MacroName = proc[0];
	free(keyname); free(proc);
    }
}

visible
tenex_editmode(newmode)
char *newmode;
{
	int len = strlen(newmode);

	if (newmode[len] == '\n')
		newmode[len] = '\0';
	editmode = (char *)xmalloc(len);
	tenex_mode = 1;
	strcpy(editmode, newmode);
	printf("Setting editmode to %s\n", newmode);
	return;
}

visible
tenex_history()
{
	register i, h;

	if (!histlist || !maxhist)
		return;
	for(i=0,h=histenter;i<maxhist;i++,h++) {
		if (h >= maxhist)
			h = 0;
		if (histlist[h])
			printf("%s\n", histlist[h]);
	}
}

#if 0
visible
tenex_set_history(newmaxhist)
char *newmaxhist;
{
	char **nhistlist;
	int nhist;

	if (! *newmaxhist) {
		printf("Current history=%d\n", maxhist);
		return;
	}
#ifdef notdef
	nhist = parse_and_eval_address(newmaxhist);
	nhistlist = (char **)xmalloc(sizeof(char *) * nhist);
#endif
	printf("not yet.\n");
	return;
}
#endif

visible 
tenex_suspend()
{
	save_tty_state();
	kill(getpid(), SIGTSTP);
	restore_tty_state();
}

/*  stablk  --  string table lookup
 *
 *  Usage:  i = stablk (arg,table,quiet);
 *
 *	int i;
 *	char *arg,**table;
 *	int quiet;
 *
 *  Stablk looks for a string in "table" which matches
 *  "arg".  Table is declared like this:
 *    char *table[] = {"string1","string2",...,0};
 *  Each string in the table is checked via stablk() to determine
 *  if its initial characters match arg.  If exactly one such
 *  string matches arg, then the index of that string is returned.
 *  If none match arg, or if several match, then -1 (respectively -2)
 *  is returned.  Also, for either of these errors, if quiet is
 *  FALSE, the user will be asked if he wants a list of the possible
 *  strings.  In the case of multiple matches, the matching strings
 *  will be marked specially.
 *
 *  HISTORY
 * 28-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Modified for 4.2 BSD.  Now puts output on std. error using fprintf and
 *	fprstab.
 *
 * 08-Sep-81  Steven Shafer (sas) at Carnegie-Mellon University
 *	Now handles case of multiple exact matches just like case of
 *	multiple initial-substring matches:  returns -2 if "quiet", else
 *	asks user which one (as if it matters).
 *
 * 19-May-80  Steven Shafer (sas) at Carnegie-Mellon University
 *	Added exactmatch and code to recognize exact match in case of
 *	ambiguity from initial prefix matching.
 *
 * 16-Apr-80  Steven Shafer (sas) at Carnegie-Mellon University
 *	Changed listing code to use prstab() instead of just printing
 *	table -- this uses multiple columns when appropriate.  To do this,
 *	it was necessary to add the "matches" array.  Too bad!
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Rewritten for VAX from Ken Greer's routine.  The error messages are
 *	different now.
 *
 *  Originally from klg (Ken Greer) on IUS/SUS UNIX.
 */


#define NOTFOUND -1
#define AMBIGUOUS -2
#define MAXSTRINGS 500

int stlmatch();

#include <stdio.h>

hidden
printprompt()
{
  extern char *prompt;

  printf("%s", prompt);
  fflush(stdout);
}


hidden
int stablk (arg,table,quiet)
char *arg, **table;
int quiet;
{
	register int i,ix,count;
	int wantlist;
	char *matches[MAXSTRINGS];
	int exactmatch;

	count = 0;
	exactmatch = 0;
	for (i=0; table[i] != 0 && exactmatch == 0; i++) {
		if (stlmatch (table[i],arg)) {
			ix = i;		/* index of last match */
			matches[count++] = table[i];
			if (strcmp(table[i],arg) == 0)  exactmatch = 1;
		}
	}
	matches[count] = 0;

	if (exactmatch) {	/* i-th entry is exact match */
		--i;		/* (actually, i-1th entry) */
		matches[0] = table[i];
		count = 1;
		for (i=i+1; table[i] != 0; i++) {
			if (strcmp(table[i],arg) == 0)  {
				matches[count++] = table[i];
				ix = i;
			}
		}
		matches[count] = 0;
	}

	if (count == 1)  return (ix);

	if (!quiet) {
		if (strcmp(arg,"?") == 0) {
			wantlist = TRUE;
		}
		else {
			fprintf (stderr,"%s is %s.  ",arg,(count ? "ambiguous" : "unknown"));
			wantlist = getbool ("Do you want a list?",TRUE);
		}
		if (wantlist) {
			fprintf (stderr,"Must match one of these:\n");
			if (count)  fprstab (stderr,matches);
			else	    fprstab (stderr,table);
		}
	}
	return (count ? AMBIGUOUS : NOTFOUND);
}

hidden
int stlmatch (big,small)
char *small, *big;
{
	register char *s, *b;
	s = small;
	b = big;
	do {
		if (*s == '\0')  return (TRUE);
	} 
	while (*s++ == *b++);
	return (FALSE);
}

#define MINCOLSPACE 5		/* min. space between columns */
#define MAXCOLS 71		/* max. cols on line */
#define MINROWS 8		/* min. rows to be printed */

hidden
prstab (list)
char **list;
{
	fprstab (stdout,list);
}

hidden
fprstab (file,list)
FILE *file;
char **list;
{
	register int nelem;	/* # elements in list */
	register int maxwidth;	/* widest element */
	register int i,l;	/* temps */
	register int row,col;	/* current position */
	register int nrow,ncol;	/* desired format */
	char format[20];	/* format for printing strings */

	maxwidth = 0;
	for (i=0; list[i]; i++) {
		l = strlen (list[i]);
		if (l > maxwidth)  maxwidth = l;
	}

	nelem = i;
	if (nelem <= 0)  return;

	ncol = MAXCOLS / (maxwidth + MINCOLSPACE);
	if (ncol < 1)  ncol = 1;	/* for very long strings */
	if (ncol > (nelem + MINROWS - 1) / MINROWS)
		ncol = (nelem + MINROWS - 1) / MINROWS;
	nrow = (nelem + ncol - 1) / ncol;

	sprintf (format,"%%-%ds",maxwidth+MINCOLSPACE);

	for (row=0; row<nrow; row++) {
		fprintf (file,"\t");
		for (col=0; col<ncol; col++) {
			i = row + (col * nrow);
			if (i < nelem) {
				if (col < ncol - 1) {
					fprintf (file,format,list[i]);
				}
				else {
					fprintf (file,"%s",list[i]);
				}
			}
		}
		fprintf (file,"\n");
	}
}

hidden
int getbool (prompt, defalt)
char *prompt;
int defalt;
{
	register int valu;
	register char ch;
	char input [100];

	fflush (stdout);
	if (defalt != TRUE && defalt != FALSE)  defalt = TRUE;
	valu = 2;				/* meaningless value */
	do {
		fprintf (stderr,"%s  [%s]  ",prompt,(defalt ? "yes" : "no"));
		fflush (stderr);			/* in case it's buffered */
		if (gets (input) == NULL) {
			valu = defalt;
		}
		else {
			ch = *input;			/* first char */
			if (ch == 'y' || ch == 'Y')		valu = TRUE;
			else if (ch == 'n' || ch == 'N')	valu = FALSE;
			else if (ch == '\0')		valu = defalt;
			else fprintf (stderr,"Must begin with 'y' (yes) or 'n' (no).\n");
		}
	} 
	while (valu == 2);			/* until correct response */
	return (valu);
}

typedef enum {FOLDUP, FOLDDOWN} FOLDMODE;

hidden
char *fold (out,in,whichway)
char *in,*out;
FOLDMODE whichway;
{
	register char *i,*o;
	register char lower;
	char upper;
	int delta;

	switch (whichway)
	{
	case FOLDUP:
		lower = 'a';		/* lower bound of range to change */
		upper = 'z';		/* upper bound of range */
		delta = 'A' - 'a';	/* amount of change */
		break;
	case FOLDDOWN:
		lower = 'A';
		upper = 'Z';
		delta = 'a' - 'A';
	}

	i = in;
	o = out;
	do {
		if (*i >= lower && *i <= upper)		*o++ = *i++ + delta;
		else					*o++ = *i++;
	} 
	while (*i);
	*o = '\0';
	return (out);
}

hidden
char *foldup (out,in)
char *in,*out;
{
	return (fold(out,in,FOLDUP));
}

hidden
char *folddown (out,in)
char *in,*out;
{
	return (fold(out,in,FOLDDOWN));
}

/* The following are called from main.c when we are attempting to stop */
hidden
save_tty_state()
{
	if (in_cbreak_mode) {
		ExitCbreakMode();
		was_in_cbreak_mode = 1;
	} else
		was_in_cbreak_mode = 0;
}

hidden
restore_tty_state()
{
	if (was_in_cbreak_mode)
		EnterCbreakMode();
}
