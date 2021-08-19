/*
 *	objc-load.m
 *	Copyright 1988, NeXT, Inc.
 *	Author:	s. naroff
 *
 */
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#include <ldsyms.h>

#import "objc-load.h"	
#import "objc-private.h"

#import "vectors.h"	
#import "objc-runtime.h"
#import "hashtable.h"
#import "Object.h"

extern void *getsectdatafromheader(struct mach_header *, char *, char *, int *);
extern const struct section *getsectbynamefromheader(struct mach_header *, char *, char *);

extern long rld_load(NXStream *, struct mach_header **, char **, char *);
extern long rld_unload(NXStream *);

extern struct mach_header *rld_get_current_header(void);

static inline void map_defs(struct objc_method_list *methods)
{
	int i;
	SEL newSel;
	Method method;

	for (i = 0; i < methods->method_count; i++) {
		method = &methods->method_list[i];
		method->method_name = _sel_registerName((STR)method->method_name);
	}		  
}

static inline void map_selrefs(SEL *sels, int cnt)
{ 
	register int i;

	/* overwrite the string with a unique identifier */
	for (i = 0; i < cnt; i++)
		sels[i] = _sel_registerName((STR)sels[i]);
}

static struct objc_method_list *get_base_method_list(Class cls)
{
	struct objc_method_list *mlist, *prev = 0;

	for (mlist = cls->methods; mlist; mlist = mlist->method_next)
		prev = mlist;

	return prev;
}

static void send_load_message_to_class(Class cls, void *header_addr)
{
	struct objc_method_list *mlist = get_base_method_list(cls->isa);
	IMP load_method;

	if (mlist) {
		load_method = 
		   class_lookupMethodInMethodList(mlist, 
				@selector(finishLoading:));

		/* go directly there, we do not want to accidentally send
	           the finishLoading: message to one of its categories...
	 	*/
		if (load_method)
			(*load_method)((id)cls, @selector(finishLoading:), 
				header_addr);
	}
}

static void send_load_message_to_category(Category cat, void *header_addr)
{
	struct objc_method_list *mlist = cat->class_methods;
	IMP load_method;
	Class cls;

	if (mlist) {
		load_method = 
		   class_lookupMethodInMethodList(mlist, 
				@selector(finishLoading:));

		cls = (Class)objc_getClass(cat->class_name);

		/* go directly there, we do not want to accidentally send
	           the finishLoading: message to one of its categories...
	 	*/
		if (load_method)
			(*load_method)((id)cls, @selector(finishLoading:), 
				header_addr);
	}
}

static void send_unload_message_to_class(Class cls)
{
	struct objc_method_list *mlist = get_base_method_list(cls->isa);
	IMP load_method;

	if (mlist) {
		load_method = 
		   class_lookupMethodInMethodList(mlist, 
				@selector(startUnloading));

		/* go directly there, we do not want to accidentally send
	           the finishLoading: message to one of its categories...
	 	*/
		if (load_method)
			(*load_method)((id)cls, 
				@selector(startUnloading));
	}
}

static void send_unload_message_to_category(Category cat)
{
	struct objc_method_list *mlist = cat->class_methods;
	IMP load_method;
	Class cls;

	if (mlist) {
		load_method = 
		   class_lookupMethodInMethodList(mlist, 
				@selector(startUnloading));

		cls = (Class)objc_getClass(cat->class_name);

		/* go directly there, we do not want to accidentally send
	           the finishLoading: message to one of its categories...
	 	*/
		if (load_method)
			(*load_method)((id)cls, 
				@selector(startUnloading));
	}
}

