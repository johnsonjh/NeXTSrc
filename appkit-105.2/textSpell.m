/*
	textTabs.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Application.h"
#import "Cell.h"
#import "NXCursor.h"
#import "Matrix.h"
#import "ScrollView.h"
#import "SelectionCell.h"
#import "Panel.h"
#import "Text.h"
#import "Window.h"
#import "textprivate.h"
#import "ss.h"
#import "spellserver.h"
#import "publicWraps.h"
#import <mach.h>
#import <objc/List.h>
#import <netname_defs.h>
#import <servers/netname.h>

#ifndef SHLIB
    /* this defines a global symbol in this category .o files so the main class can reference it.  See the main class for more details. */
    asm(".NXTextCategorySpell=0\n");
    asm(".globl .NXTextCategorySpell\n");
#endif

typedef struct {
    @defs(Cell);
} CellId;

@interface NXSpellChecker:Object
{
@public
    id guessScroll;
    id guessWindow;
    id guessMatrix;
    id matrix;
    int handle;
    port_t server;
    char *textbuf;
    int startWord, endWord;
    unsigned int guessesCnt;
    int numGuesses;
    char *guesses;
    kern_return_t ret;
    NXSize minGuessWindowSize;
}
- openHandle;
- showGuesses:sender;
- updateGuesses;
- (char *)buffer;
- spellCheckAndGuess:(int)count curPos:(int)cur;
- forgetWord:(char *)word;
- learnWord:(char *)word;

@end

static NXSpellChecker *spellChecker = nil;

@implementation Text (SpellChecking)

- checkRange:(int) start to:(int)end spell:(NXSpellChecker *)spell
{
    NXStream *text;
    int toRead, nRead, total = end - start;
    int cur = start;
    char *textbuf = [spell buffer];

    text = [self stream];
    NXSeek(text, start, NX_FROMSTART);
    while (total > 0) {
	toRead = MIN(1024, total);
	nRead = NXRead(text, textbuf, toRead);
	if (nRead < toRead || (cur + toRead) >= end) {
	    textbuf[nRead] = '\n';
	    nRead++;
	}
	if ([spell spellCheckAndGuess:nRead curPos:cur])
	    break;
	cur += spell->endWord;
	total -= spell->endWord;
	if (cur + 1 >= textLength)
	    break;
	NXSeek(text, cur, NX_FROMSTART);
    }
    return self;
}

- showGuessPanel:sender
{
    [[NXSpellChecker new] showGuesses:sender];
    return self;
}


- checkSpelling:sender
{
    int pos;
    NXSpellChecker *spell = [NXSpellChecker new];
    if (sp0.cp < 0)
	pos = 0;
    else 
	pos = spN.cp;
    [self checkRange:pos to:textLength -1 spell:spell];
    if (spell->startWord >= 0) {
	[self setSel:spell->startWord:spell->endWord];
	[self scrollSelToVisible];
	[spell updateGuesses];
	return self;
    }
    if (pos) {
	[self checkRange:0 to:pos spell:spell];
	if (spell->startWord >= 0) {
	    [self setSel:spell->startWord:spell->endWord];
	    [self scrollSelToVisible];
	    [spell updateGuesses];
	    return self;
	}
    }
    NXBeep();
    return self;
}

- selectGuess:sender
{
    id cell = [sender selectedCell];
    const char *string = [cell stringValue];
    int length, start;
    if (!string || sp0.cp < 0 || ![self isEditable]) {
	NXBeep();
	return self;
    }
    length = strlen(string);
    start = sp0.cp;
    [window disableFlushWindow];
    [self replaceSel:[cell stringValue]];
    [self setSel:start:start + length];
    [window reenableFlushWindow];
    [window flushWindow];
    return self;
}

- validSel:(char *)word
{
    int length;
    if (sp0.cp < 0 || sp0.cp == spN.cp) {
	NXBeep();
	return nil;
    }
    length = spN.cp - sp0.cp;
    if (length >= MAX_WORD_LENGTH) {
	NXBeep();
	return nil;
    }
    [self getSubstring:word start:sp0.cp length:length];
    word[length] = 0;
    return self;
}

- learnSelection:sender
{
    char word[MAX_WORD_LENGTH];
    if (![self validSel:word])
	return self;
    [[NXSpellChecker new] learnWord:word];
    return self;
}

- forgetSelection:sender
{
    char word[MAX_WORD_LENGTH];
    if (![self validSel:word])
	return self;
    [[NXSpellChecker new] forgetWord:word];
    return self;
}


@end

static int compareName(const void *v1, const void *v2)
{
    const CellId * const *p1 = v1;
    const CellId * const *p2 = v2;
    const char *name1 = (*p1)->contents;
    const char *name2 = (*p2)->contents;
    char c1 = *name1;
    char c2 = *name2;
    if (c1 > c2) 
	return 1;
    else if (c1 < c2) 
	return -1;
    return strcmp(name1, name2);
}

extern void _NXSSError(kern_return_t errorCode)
{

}


@implementation NXSpellChecker

