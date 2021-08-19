
/*
	afmStrings.c
	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
  	
	This file provides functions to support parsing font metrics files.
	We provide a stack allocator and a string table package.
*/

#import <mach.h>
#import <objc/hashtable.h>
#import <objc/error.h>
#import <stdio.h>
#import "afmprivate.h"
#import "nextstd.h"

struct _NXStack {
    NXStackType type;
    char freeBuffer;		/* do we free the data buffer? */
    char *data;
    const char *curr;
    int maxLength;
    void *userData;
};

struct _NXStringTable {
    NXStack stack;		/* string data */
    NXHashTable *index;		/* index of data */
};


NXStrTable NXCreateStringTable(NXStackType type, int sizeHint, void *userData)
{
    NXStrTable new;

    new = malloc(sizeof(struct _NXStringTable));
    new->stack = NXCreateStack(type, sizeHint, userData);
    new->index = NXCreateHashTable(NXStrPrototype, 0, NULL);
    return new;
}


NXStrTable NXCreateStringTableFromMemory(char *start, int used, int maxSize, void *userData, NXStackType type, int freeBuffer)
{
    NXStrTable new;

    new = malloc(sizeof(struct _NXStringTable));
    new->stack = NXCreateStackFromMemory(start, used, maxSize, userData, type, freeBuffer);
    new->index = NULL;
    return new;
}


const char *NXStringTableInsert(NXStrTable sTable, const char *str, int *offset)
{
    const char *newPtr = NULL;
    const char *uniqueStr;
    const char *oldData;
    const char *s;
    int len;

    if (str) {
	if (!sTable->index) {
	    sTable->index = NXCreateHashTable(NXStrPrototype, 0, NULL);
	    for (s = sTable->stack->data; s < sTable->stack->curr; s += strlen(s)+1)
		NXHashInsert(sTable->index, s);
	}
	uniqueStr = NXHashGet(sTable->index, str);
	if (!uniqueStr) {
	    len = strlen(str);
	    NX_DURING {
		if (sTable->stack->type == NX_stackGrowContig) {
		    oldData = sTable->stack->data;
		    newPtr = NXStackAllocPtr(sTable->stack, len+1, sizeof(char));
		    /* if data moves, we have to rehash it */
		    if (oldData != sTable->stack->data) {
			NXEmptyHashTable(sTable->index);
			for (s = sTable->stack->data; s < newPtr; s += strlen(s)+1)
			    NXHashInsert(sTable->index, s);
		    }
		} else
		    newPtr = NXStackAllocPtr(sTable->stack, len+1, sizeof(char));
	    } NX_HANDLER {
		if (NXLocalHandler.code == err_stackAllocFailed)
		    NX_RAISE(err_strAllocFailed, sTable, (void *)len);
		else
		    NX_RERAISE();
		return NULL;			/* for clean -Wall */
	    } NX_ENDHANDLER
	    if (newPtr) {
		bcopy(str, (char *)newPtr, len+1);
		NXHashInsert(sTable->index, newPtr);
	    }
	} else
	    newPtr = uniqueStr;
    }
    if (offset)
	*offset = newPtr ? newPtr - sTable->stack->data : -1;
    return newPtr;
}


const char *NXStringTableInsertWithLength(NXStrTable sTable, const char *str, int length, int *offset)
{
  /* ??? CHEAP IMPLEMENTATION */
    char *temp;
    const char *newStr = NULL;		/* for clean -Wall */

    temp = malloc(length+1);
    bcopy(str, temp, length);
    temp[length] = '\0';
    NX_DURING {
	newStr = NXStringTableInsert(sTable, temp, offset);
    } NX_HANDLER {
	free(temp);
	NX_RERAISE();
	return NULL;			/* for clean -Wall */
    } NX_ENDHANDLER
    free(temp);
    return newStr;
}


void NXResetStringTable(NXStrTable sTable, const char *oldData, int oldOffset)
{
    const char *s;

    if (!oldData)
	oldData = sTable->stack->data + oldOffset;
    NX_ASSERT(oldData >= sTable->stack->data && oldData <= sTable->stack->curr, "NXResetStringTable: invalid state to reset to");
    if (sTable->index)
	for (s = oldData; s < sTable->stack->curr; s += strlen(s)+1)
	    NXHashRemove(sTable->index, s);
    NXResetStack(sTable->stack, oldData, oldOffset);
}


