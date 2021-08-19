#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/loader.h>
#include <machine/thread_status.h>
#include "../include/ldsyms.h"
#include "mach_o.h"
#include "stdio.h"
#include "a.out.h"

struct mach_o_info *execinfo;

struct mach_o_info *
read_mach_o(fd)
int fd;
{
  char *image;
  struct stat sbuf;
  int nthreads, nsegs, i;
  struct mach_header *mh;
  struct load_command *lc;
  struct mach_o_info *info;

  fstat(fd, &sbuf);
  map_fd(fd, 0, &image, 1, sbuf.st_size);
/* Can't do this other people need this.
  close(fd);
*/
  info = (struct mach_o_info *)malloc(sizeof(struct mach_o_info));
  bzero(info, sizeof(*info));
  mh = info->mach_header = (struct mach_header *)image;
  if (mh->magic != MH_MAGIC) {
    fprintf(stderr, "Bad mach_o magic number");
    exit(1);
  }
  switch(mh->filetype) {
  case MH_OBJECT:
  case MH_EXECUTE:
  case MH_PRELOAD:
  case MH_FVMLIB:
  case MH_CORE:
    break;
    
  default:
    fprintf(stderr, "Bad mach_o file type");
    exit(1);
  }
  /* Make a pass through and count different file segments */
  lc = (struct load_command *)(image + sizeof(struct mach_header));
  info->load_commands = lc;
  nsegs = nthreads = 0;
  for (i=0;i<mh->ncmds;i++) {
    switch(lc->cmd) {
    case LC_SEGMENT:
      nsegs++;
      break;
    case LC_THREAD:
      nthreads++;
      break;
    case LC_SYMTAB:
    case LC_SYMSEG:
    case LC_UNIXTHREAD:
    case LC_LOADFVMLIB:
    case LC_IDFVMLIB:
    case LC_IDENT:
      break;
    default:
      printf("Unknown load command type: %d", lc->cmd);
      break;
    }
    lc = (struct load_command *)((char *)lc + lc->cmdsize);
  }
  /* Now allocate data structures */
  info->segs = (struct mach_segment_map *)
    malloc(sizeof(struct mach_segment_map) * nsegs);
  info->nsegs = nsegs;
  info->threads = (struct mach_thread_info *)
    malloc(sizeof(struct mach_thread_info) * nthreads);
  info->nthreads = nthreads;
  lc = (struct load_command *)(image + sizeof(struct mach_header));
  nsegs = nthreads = 0;
  for (i=0;i<mh->ncmds;i++) {
    switch(lc->cmd) {
    case LC_SEGMENT:
      { struct segment_command *sc;

	sc = (struct segment_command *)lc;
	info->segs[nsegs].segname = sc->segname;
	info->segs[nsegs].native_vmaddr = (char *)sc->vmaddr;
	info->segs[nsegs].size = sc->vmsize;
	/* Zero fill on demand has fileoff == 0 */
	if (sc->filesize)
	  info->segs[nsegs].local_vmaddr = (char *)(image + sc->fileoff);
	else
	  info->segs[nsegs].local_vmaddr = 0;
	nsegs++;
	break;
      }
    case LC_SYMSEG:
      { struct symseg_command *sc;
	sc = (struct symseg_command *)lc;
	info->symsegsize = sc->size;
	if (sc->offset)
	  info->symsegs = (char *)(image + sc->offset);
	break;
      }
    case LC_SYMTAB:
      { struct symtab_command *sc;
	sc = (struct symtab_command *)lc;
	info->nsyms = sc->nsyms;
	if (sc->symoff)
	  info->syms = (char *)(image + sc->symoff);
	info->strsize = sc->strsize;
	if (sc->stroff)
	  info->strings = (char *)(image + sc->stroff);
	break;
      }
    case LC_THREAD:
      { int flavor, flavor_size;
	char *cur, *end;

	cur = (char *)lc + sizeof(struct thread_command);
	end = (char *)lc + lc->cmdsize;
	while(cur < end) {
	  flavor = *(int *)cur;
	  cur += sizeof(int);
	  flavor_size = *(int *)cur;
	  cur += sizeof(int);

	  switch(flavor) {
	  case NeXT_THREAD_STATE_REGS:
	    info->threads[nthreads].regs = 
	      (struct NeXT_thread_state_regs *)cur;
	    cur += sizeof(struct NeXT_thread_state_regs);
	    break;
	  case NeXT_THREAD_STATE_68882:
	    info->threads[nthreads].fpregs = 
	      (struct NeXT_thread_state_68882 *)cur;
	    cur += sizeof(struct NeXT_thread_state_68882);
	    break;
	  default:
	    fprintf(stderr, "Unrecognized thread flavor: %d\n", flavor);
	    exit(1);
	    break;
	  }
	}
	break;
    case LC_LOADFVMLIB:
#ifdef 0
	{ char *lib_pathname;
	  struct mach_o_info *i1, *i2;
	  struct fvmlib_command *fv = (struct fvmlib_command *)lc;
	  char *lib_filename;

	  lib_filename = (char *)lc + fv->fvmlib.name.offset;
	  i1 = read_mach_o(lib_filename, &lib_pathname);
	  i2 = info;
	  while(i2->next != 0)
	    i2 = i2->next;
	  i2->next = i1;
	  break;
	}
#endif
    case LC_UNIXTHREAD:
    case LC_IDFVMLIB:
    case LC_IDENT:
	break;
      }
    default:
      printf("Unknown load command type: %d", lc->cmd);
      break;
    }
    lc = (struct load_command *)((char *)lc + lc->cmdsize);
  }
  return(info);
}

