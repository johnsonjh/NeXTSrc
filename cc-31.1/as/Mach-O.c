#ifdef Mach_O
/*
 * This file provides the means that so that GAS can put out Mach-O object files
 * with more than just text, data, and bss sections.  It does this by using
 * sub-segments.  The major limitation is that short references can't be used
 * between sub-segments or this will break.  So do NOT create two sub-segments
 * that contain instructions that branch to each other.
 */
#include <sys/loader.h>
#undef SEG_TEXT
#undef SEG_DATA
#include "as.h"
#include "subsegs.h"
#include "struc-symbol.h"
#include "symbols.h"
#include "write.h"
#include <stab.h>

extern int global_forty_flag;

/*
 * The last align 2 frag in the text section set in write_object_file() and
 * used here to pad the text section with a nop instruction rather than two
 * bytes of zero that is not a full instruction.
 */
struct frag *frag_nop = NULL;

static void fix_to_relocation_info(
    struct fix *fixP,
    unsigned long segment_address_in_file,
    struct relocation_info *riP,
    long noscattered);
static long round(
    long v,
    unsigned long r);

/* This is the nsect that the text sub-segments start at */
#define TEXT_NSECT	(1)
/* This is the nsect that the data sub-segments start at */
#define DATA_NSECT	(TEXT_NSECT+8)
/* This is the nsect for the bss which follows all the data sub-segments*/
#define BSS_NSECT	(DATA_NSECT+15)

/* The array to translate nsects to the nsects in the output file.  This is
 * used to remove unused sections from the output file.  It is directly indexed
 * with nsect (which starts at 1).
 */
static long new_nsect[BSS_NSECT + 1] = { 0 };

/*
 * The array to hold the sections that will actually be in the output file.
 * Indexed from 0 to nsects-1.
 */
static struct section *sections[BSS_NSECT];

/*
 * The segments and sub_segment are fixed.  The text, data and bss sections
 * are assumed always to be there (as the assembler allways creates them).
 * 
 *    GAS segment     Gas sub_segment	     Mach-O section number and name
 * 	SEG_ABS		n/a			0  NO_SECT
 *	SEG_TEXT	0	     TEXT_NSECT+0  (__TEXT,__text)
 *	SEG_TEXT	1	     TEXT_NSECT+1  (__TEXT,__const)
 *	SEG_TEXT	2	     TEXT_NSECT+2  (__TEXT,__static_const)
 *	SEG_TEXT	3	     TEXT_NSECT+3  (__TEXT,__cstring)
 *	SEG_TEXT	4	     TEXT_NSECT+4  (__TEXT,__literal4)
 *	SEG_TEXT	5	     TEXT_NSECT+5  (__TEXT,__literal8)
 *	SEG_TEXT	6	     TEXT_NSECT+6  (__TEXT,__fvmlib_init0)
 *	SEG_TEXT	7	     TEXT_NSECT+7  (__TEXT,__fvmlib_init1)
 *	SEG_DATA	0	     DATA_NSECT+0  (__DATA,__data)
 *	SEG_DATA	1	     DATA_NSECT+1  (__DATA,__static_data)
 *	SEG_DATA	2	     DATA_NSECT+2  (__OBJC,__class)
 *	SEG_DATA	3	     DATA_NSECT+3  (__OBJC,__meta_class)
 *	SEG_DATA	4	     DATA_NSECT+4  (__OBJC,__cat_cls_meth)
 *	SEG_DATA	5	     DATA_NSECT+5  (__OBJC,__cat_inst_meth)
 *	SEG_DATA	6	     DATA_NSECT+6  (__OBJC,__cls_meth)
 *	SEG_DATA	7	     DATA_NSECT+7  (__OBJC,__inst_meth)
 *	SEG_DATA	8	     DATA_NSECT+8  (__OBJC,__selector_refs)
 *	SEG_DATA	9	     DATA_NSECT+9  (__OBJC,__symbols)
 *	SEG_DATA	10	     DATA_NSECT+10 (__OBJC,__category)
 *	SEG_DATA	11	     DATA_NSECT+11 (__OBJC,__class_vars)
 *	SEG_DATA	12	     DATA_NSECT+12 (__OBJC,__instance_vars)
 *	SEG_DATA	13	     DATA_NSECT+13 (__OBJC,__module_info)
 *	SEG_DATA	14	     DATA_NSECT+14 (__OBJC,__selector_strs)
 *	SEG_BSS		n/a	      BSS_NSECT+0  (__DATA,__bss)
 */