void NXGetStringTableInfo(NXStrTable sTable, const char **start, int *used, int *maxSize, void **userData, NXStackType *type)
{
    NXGetStackInfo(sTable->stack, (const void **)start, used, maxSize, userData, type);
}


int NXStringTableOffsetFromPtr(NXStrTable sTable, const void *ptr)
{
    return (char *)ptr - sTable->stack->data;
}


void *NXStringTablePtrFromOffset(NXStrTable sTable, int offset)
{
    return sTable->stack->data + offset;
}


void NXDestroyStringTable(NXStrTable sTable)
{
    if (sTable->index)
	NXFreeHashTable(sTable->index);
    NXDestroyStack(sTable->stack);
    free(sTable);
}


NXStack NXCreateStack(NXStackType type, int sizeHint, void *userData)
{
    void *newData = NULL;

    if (!sizeHint)
	sizeHint = 256;
    switch (type) {
	case NX_stackGrowContig:	
	    newData = malloc(sizeHint);
	    break;
	case NX_stackFixedContig:	
	    newData = valloc(sizeHint);
	    break;
	case NX_stackGrowDiscontig:
	default:
	    NXLogError("NXCreateStack: bad stack type %d", type);
	    break;
    }
    return NXCreateStackFromMemory(newData, 0, sizeHint, userData, type, TRUE);
}


NXStack NXCreateStackFromMemory(void *start, int used, int maxSize, void *userData, NXStackType type, int freeBuffer)
{
    NXStack new;

    new = calloc(1, sizeof(struct _NXStack));
    new->type = type;
    new->freeBuffer = freeBuffer;
    switch (type) {
	case NX_stackGrowContig:	
	case NX_stackFixedContig:	
	    new->data = start;
	    break;
	case NX_stackGrowDiscontig:
	default:
	    NXLogError("NXCreateStack: bad stack type %d", type);
	    break;
    }
    new->curr = new->data + used;
    new->maxLength = maxSize;
    new->userData = userData;
    return new;
}

void *NXStackAllocPtr(NXStack stack, int size, int alignment)
{
    return stack->data + NXStackAllocOffset(stack, size, alignment);
}


int NXStackAllocOffset(NXStack stack, int size, int alignment)
{
    int startOffset;
    char *newStackData;
    int newMaxLength;
    
    startOffset = (stack->curr - stack->data + alignment - 1);
    startOffset -= startOffset % alignment;
    if (startOffset + size > stack->maxLength) {
	if (stack->type == NX_stackGrowContig) {
	    newMaxLength = stack->maxLength;
	    while (newMaxLength < stack->curr - stack->data + size)
		newMaxLength *= 2;
	    if (stack->freeBuffer)
		newStackData = realloc(stack->data, newMaxLength);
	    else {
		newStackData = malloc(newMaxLength);
		bcopy(stack->data, newStackData, stack->curr - stack->data);
		stack->freeBuffer = TRUE;
	    }
	    stack->maxLength = newMaxLength;
	    stack->curr = newStackData + (stack->curr - stack->data);
	    stack->data = newStackData;
	} else
	    NX_RAISE(err_stackAllocFailed, stack, (void *)size);
    }
    stack->curr = stack->data + startOffset + size;
    return startOffset;
}


void NXResetStack(NXStack stack, const void *oldData, int oldOffset)
{
    if (!oldData)
	oldData = stack->data + oldOffset;
    NX_ASSERT((char *)oldData >= stack->data && (char *)oldData <= stack->curr, "NXResetStack: invalid state to reset to");
    stack->curr = oldData;
}


void NXGetStackInfo(NXStack stack, const void **start, int *used, int *maxSize, void **userData, NXStackType *type)
{
    if (start)
	*start = stack->data;
    if (used)
	*used = stack->curr - stack->data;
    if (maxSize)
	*maxSize = stack->maxLength;
    if (userData)
	*userData = stack->userData;
    if (type)
	*type = stack->type;
}


int NXStackOffsetFromPtr(NXStack stack, const void *ptr)
{
    return (char *)ptr - stack->data;
}


void *NXStackPtrFromOffset(NXStack stack, int offset)
{
    return stack->data + offset;
}


void NXDestroyStack(NXStack stack)
{
    if (stack->freeBuffer)
	free(stack->data);
    free(stack);
}


/*

Modifications (starting at 0.8):

80
--
 4/09/90 trey	support for getting kerns and other AFM data

89
--
 7/29/90 trey	added NXCreateStackFromMemory, NXCreateStringTableFromMemory

*/