char *
NATIVE_TO_DSEG(a)
char *a;
{
  char *exec_file_addr();
  char *rval;

  if (a == 0)
      return(0);
  rval = exec_file_addr(a);

  if (!rval) {
    fprintf(stderr,"NATIVE_TO_DSEG: bad addr: 0x%x\n", a);
    exit(1);
  }
  return(rval);
}

#if 0
/* ??? */
initialize_shlib_fixups()
{
  register struct shlib_init **shlib_init_table_pointer;
  register struct shlib_init *shlib_init_table;

  int miscindx;

  /* Get the pointer to the table */
  miscindx = lookup_misc_func (SHLIB_INIT);
  if (miscindx < 0)
    return;
  shlib_init_table_pointer = (struct shlib_init **)
    NATIVE_TO_DSEG(misc_function_vector[miscindx].address);
  if (shlib_init_table_pointer == 0)
    return;
  while(*shlib_init_table_pointer) {

    shlib_init_table = (struct shlib_init *)
      NATIVE_TO_DSEG(*shlib_init_table_pointer);


    while (shlib_init_table->address) {

      *(int *)CHILD_TO_DSEG(shlib_init_table->address) = 
	shlib_init_table->value;
      shlib_init_table++;
    }
    /* Done with this table of entries... On to the next */
    shlib_init_table_pointer++;
  }
}
#endif

char *
exec_file_addr (memaddr)
char *memaddr;
{
  struct mach_segment_map *s;
  struct mach_o_info *info = execinfo;
  register i;

  while(info) {
  /* Need to handle address lookup in MACH_O file here */
    for (i=0, s = info->segs; i < info->nsegs; i++, s++) {
      if (memaddr >= s->native_vmaddr && 
	  memaddr < (s->native_vmaddr + s->size)) {
	if (s->local_vmaddr) {
	  return ((char *)s->local_vmaddr + (memaddr - s->native_vmaddr));
	} else
	  return(0);
      }
    }
    info = info->next;
  }
  return 0;
}

lookup_symval(name)
char *name;
{
  struct mach_o_info *info = execinfo;

  while (info) {
    struct nlist *nl;
    int nsyms;

    /* Find the symbol table segment. */
    nl = (struct nlist *)info->syms;
    nsyms = info->nsyms;
    for(;nsyms;nsyms--,nl++) {
      char *sname;

      if (!(nl->n_type&N_STAB) && (nl->n_type & N_EXT) && nl->n_un.n_strx) {
	sname = info->strings + nl->n_un.n_strx;
	if (!strcmp(name, sname))
	  return(nl->n_value);
      }
    }
    /* Next on the chain. */
    info = info->next;
  }
  return(0);
}
