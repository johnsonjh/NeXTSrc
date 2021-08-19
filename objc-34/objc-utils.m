/*
 *	objc-utils.m
 *	Copyright 1988, NeXT, Inc.
 *	Author:	s. naroff
 */
#ifdef SHLIB
#import "shlib.h"
#endif

#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <time.h>
#include "objc-dispatch.h"

#import "objc-private.h"
#import "HashTable.h"


/* Return time used so far, in microseconds.  */

int gettime ()
{
  struct rusage rusage;

  getrusage (0, &rusage);
  return (rusage.ru_utime.tv_sec * 1000000 + rusage.ru_utime.tv_usec
	  + rusage.ru_stime.tv_sec * 1000000 + rusage.ru_stime.tv_usec);
}

void print_time (str, total, tv)
     char *str;
     int total;
     struct timeval *tv;
{
  extern char **NXArgv;

  fprintf (stderr,
	   "time to `%s' for `%s':\n\t%7d.%06d (sys+usr)"
	   	"\t%7d.%01d (real).\n", str, NXArgv[0],
	   total / 1000000, total % 1000000, 
		tv->tv_sec, tv->tv_usec/100000);
}

static int cacheHits = 0;
static int cacheMisses = 0;
static int supercacheHits = 0;
static int supercacheMisses = 0;

void _objc_msgCollectStats(MSG stackframe,int fromsuper,int cacheHit)
{
	if (fromsuper)
	  cacheHit ? supercacheHits++ : supercacheMisses++;
	else
	  cacheHit ? cacheHits++ : cacheMisses++;
}

void _objc_msgPrintStats()
{
	time_t tm = time(0);

	fprintf (stderr, "%s", ctime(&tm));
	fprintf (stderr, "maxSelector: %d\n", _sel_getMaxUid());
	fprintf (stderr, "  _msg:      sent = %6d, %6d hits %6d misses",
	   cacheHits+cacheMisses, cacheHits,cacheMisses);
	fprintf (stderr, "  (%3.2f%%)\n",

	   (float)cacheHits/(cacheHits+cacheMisses) * 100);
	fprintf (stderr, "  _msgSuper: sent = %6d, %6d hits %6d misses",
	   supercacheHits+supercacheMisses, supercacheHits,supercacheMisses);
	fprintf (stderr, "  (%3.2f%%)\n",
	   (float)supercacheHits/(supercacheHits+supercacheMisses) * 100);

	// reset
	cacheHits = cacheMisses = supercacheHits = supercacheMisses = 0;
}

int _objc_class_respondsTo(Class class)
{
	Class super;
	int nMethods = 0;

	if (class->methods) {
	        struct objc_method_list *catmethods = class->methods->method_next;
		if (catmethods) {
			do {
	      	  		nMethods += catmethods->method_count; 
				catmethods = catmethods->method_next;
			} while (catmethods);
		} else {
	      	  nMethods += class->methods->method_count; 
	        }
	}

	for (super = class->super_class; super; super = super->super_class)
		if (super->methods) {
	        struct objc_method_list *catmethods = super->methods->method_next;
		if (catmethods) {
			do {
	      	  		nMethods += catmethods->method_count; 
				catmethods = catmethods->method_next;
			} while (catmethods);
		} else {
	      	  nMethods += super->methods->method_count; 
	        }
		}
	return nMethods;
}

/*
 *	Purpose: 	debug/analysis.
 *	Options:
 *	Assumptions:
 *	Side Effects: 
 *	Returns: 
 */
