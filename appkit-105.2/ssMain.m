
#define c_plusplus 1
#import "ss.h"
#import "SpellServer.h"
#import <string.h>
#import <libc.h>

void main (int argc, char *argv[])
{
    id spell;
    chdir("/");
    spell = [SpellServer new];
    [spell initServer];
    [spell serverLoop];
}


extern int _NXOpenSpelling(
    port_t server, port_t client, word_t user, path_t localDict, int *handle)
{
    id spell = [SpellServer new];
    *handle = (int) [spell openSpellingFor:user port:client path:localDict];
    return 0;
}

static int spellCheck(
    port_t server, int handle, data_t text, unsigned int textCnt,
    int start, int end, int *startPos, int *endPos, int dealloc)
{
    id spell = [SpellServer new];
    id info = [spell validInfo:handle];
    id found;
    int pos0, posN;
    if (info) {
	found = [info spellCheck:text + start length:end - start 
	    start:&pos0 end:&posN];
	if (found) {
	    *startPos = start + pos0;
	    *endPos = start + posN;
	} else {
	    *startPos = -1;
	    *endPos = start + posN;
	}
    }
    if (dealloc)
	vm_deallocate(task_self(), (vm_address_t)text, textCnt);
    return 0;
}

extern int _NXSpellCheck(
    port_t server, int handle, data_t text, unsigned int textCnt,
    int start, int end, int *startPos, int *endPos)
{
    return spellCheck(
	server, handle, text, textCnt, start, end, startPos, endPos, 1);
}

extern int _NXGuess(
    port_t server, int handle, word_t word, data_t *guesses, 
    unsigned int *guessesCnt, int *numGuesses)
{
    id spell = [SpellServer new];
    id info = [spell validInfo:handle];
    if (info) {
	*numGuesses = [info guess:word guesses:guesses length:guessesCnt];
    }
    return 0;
}

extern int _NXSpellCheckAndGuess(
    port_t server, int handle, data_t text, unsigned int textCnt, 
    int start, int end, int *startWord, int *endWord, data_t *guesses, 
    unsigned int *guessesCnt, int *numGuesses)
{
    spellCheck(server, handle, text, textCnt, start, end, 
	startWord, endWord, 0);
    if (*startWord < 0) {
	*guesses = 0;
	*guessesCnt = 0;
	*numGuesses = 0;
    } else {
	char buf[FILENAME_MAX];
	int length = *endWord - *startWord;
	strncpy(buf, text + *startWord, length + 1);
	buf[length] = 0;
	_NXGuess(server, handle, buf, guesses, guessesCnt, numGuesses);
    }
    vm_deallocate(task_self(), (vm_address_t)text, textCnt);
    return 0;
}

kern_return_t _NXLearnWord(port_t server, int handle, word_t word)
{
    id spell = [SpellServer new];
    id info = [spell validInfo:handle];
    
    [info learnWord:word];
    return 0;
}

kern_return_t _NXForgetWord(port_t server, int handle, word_t word)
{
    id spell = [SpellServer new];
    id info = [spell validInfo:handle];
    
    [info forgetWord:word];
    return 0;
}