/*
 * seg_n_sect() returns the value of the n_sect field (Mach-O section number)
 * for the specified segment and sub-segment.  This is used in calls to
 * new_symbol to set the n_sect (n_other) field.
 */
unsigned char
seg_n_sect(
    segT seg,
    int subseg)
{
	if(seg == SEG_ABSOLUTE)
	    return(0);
	if(seg == SEG_TEXT){
	    if(subseg < 0 || subseg > DATA_NSECT)
		as_fatal("Bad subseg (%d) for SEG_TEXT to seg_n_sect()\n",
			 subseg);
	    return(TEXT_NSECT + subseg);
	}
	if(seg == SEG_DATA){
	    if(subseg < 0 || subseg > BSS_NSECT)
		as_fatal("Bad subseg (%d) for SEG_DATA to seg_n_sect()\n",
			 subseg);
	    return(DATA_NSECT + subseg);
	}
	if(seg == SEG_BSS)
	    return(BSS_NSECT);
	as_fatal("Bad seg (%d) to seg_n_sect()\n", seg);
}

/*
 * n_type_n_sect() returns the value of the n_sect field (Mach-O section number)
 * for the specified N_TYPE bits of the n_type field.  This is used only in
 * setting the the n_sect (n_other) field for stabs from stab() in read.c.
 */
unsigned char
n_type_n_sect(
    int n_type)
{
	switch(n_type & N_TYPE){
	case N_TEXT:
	    return(TEXT_NSECT);
	case N_DATA:
	    return(DATA_NSECT);
	case N_BSS:
	    return(BSS_NSECT);
	case N_ABS:
	default:
	    return(0);
	}
}
/*
 * write_Mach_O() writes a Mach-O object file directly from the GAS data
 * structures using the segments and sub-segments as Mach-O sections.
 */
