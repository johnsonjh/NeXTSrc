/* This is a command line editor for csh.
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
#include "sh.h"
#undef TRUE
#undef FALSE
#include <sgtty.h>
#include <sys/file.h>

#define	Crlf	NewLine

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

typedef enum { FALSE, TRUE } boolean;
typedef enum {EMACS,VI,ESC,CNX,RET,QUO,REP,INS,OVERFLOW,DUMB,EOF } code;
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
hidden KeyBinding RegBindings [BINDINGS];
hidden KeyBinding EscBindings [BINDINGS];
hidden KeyBinding CnxBindings [BINDINGS];
hidden KeyBinding * Bindings;		/* current keymap */
hidden KeyBinding * OldBindings;

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
hidden struct Hist * curHistPos;	/* current position in history */
hidden struct sgttyb TtySgttyb;
hidden struct tchars TtyTchars;
hidden struct ltchars TtyLocalChars;
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

/* Procedure Decls */
hidden int PrintingLength ();
hidden WriteToScreen ();
hidden Backup ();
hidden BlotOut ();
hidden ReadTermcap();
hidden EnterCbreakMode();
hidden ExitCbreakMode();
hidden int GetChar ();
#ifdef	PRINTPROMPT
hidden PrintPrompt();
#endif PRINTPROMPT
hidden code SetCtrlX ();
hidden code SetEscape ();
hidden code EnterViInsert ();
hidden code EnterViAppend ();
hidden code ExitViInsert ();
hidden code Return ();
hidden code EndOfFile ();
hidden code Repetition ();
hidden code InsertChar ();
hidden code InsertLiteralChar ();
hidden int DoInsertChar ();
hidden int InsertString ();
hidden code DefaultBinding ();
hidden code Tab ();
hidden code Redisplay ();
#ifdef notdef
hidden Crlf ();
#endif notdef
hidden NewLine ();
hidden code ClearScreen ();
hidden Beep ();
hidden code TransposeChars ();
hidden boolean IsSpaceChar ();
hidden char * NextWordPos ();
hidden char * PrevWordPos ();
hidden DeleteNChars ();
hidden code DeleteWord ();
hidden code EraseWord ();
hidden code DeleteCurrentChar ();
hidden code DeletePreviousChar ();
hidden CopyNChars ();
hidden code SetMark ();
hidden code KillRegion ();
hidden code KillToEOL ();
hidden code EraseLine ();
hidden code YankKillBuffer ();
hidden MoveNChars ();
hidden code ForwardChar ();
hidden code BeginningOfLine ();
hidden code EndOfLine ();
hidden code ForwardWord ();
hidden code BackwardWord ();
hidden code BackSpace ();
hidden InitBuffers();
hidden code FilenameExpansion ();
hidden code FilenameList ();
hidden boolean PushMacro ();
hidden PopMacro ();
hidden code ExecuteMacro ();
hidden code ExecuteNamedMacro ();
hidden code ExecuteUnNamedMacro ();
hidden code StartRemembering ();
hidden code StopRemembering ();
hidden code DefineNamedMacro ();
hidden WriteMacroFile();
hidden ReadMacroFile();
hidden code SaveMacroFile();
hidden code LoadMacroFile();
hidden char * string_index ();
hidden char * string_rindex ();
hidden boolean match ();
hidden char * Index ();
hidden char * ReverseIndex ();
hidden code IncrementalSearchForward ();
hidden code IncrementalSearchReverse ();
hidden code SearchForward();
hidden code SearchReverse();
hidden int InsertCurrentHistEntry();
hidden code PreviousHistEntry();
hidden code NextHistEntry();
hidden code ChangeFollowingObject ();
hidden code DeleteFollowingObject ();
hidden code AppendToEOL ();
hidden code InsertAtBOL ();
hidden code ChangeChar ();
hidden code ChangeToEOL ();
hidden code ChangeWholeLine ();
hidden code ReplaceChar ();
hidden code ViYankKillBuffer ();
hidden InitMacros ();
hidden InitBindings ();
hidden InitEmacsBindings();
hidden InitViBindings();
hidden InitDumbBindings();
hidden InitDefaultBindings();
hidden InitSttyBindings();
hidden SetUserBindings ();
hidden int keytran ();
hidden catn ();
hidden max ();
hidden min ();
hidden copyn ();
hidden fcompare ();
hidden char filetype ();
hidden print_by_column ();
hidden char * tilde ();
hidden extract_dir_and_name ();
hidden char * getentry ();
hidden free_items ();
hidden boolean is_prefix ();
hidden search ();
hidden recognize ();
    
