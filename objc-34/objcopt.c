#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/loader.h>
#include <objc/objc-runtime.h>
#include <sys/types.h>
#include <ranlib.h>

#define TRUE	1
#define	FALSE	0
#define	NIL	0

/*
 * For system call errors the error messages allways contains
 * sys_errlist[errno] as part of the message.
 */
extern char *sys_errlist[];
extern int errno;

/* Name of this program for error messages (argv[0]) */
char *progname;

extern void _sel_writeHashTable(
	int start_addr, 
	char *myselectorstraddr, 
	char *shlibselectorstraddr,
	void **stuff, 
	int *stuffsize
);
/*
 * Declarations for the static routines in this file.
 */
static void usage();

static Module get_objc(
	long fd, 
	char *filename, 
	struct section *objcsects, 
	int nsects
);
static void print_method_list(
	struct objc_method_list *mlist_before_reloc, 
	struct section *firstobjcsect, 
	int nsects
);
static void getObjcSections(
	struct mach_header *mh,
	struct load_command *lc, 
	struct section **objcsects, 
	int *nsects
);
static void readObjcData(
	long fd,
	char *filename,
    	struct section *objcsects, 
	int nsects
);
static void *getObjcData(
    	struct section *objcsects, 
	int nsects,
	void *addr
);
static struct section *getObjcSection(
    	struct section *objcsects, 
	int nsects,
	char *name	
);

void
main(argc, argv)
int argc;
char *argv[];
{
	long fd;
	char *filename;
	struct mach_header mh;
	struct load_command *lcp;

	progname = argv[0];
	filename = argv[1];

	if(argc < 2) {
	    fprintf(stderr, "%s: At least one file must be specified\n",
		    progname);
	    usage();
	    exit(1);
	}
	
	fd = open(filename, O_RDONLY);
	if(fd < 0){
		fprintf(stderr, "%s : Can't open %s (%s)\n", progname,
			filename, sys_errlist[errno]);
	}
	lseek(fd, 0, 0);
	if(read(fd, (char *)&mh, sizeof(mh)) != sizeof(mh)){
	    fprintf(stderr, "%s : Can't read mach header of %s (%s)\n",
			progname, filename, sys_errlist[errno]);
	    return;
	}
	if(mh.magic == MH_MAGIC){
		if((lcp = (struct load_command *)malloc(mh.sizeofcmds))
		    == (struct load_command *)0){
		    fprintf(stderr, "%s : Ran out of memory (%s)\n",
			    progname, sys_errlist[errno]);
		    exit(1);
		}
		if(read(fd, (char *)lcp, mh.sizeofcmds) != mh.sizeofcmds){
		    fprintf(stderr, "%s : Can't read load commands of %s (%s)\n",
			       progname, filename, sys_errlist[errno]);
		    lcp = (struct load_command *)0;
		}
	}
	{
	struct section *firstobjcsect;
	int nsects, size;
	long vm_hashaddr;
	void *table; 
	struct section *sel;
	char *selectors, tempfilename[] = "objcoptXXXXXX";

	_sel_init(0,0,0);
	
	getObjcSections(&mh, lcp, &firstobjcsect, &nsects);
	get_objc(fd, filename, firstobjcsect, nsects);

	if (strcmp(firstobjcsect[nsects-1].sectname, "__runtime_setup") == 0)
		// we are doing a `replace'
		vm_hashaddr = firstobjcsect[nsects-1].addr;
	else  
		vm_hashaddr = round(firstobjcsect[nsects-1].addr + 
			    	firstobjcsect[nsects-1].size, sizeof(long));

	sel = getObjcSection(firstobjcsect, nsects, "__selector_strs");
	selectors = (char *)sel->reserved1;

	_sel_writeHashTable(vm_hashaddr, (char *)selectors, (char *)sel->addr,
				 &table, &size);
	mktemp(tempfilename);
	add_objc_runtime_setup(filename,tempfilename,table, size);

	close(fd);
	unlink(filename);
	if (rename(tempfilename, filename) == -1) {
		fprintf(stderr, "%s : Cannot rename temporary file from: "
				"%s to %s\n", progname, tempfilename, filename);
		exit(1);
	}
	}

	exit(0);
}

