/*
 *	objc-runtime.m
 *	Copyright 1988, NeXT, Inc.
 *	Author:	s. naroff
 *
 */
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#include <ldsyms.h>

void *getsectdatafromheader(struct mach_header *, char *, char *, int *);
const struct section *getsectbynamefromheader(struct mach_header *, 
						char *, char *);
#import "objc-private.h"

#import "vectors.h"	
#import "objc-runtime.h"
#import "hashtable.h"
#import "Object.h"

/* system vectors...created at runtime by reading the `__OBJC' segments 
 * that are a part of the application.
 */
static int module_count;
static Module *module_vector;	

struct header_info {
	struct mach_header *mhdr;
	Module	mod_ptr;
	int	mod_count;
};

static struct header_info *header_vector = 0;
static int header_count = 0;

/*
 * 	Hash table of classes...
 */
static NXHashTable *class_hash = 0;

static unsigned classHash (void *info, Class data) 
{
	return (data) ? _strhash ((unsigned char *)((Class) data)->name) : 0;
};
    
static int classIsEqual (void *info, Class name, Class cls) 
{
	return ((*(short *)name->name == *(short *)cls->name) &&
	 	(strcmp(name->name, cls->name) == 0));
};

static NXHashTablePrototype classHashPrototype = 
{
	(unsigned (*)(const void *, const void *))classHash, 
	(int (*)(const void *, const void *, const void *))classIsEqual, 
	NXNoEffectFree, 0
};

NXHashTable *objc_getClasses()
{
	return class_hash;
}

static void _objc_internal_error(char *fmt, ...) 
{ 
	va_list vp; 

	va_start(vp,fmt); 
	fflush(stderr);
	fprintf(stderr, "objc: ");
	vfprintf(stderr, fmt, vp); 
	fprintf(stderr, "\n");
}

static int _objc_defaultClassHandler(char *clsName)
{
	_objc_internal_error("class `%s' not linked into application\n", 
		clsName);
	return 0;
}

static int (*objc_classHandler)(char *) = _objc_defaultClassHandler;

void objc_setClassHandler(int (*userSuppliedHandler)(char *))
{
	objc_classHandler = userSuppliedHandler;
}

/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
id objc_getClass(STR aClassName) 
{ 
	struct objc_class cls;
	id ret;

	cls.name = aClassName;

	if (!(ret = (id)NXHashGet(class_hash, &cls))) {
	   if ((*objc_classHandler)(aClassName))
	   	ret = (id)NXHashGet(class_hash, &cls);
	}
	return ret;
}

/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
id objc_getClassWithoutWarning(STR aClassName) 
{ 
	struct objc_class cls;

	cls.name = aClassName;

	return (id)NXHashGet(class_hash, &cls);
}

/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
id objc_getMetaClass(STR aClassName) 
{ 
	struct objc_class cls;
	id ret;

	cls.name = aClassName;

	if (!(ret = (id)NXHashGet(class_hash, &cls))) {
	   if ((*objc_classHandler)(aClassName))
	   	ret = (id)NXHashGet(class_hash, &cls);
	}
	return ret ? (id)ret->isa : 0;
}

/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
void objc_addClass(Class myClass) 
{
	NXHashInsert(class_hash, myClass);
}

/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
Module *objc_getModules()
{
	if (!module_vector) {
		Module mods;
		int hidx, midx;
		Module *mvp;

		for (hidx = 0; hidx < header_count; hidx++) 
			module_count += header_vector[hidx].mod_count;

		if (!(module_vector = (Module *)malloc(
					sizeof(Module) * (module_count+1))))
			_objc_fatal("unable to allocate module vector");

		for (hidx = 0, mvp = module_vector; 
				hidx < header_count; hidx++) {
			for (mods = header_vector[hidx].mod_ptr, midx = 0;
		     		midx < header_vector[hidx].mod_count;
		     		midx++, mvp++) {
				
				*mvp = mods+midx;
			}
		}
		*mvp = 0;
	}
	return module_vector;
}

/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
Module *objc_addModule(Module myModule) 
{
	module_count++;
	if (module_vector) {
		module_vector = (Module *)realloc(module_vector,
					sizeof(Module) * (module_count + 1));
	} else {
		module_vector = (Module *)malloc(
					sizeof(Module) * (module_count + 1));
	}
	if (!module_vector)
	   _objc_fatal("unable to reallocate module vector");

	module_vector[module_count-1] = myModule;
	module_vector[module_count] = 0;
	return module_vector;
}