hidden char * FunctionNames [] = {
    "backspace",
    "backwardword",
    "beginningofline",
    "clearscreen",
    "defaultbinding",
    "definenamedmacro",
    "deletecurrentchar",
    "deletepreviouschar",
    "deleteword",
    "endoffile",
    "endofline",
    "eraseline",
    "eraseword",
    "executemacro",
    "executenamedmacro",
    "executeunnamedmacro",
    "filenameexpansion",
    "filenamelist",
    "forwardchar",
    "forwardword",
    "incrementalsearchforward",
    "incrementalsearchreverse",
    "insertchar",
    "insertliteralchar",
    "killregion",
    "killtoeol",
    "nexthistentry",
    "previoushistentry",
    "redisplay",
    "repetition",
    "return",
    "searchforward",
    "searchreverse",
    "setmark",
    "startremembering",
    "stopremembering",
    "tab",
    "transposechars",
    "yankkillbuffer",
    0    
};

hidden CodeFunction FunctionPointers [] = {
    BackSpace,
    BackwardWord,
    BeginningOfLine,
    ClearScreen,
    DefaultBinding,
    DefineNamedMacro,
    DeleteCurrentChar,
    DeletePreviousChar,
    DeleteWord,
    EndOfFile,
    EndOfLine,
    EraseLine,
    EraseWord,
    ExecuteMacro,
    ExecuteNamedMacro,
    ExecuteUnNamedMacro,
    FilenameExpansion,
    FilenameList,
    ForwardChar,
    ForwardWord,
    IncrementalSearchForward,
    IncrementalSearchReverse,
    InsertChar,
    InsertLiteralChar,
    KillRegion,
    KillToEOL,
    NextHistEntry,
    PreviousHistEntry,
    Redisplay,
    Repetition,
    Return,
    SearchForward,
    SearchReverse,
    SetMark,
    StartRemembering,
    StopRemembering,
    Tab,
    TransposeChars,
    YankKillBuffer
};

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

int
tenex_initialized()
{
    return(Initialized == TRUE);
}

dosetupterm ()
{
    Initialized = FALSE;
}

