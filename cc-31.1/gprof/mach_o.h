struct mach_segment_map {
  char *segname;
  char *native_vmaddr;
  int size;
  char *local_vmaddr;
};

struct mach_thread_info {
  struct NeXT_thread_state_regs *regs;
  struct NeXT_thread_state_68882 *fpregs;
};

struct mach_o_info {
  /* Pointer to mach header (also base of mapped file) */
  struct mach_header *mach_header;
  /* Pointer to the load commands */
  struct load_command *load_commands;
  /* Size of mapped file */
  int mach_size;
  /* # of mapped segments and pointer to table of mach_segment_map entries */
  int nsegs;
  struct mach_segment_map *segs;
  /* # of threads (most useful for core files) and pointer to table of info */
  int nthreads;
  struct mach_thread_info *threads;
  /* # of nlist entries and pointer to them. */
  unsigned long nsyms;
  char *syms;
  /* # of bytes in string table and pointer to the base */
  unsigned long strsize;
  char *strings;
  /* size of symseg segment and pointer to the base */
  unsigned long symsegsize;
  char *symsegs;
  /* Pointer to any info structs loaded with this image (e.g. shared libs) */
  struct mach_o_info *next;
};

extern struct mach_o_info *execinfo;
struct mach_o_info *read_mach_o();
