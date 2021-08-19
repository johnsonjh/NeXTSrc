/* Required for compatiblity with 1.0 to turn off .const and .cstring */
#pragma CC_NO_MACH_TEXT_SECTIONS

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "hashtable.h"

/*
 * Global const data would go here and would look like:
 * 	const int foo = 1;
 */	
/*
 * hashtable globals
 */

extern unsigned hashPtrStructKey (const void *info, const void *data);
extern int isEqualPtrStructKey (const void *info, const void *data1, const void *data2);
extern unsigned hashStrStructKey (const void *info, const void *data);
extern int isEqualStrStructKey (const void *info, const void *data1, const void *data2);

const NXHashTablePrototype NXPtrPrototype = {
    NXPtrHash, NXPtrIsEqual, NXNoEffectFree, 0
    };
const NXHashTablePrototype NXStrPrototype = {
    NXStrHash, NXStrIsEqual, NXNoEffectFree, 0
    };


const NXHashTablePrototype NXPtrStructKeyPrototype = {
    hashPtrStructKey, isEqualPtrStructKey, NXReallyFree, 0
    };

const NXHashTablePrototype NXStrStructKeyPrototype = {
    hashStrStructKey, isEqualStrStructKey, NXReallyFree, 0
    };
static const char _objc_global_text_pad[192] = {0};

/*
 * Declarations of static (literal) const data.
 */
static const char _objc_literal_text_pad[256] = {0};