/*
 *	Purpose: 	Used by objc_unloadLastModule()
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
void _objc_remove_category(struct objc_category *category)
{
	Class cls;

	if (cls = (Class)objc_getClass(category->class_name)) {
		if (category->instance_methods)
	  	      class_removeMethods(cls, category->instance_methods);

		if (category->class_methods)
		      class_removeMethods(cls->isa, category->class_methods);
	} else {
		_objc_internal_error("unable to remove category %s...\n",
				category->category_name);
		_objc_internal_error("class `%s' not linked into application\n",
				category->class_name);
	}
}

/*
 *	Purpose: 	Used by objc_loadModule()
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
void _objc_add_category(struct objc_category *category)
{
	Class cls;

	if (cls = (Class)objc_getClass(category->class_name)) {
		if (category->instance_methods) 
		  class_addInstanceMethods(cls, category->instance_methods);

		if (category->class_methods) 
		  class_addClassMethods(cls, category->class_methods);
	} else {
		_objc_internal_error("unable to add category %s...\n",
				category->category_name);
		_objc_internal_error("class `%s' not linked into application\n",
				category->class_name);
	}
}

/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
static inline void __objc_add_category(struct objc_category *category)
{
	Class cls;

	if (cls = (Class)objc_getClass(category->class_name)) {
		if (category->instance_methods) {
	  	  category->instance_methods->method_next = cls->methods;
		  cls->methods = category->instance_methods;
		}
		if (category->class_methods) {
		  category->class_methods->method_next = cls->isa->methods;
                  cls->isa->methods = category->class_methods;
		}
	} else {
		_objc_internal_error("unable to add category %s...\n",
				category->category_name);
		_objc_internal_error("class `%s' not linked into application\n",
				category->class_name);
	}
}

/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
static void _objc_add_categories()
{
	Module mods;
	int hidx, midx;

	for (hidx = 0; hidx < header_count; hidx++) {
		for (mods = header_vector[hidx].mod_ptr, midx = 0;
		     midx < header_vector[hidx].mod_count;
		     midx++) {
			int i, total = mods[midx].symtab->cls_def_cnt + 
					mods[midx].symtab->cat_def_cnt; 

			/* add categories */
			for (i = mods[midx].symtab->cls_def_cnt;  
				i < total; i++)
		  		__objc_add_category(mods[midx].symtab->defs[i]);
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
static NXHashTable *_objc_get_classes()
{
	int i, class_count = 0;
	NXHashTable *clsHash;
	Module mods;
	int hidx, midx;

	for (hidx = 0; hidx < header_count; hidx++) {
		for (mods = header_vector[hidx].mod_ptr, midx = 0;
		     midx < header_vector[hidx].mod_count;
		     midx++) {
			if (mods[midx].symtab->cls_def_cnt)
				class_count += mods[midx].symtab->cls_def_cnt;
		}
	}
    	clsHash = NXCreateHashTable(classHashPrototype, 2*class_count, nil);

	for (hidx = 0; hidx < header_count; hidx++) {
		for (mods = header_vector[hidx].mod_ptr, midx = 0;
		     midx < header_vector[hidx].mod_count;
		     midx++) {
			for (i = 0; i < mods[midx].symtab->cls_def_cnt; i++)
				NXHashInsert(clsHash, 
					mods[midx].symtab->defs[i]);
		}
	}

	return clsHash;
}


static inline void map_selrefs(SEL *sels, int cnt)
{ 
	register int i;

	/* overwrite the string with a unique identifier */
	for (i = 0; i < cnt; i++) {
		sels[i] = _sel_registerName((STR)sels[i]);
	}
}

static inline void map_methods(struct objc_method_list *methods)
{
	int i;
	Method method;

	for (i = 0; i < methods->method_count; i++) {
		method = &methods->method_list[i];
		method->method_name = _sel_registerName((STR)method->method_name);
	}		  
}

/* the following are optimized to NOT write on the Objective-C segment. */

static inline void map_selrefsUseStrings(SEL *sels, int cnt)
{ 
	register int i;

	/* overwrite the string with a unique identifier */
	for (i = 0; i < cnt; i++) {
		_sel_registerNameUseString((STR)sels[i]);
	}
}