void _objc_classPrintStats(int detail)
{
	int j;
	int n_meta = 0, b_meta = 0, n_class = 0, b_class = 0;
	int n_classes = 0, n_classes_used = 0;
	int n_class_responds = 0, n_meta_responds = 0;
	int class_responds = 0, meta_responds = 0;
	int n_class_implements = 0, n_meta_implements = 0;
	int class_implements = 0, meta_implements = 0;
	int n_categories = 0;
	Class cls, oldvalue;
	NXHashTable *class_hash;
	NXHashState state;
	void *page;
	HashTable *pagehash = [HashTable newKeyDesc:"i"];
	
	class_hash = objc_getClasses();
        state = NXInitHashState(class_hash);

	while (NXNextHashState(class_hash, &state, (void **)&cls)) {

	    Class meta = cls->isa;

	    n_classes++;

//	    if (CLS_GETINFO(meta, CLS_INITIALIZED))
	      {
	      if (CLS_GETINFO(meta, CLS_INITIALIZED))
	      	n_classes_used++;

	      page = (void *)((unsigned int)cls - ((unsigned int)cls % 0x2000));
	      oldvalue = [pagehash insertKey:page value:cls];

	      if (detail)
	      	printf("`%s' (0x%x,page #0x%x):\n", cls->name, 
					cls, page);

	      if (cls->methods) {
	        struct objc_method_list *catmethods;

		class_implements = cls->methods->method_count;
	        catmethods = cls->methods->method_next;
		if (catmethods) {
			n_categories = 0;
			do {
	      	  		class_implements += catmethods->method_count; 
				catmethods = catmethods->method_next;
				if (catmethods)
					n_categories++;
			} while (catmethods);
	        }
	      } else {
		class_implements = 0;
	      }

	      //printf("n_categories = %d\n",n_categories);

	      if (meta->methods) {
	        struct objc_method_list *catmethods;

 		meta_implements = meta->methods->method_count; 
	        catmethods = meta->methods->method_next;
		if (catmethods) {
			do {
	      	  		meta_implements += catmethods->method_count; 
				catmethods = catmethods->method_next;
			} while (catmethods);
		}
	      } else {
		meta_implements = 0;
	      }

	      if (detail)
	        printf("  implements %d instance methods, %d class methods\n",
			class_implements, meta_implements);

	      n_class_implements += class_implements;
	      n_meta_implements += meta_implements;

	      class_responds = _objc_class_respondsTo(cls);
	      meta_responds = _objc_class_respondsTo(meta);
	      
	      if (detail)
	        printf("  responds to %d instance methods, %d class methods\n",
			class_responds, meta_responds);

	      n_class_responds += class_responds;
	      n_meta_responds += meta_responds;

              if (cls->cache)
                {
		if (detail) {
	          printf("  using %d instance methods\n", cls->cache->occupied);
	          printf("  instance cache has %d slots, it is %3.2f%% full.\n",
			cls->cache->mask+1, 
			(float)cls->cache->occupied/(cls->cache->mask+1) * 100);
		}
	        b_class += sizeof(struct objc_cache) + 
			((cls->cache->mask+1) * sizeof(Method));
		n_class++;
                }
	      if (meta->cache)
                {
		if (detail) {
	          printf("  using %d class methods\n", meta->cache->occupied);
	          printf("  class cache has %d slots, it is %3.2f%% full.\n",
		       meta->cache->mask+1, 
		       (float)meta->cache->occupied/(meta->cache->mask+1) * 100);
		}
	        b_meta += sizeof(struct objc_cache) + 
			((meta->cache->mask+1) * sizeof(Method));
		n_meta++;
	        }
	      }
	  }
	{
	Module *mods = objc_getModules();
	int catcnt = 0;

	while (*mods) {
		int i, total = (*mods)->symtab->cls_def_cnt + 
				(*mods)->symtab->cat_def_cnt; 

		catcnt += (*mods)->symtab->cat_def_cnt; 

		/* add categories */
		for (i = (*mods)->symtab->cls_def_cnt;  i < total; i++) {
			Category cat = (*mods)->symtab->defs[i];
			//printf("Category `%s' for Class `%s'.\n", cat->category_name, cat->class_name);
		}
		mods++;	
	}
	printf("\nClass Info:\n\n");
	printf("%d classes (%d categories) - class descriptors spread across %d pages.\n", 
			n_classes, catcnt, [pagehash count]);
	}
	printf("%d classes currently in use\n", n_classes_used);
	printf("%d instance methods, ",n_class_implements);
	printf("%d instance responders\n",n_class_responds);
	printf("%d class methods, ",n_meta_implements);
	printf("%d class responders\n",n_meta_responds);

	if (n_class) {
		printf("%d bytes allocated to cache instance methods ",b_class);
	  	printf("(%d bytes per class)\n",
				b_class/n_class);
	}
	if (n_meta) {
		printf("%d bytes allocated to cache class methods ",b_meta);
	  	printf("(%d bytes per meta class)\n",
				b_meta/n_meta);
	}

	{
	Module *mods = objc_getModules();
	int selrefs = 0;

	while (*mods) {
		int i;
		int nsels = (*mods)->symtab->sel_ref_cnt;
#if 0
		for (i = 0; i < nsels; i++)
			printf("refs[%d] = %s\n",i, 
				sel_getName((*mods)->symtab->refs[i]));
#endif
		selrefs += nsels; 
		mods++;
	}

	printf("\nSelector Info:\n\n");
	printf("%d selector references\n", selrefs);
	printf("%d unique selectors\n", _sel_getMaxUid());
	}
}