void
write_Mach_O(
long string_byte_count,
long symbol_number)
{
    /* The structures for Mach-O relocatables */
    struct mach_header		header;
    struct segment_command	reloc_segment;
    struct symtab_command	symbol_table;
    long			i, nsects;
    struct section	text_section;		/* __TEXT, __text */
    struct section	const_section;		/* __TEXT, __const */
    struct section	static_const_section;	/* __TEXT, __static_const */
    struct section	cstring_section;	/* __TEXT, __cstring */
    struct section	literal4_section;	/* __TEXT, __literal4 */
    struct section	literal8_section;	/* __TEXT, __literal8 */
    struct section	fvmlib_init0_section;	/* __TEXT, __fvmlib_init0 */
    struct section	fvmlib_init1_section;	/* __TEXT, __fvmlib_init1 */
    struct section	data_section;		/* __DATA, __data */
    struct section	static_data_section;	/* __DATA, __static_data */
    struct section	class_section;		/* __OBJC, __class */
    struct section	meta_class_section;	/* __OBJC, __meta_class */
    struct section	cat_cls_meth_section;	/* __OBJC, __cat_cls_meth */
    struct section	cat_inst_meth_section;	/* __OBJC, __cat_inst_meth */
    struct section	cls_meth_section;	/* __OBJC, __cls_meth */
    struct section	inst_meth_section;	/* __OBJC, __inst_meth */
    struct section	selector_refs_section;	/* __OBJC, __selector_refs */
    struct section	symbols_section;	/* __OBJC, __symbols */
    struct section	category_section;	/* __OBJC, __category */
    struct section	class_vars_section;	/* __OBJC, __class_vars */
    struct section	instance_vars_section;	/* __OBJC, __instance_vars */
    struct section	module_info_section;	/* __OBJC, __module_info */
    struct section	selector_strs_section;	/* __OBJC, __selector_strs */
    struct section	bss_section;		/* __DATA, __bss */

    /* locals to fill in section struct fields */
    unsigned long addr, size, offset, address, zero;

    /* The GAS data structures */
    struct frchain *frchainP;
    struct symbol *symbolP;
    struct frag *fragP;
    struct fix *fixP;

    char *the_object_file;
    unsigned long the_object_file_size;
    char *next_object_file_charP;

	/* zero all fields in the section structures for starters */
	memset(&text_section,		'\0', sizeof(struct section));
	memset(&const_section,		'\0', sizeof(struct section));
	memset(&static_const_section,	'\0', sizeof(struct section));
	memset(&cstring_section,	'\0' ,sizeof(struct section));
	memset(&literal4_section,	'\0', sizeof(struct section));
	memset(&literal8_section,	'\0', sizeof(struct section));
	memset(&fvmlib_init0_section,	'\0', sizeof(struct section));
	memset(&fvmlib_init1_section,	'\0', sizeof(struct section));
	memset(&data_section,    	'\0', sizeof(struct section));
	memset(&static_data_section,   	'\0', sizeof(struct section));
	memset(&class_section,		'\0', sizeof(struct section));
	memset(&meta_class_section,	'\0', sizeof(struct section));
	memset(&cat_cls_meth_section,	'\0', sizeof(struct section));
	memset(&cat_inst_meth_section,	'\0', sizeof(struct section));
	memset(&cls_meth_section,	'\0', sizeof(struct section));
	memset(&inst_meth_section,	'\0', sizeof(struct section));
	memset(&selector_refs_section,	'\0', sizeof(struct section));
	memset(&symbols_section,	'\0', sizeof(struct section));
	memset(&category_section,	'\0', sizeof(struct section));
	memset(&class_vars_section,	'\0', sizeof(struct section));
	memset(&instance_vars_section,	'\0', sizeof(struct section));
	memset(&module_info_section,	'\0', sizeof(struct section));
	memset(&selector_strs_section,	'\0', sizeof(struct section));
	memset(&bss_section,      	'\0', sizeof(struct section));

	nsects = 0;
	/* 
	 * Fill in the addr and size fields of each section structure from the
	 * corresponding segment and sub-segment (only the first subsegment is
	 * always there the others may be missing).
	 */
	for(frchainP = frchain_root;
	    frchainP != NULL;
	    frchainP = frchainP->frch_next){

	    addr = frchainP->frch_root->fr_address;
	    size = frchainP->frch_last->fr_address -
		   frchainP->frch_root->fr_address;

	    /*
	    printf("frchainP = 0x%x frch_seg = %d frch_subseg = %d ",
		    frchainP, frchainP->frch_seg, frchainP->frch_subseg);
	    printf("addr 0x%x size 0x%x\n", addr, size);
	    */

	    switch(frchainP->frch_seg){
	    case SEG_TEXT:
		switch(frchainP->frch_subseg){
		case 0:
		    sections[nsects] = &text_section;
		    break;
		case 1:
		    sections[nsects] = &const_section;
		    break;
		case 2:
		    sections[nsects] = &static_const_section;
		    break;
		case 3:
		    sections[nsects] = &cstring_section;
		    break;
		case 4:
		    sections[nsects] = &literal4_section;
		    break;
		case 5:
		    sections[nsects] = &literal8_section;
		    break;
		case 6:
		    sections[nsects] = &fvmlib_init0_section;
		    break;
		case 7:
		    sections[nsects] = &fvmlib_init1_section;
		    break;
		default:
		    as_fatal("Bad subseg (%d) for SEG_TEXT in write_Mach_O()\n",
			     frchainP->frch_subseg);
		}
		new_nsect[TEXT_NSECT + frchainP->frch_subseg] = nsects + 1;
		break;
	    case SEG_DATA:
		switch(frchainP->frch_subseg){
		case 0:
		    sections[nsects] = &data_section;
		    break;
		case 1:
		    sections[nsects] = &static_data_section;
		    break;
		case 2:
		    sections[nsects] = &class_section;
		    break;
		case 3:
		    sections[nsects] = &meta_class_section;
		    break;
		case 4:
		    sections[nsects] = &cat_cls_meth_section;
		    break;
		case 5:
		    sections[nsects] = &cat_inst_meth_section;
		    break;
		case 6:
		    sections[nsects] = &cls_meth_section;
		    break;
		case 7:
		    sections[nsects] = &inst_meth_section;
		    break;
		case 8:
		    sections[nsects] = &selector_refs_section;
		    break;
		case 9:
		    sections[nsects] = &symbols_section;
		    break;
		case 10:
		    sections[nsects] = &category_section;
		    break;
		case 11:
		    sections[nsects] = &class_vars_section;
		    break;
		case 12:
		    sections[nsects] = &instance_vars_section;
		    break;
		case 13:
		    sections[nsects] = &module_info_section;
		    break;
		case 14:
		    sections[nsects] = &selector_strs_section;
		    break;
		default:
		    as_fatal("Bad subseg (%d) for SEG_DATA in write_Mach_O()\n",
			     frchainP->frch_subseg);
		}
		new_nsect[DATA_NSECT + frchainP->frch_subseg] = nsects + 1;
		break;
	    default:
		as_fatal("Bad seg (%d) in write_Mach_O()\n",frchainP->frch_seg);
	    }
	    sections[nsects]->addr = addr;
	    sections[nsects]->size = size;
	    nsects++;
	}
	new_nsect[BSS_NSECT] = nsects + 1;
	bss_section.addr = bss_address_frag.fr_address;
	bss_section.size = local_bss_counter;
	sections[nsects++] = &bss_section;

	/* fill in the Mach-O header */
	header.magic = MH_MAGIC;
	header.cputype = CPU_TYPE_MC680x0;
	if (global_forty_flag)
	  header.cpusubtype = CPU_SUBTYPE_MC68040;
	else
	  header.cpusubtype = CPU_SUBTYPE_MC68030;
	header.filetype = MH_OBJECT;
	header.ncmds = 2;
	header.sizeofcmds = sizeof(struct segment_command) +
			    nsects * sizeof(struct section) +
			    sizeof(struct symtab_command);
	header.flags = 0;

	/* fill in the segment command */
	memset(&reloc_segment, '\0', sizeof(struct segment_command));
	reloc_segment.cmd = LC_SEGMENT;
	reloc_segment.cmdsize = sizeof(struct segment_command) +
				nsects * sizeof(struct section);
	/* leave reloc_segment.segname full of zeros */
	reloc_segment.vmaddr = text_section.addr;
	reloc_segment.vmsize = 0;
	for(i = 0; i < nsects; i++)
	    reloc_segment.vmsize += sections[i]->size;
	offset = header.sizeofcmds + sizeof(struct mach_header);
	reloc_segment.fileoff = offset;
	reloc_segment.filesize = reloc_segment.vmsize - bss_section.size;
	reloc_segment.maxprot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
	reloc_segment.initprot= VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
	reloc_segment.nsects = nsects;
	reloc_segment.flags = 0;

	/* fill in the segment and section names */
	strcpy(text_section.segname,          "__TEXT");
	strcpy(text_section.sectname,          "__text");
	strcpy(const_section.segname,         "__TEXT");
	strcpy(const_section.sectname,         "__const");
	strcpy(static_const_section.segname,  "__TEXT");
	strcpy(static_const_section.sectname,  "__static_const");
	strcpy(cstring_section.segname,       "__TEXT");
	strcpy(cstring_section.sectname,       "__cstring");
	strcpy(literal4_section.segname,      "__TEXT");
	strcpy(literal4_section.sectname,      "__literal4");
	strcpy(literal8_section.segname,      "__TEXT");
	strcpy(literal8_section.sectname,      "__literal8");
	strcpy(fvmlib_init0_section.segname,  "__TEXT");
	strcpy(fvmlib_init0_section.sectname,  "__fvmlib_init0");
	strcpy(fvmlib_init1_section.segname,  "__TEXT");
	strcpy(fvmlib_init1_section.sectname,  "__fvmlib_init1");
	strcpy(data_section.segname,          "__DATA");
	strcpy(data_section.sectname,          "__data");
	strcpy(static_data_section.segname,   "__DATA");
	strcpy(static_data_section.sectname,   "__static_data");
	strcpy(class_section.segname,         "__OBJC");
	strcpy(class_section.sectname,         "__class");
	strcpy(meta_class_section.segname,    "__OBJC");
	strcpy(meta_class_section.sectname,    "__meta_class");
	strcpy(cat_cls_meth_section.segname,  "__OBJC");
	strcpy(cat_cls_meth_section.sectname,  "__cat_cls_meth");
	strcpy(cat_inst_meth_section.segname, "__OBJC");
	strcpy(cat_inst_meth_section.sectname, "__cat_inst_meth");
	strcpy(cls_meth_section.segname,      "__OBJC");
	strcpy(cls_meth_section.sectname,      "__cls_meth");
	strcpy(inst_meth_section.segname,     "__OBJC");
	strcpy(inst_meth_section.sectname,     "__inst_meth");
	strcpy(selector_refs_section.segname, "__OBJC");
	strcpy(selector_refs_section.sectname, "__message_refs");
	strcpy(symbols_section.segname,       "__OBJC");
	strcpy(symbols_section.sectname,       "__symbols");
	strcpy(category_section.segname,      "__OBJC");
	strcpy(category_section.sectname,      "__category");
	strcpy(class_vars_section.segname,    "__OBJC");
	strcpy(class_vars_section.sectname,    "__class_vars");
	strcpy(instance_vars_section.segname, "__OBJC");
	strcpy(instance_vars_section.sectname, "__instance_vars");
	strcpy(module_info_section.segname,   "__OBJC");
	strcpy(module_info_section.sectname,   "__module_info");
	strcpy(selector_strs_section.segname, "__OBJC");
	strcpy(selector_strs_section.sectname, "__selector_strs");
	strcpy(bss_section.segname,           "__DATA");
	strcpy(bss_section.sectname,           "__bss");

	/*
	 * Fill in the offset to the contents of the sections and the deafult
	 * alignment.
	 */
	for(i = 0; i < nsects; i++){
	    sections[i]->offset = offset;
	    sections[i]->align = 2;
	    offset += sections[i]->size;
	}

	/* fill in the align field of the sections that are not the default */
	text_section.align          = 1;
	cstring_section.align       = 0;
	literal8_section.align      = 3;
	selector_strs_section.align = 0;

	/* count the number of relocation entries for each section */
	for(fixP = text_fix_root;  fixP != NULL;  fixP = fixP->fx_next){
	    address = fixP->fx_frag->fr_address + fixP->fx_where;
	    for(i = 0; i < nsects; i++){
		if(address >= sections[i]->addr &&
		   address < sections[i]->addr + sections[i]->size){
		    sections[i]->nreloc++;
		    break;
		}
	    }
	}
	for(fixP = data_fix_root;  fixP != NULL;  fixP = fixP->fx_next){
	    address = fixP->fx_frag->fr_address + fixP->fx_where;
	    for(i = 0; i < nsects; i++){
		if(address >= sections[i]->addr &&
		   address < sections[i]->addr + sections[i]->size){
		    sections[i]->nreloc++;
		    break;
		}
	    }
	}

	offset = round(offset, sizeof(long));
	/* fill in the offset to the relocation entries of the sections */
	for(i = 0; i < nsects; i++){
	    sections[i]->reloff = offset;
	    offset += sections[i]->nreloc * sizeof(struct relocation_info);
	}

	/* fill in the flags field of the sections */
	cstring_section.flags       = S_CSTRING_LITERALS;
	literal4_section.flags      = S_4BYTE_LITERALS;
	literal8_section.flags      = S_8BYTE_LITERALS;
	bss_section.flags           = S_ZEROFILL;
	selector_refs_section.flags = S_LITERAL_POINTERS;
	selector_strs_section.flags = S_CSTRING_LITERALS;

	/* fill in the fields of the symtab_command */
	symbol_table.cmd = LC_SYMTAB;
	symbol_table.cmdsize = sizeof(struct symtab_command);
	symbol_table.symoff = offset;
	symbol_table.nsyms = symbol_number;
	offset += symbol_table.nsyms * sizeof(struct nlist);
	symbol_table.stroff = offset;
	symbol_table.strsize = string_byte_count;
	offset += symbol_table.strsize;

	/* allocate the buffer for the output file */
	the_object_file_size = offset;
	the_object_file = xmalloc(the_object_file_size);
	next_object_file_charP = the_object_file;

	/* put the headers in the output file's buffer */
	append(&next_object_file_charP,
	       &header,
	       sizeof(struct mach_header));
	append(&next_object_file_charP,
	       &reloc_segment,
	       sizeof(struct segment_command));
	for(i = 0; i < nsects; i++)
	    append(&next_object_file_charP, sections[i],sizeof(struct section));
	append(&next_object_file_charP,
	       &symbol_table,
	       sizeof(struct symtab_command));

	/*
	 * To get only instructions in the (__TEXT,__text) section the section
	 * must not be padded with something that is not an instruction (or a
	 * full instruction as .word 0 is not a full instruction but rather half
	 * of an "orbi #0x??,d0".  So what was done in write_object_file() was
	 * to first align to a 2 byte boundary (the will not do anything if only
	 * text is in the text section) then align to a 4 byte boundary saving
	 * a handle to the frag (in frag_nop).  This is used here to put out a
	 * nop instruction for it in place of two zeros if it has size of 2
	 * (not 0).  This is a major kludge in the fact it is using 2 bytes of
	 * fr_literal and relying on the fact that frags are not allocated on
	 * one byte boundaries.
	 */
	if(frag_nop->fr_offset == 2){
	    /* convert this frag from a 2 char variable fill frag to a 2 char
	       fixed frag */
	    frag_nop->fr_offset = 0;
	    frag_nop->fr_var = 0;
	    frag_nop->fr_fix = 2;
	    frag_nop->fr_literal[0] = 0x4e; /* 0x4e71 is a nop instruction */
	    frag_nop->fr_literal[1] = 0x71;
	}
	/* put the section contents in the output file's buffer */
	for(fragP = frchain_root->frch_root;
	    fragP != NULL;
	    fragP = fragP->fr_next){

	    long count;
	    char *fill_literal;
	    long fill_size;

	    know(fragP->fr_type == rs_fill);
	    append(&next_object_file_charP, fragP->fr_literal, fragP->fr_fix);
	    fill_literal = fragP->fr_literal + fragP->fr_fix;
	    fill_size = fragP->fr_var;
	    know(fragP->fr_offset >= 0);
	    for(count = fragP->fr_offset; count != 0 ;  count--)
		append(&next_object_file_charP, fill_literal, fill_size);
	}

	/* put the relocation entries in the output file's buffer */
	for(i = 0; i < nsects; i++)
	    sections[i]->nreloc = 0;
	for(fixP = text_fix_root;  fixP != NULL;  fixP = fixP->fx_next){
	    address = fixP->fx_frag->fr_address + fixP->fx_where;
	    for(i = 0; i < nsects; i++){
		if(address >= sections[i]->addr &&
		   address < sections[i]->addr + sections[i]->size){
		    fix_to_relocation_info(fixP, sections[i]->addr,
			    (struct relocation_info *)(the_object_file +
			    sections[i]->reloff +
			    sections[i]->nreloc *
			    sizeof(struct relocation_info)),
			    0);
		    sections[i]->nreloc++;
		    break;
		}
	    }
	}
	for(fixP = data_fix_root;  fixP != NULL;  fixP = fixP->fx_next){
	    address = fixP->fx_frag->fr_address + fixP->fx_where;
	    for(i = 0; i < nsects; i++){
		if(address >= sections[i]->addr &&
		   address < sections[i]->addr + sections[i]->size){
		    /*
		     * For the (_OBJC,__selector_strs) section the compiler
		     * allways places exactly one symbol with one string in
		     * this section and uses symbol+offset to get at the
		     * individual strings in the large string.  So references
		     * into this section are never made scattered because they
		     * are really refering to separate symbols (that are not
		     * put out) and not to the first symbol plus an offset
		     * outside it's block.  So this is the last prameter to
		     * fix_to_relocation_info.
		     */
		    fix_to_relocation_info(fixP, sections[i]->addr,
			    (struct relocation_info *)(the_object_file +
			    sections[i]->reloff +
			    sections[i]->nreloc *
			    sizeof(struct relocation_info)),
			    sections[i] == &selector_refs_section);
		    sections[i]->nreloc++;
		    break;
		}
	    }
	}

	/* put the symbols in the output file's buffer */
	next_object_file_charP = the_object_file + symbol_table.symoff;
	for(symbolP = symbol_rootP;
	    symbolP != NULL;
	    symbolP = symbolP->sy_next){

	    register char *temp;

	    temp = symbolP->sy_nlist.n_un.n_name;
	    symbolP->sy_nlist.n_un.n_strx = symbolP->sy_name_offset;
	    /* Any undefined symbols become N_EXT. */
	    if(symbolP->sy_type == N_UNDF)
		symbolP->sy_type |= N_EXT;
	    switch((symbolP->sy_type & N_TYPE)){
		case N_TEXT:
		case N_DATA:
		case N_BSS:
		    symbolP->sy_other = new_nsect[symbolP->sy_other];
		    if((symbolP->sy_type & N_STAB) == 0){
			symbolP->sy_type = N_SECT | (symbolP->sy_type & N_EXT);
		    }
		    break;
	    }
	    append(&next_object_file_charP, (char *)(&symbolP->sy_nlist),
	           sizeof(struct nlist));
	    symbolP->sy_nlist.n_un.n_name = temp;
	}

	/* put the strings in the output file's buffer */
	zero = 0;
	append(&next_object_file_charP,
	       (char *)&zero,
	       (unsigned long)sizeof(long));
	for(symbolP = symbol_rootP;
	    symbolP != NULL;
	    symbolP = symbolP->sy_next){
	    if(symbolP->sy_name != NULL){
		/* Ordinary case: not .stabd. */
		append(&next_object_file_charP,
		       symbolP->sy_name,
		       (unsigned long)(strlen(symbolP->sy_name) + 1));
	    }
	}

	/* create the output file */
	output_file_create(out_file_name);
	output_write(the_object_file, the_object_file_size);
	free(the_object_file);
	output_file_close(out_file_name);
}
#endif Mach_O