static void getObjcSections(
	struct mach_header *mh,
	struct load_command *lc, 
	struct section **objcsects, 
	int *nsects
)
{
	int i,j;
    	struct segment_command *sg;
	struct section *s;
    	struct load_command *initlc;

	initlc = lc;

	for(i = 0 ; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0)
		printf("load command %d size not a multiple of sizeof(long)\n",
		       i);
	    if((char *)lc + lc->cmdsize > (char *)initlc + mh->sizeofcmds)
		printf("load command %d extends past end of load commands\n",i);
	    if(lc->cmd == LC_SEGMENT){
		sg = (struct segment_command *)lc;
		if(mh->filetype == MH_OBJECT ||
		   strcmp(sg->segname, SEG_OBJC) == 0){

		    s = (struct section *)
			((char *)sg + sizeof(struct segment_command));

		    *objcsects = s;
		    *nsects = sg->nsects;

		    for(j = 0 ; j < sg->nsects ; j++){
			if((char *)s + sizeof(struct section) >
			   (char *)initlc + mh->sizeofcmds){
			    printf("section structure command extends past end "
				   "of load commands\n");
			}
			if((char *)s + sizeof(struct section) >
			   (char *)initlc + mh->sizeofcmds)
			    break;
			s++;
		    }
		}
	    }
	    if(lc->cmdsize <= 0){
		printf("load command %d size negative or zero (can't "
		       "advance to other load commands)\n", i);
		break;
	    }
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	    if((char *)lc > (char *)initlc + mh->sizeofcmds)
		break;
	}
	if((char *)initlc + mh->sizeofcmds != (char *)lc)
	    printf("Inconsistant mh_sizeofcmds\n");
}

static void readObjcData(
	long fd,
	char *filename,
    	struct section *objcsects, 
	int nsects
)
{
	int i;

	for (i = 0; i < nsects; i++) {

		struct section *s;

		s = objcsects + i;

		if (s->size > 0) {
	    	  if((s->reserved1 = (long)malloc(s->size)) == NIL) {
		    fprintf(stderr, "%s : Ran out of memory (%s)\n", 
			progname, sys_errlist[errno]);
		    exit(1);
	          }
	          lseek(fd, s->offset, 0);
	          if((read(fd, s->reserved1, s->size)) != s->size){
		    fprintf(stderr, "%s : Can't read modules of %s (%s)\n",
		    	progname, filename, sys_errlist[errno]);
		    free((void *)s->reserved1);
		    return;
	          }
		}
	}
}

static void *getObjcData(
    	struct section *objcsects, 
	int nsects,
	void *addr
)
{
	int i;

	if (addr == 0)
		return 0;

	for (i = 0; i < nsects; i++) {
		struct section *s;

		s = objcsects + i;
		if (((long)addr >= s->addr) && 
			((long)addr < (s->addr + s->size)))
		  return (void *)(s->reserved1 + ((long)addr - s->addr));
	}
    	fprintf(stderr, "%s : Could not `getObjcData'\n", progname);
	exit(1);
}

static struct section *getObjcSection(
    	struct section *objcsects, 
	int nsects,
	char *name	
)
{
	int i;

	for (i = 0; i < nsects; i++) {
		struct section *s;

		s = objcsects + i;
		if (strcmp(s->sectname,name) == 0)
		  return s;
	}
        return 0;
}

/*
 * Print the objc segment.
 */
