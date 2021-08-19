/*
 *	objc-sel.m
 *	Copyright 1988, NeXT, Inc.
 *	Author:	s. naroff
 *
 *	Public data:		
 *	-----------
 *
 *	Public methods:		
 *	--------------
 *
 *	Private methods:
 *	---------------
 *
 */

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "objc-private.h"

typedef struct hashedSelector 	HASH, *PHASH;
/*
 * These static data declarations have been moved to here so that all static
 * data appears first in this object so that when this is put in a shared
 * library and all the global data is moved to the begining it will remain in
 * the same place if more static data is added.
 */
static PHASH *	hashList = 0;
static int	nHashLists = 0;

static PHASH 	hash_alloc_list = 0;
static int	hash_alloc_index = 0;

struct hashedSelector {
	PHASH		next;
	STR 		key;
	SEL		uid;
};

#define HASH_ALLOC_LIST_SIZE	170

#define SIZEHASHTABLE 	821

/* NEW - snaroff */

typedef struct optHashedSelector 	OHASH, *POHASH;

static POHASH *	opthashList = 0;
static POHASH *	freezeDriedHashList = 0;

static POHASH 	ohash_alloc_list = 0;
static int	ohash_alloc_index = 0;

struct optHashedSelector {
	POHASH		next;
	STR 		key_uid;
};


static SEL _minSelector = (SEL)1, _maxSelector = (SEL)0;
static SEL _sel_min = 0, _sel_max = 0;

static inline PHASH hashNew(key, link)
STR key;
PHASH link;
{
	PHASH obj;

	if (!hash_alloc_list || hash_alloc_index >= HASH_ALLOC_LIST_SIZE) {
		hash_alloc_index = 0;
		hash_alloc_list = (PHASH)malloc(sizeof(HASH) * 
		    HASH_ALLOC_LIST_SIZE);
		if (!hash_alloc_list)
			perror ("unable to allocate in _objcInit");
	}
	obj = &hash_alloc_list[hash_alloc_index++];
	obj->next = link;
	obj->key = key;
	return obj;
}

static int nopthashents = 0;

static inline POHASH opthashNew(key, link)
STR key;
POHASH link;
{
	POHASH obj;

	/* allocating chunks saves a lot of time (given our current malloc)! */

	if (!ohash_alloc_list || ohash_alloc_index >= HASH_ALLOC_LIST_SIZE) {
		ohash_alloc_index = 0;
		ohash_alloc_list = (POHASH)malloc(sizeof(OHASH) * 
		    HASH_ALLOC_LIST_SIZE);
		if (!ohash_alloc_list)
			perror ("unable to allocate in _objcInit");
	}
	obj = &ohash_alloc_list[ohash_alloc_index++];
	nopthashents++;
	obj->next = link;
	obj->key_uid = key;
	return obj;
}

BOOL sel_isMapped(SEL sel)
{
	return ((sel >= _minSelector && sel <= _maxSelector) ||
	        (sel >= _sel_min && sel <= _sel_max));
}

/*
 *	Purpose: 
 *	Options:
 *	Assumptions:
 *	Side Effects: 
 *	Returns: 
 *	Visibility:	private extern.
 * 	Comments:	this needs to be cleaned up - snaroff (7/24/90)
 */
SEL _sel_getMaxUid()
{
	if (freezeDriedHashList && (nopthashents == 0)) {
		POHASH target;
		int n;

		for (n = 0; n < nHashLists; n++) {
		   for (target = freezeDriedHashList[n]; target; 
				target = target->next)
			nopthashents++;
		}
	}

	return (SEL)(nopthashents + (int)_maxSelector);
}

/*
 *	Purpose: 
 *	Options:
 *	Assumptions:
 *	Side Effects: 
 *	Returns: 
 */
STR sel_getName(SEL sel)
{
	if (ISSELECTOR(sel)) {
		PHASH target;
		POHASH otarget;
		int n;

		/* first, look in the optimized hash table */

		for (n = 0; n < nHashLists; n++) {
		   for (otarget = opthashList[n]; otarget; 
						otarget = otarget->next)
			if (sel == (SEL)otarget->key_uid)
				return otarget->key_uid;
		}
		for (n = 0; n < nHashLists; n++) {
		   for (target = hashList[n]; target; target = target->next)
			if (sel == target->uid)
				return target->key;
		}
	}
	return 0;
}

unsigned _strhash(s)
register unsigned char *s; 
{
	register unsigned int hash = 0;

	// unroll the loop
	for (; ; ) { 
		if (*s == '\0') 
			break;
		hash ^= *s++;
		if (*s == '\0') 
			break;
		hash ^= *s++ << 8;
		if (*s == '\0') 
			break;
		hash ^= *s++ << 16;
		if (*s == '\0') 
			break;
		hash ^= *s++ << 24;
	}
	return hash;
}

static inline unsigned _sel_hash(s)
register unsigned char *s; 
{
	register unsigned int hash = 0;

	// unroll the loop
	for (; ; ) { 
		if (*s == '\0') 
			break;
		hash ^= *s++;
		if (*s == '\0') 
			break;
		hash ^= *s++ << 8;
		if (*s == '\0') 
			break;
		hash ^= *s++ << 16;
		if (*s == '\0') 
			break;
		hash ^= *s++ << 24;
	}
	return hash;
}

/*
 *	Purpose: 
 *	Options:
 *	Assumptions:
 *	Side Effects: 
 *	Returns: 
 */
