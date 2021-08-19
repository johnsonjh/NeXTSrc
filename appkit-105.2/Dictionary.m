
#import "Dictionary.h"
#import "SpellServer.h"
#import <sys/file.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <NXCType.h>
#import <mach.h>
#import <stdio.h>
#import <libc.h>
#import <stdlib.h>
#import <libc.h>

extern int map_fd();

#ifndef NEWSTRING
#define NEWSTRING(x) ((char *) strcpy(malloc(strlen(x) + 1), x))
#endif NEWSTRING
static BOOL checkSpelling(const char *string, UserInfo *info);

static Dictionary *dictionary = nil;


extern void lowerit(char * s)
{
    for (; *s; s++) {
	if (NXIsUpper(*s))
	    *s = NXToLower(*s);
    }
}


@implementation Dictionary

+ newResource:(const char *) resourceDir
{
    char buf[FILENAME_MAX];
    if (dictionary)
	self = dictionary;
    else {
	self = [super new];
	dictionary = self;
    }
    sprintf(buf, "%s/bindict", resourceDir);
    sourceName = NEWSTRING(buf);
    return self;
}

+ new
{
    return dictionary;
}


- openfile
{
    int fd;
    DictHeader *buf = 0;
    struct stat info;
    unsigned char **cur, **last,*table;

    fd = open(sourceName, O_RDONLY, 0660);
    if (fd < 0)
	return nil;
    if (fstat(fd, &info) < 0)
	return nil;
    filesize = info.st_size;
    map_fd(fd, 0, (vm_offset_t *)&buf, 1, filesize);
    close (fd);
    last = tables + NDICTS;
    table = (unsigned char *) buf + buf->firstTable;
    for (cur = tables; cur < last; cur++) {
	*cur = table;
	table += buf->tableSize;
    }
    return self;
}

#define ISPUNCT(ch) (NXIsPunct(ch) && ch != '\'')
#define ISWHITE(ch) (ch == ' ' || ch == '\t' || ch == '\n' || NXIsDigit(ch) || ISPUNCT(ch))

- (BOOL) findMisspelled:(char *)text length:(int)length
    info:info loc1:(int *) loc1 loc2:(int *) loc2
{
    char buf[FILENAME_MAX], *s;
    int pos1, posN;
    char *cur, *last;
    
    cur = text;
    last = text + length;
    *loc1 = -1;
    *loc2 = length;
    if (!tables[0])
	[self openfile];
    while (cur < last) {
	s = buf;
	while (cur < last && ISWHITE(*cur)) {
	    cur++;
	}
	if (cur >= last) {
	    return NO;
	}
	pos1 = cur - text;
	*s++ = *cur++;
	while (cur < last && !ISWHITE(*cur)) {
	    *s++ = *cur++;
	}
	*s = 0;
	if (cur >= last) {
	    *loc2 = pos1;
	    return NO;
	}
	posN = cur - text;
	if (!checkSpelling(buf, info)) {
	    *loc1 = pos1;
	    *loc2 = posN;
	    return YES;
	}
    }
    return NO;
}

@end


typedef unsigned (*HashProc)(const char *string, int length);

static unsigned hash1(const char *entry, int length);
static unsigned hash2(const char *entry, int length);
static unsigned hash3(const char *entry, int length);
static unsigned hash4(const char *entry, int length);
static unsigned hash5(const char *entry, int length);
static unsigned hash6(const char *entry, int length);
static unsigned hash7(const char *entry, int length);
static unsigned hash8(const char *entry, int length);

static HashProc procs[] = {
    hash1, hash2, hash3, hash4, hash5, hash6, hash7, hash8, 0
};

extern BOOL inDict(const char *entry, UserInfo *info)
{
    int i, length = strlen(entry);
    HashProc proc;
    unsigned int val;
    unsigned const char *table, *addr, **tables;
    unsigned ch;
    unsigned mask;
    tables = dictionary->tables;
    for (i = 0; i < NDICTS; i++) {
	proc = procs[i];
	table = tables[i];
	val = proc(entry, length);
	val = DICT_MOD(val);
	addr = table + (val / BITS_PER_BYTE);
	ch = *addr;
	mask = 1 << (val & 7);
	if (!(mask & ch)) {
	    return info->proc(info, @selector(inDict:), entry) ? YES : NO;
	}
    }
    return YES;
}

extern void setDict(const char *entry, unsigned char *tables[])
{
    int i, length = strlen(entry);
    HashProc proc;
    unsigned int val;
    unsigned char *table, *addr;
    unsigned ch;
    unsigned setbit;
    for (i = 0; i < NDICTS; i++) {
	proc = procs[i];
	table = tables[i];
	val = proc(entry, length);
	val = DICT_MOD(val);
	addr = table + (val / BITS_PER_BYTE);
	ch = *addr;
	setbit = (1 << (val & 7));
	*addr = ch | setbit;
    }
}

static BOOL checkSpelling(const char *string, UserInfo *info)
{
    char buf[FILENAME_MAX];
    
    strcpy(buf, string);
    lowerit(buf);
    if (inDict(buf, info))
	return YES;
    return NO;
}


static unsigned hash1(const char *entry, int length)
{
    unsigned int val = 1, ch, cur = 1, fact = 2;
    while (ch = *entry++) {
	val *= (ch + fact);
	fact *= ++cur;
	val++;
    }
    return val;
}

static unsigned hash2(const char *entry, int length)
{
    unsigned int val = 1, cur = 1, ch;
    while (ch = *entry++) {
	val *= ch * cur;
	cur++;
	val++;
    }
    return val * length;
}