static inline void map_methodsUseStrings(struct objc_method_list *methods)
{
	if (methods) {
		int j;
		Method method;

		for (j = 0; j < methods->method_count; j++) {
			method = &methods->method_list[j];
			_sel_registerNameUseString((STR)method->method_name);
		}
	}
}


static const struct segment_command *
getobjcsegmentfromheader(struct mach_header *mhp)
{
	struct segment_command *sgp;
	long i;

	sgp = (struct segment_command *)
	      ((char *)mhp + sizeof(struct mach_header));
	for(i = 0; i < mhp->ncmds; i++){
	    if(sgp->cmd == LC_SEGMENT)
		if(strncmp(sgp->segname, "__OBJC", sizeof(sgp->segname)) == 0)
			return sgp;
	    sgp = (struct segment_command *)((char *)sgp + sgp->cmdsize);
	}
	return((struct segment_command *)0);
}

/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
static struct header_info *_objc_get_modules(struct mach_header **machhdrs)
{
	struct mach_header **mhp, *maxHeader = NULL;
	int size, maxSize = 0, nhdrs = 0, hidx, sidx;
	Module mod;
	struct header_info *hdrVec;

	for (mhp = machhdrs; *mhp; mhp++) {
		const struct segment_command *objcseg;

		objcseg = getobjcsegmentfromheader(*mhp);

		/* find image with the largest Objective-C segment...
		 * this is typically `libNeXT'.
		 */
		if (objcseg && (maxSize < objcseg->filesize)) {
			maxSize = objcseg->filesize;
			maxHeader = *mhp;
		}
		header_count++;
	}
	if (!(hdrVec = (struct header_info *)malloc(
				sizeof(struct header_info) * header_count)))
	   _objc_fatal("unable to allocate module vector");

	/* fill in hdrVec - place max header at the front of list */

	hdrVec[0].mhdr = maxHeader;

	mod = (Module)getsectdatafromheader(maxHeader,
				SEG_OBJC, SECT_OBJC_MODULES, &size);

	hdrVec[0].mod_ptr = mod;
	hdrVec[0].mod_count = size / sizeof(struct objc_module);

	for (hidx = 0, sidx = 1, mhp = machhdrs; 
	     hidx < header_count; 
	     hidx++, mhp++) {
		if (*mhp != maxHeader) {
			hdrVec[sidx].mhdr = *mhp;

			mod = (Module)getsectdatafromheader(*mhp,
					SEG_OBJC, SECT_OBJC_MODULES, &size);

			hdrVec[sidx].mod_ptr = mod;
			hdrVec[sidx].mod_count = 
					size / sizeof(struct objc_module);
			sidx++;
		}
	}

	return hdrVec;
}

/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
void __objc_error(id rcv, char *fmt, ...) 
{ 
	va_list vp; 

	va_start(vp,fmt); 
	(*_error)(rcv, fmt, vp); 
}

/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
void _objc_error(id self, char *fmt, va_list ap) 
{ 
	fflush(stderr);
	fprintf(stderr, "error: %s ", object_getClassName(self));
	vfprintf(stderr, fmt, ap); 
	fprintf(stderr, "\n");
	abort();		/* generates a core file */
}

/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
void _objc_fatal(char *msg)
{
	fprintf(stderr,"objc: %s\n", msg);
	exit(1);
}

/*
 *	Purpose: 
 *	Options:
 *	Assumptions:
 *	Side Effects: 
 *	Returns: 
 */
