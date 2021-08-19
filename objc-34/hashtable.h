/*
    hashtable.h
    Scalable hash table.
    Copyright 1989 NeXT, Inc.
*/

#import "objc.h"
#import <zone.h>

/*************************************************************************
 *	Hash tables of arbitrary data
 *************************************************************************/

/* This module allows hashing of arbitrary data.  Such data must be pointers or integers, and client is responsible for allocating/deallocating this data.  A deallocation call-back is provided.
The objective C class HashTable is prefered when dealing with (key, values) associations because it is easier to use in that situation.
As well-behaved scalable data structures, hash tables double in size when they start becoming full, thus guaranteeing both average constant time access and linear size. */

typedef struct {
    unsigned	(*hash)(const void *info, const void *data);
    int		(*isEqual)(const void *info, const void *data1, const void *data2);
    void	(*free)(const void *info, void *data);
    int		style; /* reserved for future expansion; currently 0 */
    } NXHashTablePrototype;
    
/* the info argument allows a certain generality, such as freeing according to some owner information */
/* invariants assumed by the implementation: 
	1 - data1 = data2 => hash(data1) = hash(data2)
	    when data varies over time, hash(data) must remain invariant
		    e.g. if data hashes over a string key, the string must not be changed
	2- isEqual (data1, data2) => data1= data2
 */

typedef struct {
    const NXHashTablePrototype	*prototype;
    unsigned			count;
    unsigned			nbBuckets;
    void			*buckets;
    const void			*info;
   } NXHashTable;
    /* private data structure; may change */
    
extern NXHashTable *NXCreateHashTableFromZone (NXHashTablePrototype prototype, unsigned capacity, const void *info, NXZone *zone);
extern NXHashTable *NXCreateHashTable (NXHashTablePrototype prototype, unsigned capacity, const void *info);
    /* if hash is 0, pointer hash is assumed */
    /* if isEqual is 0, pointer equality is assumed */
    /* if free is 0, free is called */
    /* capacity is only a hint; 0 creates a small table */
    /* info allows call backs to be very general */

extern void NXFreeHashTable (NXHashTable *table);
    /* calls free for each data, and recovers table */
	
extern void NXEmptyHashTable (NXHashTable *table);
    /* does not deallocate table nor data; keeps current capacity */

extern void NXResetHashTable (NXHashTable *table);
    /* frees each entry; keeps current capacity */

extern BOOL NXCompareHashTables (NXHashTable *table1, NXHashTable *table2);
    /* Returns YES if the two sets are equal (each member of table1 in table2, and table have same size) */

extern NXHashTable *NXCopyHashTable (NXHashTable *table);
    /* makes a fresh table, copying data pointers, not data itself.  */
	
extern unsigned NXCountHashTable (NXHashTable *table);
    /* current number of data in table */
	
extern int NXHashMember (NXHashTable *table, const void *data);
    /* returns non-0 iff data is present in table.
    Example of use when the hashed data is a struct containing the key,
    and when the callee only has a key:
	MyStruct	pseudo;
	pseudo.key = myKey;
	return NXHashMember (myTable, &pseudo)
    */
	
extern void *NXHashGet (NXHashTable *table, const void *data);
    /* return original table data or NULL.
    Example of use when the hashed data is a struct containing the key,
    and when the callee only has a key:
	MyStruct	pseudo;
	MyStruct	*original;
	pseudo.key = myKey;
	original = NXHashGet (myTable, &pseudo)
    */
	
extern void *NXHashInsert (NXHashTable *table, const void *data);
    /* previous data or NULL is returned. */
	
extern void *NXHashInsertIfAbsent (NXHashTable *table, const void *data);
    /* If data already in table, returns the one in table
    else adds argument to table. */
	
extern void *NXHashRemove (NXHashTable *table, const void *data);
    /* previous data or NULL is returned */
	