static unsigned hash3(const char *entry, int length)
{
    unsigned int val = 1, ch, cur = 1;
    while (ch = *entry++) {
	val *= ch * ((ch & 7) + 1) * cur;
	cur++;
	val++;
    }
    return val * length;
}

static unsigned hash4(const char *entry, int length)
{
    unsigned int val = 1, ch, cur = 1;
    while (ch = *entry++) {
	val *= ch + (cur + ((ch & 15) + 1));
	cur++;
	val++;
    }
    return val * length;
}

static unsigned hash5(const char *entry, int length)
{
    unsigned int val = 1, ch, cur = length;
    while (ch = *entry++) {
	val *= ch + cur;
	cur += 7;
	val++;
    }
    return val;
}

static unsigned hash6(const char *entry, int length)
{
    unsigned int val = 1, ch, cur = 1;
    while (ch = *entry++) {
	val *= cur * cur * ch ;
	cur++;
	val++;
    }
    return val * length;
}

static unsigned hash7(const char *entry, int length)
{
    unsigned int val = 1, ch, cur = 1;
    while (ch = *entry++) {
	val *= (ch + cur) * cur;
	cur++;
	val++;
    }
    return val * length;
}

static unsigned hash8(const char *entry, int length)
{
    unsigned int val = 1, ch, cur = length;
    while (ch = *entry++) {
	val *= ch - cur;
	cur++;
	val++;
    }
    return val;
}

static void addGuess(const char *guess, UserInfo *info)
{
    int length = strlen(guess) + 1;
    if ((info->curGuess + length) > (info->guesses + info->guessSize)) {
	int newSize = info->guessSize * 2;
	vm_address_t newGuess = 0;
	vm_address_t guesses = (vm_address_t) info->guesses;
	int consumed = info->curGuess - info->guesses;
	
	vm_allocate(task_self(), &newGuess, newSize, 1);
	vm_copy(task_self(), guesses, info->guessSize, newGuess);
	vm_deallocate(task_self(), guesses, info->guessSize);
	info->guessSize = newSize;
	info->guesses = (char *)newGuess;
	info->curGuess = info->guesses + consumed;
    }
    strcpy(info->curGuess, guess);
    info->curGuess += length;
}

static int transpose(const char *string, UserInfo *info)
{
    char *s, temp;
    char buf[FILENAME_MAX];
    int count = 0;
    
    strcpy(buf, string);
    for (s = buf; *s; s++) {
	if (!(*(s+1)))
	    continue;
	else {
	    temp = *s;
	    *s = *(s+1);
	    *(s+1) = temp;
	    if (inDict(buf, info)) {
		addGuess(buf, info);
		count++;
	    }
	    temp = *s;
	    *s = *(s+1);
	    *(s+1) = temp;
	}
	if (!(*(s+2)))
	    continue;
	else {
	    temp = *s;
	    *s = *(s+2);
	    *(s+2) = temp;
	    if (inDict(buf, info)) {
		addGuess(buf, info);
		count++;
	    }
	    temp = *s;
	    *s = *(s+2);
	    *(s+2) = temp;
	}
	if (!(*(s+3)))
	    continue;
	else {
	    temp = *s;
	    *s = *(s+3);
	    *(s+3) = temp;
	    if (inDict(buf, info)) {
		addGuess(buf, info);
		count++;
	    }
	    temp = *s;
	    *s = *(s+3);
	    *(s+3) = temp;
	}
    }
    return count;
}

static int replace(const char *string, UserInfo *info)
{
    char *s, temp, ch;
    char buf[FILENAME_MAX];
    int count = 0;

    strcpy(buf, string);
    for (s = buf; *s; s++) {
	temp = *s;
	for (ch = 'a'; ch <= 'z'; ch++) {
	    *s = ch;
	    if (inDict(buf, info)) {
		addGuess(buf, info);
		count++;
	    }
	}
	*s = '\'';
	if (inDict(buf, info)) {
	    addGuess(buf, info);
	    count++;
	}
	*s = temp;
    }
    return count;
}

static int delete(const char *string, UserInfo *info)
{
    char *dst;
    const char *s, *src;
    char buf[FILENAME_MAX];
    int count = 0;

    for (s = string; *s; s++) {
	dst = buf;
	for (src = string; *src; src++) {
	    if (src != s)
		*dst++ = *src;
	}
	*dst = 0;
	if (inDict(buf, info)) {
	    addGuess(buf, info);
	    count++;
	}
    }
    return count;
}

static int insert(const char *string, UserInfo *info)
{
    char *skip, *dst, ch;
    const char *s, *last;
    char buf[FILENAME_MAX];
    int count = 0, length, i;

    length = strlen(string);
    last = string + length + 1;
    buf[length + 1] = 0;
    for (i = 0; i <= length; i++) {
	skip = buf + i;
	dst = buf;
	for (s = string; s < last; s++) {
	    if (dst == skip)
		dst++;
	    *dst++ = *s;
	}
	for (ch = 'a'; ch <= 'z'; ch++) {
	    *skip = ch;
	    if (inDict(buf, info)) {
		addGuess(buf, info);
		count++;
	    }
	}
    }
    strcpy(buf, string);
    return count;
}

extern int guess(const char *string, id _info)
{
    int count = 0;
    UserInfo *info = _info;
    char buf[FILENAME_MAX];
    
    strcpy(buf, string);
    lowerit(buf);
    info->curGuess = info->guesses;
    count += transpose(buf, info);
    count += replace(buf, info);
    count += delete(buf, info);
    count += insert(buf, info);
    return count;
}