static void _objc_map_selectors()
{
	Module mods;
	int hidx, midx;
        int opthashtable_size;
	void *opthashtable;
	const struct section *strsect, *selsect, *msgsect;

	/* perform some optimizations and initialize the selector hash... */

	strsect = getsectbynamefromheader(header_vector[0].mhdr,
			SEG_OBJC, SECT_OBJC_STRINGS);

	opthashtable = (Module)getsectdatafromheader(header_vector[0].mhdr,
			SEG_OBJC, "__runtime_setup", &opthashtable_size);

	_sel_init(strsect->addr, strsect->addr + strsect->size, opthashtable);

	if (!opthashtable) {
	
		/* this should go away before we release v2.0 */
		selsect = getsectbynamefromheader(header_vector[0].mhdr,
				SEG_OBJC, "__selector_refs");
		if (selsect) {
			int size;
			SEL *sels = (SEL *)getsectdatafromheader(
					header_vector[0].mhdr, 
					SEG_OBJC, "__selector_refs", &size);
			map_selrefsUseStrings(sels, size/sizeof(SEL));
		}
		msgsect = getsectbynamefromheader(header_vector[0].mhdr,
				SEG_OBJC, "__message_refs");
		if (msgsect) {
			int size;
			SEL *sels = (SEL *)getsectdatafromheader(
					header_vector[0].mhdr, 
					SEG_OBJC, "__message_refs", &size);
			map_selrefsUseStrings(sels, size/sizeof(SEL));
		}
		for (mods = header_vector[0].mod_ptr, midx = 0;
		     midx < header_vector[0].mod_count;
		     midx++) {
			int i;

			/* handle v1.0 registration of selector references */
			if (mods[midx].version == 1) {
				map_selrefsUseStrings(mods[midx].symtab->refs,
				    mods[midx].symtab->sel_ref_cnt);
			}
			/* register class defs */
			for (i = 0; i < mods[midx].symtab->cls_def_cnt; i++) {
			        Class cls = mods[midx].symtab->defs[i];

				if (cls->methods)
				  map_methodsUseStrings(cls->methods);
				if (cls->isa->methods)
				  map_methodsUseStrings(cls->isa->methods);
			}

			/* register category defs */
			for (i = mods[midx].symtab->cls_def_cnt;  
				i < (mods[midx].symtab->cls_def_cnt + 
				     mods[midx].symtab->cat_def_cnt); i++) {
				Category cat = mods[midx].symtab->defs[i];

				if (cat->instance_methods)
				  map_methodsUseStrings(cat->instance_methods);
				if (cat->class_methods)
				  map_methodsUseStrings(cat->class_methods);
			}
		}
	}
	/* skip header 0...it is the largest and has already been processed */
	for (hidx = 1; hidx < header_count; hidx++) {

		/* this should go away before we release v2.0 */
		selsect = getsectbynamefromheader(header_vector[hidx].mhdr,
				SEG_OBJC, "__selector_refs");
		if (selsect) {
			int size;
			SEL *sels = (SEL *)getsectdatafromheader(
					header_vector[hidx].mhdr, 
					SEG_OBJC, "__selector_refs", &size);
			map_selrefs(sels, size/sizeof(SEL));
		}
		msgsect = getsectbynamefromheader(header_vector[hidx].mhdr,
				SEG_OBJC, "__message_refs");
		if (msgsect) {
			int size;
			SEL *sels = (SEL *)getsectdatafromheader(
					header_vector[hidx].mhdr, 
					SEG_OBJC, "__message_refs", &size);
			map_selrefs(sels, size/sizeof(SEL));
		}

		for (mods = header_vector[hidx].mod_ptr, midx = 0;
		     midx < header_vector[hidx].mod_count;
		     midx++) {
			int i;
			
			/* handle v1.0 mapping of selector references */
			if (mods[midx].version == 1) {
				map_selrefs(mods[midx].symtab->refs,
					    mods[midx].symtab->sel_ref_cnt);
			}

			/* map class defs */
			for (i = 0; i < mods[midx].symtab->cls_def_cnt; i++) {
			        Class cls = mods[midx].symtab->defs[i];

				if (cls->methods)
				  map_methods(cls->methods);
				if (cls->isa->methods)
				  map_methods(cls->isa->methods);

				_class_install_relationships(
					cls, 
					mods[midx].version);
			}

			/* map category defs */
			for (i = mods[midx].symtab->cls_def_cnt;  
				i < (mods[midx].symtab->cls_def_cnt + 
				     mods[midx].symtab->cat_def_cnt); i++) {
				Category cat = mods[midx].symtab->defs[i];

				if (cat->instance_methods)
				  map_methods(cat->instance_methods);
				if (cat->class_methods)
				  map_methods(cat->class_methods);
			}
		}
	}
	/* go back and patch classes in optimized shlib */

	for (mods = header_vector[0].mod_ptr, midx = 0;
	     midx < header_vector[0].mod_count;
	     midx++) {
		int i;

		for (i = 0; i < mods[midx].symtab->cls_def_cnt; i++) 
		  _class_install_relationships(mods[midx].symtab->defs[i],
		                               mods[midx].version);
	}
}

