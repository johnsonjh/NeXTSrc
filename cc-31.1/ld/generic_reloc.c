#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
 * This file contains the routines to do generic relocation.  Which can be used
 * for such machines that have simple 1, 2, and 4 byte relocation lengths.
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/loader.h>
#include <reloc.h>
#include <nlist.h>
#include <mach.h>

#include "ld.h"
#include "objects.h"
#include "sections.h"
#include "symbols.h"
#include "pass2.h"
#include "generic_reloc.h"

/*
 * generic_reloc() relocates the contents of the specified section for the 
 * relocation entries using the section map from the current object (cur_obj).
 */
void
generic_reloc(
char *contents,
struct relocation_info *relocs,
struct section_map *section_map)
{
    long i, j, symbolnum, value, input_pc, output_pc;
    struct nlist *nlists;
    struct undefined_map *undefined_map;
    struct merged_symbol *merged_symbol;
    struct section_map *local_map;
    struct relocation_info *reloc;
    struct scattered_relocation_info *sreloc;
    unsigned long r_address, r_symbolnum, r_pcrel, r_length, r_extern,
		  r_scattered, r_value;
    long offset;

#ifdef DEBUG
	/*
	 * The compiler "warnings: `merged_symbol', `local_map' and `offset'
	 * may be used uninitialized in this function" can safely be ignored
	 */
	merged_symbol = NULL;
	local_map = NULL;
	offset = 0;
#endif DEBUG

