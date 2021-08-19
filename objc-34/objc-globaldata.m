#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "objc-class.h"
#import "objc-private.h"
#import <zone.h>

/*
 * Declarations of non-const global data.
 */
extern id _class_createInstance(Class, unsigned);
extern id _class_createInstanceFromZone(Class, unsigned, NXZone *);
extern id _object_dispose(id);
extern id _object_realloc(id, unsigned);
extern id _object_reallocFromZone(id, unsigned, NXZone *);
extern id _object_copy(id, unsigned);
extern id _object_copyFromZone(id, unsigned, NXZone *);

id (*_poseAs)() = (id (*)())class_poseAs;
id (*_alloc)(Class, unsigned) = _class_createInstance;
id (*_copy)(id, unsigned) = _object_copy;
id (*_realloc)(id, unsigned) = _object_realloc;
id (*_dealloc)(id)  = _object_dispose;

extern id objc_getClassWithoutWarning(STR);
extern SEL sel_getUid(STR);
extern void _objc_error();

id (*_cvtToId)(STR)= objc_getClassWithoutWarning;
SEL (*_cvtToSel)(STR)= sel_getUid;
void (*_error)() = _objc_error;

id (*_zoneAlloc)(Class, unsigned, NXZone *) = _class_createInstanceFromZone;
id (*_zoneCopy)(id, unsigned, NXZone *) = _object_copyFromZone;
id (*_zoneRealloc)(id, unsigned, NXZone *) = _object_reallocFromZone;

char _objc_global_data_pad[468] = {0};