hidden
ReadTermcap()
{
    register struct varent * v;
    register char ** vp;
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
    if ((v = adrof("term")) && (vp = v -> vec)) {
        char termbuf [1024];	    /* size required by tgetent() */
	char * area = areabuf;
	
	if (tgetent(termbuf,*vp) != 1) {    /* 1 == success */
	    printf("Warning: no termcap entry for %s.  Editing disabled.\n",
		    *vp);
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

	if (adrof("nobell") || TermKnown == FALSE) {
	    AudibleBell = VisibleBell = FALSE;
	}
	else {
	    AudibleBell = TRUE;
	    if (VisibleBell = (adrof("visiblebell") ? TRUE : FALSE)) {
		VisibleBellSequence = area;
		tgetstr("vb",&area);	/* get visible bell sequence */
		LengthOfVisibleBellSequence = strlen(VisibleBellSequence);
		if (LengthOfVisibleBellSequence == 0) {
		    VisibleBell = FALSE;	/* tty can't do visible bell */
		}
		else {
		    AudibleBell = (adrof("audiblebell") ? TRUE : FALSE);
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

hidden int InCbreakMode;
hidden bool Echoing;
hidden struct ltchars TenexLchars;
hidden struct tchars TenexTchars;
hidden struct sgttyb TenexSgttyb;

hidden
EnterCbreakMode()
{

    if (! InCbreakMode) {
	    
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

	TenexSgttyb = TtySgttyb;

	EraseChar = TtySgttyb.sg_erase;
	KillChar  = TtySgttyb.sg_kill;

	TenexSgttyb.sg_flags |= CBREAK;
	TenexSgttyb.sg_flags &= ~(ECHO | RAW);

	Echoing = TtySgttyb.sg_flags & ECHO;
	InitSttyBindings();

	InCbreakMode = 1;
    }
    ioctl (SHIN, TIOCSETC, &TenexTchars);
    ioctl (SHIN, TIOCSLTC, &TenexLchars);
    ioctl (SHIN, TIOCSETN, &TenexSgttyb);
}

hidden
ExitCbreakMode()
{
    ioctl (SHIN, TIOCSETC, &TtyTchars);
    ioctl (SHIN, TIOCSLTC, &TtyLocalChars);
    ioctl (SHIN, TIOCSETN, &TtySgttyb);
    InCbreakMode = 0;
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
    if (ShellTypeAheadToTenex) {    /* from history expansion */
	InsertString( ShellTypeAheadToTenex, ShellTypeAheadToTenex +
		strlen( ShellTypeAheadToTenex));
	ShellTypeAheadToTenex = 0;
    }
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

#if	defined(NeXT) && defined(NOTYET)
    *buf &= 0377;
#else
    *buf &= 0177;
#endif	NeXT

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

/*---------------------------------------------------------------------------
 * Tenex -- this is the main routine
 *---------------------------------------------------------------------------
 */
visible int
tenex (inputline, inputlinesize)
char * inputline;
int inputlinesize;
{
    static boolean interrupted = FALSE;	   /* macros can be interrupted */
    
 /* initialize */

    InputBufferSize = inputlinesize;
    Mark = InputPos = InputTail = InputBuffer = inputline;
    RepetitionCount = 1;
    Bindings = OldBindings = RegBindings;
    curHistPos = 0;
    ControlCharsInInput = FALSE;

    if (! Initialized) {
	struct varent * v = adrof("editmode");
	char * ed = 0, ** vp;

	if (v && (vp = v -> vec)) ed = *vp;
		
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
	
#ifdef NeXT_MOD
     /* Duplicate action of standard csh on ^D at beginning of line. */
	if (CurrentChar == '\004' && !IgnoreEOF && InputTail == InputBuffer)
	    goto endloop;
#endif

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

    if (Echoing)
	    NewLine();
    interrupted = FALSE;
    return (InputTail - InputBuffer);
}

#ifdef	PRINTPROMPT
/*---------------------------------------------------------------------------
 * Prints the user's prompt.  This is used after the user runs the
 * FilenameList routine.
 *---------------------------------------------------------------------------
 */
hidden
PrintPrompt()
{
    register char * p; 
    
    for (p = value ("prompt"); *p ; p++) {
	if (*p == LINEFEED) {
	    flush();
	    Crlf();
	    continue;
	}
	if (*p == HIST) {
	    printf ("%d", eventno + 1);
 	}
	else {
	    if (*p == '\\' && p[1] == HIST) {
	        p++;
	    }
	    putchar (*p | QUOTE);
	}
    }
    flush();
}
#endif	PRINTPROMPT

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

hidden code
SetEscape ()
{
/* 
 * #if	NeXT
 *    FilenameExpansion();
 * #endif	NeXT
 */
    OldBindings = Bindings;
    Bindings = EscBindings;
    return ESC;
}

hidden code
EnterViInsert ()
{
    if (Editor == VI) {
	Bindings = OldBindings = RegBindings;
	return INS;
    }
    return DEF;
}

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

hidden code
ExitViInsert ()
{
    if (Editor == VI) {
	Bindings = OldBindings = CnxBindings;
	return INS;
    }
    return DEF;
}

hidden code
Return ()
{
    extern code EndOfLine();
    
    EndOfLine();
    *InputTail++ = '\n';
    return RET;
}

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
	if (Echoing) {
	    WriteToScreen (oldPos, InputTail - oldPos);
	    Backup (InputPos, InputTail - InputPos);
	}
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
	if (Echoing)
	    WriteToScreen (oldPos, InputTail - oldPos);
    }

    if (InputTail == final) {
        return OVERFLOW;
    }
    
    return INS;
}

hidden code
InsertLiteralChar ()
{
    GetChar (&CurrentChar);
    return InsertChar ();
}

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

#ifdef notdef
hidden
Crlf ()
{
    static char * crlf = "\r\n";
    write (SHOUT, crlf, 2);
}
#endif notdef

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

hidden
InitBuffers()
{
    register struct varent *v = adrof("breakchars");
    register char **vp, *p;
    register int    i;

    for (i = 0; i < MAXSPACES; i++) {
	SpaceChar[i] = 0;
    }

    if (v == 0 || (vp = v -> vec) == 0) {
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
	for (p = *vp; *p; p++) {
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
 * Tenex like filename expansion and listing of files matching a prefix
 *---------------------------------------------------------------------------
 */
hidden code
FilenameExpansion ()
{
    extern char * index();
    static char *delims = " '\"\t;&<>()|^%";
    register char  *str_end, *word_start;
    register int    space_left, numitems, not_eol;

    *InputTail = '\0';
    str_end = InputTail;
 /* 
  * Find LAST occurence of a delimiter in the inputline.
  * The word start is one character past it.
  */
    for (word_start = str_end; word_start > InputBuffer; --word_start)
	if (index (delims, word_start[-1]))
	    break;

    space_left = InputBufferSize - (word_start - InputBuffer) - 1;
    numitems = search (word_start, RECOGNIZE, space_left);
    InputTail += strlen(word_start) - (str_end - word_start);
    WriteToScreen (InputPos, InputTail - InputPos);
    InputPos = InputTail;

    if (numitems != 1) {
        Beep();
    }

    return DEF;
}

hidden code
FilenameList ()
{
    static char *delims = " '\"\t;&<>()|^%";
    register char  *str_end, *word_start, last_char, should_retype;
    register int    space_left, numitems;

#ifndef NeXT_MOD
 /* Duplicate action of standard csh on ^D at beginning of line. */

    if (Editor == DUMB && !IgnoreEOF && InputTail == InputBuffer) {
        return EOF;
    }
#endif
    
    *InputTail = '\0';
    str_end = InputTail;
 /* 
  * Find LAST occurence of a delimiter in the inputline.
  * The word start is one character past it.
  */
    for (word_start = str_end; word_start > InputBuffer; --word_start)
	if (index (delims, word_start[-1]))
	    break;

    space_left = InputBufferSize - (word_start - InputBuffer) - 1;

    Crlf();
    numitems = search (word_start, LIST, space_left);
    printprompt();
    WriteToScreen (InputBuffer, InputTail - InputBuffer);
    Backup (InputPos, InputTail - InputPos);

    return DEF;
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
    extern char * malloc(), * realloc();
    
    if (MacroStackDepth >= MacroStackLimit) {
	MacroStackLimit += MACRO_STACK_INCREMENT;
	if (MacroStack) {
	    MacroStack = (String *) realloc( MacroStack,
		    MacroStackLimit * sizeof( String));
	}
	else {
	    MacroStack = (String *) malloc( MacroStackLimit * sizeof( String));
	}
	if (! MacroStack) {
	    printf("\nout of memory\n");
	    if (MacroStackLimit > MACRO_STACK_INCREMENT) {
	        MacroStackLimit = MACRO_STACK_INCREMENT;
		MacroStack = (String *) malloc( MacroStackLimit * 
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
	    MacroStack = (String *) malloc( MacroStackLimit * sizeof( String));
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
	   malloc(0) */
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
    extern char * malloc();
    
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
	    malloc (len = UnNamedMacro.Tail - UnNamedMacro.Head))) {
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
      char *filbuf = malloc( size);
      char *end = filbuf + size;
      char *curpos = filbuf;
      char curmacro;			/* the macro we are defining */
      int *macrosize;			/* and its size */

      if (!filbuf) return -1;		/* malloc failure */

      read( fd, filbuf, size);		/* vread is known not to work here! */

      while (curpos < end) {
        curmacro = *curpos++;		/* extract the macro name */
	macrosize = (int *)curpos;	/* Make a pointer to the size */
	curpos += 4;			/* And skip over it */

	if ((*macrosize<= 0) || (*macrosize > 1024)) break;

#define CurMac KeyboardMacro[ curmacro]
	if (CurMac.Head) free( CurMac.Head);  /* reclaim existing macro */

	if (!(CurMac.Tail = CurMac.Head = malloc( *macrosize))) break;

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
	if (*(searchstring.Tail) != CTRL_S) {
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
	if (*(searchstring.Tail) != CTRL_R) {
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
    
    if (found = index( InputPos, lookfor)) {
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

/*---------------------------------------------------------------------------
 * Doug's code for accessing the history list
 *---------------------------------------------------------------------------
 */
hidden int
InsertCurrentHistEntry()
{
  register struct wordent *stop = &(curHistPos->Hlex);
  register struct wordent *wordl = stop->next;
  int oldrep = RepetitionCount;
  RepetitionCount = 1;
  MoveNChars( InputPos-InputBuffer, BACK);
  DeleteNChars( InputTail-InputBuffer, FORW);
  loop {
    register char *temp = wordl->word;

    if (! InsertString( temp, (temp)+strlen(temp))) return 0;
    wordl = wordl->next;
    if (wordl->next == stop) break;  /* Last word is a newline string */
    CurrentChar = ' '; 
    if (! DoInsertChar()) return 0;
  }
  RepetitionCount = oldrep;
  return 1;
}

hidden code
PreviousHistEntry()
{
  register int repl;
  for( repl = 0; repl < RepetitionCount; repl++ ) {
    if (curHistPos) {
      if ((curHistPos = curHistPos->Hnext) == 0)
        curHistPos = Histlist.Hnext;
    }
    else {
      curHistPos = Histlist.Hnext;
    }
  }
  if (! curHistPos || ! InsertCurrentHistEntry())
    Beep();
  return DEF;
}

hidden code
NextHistEntry()
{
  register struct Hist *target;
  register int repl;

  for( repl = 0; repl < RepetitionCount; repl++) {
    target = curHistPos;  /* save where we are first, then */
    curHistPos = Histlist.Hnext;  /* always start at top.  Its no slower. */
    if (Histlist.Hnext != 0) {
      if (target == Histlist.Hnext) target = 0;
      while (curHistPos->Hnext != target)
        if ((curHistPos = curHistPos->Hnext) == 0) curHistPos = Histlist.Hnext;
    }
  }
  if (! curHistPos || ! InsertCurrentHistEntry())
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

/*---------------------------------------------------------------------------
 * InitMacros -- read macro files during initialization
 *---------------------------------------------------------------------------
 */
hidden
InitMacros ()
{
    register struct varent * v = adrof ("macrofiles");
    register char ** vp;
    register char * name;

    if (v == 0 || (vp = v -> vec) == 0) {    /* no macro file */
        return;
    }
    
    loop {
        if ( (name = *vp++) == 0 ) return;
	ReadMacroFile (name);
    }
}

visible	   /* so it can be called from goodbye() */
SaveMacros ()
{
    register struct varent * v = adrof ("savemacros");
    register char ** vp;
    register char * name;

 /* trey - let the DUMB mode save its macros
    if (Editor == DUMB || v == 0 || (vp = v -> vec) == 0) {
 */
    if (v == 0 || (vp = v -> vec) == 0) {
        return;
    }
    
    if ( (name = *vp) == 0 ) return;
    WriteMacroFile (name);
}

/*---------------------------------------------------------------------------
 * Sets the bindings for keymaps.
 *---------------------------------------------------------------------------
 */

/* dobindings is called from doset (sh.set.c) and during tenex
   initialization */
visible int
dobindings (val)
char * val;
{
    if (! Initialized) return;	  /* only works when tenex is running */
    
    InitBindings( val);
    InitMacros();		/* let DUMB mode set bindings. It used */
    SetUserBindings();		/* not execute these four lines */
    srccatdef("/bindings.std");
    srccathome("/.bindings");
}

hidden
InitBindings (ed)
char * ed;
{
    extern char * folddown();

    if (ed == 0 || TermKnown == FALSE) {
        Editor = DUMB;
    } else {
	folddown (ed, ed);
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
#ifdef NeXT_MOD
    IgnoreEOF = (adrof("ignoreeof") ? TRUE : FALSE);
#endif
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
    ESCBIND (FilenameExpansion, ESCAPE);
    ESCBIND (FilenameList, 'l');
    ESCBIND (ForwardWord, 'f');
    ESCBIND (ExecuteNamedMacro, 'e');
    ESCBIND (ExecuteNamedMacro, 'x');

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
    CNXBIND (FilenameExpansion, ESCAPE);
    CNXBIND (FilenameList, CTRL_D);
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
    CNXBIND (SearchForward, 'f');
    CNXBIND (SearchReverse, 'F');
    CNXBIND (SetMark, 'm');
    CNXBIND (EnterViAppend, 'a');
    CNXBIND (EnterViInsert, 'i');
    CNXBIND (ViYankKillBuffer, 'p');
    CNXBIND (ViYankKillBuffer, 'P');
    
    REGBIND (DeletePreviousChar, CTRL_H);
    REGBIND (EraseWord, CTRL_W);
    REGBIND (ExitViInsert, ESCAPE);
    REGBIND (FilenameList, CTRL_D);
    REGBIND (InsertLiteralChar, CTRL_Q);
    REGBIND (InsertLiteralChar, CTRL_V);
    REGBIND (KillRegion, CTRL_U);
    REGBIND (NextHistEntry, CTRL_N);
    REGBIND (PreviousHistEntry, CTRL_P);
    REGBIND (Redisplay, CTRL_L);
    REGBIND (Redisplay, CTRL_R);
    REGBIND (Return, LINEFEED);
    REGBIND (Return, RETURN);
    REGBIND (Tab, TAB);
}    

hidden
InitDumbBindings()
{
    REGBIND (ClearScreen, CTRL_L);
    REGBIND (FilenameExpansion, ESCAPE);
    REGBIND (FilenameList, CTRL_D);
    REGBIND (Return, LINEFEED);
    REGBIND (Return, RETURN);
}

hidden
InitSttyBindings()
{
    if (EraseChar != (char)0xff)
	    REGBIND (DeletePreviousChar, EraseChar);
    if (ReprintChar != (char)0xff)
	    REGBIND (Redisplay, ReprintChar);
    if (KillChar != (char)0xff)
	    REGBIND (KillRegion, KillChar);
    if (WordErase != (char)0xff)
	    REGBIND (EraseWord, WordErase);
    if (LiteralNext != (char)0xff)
	    REGBIND (InsertLiteralChar, LiteralNext);
}

/*---------------------------------------------------------------------------
 * dobind -- a new cshell command to allow the user to interactively bind
 * internal functions and macros to keys.  Its arguments should be a list
 * of function name and key name pairs.
 *---------------------------------------------------------------------------
 */
visible int
dobind (arglist)
register char ** arglist;
{
    register char ** vp = arglist;
    register char * proc, * keyname;
    register int index, keylen, i;
    KeyBinding * keymap;
    char key, *folddown();

    if (++vp == 0) {    /* no bindings */
        return;
    }
    
    loop {
        if ( ((proc = *vp++) == 0) || ((keyname = *vp++) == 0) ) return;
	keylen = strlen( keyname);
	for (i=0; i < keylen; i++) keyname[i] &= 0177;
	
	if (proc[1] == 0 && KeyboardMacro[ proc[0] ].Head) {
	    index = stablk( "executemacro", FunctionNames, 1);
	}
	else {
	    folddown (proc, proc);
	    if ((index = stablk (proc, FunctionNames, 1)) < 0 ) {
		printf("bind: unknown function '%s'\n", proc);
		return;
	    }
	}
	if (keytran (keyname, &keymap, &key) == 0) {
	    printf("bind: illegal key binding for %s\n", proc);
	    return;
	}
	keymap[key].Routine = FunctionPointers [index];
	keymap[key].MacroName = proc[0];
    }
}

/* The following routine sets key bindings from user instructions in the
   shell variable "bindings".  The value of this variable should be a list
   of procedure/key pairs:  (proc1 key1 proc2 key2 ...).  The legal
   procedure names are defined in the FunctionNames table. */

hidden
SetUserBindings ()
{
    register struct varent * v = adrof ("bindings");
    register char ** vp;
    register char * proc, * keyname;
    register int index;
    KeyBinding * keymap;
    char key, *folddown();

    if (v == 0 || (vp = v -> vec) == 0) {    /* no user bindings */
        return;
    }
    
    loop {
        if ( ((proc = *vp++) == 0) || ((keyname = *vp++) == 0) ) return;
	if (proc[1] == 0 && KeyboardMacro[ proc[0] ].Head) {
	    index = stablk( "executemacro", FunctionNames, 1);
	}
	else {
	    folddown (proc, proc);
	    if ((index = stablk (proc, FunctionNames, 1)) < 0 ) {
		printf("bindings: unknown function '%s'\n", proc);
		return;
	    }
	}
	if (keytran (keyname, &keymap, &key) == 0) {
	    printf("bindings: illegal key binding for %s\n", proc);
	    return;
	}
	keymap[key].Routine = FunctionPointers [index];
	keymap[key].MacroName = proc[0];
    }
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


/* The following code was written by Ken Greer for the old version of
 * tenex, which simply implemented filename expansion and listing.  These
 * are the support routines for the toplevel tenex, which has been re-
 * written.  Not much has been changed in the following.  (Duane)
 *
 * Author: Ken Greer, Sept. 1975, CMU.
 * Finally got around to adding to the Cshell., Ken Greer, Dec. 1981.
 */
#include <sys/dir.h>
#include <pwd.h>

/*
 * Concatonate src onto tail of des.
 * Des is a string whose maximum length is count.
 * Always null terminate.
 */
hidden
catn (des, src, count)
register char *des, *src;
register count;
{
    while (--count >= 0 && *des)
	des++;
    while (--count >= 0)
	if ((*des++ = *src++) == 0)
	    return;
    *des = '\0';
}

hidden
max (a, b)
{
    if (a > b)
	return (a);
    return (b);
}

hidden
min (a, b)
{
    if (a > b)
	return (b);
    return (a);
}

/*
 * like strncpy but always leave room for trailing \0
 * and always null terminate.
 */
hidden
copyn (des, src, count)
register char *des, *src;
register count;
{
    while (--count >= 0)
	if ((*des++ = *src++) == 0)
	    return;
    *des = '\0';
}

/*
 * For qsort()
 */
hidden
fcompare (file1, file2)
char  **file1, **file2;
{
    return (strcmp (*file1, *file2));
}

hidden char
filetype (dir, file)
char *dir, *file;
{
    if (dir)
    {
	char path[512];
	struct stat statb;
	extern char *strcpy ();
	catn (strcpy (path, dir), file, sizeof path);
	if (stat (path, &statb) >= 0)
	{
	    if ((statb.st_mode & S_IFMT) == S_IFDIR)
		return ('/');
	    if (statb.st_mode & 0111)
		return ('*');
	}
    }
    return (' ');
}

/*
 * Print sorted down columns
 */
hidden
print_by_column (dir, items, count)
char *dir, *items[];
{
    register int i, rows, r, c, maxwidth = 0, columns;

    for (i = 0; i < count; i++)
	maxwidth = max (maxwidth, strlen (items[i]));
    maxwidth += 2;	  		/* for the file tag and space */
    columns = 78 / maxwidth;
    rows = (count + (columns - 1)) / columns;
    for (r = 0; r < rows; r++)
    {
	for (c = 0; c < columns; c++)
	{
	    i = c * rows + r;
	    if (i < count)
	    {
		register int w;
		char buf[1];
		
		w = strlen(items[i]);
		WriteToScreen (items[i], w);
		buf[0] = filetype (dir, items[i]);    /* '/' or '*' or ' ' */
		WriteToScreen (buf, 1);
		if (c < (columns - 1))			/* Not last column? */
		    BlotOut (SpaceBuffer, maxwidth - (w + 1));
	    }
	}
	Crlf();
    }
}

/*
 * expand file name with possible tilde usage
 *		~person/mumble
 * expands to
 *		home_directory_of_person/mumble
 *
 * Usage: tilde (new, old) char *new, *old;
 */
hidden char *
tilde (new, old)
char *new, *old;
{
    extern char *strcpy ();
    extern struct passwd *getpwuid (), *getpwnam ();

    register char *o, *p;
    register struct passwd *pw;
    static char person[40] = {0};

    if (old[0] != '~')
	return (strcpy (new, old));

    for (p = person, o = &old[1]; *o && *o != '/'; *p++ = *o++);
    *p = '\0';

    if (person[0] == '\0')			/* then use current uid */
	pw = getpwuid (getuid ());
    else
	pw = getpwnam (person);

    if (pw == NULL)
	return (NULL);

    strcpy (new, pw -> pw_dir);
    (void) strcat (new, o);
    return (new);
}

/*
 * parse full path in file into 2 parts: directory and file names
 * Should leave final slash (/) at end of dir.
 */
hidden
extract_dir_and_name (path, dir, name)
char   *path, *dir, *name;
{
    extern char *rindex ();
    register char  *p;
    p = rindex (path, '/');
    if (p == NULL)
    {
	copyn (name, path, MAXNAMLEN);
	dir[0] = '\0';
    }
    else
    {
	p++;
	copyn (name, p, MAXNAMLEN);
	copyn (dir, path, p - path);
    }
}


hidden char *
getentry (dir_fd, looking_for_lognames)
DIR *dir_fd;
{
    if (looking_for_lognames)		/* Is it login names we want? */
    {
	extern struct passwd *getpwent ();
	register struct passwd *pw;
	if ((pw = getpwent ()) == NULL)
	    return (NULL);
	return (pw -> pw_name);
    }
    else				/* It's a dir entry we want */
    {
	register struct direct *dirp;
	if (dirp = readdir (dir_fd))
	    return (dirp -> d_name);
	return (NULL);
    }
}

hidden
free_items (items)
register char **items;
{
    register int i;
    for (i = 0; items[i]; i++)
	free (items[i]);
    free (items);
}

#define FREE_ITEMS(items)\
{\
    free_items (items);\
    items = NULL;\
}

/*
 * return true if check items initial chars in template
 * This differs from PWB imatch in that if check is null
 * it items anything
 */
hidden boolean
is_prefix (check, template)
char   *check,
       *template;
{
    register char  *check_char,
                   *template_char;

    check_char = check;
    template_char = template;
    do
	if (*check_char == 0)
	    return (TRUE);
    while (*check_char++ == *template_char++);
    return (FALSE);
}

/*
 * Perform a RECOGNIZE or LIST command on string "word".
 */
hidden
search (word, command, max_word_length)
char   *word;
COMMAND command;
{
#   define MAXITEMS 1024
    register DIR *dir_fd;
    register numitems,
	    name_length,		/* Length of prefix (file name) */
	    looking_for_lognames;	/* True if looking for login names */
    char    tilded_dir[MAXPATHLEN + 1],	/* dir after ~ expansion */
	    dir[MAXPATHLEN + 1],	/* /x/y/z/ part in /x/y/z/f */
            name[MAXNAMLEN + 1],	/* f part in /d/d/d/f */
            extended_name[MAXNAMLEN+1],	/* the recognized (extended) name */
            *entry;			/* single directory entry or logname */
    static char
           **items = NULL;		/* file names when doing a LIST */

    if (items != NULL)
	FREE_ITEMS (items);

    looking_for_lognames = (*word == '~') && (index (word, '/') == NULL);
    if (looking_for_lognames)			/* Looking for login names? */
    {
	setpwent ();				/* Open passwd file */
	copyn (name, &word[1], MAXNAMLEN);	/* name sans ~ */
    }
    else
    {						/* Open directory */
	extract_dir_and_name (word, dir, name);
	if (tilde (tilded_dir, dir) == 0)	/* expand ~user/... stuff */
	    return (0);
	if ((dir_fd = opendir (*tilded_dir ? tilded_dir : ".")) == NULL)
	   return (0);
    }

    name_length = strlen (name);

    for (numitems = 0; entry = getentry (dir_fd, looking_for_lognames);)
    {
	if (!is_prefix (name, entry))
	    continue;

	/* Don't match . files on null prefix match */
	if (name_length == 0 && entry[0] == '.' && !looking_for_lognames)
	    continue;

	if (command == LIST)		/* LIST command */
	{
	    extern char *malloc ();
	    if (numitems >= MAXITEMS)
	    {
		printf ("\nYikes!! Too many %s!!\n",
		    looking_for_lognames ? "names in password file":"files");
		break;
	    }
	    if (items == NULL)
	    {
		items = (char **) calloc (sizeof (items[1]),  MAXITEMS + 1);
		if (items == NULL)
		    break;
	    }
	    if ((items[numitems] = malloc (strlen(entry) + 1)) == NULL)
	    {
		printf ("\nout of memory\n");
		break;
	    }
	    copyn (items[numitems], entry, MAXNAMLEN);
	    numitems++;
	}
	else					/* RECOGNIZE command */
	    if (recognize (extended_name, entry, name_length, ++numitems))
		break;
    }

    if (looking_for_lognames)
	endpwent ();
    else
	closedir (dir_fd);

    if (command == RECOGNIZE && numitems > 0)
    {
	if (looking_for_lognames)
	    copyn (word, "~", 1);
	else
	    copyn (word, dir, max_word_length);		/* put back dir part */
	catn (word, extended_name, max_word_length);	/* add extended name */
	return (numitems);
    }

    if (command == LIST)
    {
	register int i;
	qsort (items, numitems, sizeof (items[1]), fcompare);
	print_by_column (looking_for_lognames ? NULL : tilded_dir, 
		items, numitems);
	if (items != NULL)
	    FREE_ITEMS (items);
    }
    return (0);
}

/*
 * Object: extend what user typed up to an ambiguity.
 * Algorithm:
 * On first match, copy full entry (assume it'll be the only match) 
 * On subsequent matches, shorten extended_name to the first
 * character mismatch between extended_name and entry.
 * If we shorten it back to the prefix length, stop searching.
 */
hidden
recognize (extended_name, entry, name_length, numitems)
char *extended_name, *entry;
{
    if (numitems == 1)				/* 1st match */
	copyn (extended_name, entry, MAXNAMLEN);
    else				/* 2nd and subsequent matches */
    {
	register char *x, *ent;
	register int len = 0;
	for (x = extended_name, ent = entry; *x && *x == *ent++; x++, len++);
	*x = '\0';				/* Shorten at 1st char diff */
	if (len == name_length)			/* Ambiguous to prefix? */
	    return (-1);			/* So stop now and save time */
    }
    return (0);
}