/*
 *	Purpose: The idea here is to call objc_msgSend() with the 
 *		 arguments in args. The args must already be in the 
 *		 appropriate format for the stack (perhaps copied 
 *		 form some other stack frame).
 *
 *  		 The final call to objc_msgSend () is thus equivalent to
 *		 objc_msgSend (self, op, arg[0], arg[1], ..., arg[size - 1]).
 *	Options:
 *	Assumptions: Do not compile this with -fomit-frame-pointer!
 *	Side Effects: 
 *	Returns: 
 */
id objc_msgSendv (id self, SEL sel, unsigned size, marg_list args) 
{
	unsigned int i;
  
	size -= (sizeof(id) + sizeof(SEL));
	args += (sizeof(id) + sizeof(SEL));

	for (i = (size + 1) / sizeof (short); i > 0; i--)
		asm volatile ("movew %0,sp@-" : : "a" (((short *) args)[i-1]));

	return objc_msgSend (self, sel);
}

static void _objc_callLoads ()
{
  extern IMP class_lookupMethodInMethodList(struct objc_method_list *mlist,
                                            SEL sel);
  struct header_info *header;   
  Class class, *pClass;
  IMP load_method;
  struct objc_method_list *methods;
  int nHeaders, nModules, nClasses;
  struct objc_module *module;
  struct objc_symtab *symtab;
  
  for (nHeaders = header_count, header = header_vector;
       nHeaders;
       nHeaders--, header++) {
    for (nModules = header->mod_count, module = header->mod_ptr;
         nModules;
	 nModules--, module++) {
      symtab = module->symtab;
      for (nClasses = symtab->cls_def_cnt, pClass = (Class *)symtab->defs;
	   nClasses;
	   nClasses--, pClass++) {
	class = *pClass;
        for (methods = class->isa->methods; 
	     methods;
	     methods = methods->method_next) {
	  load_method = class_lookupMethodInMethodList(methods,
						       @selector(load));
	  if (load_method)
	    (*load_method)((id)class, @selector(load));
	}
      }
    }
  }
}

#ifdef OBJC_TIME

static int _objc_time = 1;
/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
void _objcInit(debug)
	int debug;
{
	struct timeval before, after;
	int start_time = 0, finish_time = 0;
	extern struct mach_header **getmachheaders();
	struct mach_header **machheaders;

        {
	char str[256];

	mstats();
	sprintf(str,"/bin/pmem %d",getpid());
	system(str);
	sprintf(str,"/bin/residentwatch -p %d ",getpid());
	system(str);
        }
	if (_objc_time) {
		start_time = gettime();
		gettimeofday(&before, 0);
	}

	_objc_create_zone();
	if (machheaders = getmachheaders()) {
	  if (header_vector = _objc_get_modules(machheaders))
 	    class_hash = _objc_get_classes();
	  free(machheaders);
	} 
	else 
	  _objc_fatal("cannot read mach headers");

	if (_objc_time) {
		finish_time = gettime();
		gettimeofday(&after, 0);
	
		after.tv_sec -= before.tv_sec;
		after.tv_usec -= before.tv_usec;
		if (after.tv_usec < 0)
			after.tv_sec--, after.tv_usec += 1000000;
		print_time("get modules/classes", finish_time-start_time, &after);
		start_time = finish_time;
		gettimeofday(&before, 0);
	}
	_objc_map_selectors();
 	_objc_add_categories();
	_objc_callLoads(); 

	if (_objc_time) {
		extern int seldef_total;

		finish_time = gettime();
		gettimeofday(&after, 0);
		after.tv_sec -= before.tv_sec;
		after.tv_usec -= before.tv_usec;
		if (after.tv_usec < 0)
			after.tv_sec--, after.tv_usec += 1000000;
		print_time("map sel refs/defs", finish_time-start_time, &after);
	}
        {
	char str[256];

	mstats();
	sprintf(str,"/bin/pmem %d ",getpid());
	system(str);
	sprintf(str,"/bin/residentwatch -p %d ",getpid());
	system(str);
        }
}
#else
/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
void _objcInit(debug)
	int debug;
{
	extern struct mach_header **getmachheaders();
	struct mach_header **machheaders;

	_objc_create_zone();
	if (machheaders = getmachheaders()) {
	  if (header_vector = _objc_get_modules(machheaders))
 	    class_hash = _objc_get_classes();
	  free(machheaders);
	} 
	else 
	  _objc_fatal("cannot read mach headers");

	_objc_map_selectors();
 	_objc_add_categories();
	_objc_callLoads(); 
}
#endif
