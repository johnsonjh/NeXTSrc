#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <varargs.h>
#include <a.out.h>
#include <sys/loader.h>
#define OBJC_SYM "__OBJC_SYMBOLS"
#define OBJC_MOD "__OBJC_MODULES"
#define OBJC_SEL "__OBJC_STRINGS"
#include <symseg.h>

/* The structures for mach relocatables */
struct mach_header		header;
struct segment_command		reloc_segment;
struct section			text_section;	/* __TEXT, __text */
struct section			data_section;	/* __DATA, __data */
struct section			bss_section;	/* __DATA, __bss */
struct section			sym_section;	/* __OBJC, __symbol_table */
struct section			mod_section;	/* __OBJC, __module_info */
struct section			sel_section;	/* __OBJC, __selector_strs */
struct symtab_command		symbol_table;
struct symseg_command		symbol_segment;

/*
 * The data relocation entried which are looked through to divide up into the
 * objective-C section's relocation entries and the data section's.
 */
struct relocation_info *data_reloc;
long data_reloff, data_nreloc;

a_out_to_mach_O(a_out, size_a_out, file_name, gdbsize, gdbpointer)
     char *a_out;
     int size_a_out;
     char *file_name;
     int gdbsize;
     char *gdbpointer;
{
  char *p, *ap;
  long header_size, fileoff, strsize;
  struct exec *exec;
  struct relocation_info *preloc;
  struct scattered_relocation_info *psreloc;
  unsigned long r_address;
  struct nlist *pnlist;
  struct symbol_root *psymbol_root;
  struct mach_root *pmach_root, mach_root;
  
  exec = (struct exec *)a_out;

  /*
   * For converted a.out files the resulting Mach-O object file will
   * look like:
   *
   *	mach header
   *	mach load commands
   *	the text segment
   *	the data segment
   *	the objective-C segment
   *	the relocation entries
   *	the 4.3bsd symbol table
   *	the 4.3bsd string table
   *	the gdb symbol segments
   */
  
  header.magic = MH_MAGIC;
  header.cputype = CPU_TYPE_MC680x0;
  header.cpusubtype = CPU_SUBTYPE_MC68030;
  header.filetype = MH_OBJECT;
  header.ncmds = 3;
  header.sizeofcmds =
    sizeof(struct segment_command) +
      6 * sizeof(struct section) +
	sizeof(struct symtab_command) +
	  sizeof(struct symseg_command);
  header.flags = 0;
  header_size = header.sizeofcmds + sizeof(struct mach_header);
  
  reloc_segment.cmd = LC_SEGMENT;
  reloc_segment.cmdsize = sizeof(struct segment_command) +
    6 * sizeof(struct section);
  /* leave reloc_segment.segname full of zeros */
  reloc_segment.vmaddr = 0;
  reloc_segment.vmsize = exec->a_text + exec->a_data + exec->a_bss;
  reloc_segment.fileoff = header_size;
  reloc_segment.filesize = exec->a_text + exec->a_data;
  reloc_segment.maxprot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
  reloc_segment.initprot= VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
  reloc_segment.nsects = 6;
  reloc_segment.flags = 0;

  strcpy(text_section.sectname, SECT_TEXT);
  strcpy(text_section.segname, SEG_TEXT);
  text_section.addr = 0;
  text_section.size = exec->a_text;
  text_section.offset = header_size;
  text_section.align = 1;
  if(exec->a_trsize != 0) {
    text_section.reloff = header_size + exec->a_text + exec->a_data;
    text_section.nreloc = exec->a_trsize / sizeof (struct relocation_info);
  } else {
    text_section.reloff = 0;
    text_section.nreloc = 0;
  }
  text_section.flags = 0;
  text_section.reserved1 = 0;
  text_section.reserved2 = 0;
  
  strcpy(data_section.sectname, SECT_DATA);
  strcpy(data_section.segname, SEG_DATA);
  data_section.addr = exec->a_text;
  data_section.offset = header_size + exec->a_text;
  data_section.align = 2;
  data_section.flags = 0;
  data_section.reserved1 = 0;
  data_section.reserved2 = 0;

  strcpy(sym_section.sectname, SECT_OBJC_SYMBOLS);
  strcpy(sym_section.segname, SEG_OBJC);
  sym_section.align = 2;
  strcpy(mod_section.sectname, SECT_OBJC_MODULES);
  strcpy(mod_section.segname, SEG_OBJC);
  mod_section.align = 2;
  strcpy(sel_section.sectname, SECT_OBJC_STRINGS);
  strcpy(sel_section.segname, SEG_OBJC);
  sel_section.align = 0;
  
  /*
   * The symbols that start each of the objective-C sections have prevoiusly
   * been looked up in write.c and the addresses and sizes of these sections
   * has been set into the section structures.  These addresses are used to
   * to divide up the data section.  The assumptions are as follows:
   *		the data for these are at the end of the data section
   *		they are in the following order (if present)
   *			symbol table section
   *			module information section
   *			selector strings section
   */
  
  /*
   * Now knowing the sizes of all the objective-C sections the size
   * of the data section and the file offsets can be calculated.
   */
  data_section.size = exec->a_data -
    (sel_section.size + sym_section.size + mod_section.size);
  
  sym_section.offset = data_section.offset + data_section.size;
  mod_section.offset =  sym_section.offset + sym_section.size;
  sel_section.offset =  mod_section.offset + mod_section.size;
  
  /*
   * Now the data relocation entries have to be divided up.  This
   * is relying on the fact that the assembler produces relocation
   * entries in decreasing order based on the r_address field.  Note
   * the r_address field is really an offset into the section.
   */
  if(exec->a_drsize != 0) {
    data_reloc = (struct relocation_info *)xmalloc(exec->a_drsize);
    ap = a_out + N_TXTOFF(*exec) + exec->a_text + exec->a_data + exec->a_trsize;
    bcopy(ap, data_reloc, exec->a_drsize);
    data_reloff = header_size + exec->a_text + exec->a_data + exec->a_trsize;
    data_nreloc = exec->a_drsize / sizeof (struct relocation_info);
    
    data_section.reloff = data_reloff;
    data_section.nreloc = 0;
    sym_section.reloff  = data_reloff;
    sym_section.nreloc = 0;
    mod_section.reloff  = data_reloff;
    mod_section.nreloc = 0;
    sel_section.reloff  = data_reloff;
    sel_section.nreloc = 0;
    
    for(preloc = (struct relocation_info *)data_reloc;
	preloc < data_reloc + data_nreloc;
	preloc++){
      if((preloc->r_address & R_SCATTERED) != 0){
         psreloc = (struct scattered_relocation_info *)preloc;
         r_address = psreloc->r_address;
      }
      else
         r_address = preloc->r_address;

      if(r_address >= (data_section.addr + data_section.size) - exec->a_text)
	data_section.reloff += sizeof(struct relocation_info);
      else if(r_address >= data_section.addr - exec->a_text)
	data_section.nreloc++;
      
      if(r_address >= (sym_section.addr + sym_section.size) - exec->a_text)
	sym_section.reloff += sizeof(struct relocation_info);
      else if(r_address >= sym_section.addr - exec->a_text)
	sym_section.nreloc++;
      
      if(r_address >= (mod_section.addr + mod_section.size) - exec->a_text)
	mod_section.reloff += sizeof(struct relocation_info);
      else if(r_address >= mod_section.addr - exec->a_text)
	mod_section.nreloc++;
      
      if(r_address >= (sel_section.addr + sel_section.size) - exec->a_text)
	sel_section.reloff += sizeof(struct relocation_info);
      else if(r_address >= sel_section.addr - exec->a_text)
	sel_section.nreloc++;
    }
  }
  else{
    data_section.reloff = 0;
    data_section.nreloc = 0;
    sym_section.reloff = 0;
    sym_section.nreloc = 0;
    mod_section.reloff = 0;
    mod_section.nreloc = 0;
    sel_section.reloff = 0;
    sel_section.nreloc = 0;
  }
  sym_section.flags = 0;
  sym_section.reserved1 = 0;
  sym_section.reserved2 = 0;

  mod_section.flags = 0;
  mod_section.reserved1 = 0;
  mod_section.reserved2 = 0;

  sel_section.flags = S_CSTRING_LITERALS;
  sel_section.reserved1 = 0;
  sel_section.reserved2 = 0;

  strcpy(bss_section.sectname, SECT_BSS);
  strcpy(bss_section.segname, SEG_DATA);
  bss_section.addr = exec->a_text + exec->a_data;
  bss_section.size = exec->a_bss;
  bss_section.offset = 0;
  bss_section.align = 2;
  bss_section.reloff = 0;
  bss_section.nreloc = 0;
  bss_section.flags = S_ZEROFILL;
  bss_section.reserved1 = 0;
  bss_section.reserved2 = 0;
  
  symbol_table.cmd = LC_SYMTAB;
  symbol_table.cmdsize = sizeof(struct symtab_command);
  symbol_table.symoff = 0;
  symbol_table.nsyms = 0;
  symbol_table.stroff = 0;
  symbol_table.strsize = 0;
  
  symbol_segment.cmd = LC_SYMSEG;
  symbol_segment.cmdsize = sizeof(struct symseg_command);
  symbol_segment.offset = 0;
  symbol_segment.size = 0;
  
  if(exec->a_syms != 0){
    fileoff = header_size + exec->a_text + exec->a_data + exec->a_trsize +
      exec->a_drsize;
    symbol_table.symoff = fileoff;
    symbol_table.nsyms = exec->a_syms / sizeof(struct nlist);
    fileoff += exec->a_syms;
    symbol_table.stroff = fileoff;

    p = a_out + N_STROFF(*exec);
    strsize = *(int *)p;
    symbol_table.strsize = strsize;
    fileoff += strsize;
    
    symbol_segment.cmd = LC_SYMSEG;
    symbol_segment.cmdsize = sizeof(struct symseg_command);
    symbol_segment.offset = fileoff;
    symbol_segment.size = gdbsize;
  }
  
  output_seek(0);
  output_write(&header, sizeof(struct mach_header));
  output_write(&reloc_segment, sizeof(struct segment_command));
  output_write(&text_section, sizeof(struct section));
  output_write(&data_section, sizeof(struct section));
  output_write(&bss_section, sizeof(struct section));
  output_write(&sym_section, sizeof(struct section));
  output_write(&mod_section, sizeof(struct section));
  output_write(&sel_section, sizeof(struct section));
  output_write(&symbol_table, sizeof(struct symtab_command));
  output_write(&symbol_segment, sizeof(struct symseg_command));

  /* leave the pad after the headers */
  output_seek(header_size);

  ap = a_out + N_TXTOFF(*exec);
  output_write(ap, exec->a_text);
  ap += exec->a_text;
  output_write(ap, exec->a_data);
  ap += exec->a_data;

  /*
   * The conversion of r_symbolnum is now done in emit_relocation() in
   * write.c.  This fixed a bug where the item to be relocated is symbol
   * plus offset and here we can't tell what the original symbol's address
   * is and we might use the wrong section number which will result an
   * incorrect relocation entry.
   */

  /* read and write the text relocation entries */
  if(exec->a_trsize != 0){
    p = ap;
    ap += exec->a_trsize;
    output_seek(text_section.reloff);
    output_write(p, exec->a_trsize);
  }
  
  /* read and write the data relocation entries */
  if(exec->a_drsize != 0){
    if (data_reloc != NULL) {
      p = (char *)data_reloc;
    } else {
      p = ap;
    }
    ap += exec->a_drsize;
    for(preloc = (struct relocation_info *)p;
	preloc < (struct relocation_info *)(p + exec->a_drsize);
	preloc++){
      /*
       * Adjust the r_address field (which is an
       * offset) to be an offset into the section that
       * the address it is refering to is in.
       */
      if((preloc->r_address & R_SCATTERED) != 0){
        psreloc = (struct scattered_relocation_info *)preloc;
	if(psreloc->r_address >= sel_section.addr - exec->a_text)
	  psreloc->r_address -=
	    (sel_section.addr - data_section.addr);
	else if(psreloc->r_address >= mod_section.addr - exec->a_text)
	  psreloc->r_address -=
	    (mod_section.addr - data_section.addr);
	else if(psreloc->r_address >= sym_section.addr - exec->a_text)
	  psreloc->r_address -=
	    (sym_section.addr - data_section.addr);
	/* else its in the data section an it's fine as is*/
      }
      else{
	if(preloc->r_address >= sel_section.addr - exec->a_text)
	  preloc->r_address -=
	    (sel_section.addr - data_section.addr);
	else if(preloc->r_address >= mod_section.addr - exec->a_text)
	  preloc->r_address -=
	    (mod_section.addr - data_section.addr);
	else if(preloc->r_address >= sym_section.addr - exec->a_text)
	  preloc->r_address -=
	    (sym_section.addr - data_section.addr);
	/* else its in the data section an it's fine as is*/
      }
    }
    output_seek(data_reloff);
    output_write(p, exec->a_drsize);
  }
  
  /* read and write the symbol table */
  if(exec->a_syms != 0){
    p = ap;
    ap += exec->a_syms;
    /* convert symbol table entries to Mach-O style */
    for(pnlist = (struct nlist *)p;
	pnlist < (struct nlist *)(p + exec->a_syms);
	pnlist++){
      switch(pnlist->n_type & N_TYPE){
      case N_TEXT:
	pnlist->n_sect = 1;
	if((pnlist->n_type & N_STAB) == 0)
	  pnlist->n_type = N_SECT | (pnlist->n_type & N_EXT);
	break;
      case N_DATA:
	{
	  if(pnlist->n_value >= sel_section.addr + sel_section.size)
	    pnlist->n_sect = 2;
	  else if(pnlist->n_value >= sel_section.addr)
	    pnlist->n_sect = 6;
	  else if(pnlist->n_value >= mod_section.addr)
	    pnlist->n_sect = 5;
	  else if(pnlist->n_value >= sym_section.addr)
	    pnlist->n_sect = 4;
	  else
	    pnlist->n_sect = 2;
	}
	if((pnlist->n_type & N_STAB) == 0)
	  pnlist->n_type = N_SECT | (pnlist->n_type & N_EXT);
	break;
      case N_BSS:
	pnlist->n_sect = 3;
	if((pnlist->n_type & N_STAB) == 0)
	  pnlist->n_type = N_SECT | (pnlist->n_type & N_EXT);
	break;
      default:
	pnlist->n_sect = NO_SECT;
      }
    }
    output_seek(symbol_table.symoff);
    output_write(p, exec->a_syms);
  
    /* read and write the string table */
    p = ap;
    ap += strsize;
    output_seek(symbol_table.stroff);
    output_write(p, strsize);
  }
  
  /* read and write the symbol segment */
  if(symbol_segment.size != 0){
    p = ap;
    ap += symbol_segment.size;
    /*
     * Convert only the symbol_root to a mach_symbol_root.
     * This conversion is quite ugly.  It relies on the fact that
     * sizeof(struct symbol_root) - sizeof(struct mach_symbol_root)
     * 	>= sizeof(struct loadmap)
     * It replaces the symbol_root with a mach_symbol_root copying
     * the like fields.  Then it sets the loadmap pointer (offset
     * since it's in a file) to the end of the mach_symbol_root.
     * The load map is really just a struct loadmap field struct
     * with the nmaps field zero for a relocatable object.  It gets
     * the zero by bzero'ing the sizeof the symbol_root.  This also
     * assumes that in an OMAGIC file there is just one symbol_root.
     * I did say this was ugly.
     */
    p = gdbpointer;
    if(symbol_segment.size < sizeof(struct symbol_root))
      as_fatal("Invalid size of gdb symbol segment (smaller than a "
	    "symbol root)");
    psymbol_root = (struct symbol_root *)p;
    mach_root.format = MACH_ROOT_FORMAT;
    mach_root.length = psymbol_root->length;
    mach_root.ldsymoff = psymbol_root->ldsymoff;
    mach_root.filename = psymbol_root->filename;
    mach_root.filedir = psymbol_root->filedir;
    mach_root.blockvector = psymbol_root->blockvector;
    mach_root.typevector = psymbol_root->typevector;
    mach_root.language = psymbol_root->language;
    mach_root.version = psymbol_root->version;
    mach_root.compilation = psymbol_root->compilation;
    mach_root.sourcevector = psymbol_root->sourcevector;
    
    mach_root.loadmap =
      (struct loadmap *)(sizeof(struct mach_root));
    bzero(p, sizeof(struct symbol_root));
    pmach_root = (struct mach_root *)p;
    *pmach_root = mach_root;
    output_seek(symbol_segment.offset);
    output_write(p, symbol_segment.size);
  }
}