/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
long objc_loadModules(
	char *modlist[], 
	NXStream *errStream,
	void (*class_callback)(Class, Category),
	struct mach_header **hdr_addr,
	char *debug_file
)
{
	struct mach_header *header_addr;
	const struct section *selsect, *msgsect;
	int errflag = 0;

	if (rld_load(errStream, &header_addr, modlist, debug_file)) {
          Module mod, modhdr;
          int size, saveSize, i;
 
	  mod = modhdr = (Module)getsectdatafromheader(header_addr,
			"__OBJC", "__module_info", &size);

	  /* this should go away before we release v2.0 */
	  selsect = getsectbynamefromheader(header_addr,
				SEG_OBJC, "__selector_refs");
	  if (selsect) {
		int size;
		SEL *sels = (SEL *)getsectdatafromheader(header_addr,
				SEG_OBJC, "__selector_refs", &size);
		map_selrefs(sels, size/sizeof(SEL));
	  }
	  msgsect = getsectbynamefromheader(header_addr,
				SEG_OBJC, "__message_refs");
	  if (msgsect) {
		int size;
		SEL *sels = (SEL *)getsectdatafromheader(header_addr,
				SEG_OBJC, "__message_refs", &size);
		map_selrefs(sels, size/sizeof(SEL));
	  }

	  saveSize = size;

	  if (mod && size) {
		do {
		  /* process class definitions */
		  for (i = 0; i < mod->symtab->cls_def_cnt; i++) {
	            Class cls = mod->symtab->defs[i];

		    if (cls->methods)
			map_defs(cls->methods);
		    if (cls->isa->methods)
		  	map_defs(cls->isa->methods);

		    _class_install_relationships(cls, mod->version);

		    if (cls = NXHashGet(objc_getClasses(), mod->symtab->defs[i])) {
			NXPrintf(errStream, "objc_loadModule(): "
			"Duplicate definition for class `%s'\n", [(id)cls name]);
			errflag = 1;
		    } else {
			NXHashInsert(objc_getClasses(), mod->symtab->defs[i]);
		    }
		  }

		  /* process category definitions */
		  for ( ; i < mod->symtab->cls_def_cnt + 
				mod->symtab->cat_def_cnt; i++) {
		    Category cat = mod->symtab->defs[i];

		    if (cat->instance_methods)
		     	map_defs(cat->instance_methods);
		    if (cat->class_methods)
		     	map_defs(cat->class_methods);

		    _objc_add_category(mod->symtab->defs[i]);
		  }
		  /* handle v1.0 mapping of selector references */
		  if (mod->version == 1) {
			map_selrefs(mod->symtab->refs,
				    mod->symtab->sel_ref_cnt);
		  }

		  if (size -= mod->size)
		    mod = (Module)((char *)mod + mod->size);
		} while (size);

		if (!errflag) {

		  mod = modhdr;
		  size = saveSize;

		  /*
		   * send load messages and activate call backs...
		   */
		  do {
		    /* process class definitions */
		    for (i = 0; i < mod->symtab->cls_def_cnt; i++) {

		      if (class_callback)
			(*class_callback)(mod->symtab->defs[i], 0);

		      send_load_message_to_class(mod->symtab->defs[i],
				header_addr);
		    }

		    /* process category definitions */
		    for ( ; i < mod->symtab->cls_def_cnt + 
				mod->symtab->cat_def_cnt; i++) {
		      if (class_callback)
			(*class_callback)(
	(Class)objc_getClass(((Category)mod->symtab->defs[i])->class_name),
				(Category)mod->symtab->defs[i]);

		      send_load_message_to_category((Category)mod->symtab->defs[i],
				header_addr);
		    }

		    if (size -= mod->size)
		      mod = (Module)((char *)mod + mod->size);
		  } while (size);
		}
	  }
	  
	  if (errflag) {
		return 1;
	  } else {
		/* return optional output parameter */
		if (hdr_addr)
			*hdr_addr = header_addr;
	  	return 0;
	  }
	} else { /* rld_load() error */
		return 1;
	}
}

/*
 *	Purpose: 	
 *	Options:
 *	Assumptions:	
 *	Side Effects: 
 *	Returns: 
 */
long objc_unloadModules(
	NXStream *errStream,
	void (*unload_callback)(Class, Category)
)
{
	struct mach_header *header_addr;

	if (header_addr = rld_get_current_header()) {

          Module mod, modhdr;
          int size, i, saveSize, strsize;
	  char *strs;
 
	  mod = modhdr = (Module)getsectdatafromheader(header_addr,
			"__OBJC", "__module_info", &size);

	  saveSize = size;

	  if (mod && size) {
		do {

		  /*
		   * send unload messages and activate call backs...
		   */

		  /* process category definitions */
		  for (i = mod->symtab->cls_def_cnt ; 
			i < mod->symtab->cls_def_cnt + 
				mod->symtab->cat_def_cnt; i++) { 

			if (unload_callback)
			(*unload_callback)(
	(Class)objc_getClass(((Category)mod->symtab->defs[i])->class_name),
				(Category)mod->symtab->defs[i]);

		        send_unload_message_to_category(mod->symtab->defs[i]);
		  }

		  /* process class definitions */
		  for (i = 0; i < mod->symtab->cls_def_cnt; i++) {

			if (unload_callback)
				(*unload_callback)(mod->symtab->defs[i], 0);

		        send_unload_message_to_class(mod->symtab->defs[i]);
		  }

		  if (size -= mod->size)
		    mod = (Module)((char *)mod + mod->size);
		} while (size);

		mod = modhdr;
		size = saveSize;
		do {

		  /* process category definitions */
		  for (i = mod->symtab->cls_def_cnt ; 
			i < mod->symtab->cls_def_cnt + 
				mod->symtab->cat_def_cnt; i++) 
			_objc_remove_category(mod->symtab->defs[i]);

		  /* process class definitions */
		  for (i = 0; i < mod->symtab->cls_def_cnt; i++) 
			NXHashRemove(objc_getClasses(), (id)mod->symtab->defs[i]);

		  if (size -= mod->size)
		    mod = (Module)((char *)mod + mod->size);
		} while (size);

		/* unload any selectors that might have been unique to
	         * this module...
		 */
	  	strs = (char *)getsectdatafromheader(header_addr,
				"__OBJC", "__selector_strs", &strsize);

		_sel_unloadSelectors(strs, strs+strsize);

		if (rld_unload(errStream)) {
			return 0;
		} else {
			return 1;
		}
	  }
	} else {
		NXPrintf(errStream, "objc_unloadLastModule(): " 
					"No module to unload\n");
		return 1;
	}
}


/* old interface - this will go away before v2.0 ships */

NXStream *objc_loadModule(
	char *modlist[], 
	void (*class_callback)(Class, Category),
	struct mach_header **hdr_addr
)
{
	NXStream *stream = NXOpenMemory(NULL, 0, NX_WRITEONLY);

	if (objc_loadModules(modlist,stream,class_callback,hdr_addr,0)) {
		return stream;
	} else {
		NXCloseMemory(stream, NX_FREEBUFFER);
		return 0;
	}
}

NXStream *objc_unloadLastModule(
	void (*unload_callback)(Class, Category)
)
{
	NXStream *stream = NXOpenMemory(NULL, 0, NX_WRITEONLY);

	if (objc_unloadModules(stream, unload_callback)) {
		return stream;
	} else {	
		NXCloseMemory(stream, NX_FREEBUFFER);
		return 0;
	}
}