static
struct objc_module *
get_objc(fd, filename, firstobjcsect, nsects)
long fd;
char *filename;
struct section *firstobjcsect;
int nsects;
{
	long i, j;
	struct section *modsect, *msgsect, *selsect;
	struct objc_module *modules, *m;
	struct objc_symtab *t;

	readObjcData(fd, filename, firstobjcsect, nsects);

	modsect = getObjcSection(firstobjcsect, nsects, "__module_info");
	msgsect = getObjcSection(firstobjcsect, nsects, "__message_refs");
  	if (msgsect) {
		int i, cnt = msgsect->size/sizeof(SEL);
   		SEL *sels = msgsect->reserved1;

		/* overwrite the string with a unique identifier */
		for (i = 0; i < cnt; i++) 
			sels[i] = (SEL)_sel_registerNameUseString(
   				(SEL)getObjcData(firstobjcsect, nsects,
						sels[i]));
	}
	selsect = getObjcSection(firstobjcsect, nsects, "__selector_refs");
  	if (selsect) {
		int i, cnt = selsect->size/sizeof(SEL);
   		SEL *sels = selsect->reserved1;

		/* overwrite the string with a unique identifier */
		for (i = 0; i < cnt; i++) 
			sels[i] = (SEL)_sel_registerNameUseString(
   				(SEL)getObjcData(firstobjcsect, nsects,
						sels[i]));
	}
	modules = (Module)modsect->reserved1;

	for(m = modules ;
	    (char *)m < (char *)modules + modsect->size;
	    m = (struct objc_module *)((char *)m + m->size) ){

		/* relocate fields in module structure */

		m->name = (char *)getObjcData(firstobjcsect, nsects, 
						m->name);
		m->symtab = t = (Symtab)getObjcData(firstobjcsect, nsects, 
						m->symtab);

		/* Simulate map! */
		if (m->version == 1) { 
		  int i, cnt = m->symtab->sel_ref_cnt;
   		  SEL *sels = (SEL *)getObjcData(firstobjcsect, nsects,
						m->symtab->refs);

		  /* overwrite the string with a unique identifier */
		  for (i = 0; i < cnt; i++) 
			sels[i] = (SEL)_sel_registerNameUseString(
   				(SEL)getObjcData(firstobjcsect, nsects,
						sels[i]));
		}

		for(i = 0; i < t->cls_def_cnt; i++){

    			struct objc_class *objc_class;

			t->defs[i] = objc_class = 
   				(Class)getObjcData(firstobjcsect, nsects,
						t->defs[i]);
print_objc_class:
			/* relocate class structure */

			if(CLS_GETINFO(objc_class, CLS_META)){
				objc_class->isa = (Class)getObjcData(
					firstobjcsect, nsects, objc_class->isa);
			}

			objc_class->super_class = (Class)getObjcData(
						firstobjcsect, nsects, 
						objc_class->super_class);

			objc_class->name = (char *)getObjcData(
						firstobjcsect, nsects, 
						objc_class->name);

			if (objc_class->ivars) {
    				struct objc_ivar_list *ilist;
    				struct objc_ivar *ivar;

				objc_class->ivars = ilist = 
					(struct objc_ivar_list *)getObjcData(
						firstobjcsect, nsects, 
						objc_class->ivars);

				ivar = ilist->ivar_list;
				for(j = 0; j < ilist->ivar_count; j++, ivar++){
					ivar->ivar_name= (char *)getObjcData(
						firstobjcsect, nsects, 
						ivar->ivar_name);

					ivar->ivar_type= (char *)getObjcData(
						firstobjcsect, nsects, 
						ivar->ivar_type);
				}
			}

			if (objc_class->methods) {
				print_method_list(objc_class->methods, 
						firstobjcsect, nsects);
			}

			if(CLS_GETINFO(objc_class, CLS_CLASS)){
			    objc_class->isa = objc_class = 
   					(Class)getObjcData(
						firstobjcsect, nsects,
						objc_class->isa);
			    goto print_objc_class;
			}
		}

		for(i = 0; i < t->cat_def_cnt; i++){

    			struct objc_category *objc_category;

			t->defs[i + t->cls_def_cnt] = objc_category = 
   				(Category)getObjcData(firstobjcsect, nsects,
						t->defs[i+t->cls_def_cnt]);

			objc_category->category_name = (char *)getObjcData(
						firstobjcsect, nsects, 
				       		objc_category->category_name);

			objc_category->class_name = (char *)getObjcData(
						firstobjcsect, nsects, 
				       		objc_category->class_name);

			if (objc_category->instance_methods) {
			    print_method_list(objc_category->instance_methods, 
						firstobjcsect, nsects);
			}

			if (objc_category->class_methods) {
			    print_method_list(objc_category->class_methods, 
						firstobjcsect, nsects);
			}
		}
	}
	return modules;
}

static
void
print_method_list(struct objc_method_list *mlist_before_reloc, 
		  struct section *firstobjcsect, 
		  int nsects)
{
    long i;
    struct objc_method *method;
    struct objc_method_list *mlist;

	mlist = (struct objc_method_list *)getObjcData(firstobjcsect, nsects,
				    		mlist_before_reloc);
	method = mlist->method_list;
	for(i = 0; i < mlist->method_count; i++, method++){

	    method->method_name = (SEL)getObjcData(firstobjcsect, nsects, 
		           			method->method_name);

	    method->method_types = (char *)getObjcData(firstobjcsect, nsects, 
		           			method->method_types);

	    method->method_name = 
		(SEL)_sel_registerNameUseString((STR)method->method_name);

	}
}

/*
 * Print the current usage message.
 */
static
void
usage()
{
	fprintf(stderr,
		"Usage: %s <object file> ...\n",
		progname);
}