/* Iteration over all elements of a table consists in setting up an iteration state and then to progress until all entries have been visited.  An example of use for counting elements in a table is:
    unsigned	count = 0;
    MyData	*data;
    NXHashState	state = NXInitHashState(table);
    while (NXNextHashState(table, &state, &data)) {
	count++;
    }
*/

typedef struct {int i; int j;} NXHashState;
    /* callers should not rely on actual contents of the struct */

extern NXHashState NXInitHashState(NXHashTable *table);

extern int NXNextHashState(NXHashTable *table, NXHashState *state, void **data);
    /* returns 0 when all elements have been visited */

/*************************************************************************
 *	Conveniences for writing hash, isEqual and free functions
 *	and common prototypes
 *************************************************************************/

extern unsigned NXPtrHash(const void *info, const void *data);
    /* scrambles the address bits; info unused */
extern unsigned NXStrHash(const void *info, const void *data);
    /* string hashing; info unused */
extern int NXPtrIsEqual(const void *info, const void *data1, const void *data2);
    /* pointer comparison; info unused */
extern int NXStrIsEqual(const void *info, const void *data1, const void *data2);
    /* string comparison; NULL ok; info unused */
extern void NXNoEffectFree(const void *info, void *data);
    /* no effect; info unused */
extern void NXReallyFree(const void *info, void *data);
    /* frees it; info unused */

/* The two following prototypes are useful for manipulating set of pointers or set of strings; For them free is defined as NXNoEffectFree */
extern const NXHashTablePrototype NXPtrPrototype;
    /* prototype when data is a pointer (void *) */
extern const NXHashTablePrototype NXStrPrototype;
    /* prototype when data is a string (char *) */

/* following prototypes help describe mappings where the key is the first element of a struct and is either a pointer or a string.
For example NXStrStructKeyPrototype can be used to hash pointers to Example, where Example is:
	typedef struct {
	    char	*key;
	    int		data1;
	    ...
	    } Example
    
For the following prototypes, free is defined as NXReallyFree.
 */
extern const NXHashTablePrototype NXPtrStructKeyPrototype;
extern const NXHashTablePrototype NXStrStructKeyPrototype;

/*************************************************************************
 *	Unique strings and buffers
 *************************************************************************/

/* Unique strings allows C users to enjoy the benefits of Lisp's atoms:
A unique string is a string that is allocated once for all (never de-allocated) and that has only one representant (thus allowing comparison with == instead of strcmp).  A unique string should never be modified (and in fact some memory protection is done to ensure that).  In order to more explicitly insist on the fact that the string has been uniqued, a synonym of (const char *) has been added, NXAtom. */

typedef const char *NXAtom;

extern NXAtom NXUniqueString(const char *buffer);
    /* assumes that buffer is \0 terminated, and returns
     a previously created string or a new string that is a copy of buffer.
    If NULL is passed returns NULL.
    Returned string should never be modified.  To ensure this invariant,
    allocations are made in a special read only zone. */
	
extern NXAtom NXUniqueStringWithLength(const char *buffer, int length);
    /* assumes that buffer is a non NULL buffer of at least 
    length characters.  Returns a previously created string or 
    a new string that is a copy of buffer. 
    If buffer contains \0, string will be truncated.
    As for NXUniqueString, returned string should never be modified.  */
	
extern NXAtom NXUniqueStringNoCopy(const char *string);
    /* If there is already a unique string equal to string, returns the original.  
    Otherwise, string is entered in the table, without making a copy.  Argument should then never be modified.  */
	
extern char *NXCopyStringBuffer(const char *buffer);
    /* given a buffer, allocates a new string copy of buffer.  
    Buffer should be \0 terminated; returned string is \0 terminated. */

extern char *NXCopyStringBufferFromZone(const char *buffer, NXZone *zone);
    /* given a buffer, allocates a new string copy of buffer.  
    Buffer should be \0 terminated; returned string is \0 terminated. */