static inline SEL _sel_registerName_1(STR key, BOOL useString)
{
	PHASH target;
	POHASH otarget;
	int slot;

	if (! key) return 0;	/* added by BS for the archiver */

	/* optimization - snaroff */
	if ((SEL)key >= _sel_min && (SEL)key <= _sel_max) {

		if (!freezeDriedHashList) {
			slot = _sel_hash(key) % nHashLists;
			otarget = opthashList[slot];

			while (otarget && 
  			  ((*(short *)key != *(short *)otarget->key_uid) ||
	 		   (strcmp(key, otarget->key_uid) != 0)))
				otarget = otarget->next;

			if (!otarget) {
				otarget = opthashList[slot] = 
					opthashNew(key, opthashList[slot]);
			}
		}
		return (SEL)key;
	}

	slot = _sel_hash(key) % nHashLists;

	/* first, look in the optimized (maybe freeze dried) hash table */

	otarget = opthashList[slot];

	while (otarget && 
		((*(short *)key != *(short *)otarget->key_uid) ||
 		(strcmp(key, otarget->key_uid) != 0)))
		otarget = otarget->next;

	if (otarget)
		return (SEL)otarget->key_uid;

	/* now look in hash table for the app */

	target = hashList[slot];

	while (target && ((*(short *)key != *(short *)target->key) ||
	 	(strcmp(key, target->key) != 0)))
		target = target->next;

	/* add to list... */

	if (!target) {
		target = hashList[slot] = hashNew(key, hashList[slot]);
		++(unsigned int)_maxSelector;
		target->uid = useString ? (SEL)key : (SEL)_maxSelector;
	}
	return target->uid;
}

SEL _sel_registerName(STR key)
{
	return _sel_registerName_1(key, NO);
}

SEL _sel_registerNameUseString(STR key)
{
	return _sel_registerName_1(key, YES);
}

void _sel_unloadSelectors(char *min, char *max)
{
	PHASH target, prev;
	int n;

	for (n = 0; n < nHashLists; n++) {

	   for (target = hashList[n], prev = 0; target; 
		prev = target, target = target->next)

		if (target->key >= min && target->key <= max) {

		  /* remove it! - Note leak: because of the way we 
		   * allocate these entries in chunks I can't free 
		   * this entry...
		   */
		  if (prev)
		  	prev->next = target->next;
		  else
		     	hashList[n] = target->next;
		}
	}
}

/*
 *	Purpose: 
 *	Options:
 *	Assumptions:
 *	Side Effects: 
 *	Returns: 
 */
SEL sel_getUid(STR key)
{
	PHASH target;
	POHASH otarget;
	int slot;

	if (!key) 
	  return (SEL)0;

	slot = _sel_hash(key) % nHashLists;

	/* first, look in the optimized (maybe freeze dried) hash table */

	otarget = opthashList[slot];

	while (otarget && 
		((*(short *)key != *(short *)otarget->key_uid) ||
 		(strcmp(key, otarget->key_uid) != 0)))
		otarget = otarget->next;

	if (otarget)
		return (SEL)otarget->key_uid;

	/* now look in app hash table */

	target = hashList[slot];

	while (target && ((*(short *)key != *(short *)target->key) ||
	 	(strcmp(key, target->key) != 0)))
		target = target->next;

	if (target)
		return target->uid;
	else
		return 0;
}

#ifdef FREEZE

void _sel_printHashTable()
{
	POHASH target;
	int n;

	for (n = 0; n < nHashLists; n++) {
		printf("opthashList[%d] = ",n);
		for (target = opthashList[n]; target; target = target->next)
			printf("<%s,%d>",target->key_uid, target->key_uid);
		printf("\n");
		printf("freezeDriedHashList[%d] = ",n);
		for (target = freezeDriedHashList[n]; target; target = target->next)
			printf("<%s,%d>",target->key_uid, target->key_uid);
		printf("\n");
	}
}

/* used by `objcopt' to freeze dry a hash table */

void _sel_writeHashTable(int start_addr, 
			 char *myAddressSpace,
			 char *shlibAddressSpace,
			 void **addr, int *size)
{
	PHASH target;
	int n;
	int hash_size;
	POHASH *hash_table_addr;
	POHASH hash_entry_addr, vm_hashEntryList;

	vm_hashEntryList = start_addr + (nHashLists * sizeof(PHASH));

	hash_size = (nHashLists * sizeof(POHASH)) + 
		((unsigned)_maxSelector * sizeof(OHASH));

	hash_table_addr = (POHASH *)malloc(hash_size);
	hash_entry_addr = (char *)hash_table_addr + (nHashLists * sizeof(POHASH));

	for (n = 0; n < nHashLists; n++) {
		if (hashList[n]) {
		  hash_table_addr[n] = vm_hashEntryList;
  
		  for (target = hashList[n]; target; target = target->next) {
			hash_entry_addr->key_uid = 
				(target->key - myAddressSpace) + 
					shlibAddressSpace;
			if (target->next)
				hash_entry_addr->next = vm_hashEntryList + 1;
			hash_entry_addr++, vm_hashEntryList++;
		  }
		}
	}
	*addr = hash_table_addr;
	*size = hash_size;
}
#endif

/*
 *	Purpose:
 *	Options:
 *	Assumptions:
 *	Side effects:
 *	Returns:
 */
void _sel_init(SEL min, SEL max, void *frozenHashTable)
{
	int i;

	_sel_min = min;
	_sel_max = max;
	freezeDriedHashList = frozenHashTable;

	nHashLists = SIZEHASHTABLE;

	hashList = (PHASH *)malloc(nHashLists * sizeof(PHASH));
	for (i = 0; i < nHashLists; i++) 
		hashList[i] = 0;

	if (freezeDriedHashList)
		opthashList = freezeDriedHashList;
	else {
		opthashList = (POHASH *)malloc(nHashLists * sizeof(POHASH));
		for (i = 0; i < nHashLists; i++) 
			opthashList[i] = 0;
	}

	if (!hashList || !opthashList)
		perror("unable to allocate space");
}

