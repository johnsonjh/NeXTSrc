#include <stdio.h>
#include <sys/loader.h>
#include "mlist.h"
#include <objc/objc-runtime.h>

struct objc_module *getobjcseg(long fd, long offset, struct mach_header *mh,
    			       struct load_command *lc, long *modsize);

static int method_list(struct objc_method_list *mlist, struct section *sym,
    		       struct section *sel, struct objc_symtab *symbols,
		       char *selectors);
/*
 * getobjcseg() gets the objective-C segment out of the file represented by the
 * open file descriptor, fd, and the pointers to it's mach header and load
 * commands, mh and lc.  offset is the offset from the begining of the file
 * descriptor where the object starts (zero for object files and non-zero for
 * archives).  This routine returns a pointer to the module section and the
 * size of the section (in bytes) indirectly through modsize.  All pointers
 * are fixed up so they can be used directly.  If any error occurs then this
 * routines returns NULL.  The space for all the sections of the __OBJC
 * segment are allocated contiguously as one block and the pointer to the
 * module section returned is the start of that block.  This allows a single
 * free() call to be done on the return value of this function.
 */
struct objc_module *
getobjcseg(
long fd,
long offset,
struct mach_header *mh,
struct load_command *lc,
long *modsize)
{
    long i, j;
    struct segment_command *sg;
    struct section *s, *sym, *mod, *sel;
    struct objc_module *modules, *m;
    struct objc_symtab *symbols, *t;
    char *buf, *selectors, *p, **sel_ref;
    struct objc_class *objc_class;
    struct objc_category *objc_category;
    struct objc_ivar_list *ilist;
    struct objc_ivar *ivar;
    struct objc_method_list *mlist;

	*modsize = 0;

	sym = NULL;
	mod = NULL;
	sel = NULL;
	for(i = 0 ; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0)
		return(NULL);
	    if(lc->cmd == LC_SEGMENT){
		sg = (struct segment_command *)lc;
		if(mh->filetype == MH_OBJECT ||
		   strcmp(sg->segname, SEG_OBJC) == 0){
		    s = (struct section *)
			((char *)sg + sizeof(struct segment_command));
		    for(j = 0 ; j < sg->nsects ; j++){
			if(strcmp(s->sectname, SECT_OBJC_SYMBOLS) == 0){
			    if(sym != NULL)
				return(NULL);
			    else
				sym = s;
			}
			else if(strcmp(s->sectname, SECT_OBJC_MODULES) == 0){
			    if(mod != NULL)
				return(NULL);
			    else
				mod = s;
			}
			else if(strcmp(s->sectname, SECT_OBJC_STRINGS) == 0){
			    if(sel != NULL)
				return(NULL);
			    else
				sel = s;
			}
			s++;
		    }
		}
	    }
	    if(lc->cmdsize <= 0)
		return(NULL);
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
	if(mod == NULL || sym == NULL || sel == NULL)
	    return(NULL);


	if((buf = (char *)malloc(mod->size + sym->size + sel->size)) == NULL)
	    return(NULL);
	modules = (struct objc_module *)buf;
	lseek(fd, offset + mod->offset, 0);
	if((read(fd, modules, mod->size)) != mod->size){
	    free(buf);
	    return(NULL);
	}
	symbols = (struct objc_symtab *)(buf + mod->size);
	lseek(fd, offset + sym->offset, 0);
	if((read(fd, symbols, sym->size)) != sym->size){
	    free(buf);
	    return(NULL);
	}
	selectors = buf + mod->size + sym->size;
	lseek(fd, offset + sel->offset, 0);
	if((read(fd, selectors, sel->size)) != sel->size){
	    free(buf);
	    return(NULL);
	}

	for(m = modules;
	    (char *)m < (char *)modules + mod->size;
	    m = (struct objc_module *)((char *)m + m->size) ){

	    if((char *)m + m->size > (char *)m + mod->size){
		free(buf);
		return(NULL);
	    }
	    if((long)m->name < sel->addr ||
	       (long)m->name >= sel->addr + sel->size){
		free(buf);
		return(NULL);
	    }
	    m->name = selectors + (long)m->name - sel->addr;

	    if((long)m->symtab < sym->addr ||
	       (long)m->symtab >= sym->addr + sym->size){
		free(buf);
		return(NULL);
	    }
	    m->symtab = (struct objc_symtab *)((char *)symbols +
				       ((long)m->symtab - sym->addr));
	    t = m->symtab;
	    if((char *)t + sizeof(struct objc_symtab) >
	       (char *)symbols + sym->size){
		free(buf);
		return(NULL);
	    }
	    if(t->refs == NULL){
		if(t->sel_ref_cnt != 0){
		    free(buf);
		    return(NULL);
		}
	    }
	    else{
	        if((long)t->refs < sym->addr ||
	           (long)t->refs >= sym->addr + sym->size){
		    free(buf);
		    return(NULL);
		}
		if((long)t->refs + t->sel_ref_cnt * sizeof(char *) >
						    sym->addr + sym->size){
		    free(buf);
		    return(NULL);
		}
		t->refs = (SEL *)((char *)symbols +
				    ((long)t->refs - sym->addr));
		sel_ref = (char **)t->refs;
		for(i = 0; i < t->sel_ref_cnt; i++){
		    if((long)sel_ref[i] < sel->addr &&
		       (long)sel_ref[i] >= sel->addr + sel->size){
			    free(buf);
			    return(NULL);
		    }
		    sel_ref[i] = selectors + (long)sel_ref[i] - sel->addr;
		}
	    }

	    for(i = 0; i < t->cls_def_cnt; i++){
		if((long)&(t->defs[i]) - (long)symbols > sym->size){
		    free(buf);
		    return(NULL);
		}
		if((long)t->defs[i] < sym->addr ||
		   (long)t->defs[i] >= sym->addr + sym->size){
		    free(buf);
		    return(NULL);
		}

		t->defs[i] = (struct objc_class *)((char *)symbols +
			      ((long)t->defs[i] - sym->addr));
		objc_class = (struct objc_class *)t->defs[i];
objc_class:
		if((long)objc_class + sizeof(struct objc_class) -
		   (long)symbols > sym->size){
		    free(buf);
		    return(NULL);
		}
		if(CLS_GETINFO(objc_class, CLS_META)){
		    if((long)objc_class->isa < sel->addr ||
		       (long)objc_class->isa >= sel->addr + sel->size){
			free(buf);
			return(NULL);
		    }
		    /*
		     * For META classes the isa is a pointer to a class name
		     * in the object file.  The objective-C runtime will change
		     * it to point to a objc_class structure.
		     */
		    objc_class->isa = (struct objc_class *)(selectors +
			       ((long)objc_class->isa - sel->addr));
		}
		/*
		 * For all classes the super_class is a pointer to a class name
		 * in the object file.  Again the objective-C runtime will
		 * change it to point to a objc_class structure.  It can be a
		 * NULL pointer.
		 */
		if(objc_class->super_class != NULL){
		    if((long)objc_class->super_class < sel->addr ||
		       (long)objc_class->super_class >= sel->addr + sel->size){
			free(buf);
			return(NULL);
		    }
		    objc_class->super_class = (struct objc_class *)(selectors +
			   ((long)objc_class->super_class - sel->addr));
		}

		if((long)objc_class->name < sel->addr ||
		   (long)objc_class->name >= sel->addr + sel->size){
		    free(buf);
		    return(NULL);
		}
	        objc_class->name  = selectors +
		       		    ((long)objc_class->name - sel->addr);

		if(objc_class->ivars != NULL){
		    if((long)objc_class->ivars < sym->addr ||
		       (long)objc_class->ivars >= sym->addr + sym->size){
			free(buf);
			return(NULL);
		    }
		    objc_class->ivars = (struct objc_ivar_list *)
					((char *)symbols +
			    		((long)objc_class->ivars - sym->addr));
		    ilist = objc_class->ivars;
		    if((char *)ilist + sizeof(struct objc_ivar_list) >
		       (char *)symbols + sym->size){
			free(buf);
			return(NULL);
		    }
		    ivar = ilist->ivar_list;
		    for(j = 0; j < ilist->ivar_count; j++, ivar++){
			if((char *)ivar > (char *)symbols + sym->size){
			    free(buf);
			    return(NULL);
			}
			if((long)ivar->ivar_name < sel->addr ||
			   (long)ivar->ivar_name >= sel->addr + sel->size){
			    free(buf);
			    return(NULL);
			}
		        ivar->ivar_name = selectors +
					  ((long)ivar->ivar_name - sel->addr);

			if((long)ivar->ivar_type < sel->addr &&
			   (long)ivar->ivar_type >= sel->addr + sel->size){
			    free(buf);
			    return(NULL);
			}
			ivar->ivar_type = selectors +
				          ((long)ivar->ivar_type - sel->addr);
		    }
		}

		if(objc_class->methods != NULL){
		    if((long)objc_class->methods < sym->addr ||
		       (long)objc_class->methods >= sym->addr + sym->size){
			free(buf);
			return(NULL);
		    }
		    objc_class->methods = (struct objc_method_list *)
				       ((char *)symbols +
				       ((long)objc_class->methods - sym->addr));
		    if(method_list(objc_class->methods, sym, sel, symbols,
				   selectors)){
			free(buf);
			return(NULL);
		    }
		}

		if(CLS_GETINFO(objc_class, CLS_CLASS)){
		    if((long)objc_class->isa < sym->addr ||
		       (long)objc_class->isa >= sym->addr + sym->size){
			free(buf);
			return(NULL);
		    }
		    objc_class->isa = (struct objc_class *) ((char *)symbols +
				      ((long)(objc_class->isa) - sym->addr));
		    objc_class = objc_class->isa;
		    goto objc_class;
		}
	    }
	    for(i = 0; i < t->cat_def_cnt; i++){
		if((long)&(t->defs[i + t->cls_def_cnt]) - (long)symbols >
		    sym->size){
		    free(buf);
		    return(NULL);
		}
		if((long)t->defs[i + t->cls_def_cnt] < sym->addr ||
		   (long)t->defs[i + t->cls_def_cnt] >= sym->addr + sym->size){
		    free(buf);
		    return(NULL);
		}
		t->defs[i + t->cls_def_cnt] = (struct objc_category *)
			    ((char *)symbols +
			    ((long)t->defs[i + t->cls_def_cnt] - sym->addr));
		objc_category = (struct objc_category *)
				t->defs[i + t->cls_def_cnt];

		if((long)objc_category + sizeof(struct objc_category) -
		   (long)symbols > sym->size){
		    free(buf);
		    return(NULL);
		}
		if((long)objc_category->category_name < sel->addr ||
		   (long)objc_category->category_name >= sel->addr + sel->size){
		    free(buf);
		    return(NULL);
		}
		objc_category->category_name = selectors +
				(long)objc_category->category_name - sel->addr;

		if((long)objc_category->class_name < sel->addr ||
		   (long)objc_category->class_name >= sel->addr + sel->size){
		    free(buf);
		    return(NULL);
		}
		objc_category->class_name = selectors +
				   (long)objc_category->class_name - sel->addr;

		if(objc_category->instance_methods != NULL){
		    if((long)objc_category->instance_methods < sym->addr ||
		       (long)objc_category->instance_methods >=
							sym->addr + sym->size){
			free(buf);
			return(NULL);
		    }
		    objc_category->instance_methods =
			       (struct objc_method_list *) ((char *)symbols +
			       ((long)objc_category->instance_methods -
			       sym->addr));
		    if(method_list(objc_category->instance_methods, sym, sel,
				   symbols, selectors)){
			free(buf);
			return(NULL);
		    }
		}

		if(objc_category->class_methods != NULL){
		    if((long)objc_category->class_methods < sym->addr ||
		       (long)objc_category->class_methods >=
							sym->addr + sym->size){
			free(buf);
			return(NULL);
		    }
		    objc_category->class_methods = (struct objc_method_list *)
			    ((char *)symbols +
			    ((long)objc_category->class_methods - sym->addr));

		    if(method_list(objc_category->class_methods, sym, sel,
				   symbols, selectors)){
			free(buf);
			return(NULL);
		    }
		}
	    }
	}
	*modsize = mod->size;
	return(modules);
}

static
int
method_list(
    struct objc_method_list *mlist,
    struct section *sym,
    struct section *sel,
    struct objc_symtab *symbols,
    char *selectors)
{
	long i;
	struct objc_method *method;

	if((char *)mlist + sizeof(struct objc_method_list) >
	   (char *)symbols + sym->size)
	    return(1);
	
	method = mlist->method_list;
	for(i = 0; i < mlist->method_count; i++, method++){
	    if((char *)method > (char *)symbols + sym->size)
		return(1);
	    if((long)method->method_name < sel->addr ||
	       (long)method->method_name >= sel->addr + sel->size)
		return(1);

	    method->method_name = (SEL)(selectors +
				       ((long)method->method_name - sel->addr));
	    if((long)method->method_types < sel->addr ||
	       (long)method->method_types >= sel->addr + sel->size)
		return(1);
	    method->method_types = selectors +
				   ((long)method->method_types - sel->addr);
	}
	return(0);
}