	for(i = 0; i < section_map->s->nreloc; i++){
	    /*
	     * Break out the fields of the relocation entry and set pointer to
	     * the type of relocation entry it is (for updating later).
	     */
	    if((relocs[i].r_address & R_SCATTERED) != 0){
		sreloc = (struct scattered_relocation_info *)(relocs + i);
		reloc = NULL;
		r_scattered = 1;
		r_address = sreloc->r_address;
		r_pcrel = sreloc->r_pcrel;
		r_length = sreloc->r_length;
		r_value = sreloc->r_value;
		r_extern = 0;
		/* calculate the r_symbolnum (n_sect) from the r_value */
		r_symbolnum = 0;
		for(j = 0; j < cur_obj->nsection_maps; j++){
		    if(r_value >= cur_obj->section_maps[j].s->addr &&
		       r_value < cur_obj->section_maps[j].s->addr +
				 cur_obj->section_maps[j].s->size){
			r_symbolnum = j + 1;
			break;
		    }
		}
		if(r_symbolnum == 0){
		    error_with_cur_obj("r_value (0x%x) field of relocation "
			"entry %d in section (%0.16s,%0.16s) out of range",
			r_value, i, section_map->s->segname,
			section_map->s->sectname);
		    return;
		}
	    }
	    else{
		reloc = relocs + i;
		sreloc = NULL;
		r_scattered = 0;
		r_address = reloc->r_address;
		r_pcrel = reloc->r_pcrel;
		r_length = reloc->r_length;
		r_extern = reloc->r_extern;
		r_symbolnum = reloc->r_symbolnum;
		r_value = 0;
	    }
	    /*
	     * The r_address field is really an offset into the contents of the
	     * section and must reference something inside the section.
	     */
	    if(r_address < 0 || r_address >= section_map->s->size){
		error_with_cur_obj("r_address (0x%x) field of relocation entry "
		    "%d in section (%0.16s,%0.16s) out of range", r_address, i,
		    section_map->s->segname, section_map->s->sectname);
		return;
	    }
	    /*
	     * If r_extern is set this relocation entry is an external entry
	     * else it is a local entry (or scattered entry).
	     */
	    if(r_extern){
		/*
		 * This is an external relocation entry.  So the value to be
		 * added to the item to be relocated is the value of the symbol.
		 * r_symbolnum is an index into the input file's symbol table
		 * of the symbol being refered to.  The symbol must be an
		 * undefined symbol to be used in an external relocation entry.
		 */
		if(r_symbolnum >= cur_obj->symtab->nsyms){
		    error_with_cur_obj("r_symbolnum (%d) field of external "
			"relocation entry %d in section (%0.16s,%0.16s) out of "
			"range", r_symbolnum, i, section_map->s->segname,
			section_map->s->sectname);
		    return;
		}
		symbolnum = r_symbolnum;
		undefined_map = bsearch(&symbolnum, cur_obj->undefined_maps,
		    cur_obj->nundefineds, sizeof(struct undefined_map),
		    (int (*)(const void *, const void *))undef_bsearch);
		if(undefined_map == NULL){
		    nlists = (struct nlist *)(cur_obj->obj_addr +
					      cur_obj->symtab->symoff);
		    if((nlists[symbolnum].n_type & N_EXT) != N_EXT){
			error_with_cur_obj("r_symbolnum (%d) field of external "
			    "relocation entry %d in section (%0.16s,%0.16s) "
			    "refers to a non-external symbol", symbolnum, i,
			    section_map->s->segname, section_map->s->sectname);
			return;
		    }
		    if(nlists[symbolnum].n_type != (N_EXT | N_UNDF)){
			error_with_cur_obj("r_symbolnum (%d) field of external "
			    "relocation entry %d in section (%0.16s,%0.16s) "
			    "refers to a non-undefined symbol", symbolnum, i,
			    section_map->s->segname, section_map->s->sectname);
			return;
		    }
		    print_obj_name(cur_obj);
		    fatal("internal error, in generic_reloc() symbol index %d "
			"in above file not in undefined map", symbolnum);
		}
		merged_symbol = undefined_map->merged_symbol;
		/*
		 * If this is an indirect symbol resolve indirection (all chains
		 * of indirect symbols have been resolved so that they point at
		 * a symbol that is not an indirect symbol).
		 */
		if(merged_symbol->nlist.n_type == (N_EXT | N_INDR))
		    merged_symbol = (struct merged_symbol *)
				    merged_symbol->nlist.n_value;
		/*
		 * If the symbol is undefined (or common) no relocation is done.
		 */
		if(merged_symbol->nlist.n_type == (N_EXT | N_UNDF))
		    value = 0;
		else{
		    value = merged_symbol->nlist.n_value;
#ifdef mc68000
		    /*
		     * To know which type (local or scattered) of relocation
		     * entry to convert this one to (if relocation entries are
		     * saved) the offset to be added to the symbol's value is
		     * needed to see if it reaches outside the block in which
		     * the symbol is in.  In here if the offset is not zero then
		     * it is assumed to reach out of the block and a scattered
		     * relocation entry is used.
		     * 
		     * On real machines the alignment of: contents + r_address
		     * would have to be checked with respect to the r_length to
		     * make sure we don't cause a fault here.  But for now it
		     * works on the mc68000 machines.
		     */
		    switch(r_length){
		    case 0: /* byte */
			offset = *(char *)(contents + r_address);
			break;
		    case 1: /* word (2 byte) */
			offset = *(short *)(contents + r_address);
			break;
		    case 2: /* long (4 byte) */
			offset = *(long *)(contents + r_address);
			break;
		    default:
			/* the error check is catched below */
			break;
		    }
#else !mc68000
#error "generic_reloc() must be updated for non-mc68000 machines"
#endif
		}

		if(merged_symbol->nlist.n_type == (N_EXT | N_SECT))
		    output_sections[merged_symbol->nlist.n_sect]->referenced =
									   TRUE;
	    }
	    else{
		/*
		 * This is a local relocation entry (the value to which the item
		 * to be relocated is refering to is defined in section number
		 * r_symbolnum).  So the address of that section in the input
		 * file is subtracted and the value of that section in the
		 * output is added to the item being relocated.
		 */
		value = 0;
		/*
		 * If the symbol is not in any section the value to be added to
		 * the item to be relocated is the zero above and any pc
		 * relative change in value added below.
		 */
		if(r_symbolnum != R_ABS){
		    if(r_symbolnum > cur_obj->nsection_maps){
			error_with_cur_obj("r_symbolnum (%d) field of local "
			    "relocation entry %d in section (%0.16s,%0.16s) "
			    "out of range", r_symbolnum, i,
			    section_map->s->segname, section_map->s->sectname);
			return;
		    }
		    local_map = &(cur_obj->section_maps[r_symbolnum - 1]);
		    local_map->output_section->referenced = TRUE;
		    if(local_map->nfine_relocs == 0){
			value = - local_map->s->addr
				+ local_map->output_section->s.addr +
				  local_map->offset;
		    }
		    else{
			/*
			 * For items to be relocated that refer to a section
			 * with fine relocation the value is set (not adjusted
			 * with addition).  So the new value is directly
			 * calculated from the old value.
			 */
			if(r_pcrel){
			    input_pc = section_map->s->addr +
				       r_address;
			    if(section_map->nfine_relocs == 0)
				output_pc = section_map->output_section->s.addr
					    + section_map->offset +
					    r_address;
			    else
				output_pc = section_map->output_section->s.addr
					    + 
					fine_reloc_output_offset(section_map,
								 r_address);
			}
			else{
			    input_pc = 0;
			    output_pc = 0;
			}
			/*
			 * Get the value of the expresion of the item to be
			 * relocated (errors of r_length are checked later).
			 */
			switch(r_length){
			case 0:
			    value = *(char *)(contents + r_address);
			    break;
			case 1:
			    value = *(short *)(contents + r_address);
			    break;
			case 2:
			    value = *(long *)(contents + r_address);
			    break;
			}
			/*
			 * If the relocation entry is not a scattered relocation
			 * entry then the relocation is based on the value of
			 * value of the expresion of the item to be relocated.
			 * If it is a scattered relocation entry then the 
			 * relocation is based on the r_value in the relocation
			 * entry and the offset part of the expression at the
			 * item to be relocated is extracted so it can be
			 * added after the relocation is done.
			 */
			value += input_pc;
			if(r_scattered == 0){
			    r_value = value;
			    offset = 0;
			}
			else{
			    offset = value - r_value;
			}
			value = local_map->output_section->s.addr +
				fine_reloc_output_offset(local_map,
				     r_value - local_map->s->addr)
				- output_pc + offset;
			switch(r_length){
			case 0:
			    *(char *)(contents + r_address) = value;
			    break;
			case 1:
			    *(short *)(contents + r_address) = value;
			    break;
			case 2:
			    *(long *)(contents + r_address) = value;
			    break;
			default:
			    error_with_cur_obj("r_length field of relocation "
				"entry %d in section (%0.16s,%0.16s) invalid",
				i, section_map->s->segname,
				section_map->s->sectname);
			    return;
			}
			goto update_reloc;
		    }
		}
	    }
	    if(r_pcrel){
		/*
		 * This is a relocation entry is also pc relative which means
		 * the value of the pc will get added to it when it is executed.
		 * The item being relocated has the value of the pc in the input
		 * file subtracted from it.  So to relocate this the value of
		 * pc in the input file is added and then value of the output
		 * pc is subtracted (since the offset into the section remains
		 * constant it is not added in and then subtracted out).
		 */
		if(section_map->nfine_relocs == 0)
		    value += + section_map->s->addr /* + r_address */
			     - (section_map->output_section->s.addr +
				section_map->offset /* + r_address */);
		else
		    value += + section_map->s->addr + r_address
			     - (section_map->output_section->s.addr +
			        fine_reloc_output_offset(section_map,
							 r_address));
	    }
#ifdef mc68000
	    /*
	     * On real machines the alignment of: contents + r_address
	     * would have to be checked with respect to the r_length to make
	     * sure we don't cause a fault here.  But for now it works on the
	     * mc68000 machines.
	     */
	    switch(r_length){
	    case 0: /* byte */
		*(char *)(contents + r_address) += value;
		break;
	    case 1: /* word (2 byte) */
		*(short *)(contents + r_address) += value;
		break;
	    case 2: /* long (4 byte) */
		*(long *)(contents + r_address) += value;
		break;
	    default:
		error_with_cur_obj("r_length field of relocation entry %d in "
		    "section (%0.16s,%0.16s) invalid", i,
		    section_map->s->segname, section_map->s->sectname);
		return;
	    }
#else !mc68000
#error "generic_reloc() must be updated for non-mc68000 machines"
#endif
	    /*
	     * If relocation entries are to be saved in the output file then
	     * update the entry for the output file.
	     */
update_reloc:
	    ;
#ifndef RLD
	    if(save_reloc){
		if(r_extern){
		    /*
		     * For external relocation entries that the symbol is
		     * defined (not undefined or common) it is changed to a
		     * local relocation entry using the section that symbol is
		     * defined in.  If the still undefined then the index of the
		     * symbol in the output file is set into r_symbolnum.
		     */
		    if(merged_symbol->nlist.n_type != (N_EXT | N_UNDF)){
			reloc->r_extern = 0;
			/*
			 * If this symbol was in the base file then no futher
			 * relocation can ever be done (the symbols in the base
			 * file are fixed). Or if the symbol was an absolute
			 * symbol.
			 */
			if(merged_symbol->definition_object == base_obj ||
			   (merged_symbol->nlist.n_type & N_TYPE) == N_ABS){
				reloc->r_symbolnum = R_ABS;
			}
			else{
			    /*
			     * The symbol that this relocation entry is refering
			     * to is defined so convert this external relocation
			     * entry into a local or scattered relocation entry.
			     * If the item to be relocated has an offset added
			     * to the symbol's value make it a scattered
			     * relocation entry else make it a local relocation
			     * entry.
			     */
			    if(offset == 0){
				reloc->r_symbolnum =merged_symbol->nlist.n_sect;
			    }
			    else{
				sreloc = (struct scattered_relocation_info *)
					 reloc;
				r_scattered = 1;
				sreloc->r_scattered = r_scattered;
				sreloc->r_address = r_address;
				sreloc->r_pcrel = r_pcrel;
				sreloc->r_length = r_length;
				sreloc->r_reserved = 0;
				sreloc->r_value = merged_symbol->nlist.n_value;
			    }
			}
		    }
		    else{
			reloc->r_symbolnum =
				      merged_symbol_output_index(merged_symbol);
		    }
		}
		else if(r_scattered == 0){
		    /*
		     * For local relocation entries the section number is
		     * changed to the section number in the output file.
		     */
		    if(reloc->r_symbolnum != R_ABS)
			reloc->r_symbolnum =
				      local_map->output_section->output_sectnum;
		}
		else{
		    /*
		     * For scattered relocation entries the r_value field is
		     * relocated.
		     */
		    if(local_map->nfine_relocs == 0)
			sreloc->r_value += - local_map->s->addr
					   + local_map->output_section->s.addr +
					   local_map->offset;
		    else
			sreloc->r_value = fine_reloc_output_offset(local_map,
				     		r_value - local_map->s->addr)
					   + local_map->output_section->s.addr +
					   local_map->offset;
		}
		/*
		 * If this section that the reloation is being done for has fine
		 * relocation then the offset in the r_address field has to be
		 * set to where it will end up in the output file.  Otherwise
		 * it simply has to have the offset to where this contents
		 * appears in the output file. 
		 */
		if(r_scattered == 0){
		    if(section_map->nfine_relocs == 0){
			reloc->r_address += section_map->offset;
		    }
		    else{
			reloc->r_address = fine_reloc_output_offset(section_map,
								    r_address);
		    }
		}
		else{
		    if(section_map->nfine_relocs == 0){
			sreloc->r_address += section_map->offset;
		    }
		    else{
			sreloc->r_address =fine_reloc_output_offset(section_map,
								    r_address);
		    }
		}
	    }
#endif !defined(RLD)
	}
}

int
undef_bsearch(
const *index,
const struct undefined_map *undefined_map)
{
	return(*index - undefined_map->index);
}