/*
 * From a fix structure create a relocation entry.
 */
static
void
fix_to_relocation_info(
struct fix *fixP,
unsigned long segment_address_in_file,
struct relocation_info *riP,
long noscattered)
{
    struct scattered_relocation_info sri;
    struct symbol *symbolP;

	memset(riP, '\0', sizeof(struct relocation_info));
	symbolP = fixP->fx_addsy;
	if(symbolP != NULL){
	    switch(fixP->fx_size){
		case 1:
		    riP->r_length = 0;
		    break;
		case 2:
		    riP->r_length = 1;
		    break;
		case 4:
		    riP->r_length = 2;
		    break;
		default:
		    as_fatal("Bad fx_size (0x%x) in fix_to_relocation_info()\n",
			     fixP->fx_size);
	    }
	    riP->r_pcrel = fixP->fx_pcrel;
	    riP->r_address = fixP->fx_frag->fr_address + fixP->fx_where -
			   segment_address_in_file;
	    if((symbolP->sy_type & N_TYPE) == N_UNDF){
		riP->r_extern = 1;
		riP->r_symbolnum = symbolP->sy_number;
	    }
	    else{
		riP->r_extern = 0;
		riP->r_symbolnum = new_nsect[symbolP->sy_other];
		/*
		 * To allow the link editor to scatter the contents of a
		 * section a local relocation can't be used when an offset
		 * is added to the symbol's value.  For the link editor to
		 * get the relocation right when it divides up a section
		 * that has an item to be relocated to a symbol plus an
		 * offset the link editor needs to know the value of the
		 * symbol (before the offset is added) as what to base the
		 * relocation on.  If this wasn't done and the value of the
		 * symbol plus offset expression did not happen to be in the
		 * same block as the value of symbol the link editor would
		 * get the relocation wrong.  Since it would be difficult
		 * to determine the lengths of the blocks in here the
		 * assumption is that any non-zero offset would fall outside
		 * the block that contains the symbol.  This could be done
		 * for all local relocation entries but since there is no
		 * room in a scattered relocation entry to place the section
		 * ordinal and the symbol value they are more expensive in
		 * the link editor to process since the section ordinal
		 * needs to be "looked up" from the value.  So these are
		 * used only when necessary.  (Also see the comments in
		 * <reloc.h>).
		 */
		if((fixP->fx_offset != 0 &&
		    (fixP->fx_offset != 4 || !(fixP->fx_pcrel & 2)) ) &&
		   ((symbolP->sy_type & N_TYPE) & ~N_EXT) != N_ABS &&
		    (symbolP->sy_other != DATA_NSECT+14 &&
		     symbolP->sy_other != DATA_NSECT+8)){
		    memset(&sri, '\0',sizeof(struct scattered_relocation_info));
		    sri.r_scattered = 1;
		    sri.r_length    = riP->r_length;
		    sri.r_pcrel     = riP->r_pcrel;
		    sri.r_address   = riP->r_address;
		    sri.r_value     = symbolP->sy_value;
		    *riP = *((struct relocation_info *)&sri);
		}
	    }
	}
}

/*
 * round() rounds v to a multiple of r.
 */
static
long
round(
long v,
unsigned long r)
{
	r--;
	v += r;
	v &= ~(long)r;
	return(v);
}