- (char *)buffer
{
    if (!textbuf) {
	NXZone *zone = [self zone];
	textbuf = NXZoneMalloc(zone, vm_page_size);
    }
    return textbuf;
}

- learnWord:(char *)word
{
    if (!handle) 
	[[NXSpellChecker new] openHandle];
    _NXLearnWord(server, handle, word);
    return self;
}

- forgetWord:(char *)word
{
    if (!handle) 
	[[NXSpellChecker new] openHandle];
    _NXForgetWord(server, handle, word);
    return self;
}

+ new
{
    if (spellChecker)
	return spellChecker;
    self = [super allocFromZone:[NXApp zone]];
    spellChecker = self;
    return self;
}

- setGuessScroll:anObject
{
    guessScroll = anObject;
    return self;
}

- setGuessWindow:anObject
{
    guessWindow = anObject;
    return self;
}

- spellCheckAndGuess:(int)count curPos:(int)cur
{
    if (!handle) 
	[self openHandle];
    if (guesses) {
	vm_deallocate(task_self(), (vm_address_t) guesses, guessesCnt);
	guesses = 0;
    }
    _NXSpellCheckAndGuess(
	server, handle, textbuf, vm_page_size, 0, count, 
	&startWord, &endWord, &guesses, &guessesCnt, &numGuesses);
    if (startWord >= 0) {
	startWord += cur;
	endWord += cur;
	return self;
    }
    return nil;
}

- openHandle
{
    ret = netname_look_up(name_server_port, "", SS_NAME, &server);
    if (ret != KERN_SUCCESS) {
	if (_NXExecDetached(NULL, "/usr/etc/spelld"))
	    while (ret != KERN_SUCCESS) {
		sleep(1);
		ret = netname_look_up(name_server_port, "", SS_NAME, &server);
	    }
    }
    _NXOpenSpelling(server, [NXApp replyPort], (char *) NXUserName(), "LocalDictionary", &handle);
    return self;
}

- windowWillResize:sender toSize:(NXSize *)frameSize
{
    if (frameSize->width < minGuessWindowSize.width) {
	frameSize->width = minGuessWindowSize.width;
    }
    if (frameSize->height < minGuessWindowSize.height) {
	frameSize->height = minGuessWindowSize.height;
    }
    return self;
}

- showGuesses:sender
{
    if (!matrix) {
	NXRect rect = {0};
	NXSize size = {0};
	if (!guessWindow) {
	    _NXLoadNib("__APPKIT_PANELS", "SpellChecker", self, [NXApp zone]);
	}
	[guessScroll getContentSize:&rect.size];
	matrix = [[Matrix allocFromZone:[guessScroll zone]] initFrame:&rect mode:NX_RADIOMODE cellClass:[SelectionCell class] numRows:0 numCols:1];
	[matrix setIntercell:&size];
	[matrix getCellSize:&size];
	size.width = rect.size.width;
	[matrix setCellSize:&size];
	[matrix setOpaque:YES];
	[matrix setBackgroundGray:NX_LTGRAY];
	[matrix setAutodisplay:NO];
	[matrix setAutoscroll:YES];
	[matrix setAutosizeCells:YES];
	[matrix setAutosizing:NX_WIDTHSIZABLE];
	[matrix setDoubleAction:@selector(selectGuess:)];
	[[guessScroll setDocView:matrix] free];
	[guessScroll setDocCursor:NXArrow];
	[guessScroll setBackgroundGray:NX_LTGRAY];
	[guessWindow getFrame:&rect];
	minGuessWindowSize = rect.size;
	[guessMatrix setAutosizeCells:YES];
    }
    [guessWindow orderFront:self];
    NXPing();
    [self updateGuesses];
    return self;
}

- updateGuesses
{
    char *cur;
    id cellList;
    int i;
    
    if ([guessWindow isVisible]) {
	id win = [matrix window];
	int nrows, ncols;
	[matrix getNumRows:&nrows numCols:&ncols];
	if (!nrows && !numGuesses)
	    return self;
	[win disableFlushWindow];
	[matrix renewRows:numGuesses cols:1];
	cellList = [matrix cellList];
	cur = guesses;
	for (i = 0; i < numGuesses; i++) {
	    id cell = [cellList objectAt:i];
	    [cell setStringValueNoCopy:cur];
	    cur += strlen(cur) + 1;
	}
	qsort(NX_ADDRESS(cellList), numGuesses, sizeof(id), compareName);
	[matrix sizeToCells];
	[guessScroll display];
	[win reenableFlushWindow];
	[win flushWindow];
    }
    return self;
}

@end

/*

83
--
  4/25/90 bgy	fixed the bug where spell checking would hang if there was
  		 no CR or space at the end of the text object.

85
--
 4/19/90 trey	nuked use of NXWait
 6/04/90 pah	Fixed bug where if you spell check a Text object which has only
		 one misspelled word, it infinite loops.

92
--
 8/20/90 gcockrft  Don't let spelling modify non-editable documents.

94
--
 9/26/90 aozer	Added code to prevent the guess window from becoming too small.
		Also made the guess matrix & buttons autosize cells.

*/
