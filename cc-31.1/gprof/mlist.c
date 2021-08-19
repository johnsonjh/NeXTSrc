#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/loader.h>
#include "mlist.h"
#include <objc/objc-runtime.h>

static long compare(struct mlist *sym1, struct mlist *sym2);

extern struct objc_module *getobjcseg(long fd, long offset,
				      struct mach_header *mh,
				      struct load_command *lc, long *modsize);
/*
 * mlist() returns a pointer to an array of mlist structures and returns the
 * number of sturctures in the array indirectly through mlist_cnt out of
 * the file represented by the open file descriptor, fd, and the pointers to
 * it's mach header and load commands, mh and lc.  offset is the offset from
 * the begining of the file descriptor where the object starts (zero for object
 * files and non-zero for archives).  If any error occurs then this routine
 * returns NULL and zero indirectly through mlist_cnt.  The space for the mlist
 * structures and the strings are allocated contiguously as one block and the
 * pointer to the mlist structrures returned is the start of that block.  This
 * allows a single free() call to be done on the return value of this routine.
 */
struct mlist *
mlist(
long fd,
long offset,
struct mach_header *mh,
struct load_command *lc,
long *mlist_cnt)
{
    long modsize, strsize = 0, i, j;
    struct objc_module *modules, *mod;
    struct objc_symtab *t;
    struct objc_class *objc_class, *meta_class;
    struct objc_category *objc_category;
    struct objc_method_list *mlist;
    struct mlist *m;
    char *p, *buf;

	*mlist_cnt = 0;
	modules = getobjcseg(fd, offset, mh, lc, &modsize);
	if(modules == NULL)
	    return(NULL);

	/*
	 * First size how much space to allocate for the mlist structures and
	 * the strings to be created.
	 */
	for(mod = modules;
	    (char *)mod < (char *)modules + modsize;
	    mod = (struct objc_module *)((char *)mod + mod->size) ){
	    t = mod->symtab;
	    for(i = 0; i < t->cls_def_cnt; i++){
		objc_class = (struct objc_class *)t->defs[i];
		mlist = objc_class->methods;
		if(mlist != NULL){
		    for(j = 0; j < mlist->method_count; j++)
			strsize += strlen((const char *)
					  mlist->method_list[j].method_name);
		    strsize += mlist->method_count *
			       (5 + strlen(objc_class->name));
		    *mlist_cnt += mlist->method_count;
		}
		if(CLS_GETINFO(objc_class, CLS_CLASS)){
		    meta_class = objc_class->isa;
		    mlist = meta_class->methods;
		    if(mlist != NULL){
			for(j = 0; j < mlist->method_count; j++)
			    strsize +=strlen((const char *)
			    		     mlist->method_list[j].method_name);
			strsize += mlist->method_count *
				   (5 + strlen(meta_class->name));
			*mlist_cnt += mlist->method_count;
		    }
		}
	    }
	    for(i = 0; i < t->cat_def_cnt; i++){
		objc_category = (struct objc_category *)
				t->defs[i + t->cls_def_cnt];
		mlist = objc_category->instance_methods;
		if(mlist != NULL){
		    for(j = 0; j < mlist->method_count; j++)
			strsize += strlen((const char *)
					  mlist->method_list[j].method_name);
		    strsize += mlist->method_count *
			       (7 + strlen(objc_category->class_name)
				  + strlen(objc_category->category_name));
		    *mlist_cnt += mlist->method_count;
		}
		mlist = objc_category->class_methods;
		if(mlist != NULL){
		    for(j = 0; j < mlist->method_count; j++)
			strsize += strlen((const char *)
					  mlist->method_list[j].method_name);
		    strsize += mlist->method_count *
			       (7 + strlen(objc_category->class_name)
				  + strlen(objc_category->category_name));
		    *mlist_cnt += mlist->method_count;
		}
	    }
	}

	/*
	 * Allocate the space (contiguously) for the mlist structs and strings.
	 */
	buf = (char *)malloc(*mlist_cnt * sizeof(struct mlist) + strsize);
	if(buf == NULL){
	    *mlist_cnt = 0;
	    free(modules);
	    return(NULL);
	}
	m = (struct mlist *)buf;
	p = (char *)(buf + *mlist_cnt * sizeof(struct mlist));

	for(mod = modules;
	    (char *)mod < (char *)modules + modsize;
	    mod = (struct objc_module *)((char *)mod + mod->size) ){
	    t = mod->symtab;
	    for(i = 0; i < t->cls_def_cnt; i++){
		objc_class = (struct objc_class *)t->defs[i];
		mlist = objc_class->methods;
		if(mlist != NULL){
		    for(j = 0; j < mlist->method_count; j++){
			m->m_value = (long)mlist->method_list[j].method_imp;
			m->m_name = p;
			sprintf(p, "-[%s %s]", objc_class->name,
				mlist->method_list[j].method_name);
			m++;
			p += 5 + strlen((const char *)
					mlist->method_list[j].method_name) +
				 strlen(objc_class->name);
		    }
		}
		if(CLS_GETINFO(objc_class, CLS_CLASS)){
		    meta_class = objc_class->isa;
		    mlist = meta_class->methods;
		    if(mlist != NULL){
			for(j = 0; j < mlist->method_count; j++){
			    strsize +=strlen((const char *)
			    		     mlist->method_list[j].method_name);
			    m->m_value = (long)mlist->method_list[j].method_imp;
			    m->m_name = p;
			    sprintf(p, "+[%s %s]", meta_class->name,
				    mlist->method_list[j].method_name);
			    m++;
			    p += 5 + strlen((const char *)
			    		    mlist->method_list[j].method_name) +
				     strlen(meta_class->name);
			}
		    }
		}
	    }
	    for(i = 0; i < t->cat_def_cnt; i++){
		objc_category = (struct objc_category *)
				t->defs[i + t->cls_def_cnt];
		mlist = objc_category->instance_methods;
		if(mlist != NULL){
		    for(j = 0; j < mlist->method_count; j++){
			m->m_value = (long)mlist->method_list[j].method_imp;
			m->m_name = p;
			sprintf(p, "-[%s(%s) %s]", objc_category->class_name,
				objc_category->category_name,
				mlist->method_list[j].method_name);
			m++;
			p += 7 + strlen((const char *)
					mlist->method_list[j].method_name) +
				 strlen(objc_category->category_name) +
				 strlen(objc_category->class_name);
		    }
		}
		mlist = objc_category->class_methods;
		if(mlist != NULL){
		    for(j = 0; j < mlist->method_count; j++){
			m->m_value = (long)mlist->method_list[j].method_imp;
			m->m_name = p;
			sprintf(p, "+[%s(%s) %s]", objc_category->class_name,
				objc_category->category_name,
				mlist->method_list[j].method_name);
			m++;
			p += 7 + strlen((const char *)
					mlist->method_list[j].method_name) +
				 strlen(objc_category->category_name) +
				 strlen(objc_category->class_name);
		    }
		}
	    }
	}

	qsort(buf, *mlist_cnt, sizeof(struct mlist),
	      (int (*)(const void *, const void *)) &compare);

#ifdef DEBUG
	m = (struct mlist *)buf;
	for(i = 0; i < *mlist_cnt; i++){
	    printf("mlist[%d] 0x%08x %s\n", i, m->m_value, m->m_name);
	    m++;
	}
#endif DEBUG

	free(modules);
	return((struct mlist *)buf);
}

/*
 * Function for qsort for comparing symbols.
 */
static
long
compare(
    const struct mlist *sym1,
    const struct mlist *sym2)
{
	return(sym1->m_value - sym2->m_value);
}



