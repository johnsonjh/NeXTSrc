#ifdef SHLIB
#include "shlib.h"
#endif SHLIB
/*
 * This file contains the routines that deal with literal 'C' string sections.
 * A string in this section must beable to me moved freely with respect to other
 * strings or data.  This means relocation must not reach outside the string and
 * things like: "abc"[i+20] can't be in this type of section.  Also strings
 * like: "foo\0bar" can not be in this type of section.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/loader.h>
#include <mach.h>

#include "ld.h"
#include "objects.h"
#include "sections.h"
#include "cstring_literals.h"
#include "pass2.h"
#include "hash_string.h"

/*
 * cstring_merge() merges cstring literals from the specified section in the
 * current object file (cur_obj).  It allocates a fine relocation map and
 * sets the fine_relocs field in the section_map to it (as well as the count).
 */
void
cstring_merge(
struct cstring_data *data,
struct merged_section *ms,
struct section *s,
struct section_map *section_map)
{
    long ncstrings, i;
    char *cstrings, *p;
    struct fine_reloc *fine_relocs;
    
	if(s->size == 0){
	    section_map->fine_relocs = NULL;
	    section_map->nfine_relocs = 0;
	    return;
	}
	/*
	 * Count the number of strings so the size of the fine relocation
	 * structures can be allocated.
	 */
	ncstrings = 0;
	cstrings = cur_obj->obj_addr + s->offset;
	if(*(cstrings + s->size - 1) != '\0'){
	    error_with_cur_obj("literal C string section (%0.16s,%0.16s) does "
			       "not end with a '\0'");
	    return;
	}
	for(p = cstrings; p < cstrings + s->size; p += strlen(p) + 1)
	    ncstrings++;
#ifdef DEBUG
	data->nfiles++;
	data->nbytes += s->size;
	data->ninput_strings += ncstrings;
#endif DEBUG

	fine_relocs = allocate(ncstrings * sizeof(struct fine_reloc));

	/*
	 * lookup and enter each C string in the section and record the offsets
	 * in the input file and in the output file.
	 */
	p = cstrings;
	for(i = 0; i < ncstrings; i++){
	    fine_relocs[i].input_offset = p - cstrings;
	    fine_relocs[i].output_offset = lookup_cstring(p, data, ms);
	    p += strlen(p) + 1;
	}
	section_map->fine_relocs = fine_relocs;
	section_map->nfine_relocs = ncstrings;
}

/*
 * cstring_order() enters cstring literals from the order_file from the merged
 * section structure.  Since this is called before any call to cstring_merge
 * and it enters the strings in the order of the file it causes the section
 * to be ordered.
 */
void
cstring_order(
struct cstring_data *data,
struct merged_section *ms)
{
    long i, line_number, line_length, max_line_length;
    char *buffer;
    kern_return_t r;

	/*
	 * Parse the load order file by changing '\n' to '\0'.  Also check for
	 * '\0 in the file and flag them as errors.  Also determine the maximum
	 * line length of the file for the needed buffer to allocate for
	 * character translation.
	 */
	line_number = 1;
	line_length = 1;
	max_line_length = 1;
	for(i = 0; i < ms->order_size; i++){
	    if(ms->order_addr[i] == '\0'){
		fatal("format error in -sectorder file: %s line %d character "
		      "possition %d for section (%0.16s,%0.16s) (illegal null "
		      "character \'\\0\' found)", ms->order_filename,
		      line_number, line_length, ms->s.segname, ms->s.sectname);
	    }
	    if(ms->order_addr[i] == '\n'){
		ms->order_addr[i] = '\0';
		if(line_length > max_line_length)
		    max_line_length = line_length;
		line_length = 1;
	    }
	    else
		line_length++;
	}

	/*
	 * Allocate the buffer to translate the order file lines' escape
	 * characters into real characters.
	 */
	buffer = allocate(max_line_length + 1);

	/*
	 * Process each line in the order file by translating all escape
	 * characters and then entering the cstring using lookup_cstring().
	 */
	line_number = 1;
	for(i = 0; i < ms->order_size; i++){
	    get_cstring_from_sectorder(ms, &i, buffer, line_number, 1);
	    (void)lookup_cstring(buffer, data, ms);
	    line_number++;
	}

	/* deallocate the buffer */
	free(buffer);

	/*
	 * Deallocate the memory for the load order file now that it is
	 * nolonger needed (since the memory has been written on it is
	 * allways deallocated so it won't get written to the swap file
	 * unnecessarily).
	 */
	if((r = vm_deallocate(task_self(), (vm_address_t)
	    ms->order_addr, ms->order_size)) != KERN_SUCCESS)
	    mach_fatal(r, "can't vm_deallocate() memory for -sectorder "
		       "file: %s for section (%0.16s,%0.16s)",
		       ms->order_filename, ms->s.segname,
		       ms->s.sectname);
	ms->order_addr = NULL;
}

/*
 * get_cstring_from_sectorder() parses a cstring from a order file for the
 * specified merged_section, ms, starting from the index, *index the order file
 * must have had its newlines changed to '\0's previouly.  It places the parsed
 * cstring in the specified buffer, buffer and advances the index over the
 * cstring it parsed.  line_number and char_pos are used for printing error
 * messages and refer the line_number and character possition the index is at.
 */
void
get_cstring_from_sectorder(
struct merged_section *ms,
long *index,
char *buffer,
long line_number,
long char_pos)
{
    long i, j, k, char_value;
    char octal[4], hex[9];

	j = 0;
	/*
	 * See that this is not the end of a line in the order file.
	 */
	for(i = *index; i < ms->order_size && ms->order_addr[i] != '\0'; i++){
	    /*
	     * See if this character the start of an escape sequence.
	     */
	    if(ms->order_addr[i] == '\\'){
		if(i + 1 >= ms->order_size || ms->order_addr[i + 1] == '\0')
		    fatal("format error in -sectorder file: %s line %d "
			  "character possition %d for section (%0.16s,"
			  "%0.16s) (\'\\\' at the end of the line)",
			  ms->order_filename, line_number, char_pos,
			  ms->s.segname, ms->s.sectname);
		/* move past the '\\' */
		i++;
		char_pos++;
		if(ms->order_addr[i] >= '0' && ms->order_addr[i] <= '7'){
		    /* 1, 2 or 3 octal digits */
		    k = 0;
		    octal[k++] = ms->order_addr[i];
		    char_pos++;
		    if(i+1 < ms->order_size &&
		       ms->order_addr[i+1] >= '0' &&
		       ms->order_addr[i+1] <= '7'){
			octal[k++] = ms->order_addr[++i];
			char_pos++;
		    }
		    if(i+1 < ms->order_size &&
		       ms->order_addr[i+1] >= '0' &&
		       ms->order_addr[i+1] <= '7'){
			octal[k++] = ms->order_addr[++i];
			char_pos++;
		    }
		    octal[k] = '\0';
		    char_value = strtol(octal, NULL, 8);
		    if(char_value > CHAR_MAX){
			error("format error in -sectorder file: %s line %d "
			      "for section (%0.16s,%0.16s) (escape sequence"
			      " ending at character possition %d out of "
			      "range for character)", ms->order_filename,
			      line_number, ms->s.segname, ms->s.sectname,
			      char_pos - 1);
		    }
		    buffer[j++] = (char)char_value;
		}
		else{
		    switch(ms->order_addr[i]){
		    case 'n':
			buffer[j++] = '\n';
			char_pos++;
			break;
		    case 't':
			buffer[j++] = '\t';
			char_pos++;
			break;
		    case 'v':
			buffer[j++] = '\v';
			char_pos++;
			break;
		    case 'b':
			buffer[j++] = '\b';
			char_pos++;
			break;
		    case 'r':
			buffer[j++] = '\r';
			char_pos++;
			break;
		    case 'f':
			buffer[j++] = '\f';
			char_pos++;
			break;
		    case 'a':
			buffer[j++] = '\a';
			char_pos++;
			break;
		    case '\\':
			buffer[j++] = '\\';
			char_pos++;
			break;
		    case '\?':
			buffer[j++] = '\?';
			char_pos++;
			break;
		    case '\'':
			buffer[j++] = '\'';
			char_pos++;
			break;
		    case '\"':
			buffer[j++] = '\"';
			char_pos++;
			break;
		    case 'x':
			/* hex digits */
			k = 0;
			while(i+1 < ms->order_size &&
			      ((ms->order_addr[i+1] >= '0' &&
				ms->order_addr[i+1] <= '9') ||
			       (ms->order_addr[i+1] >= 'a' &&
				ms->order_addr[i+1] <= 'f') ||
			       (ms->order_addr[i+1] >= 'A' &&
				ms->order_addr[i+1] <= 'F')) ){
			    if(k <= 8)
				hex[k++] = ms->order_addr[++i];
			    else
				++i;
			    char_pos++;
			}
			if(k > 8){
			    error("format error in -sectorder file: %s line"
				  " %d for section (%0.16s,%0.16s) (hex "
				  "escape ending at character possition "
				  "%d out of range)", ms->order_filename,
				  line_number, ms->s.segname,
				  ms->s.sectname, char_pos);
			    break;
			}
			hex[k] = '\0';
			char_value = strtol(hex, NULL, 16);
			if(char_value > CHAR_MAX){
			    error("format error in -sectorder file: %s line"
				  " %d for section (%0.16s,%0.16s) (escape "
				  "sequence ending at character possition "
				  "%d out of range for character)",
				  ms->order_filename, line_number,
				  ms->s.segname, ms->s.sectname, char_pos);
			}
			buffer[j++] = (char)char_value;
			char_pos++;
			break;
		    default:
			error("format error in -sectorder file: %s line %d "
			      "for section (%0.16s,%0.16s) (unknown escape "
			      "sequence ending at character possition %d)",
			      ms->order_filename, line_number,
			      ms->s.segname, ms->s.sectname, char_pos);
			buffer[j++] = ms->order_addr[i];
			char_pos++;
			break;
		    }
		}
	    }
	    /*
	     * This character is not the start of an escape sequence so take
	     * it as it is.
	     */
	    else{
		buffer[j] = ms->order_addr[i];
		char_pos++;
		j++;
	    }
	}
	buffer[j] = '\0';
	*index = i;
}

/*
 * lookup_cstring() looks up the cstring passed to it in the cstring_data
 * passed to it and returns the offset the cstring will have in the output
 * file.  It creates the hash table as needed and the blocks to store the
 * strings and attaches them to the cstring_data passed to it.  The total
 * size fo the section is accumulated in ms->s.size which is the merged
 * section for this literal section.  The string is aligned to the alignment
 * in the merged section (ms->s.align).
 */
long
lookup_cstring(
char *cstring,
struct cstring_data *data,
struct merged_section *ms)
{
    long hashval, len, cstring_len;
    struct cstring_bucket *bp;
    struct cstring_block **p, *cstring_block;

	if(data->hashtable == NULL){
	    data->hashtable = allocate(sizeof(struct cstring_bucket *) *
				       CSTRING_HASHSIZE);
	    memset(data->hashtable, '\0', sizeof(struct cstring_bucket *) *
					  CSTRING_HASHSIZE);
	}
#if defined(DEBUG) && defined(PROBE_COUNT)
	    data->nprobes++;
#endif
	hashval = hash_string(cstring) % CSTRING_HASHSIZE;
	for(bp = data->hashtable[hashval]; bp; bp = bp->next){
	    if(strcmp(cstring, bp->cstring) == 0)
		return(bp->offset);
#if defined(DEBUG) && defined(PROBE_COUNT)
	    data->nprobes++;
#endif
	}

	cstring_len = strlen(cstring) + 1;
	len = round(cstring_len, 1 << ms->s.align);
	bp = allocate(sizeof(struct cstring_bucket));
	for(p = &(data->cstring_blocks); *p ; p = &(cstring_block->next)){
	    cstring_block = *p;
	    if(cstring_block->full)
		continue;
	    if(len > cstring_block->size - cstring_block->used){
		cstring_block->full = 1;
		continue;
	    }
	    strcpy(cstring_block->cstrings + cstring_block->used, cstring);
	    memset(cstring_block->cstrings + cstring_block->used + cstring_len,
		   '\0', len - cstring_len);
	    bp->cstring = cstring_block->cstrings + cstring_block->used;
	    cstring_block->used += len;
	    bp->offset = ms->s.size;
	    bp->next = data->hashtable[hashval];
	    data->hashtable[hashval] = bp;
	    ms->s.size += len;
#ifdef DEBUG
	    data->noutput_strings++;
#endif DEBUG
	    return(bp->offset);
	}
	*p = allocate(sizeof(struct cstring_block));
	cstring_block = *p;
	cstring_block->size = (len > host_pagesize ? len : host_pagesize);
	cstring_block->used = len;
	cstring_block->full = (len == cstring_block->size ? TRUE : FALSE);
	cstring_block->next = NULL;
	cstring_block->cstrings = allocate(cstring_block->size);
	strcpy(cstring_block->cstrings, cstring);
	memset(cstring_block->cstrings + cstring_len, '\0', len - cstring_len);
	bp->cstring = cstring_block->cstrings;
	bp->offset = ms->s.size;
	bp->next = data->hashtable[hashval];
	data->hashtable[hashval] = bp;
	ms->s.size += len;
#ifdef DEBUG
	data->noutput_strings++;
#endif DEBUG
	return(bp->offset);
}

/*
 * cstring_output() copies the cstrings for the data passed to it into the 
 * output file's buffer.  The pointer to the merged section passed to it is
 * used to tell where in the output file this section goes.  Then this routine
 * called cstring_free() to free() up all space used by this data block except
 * the data block itself.
 */
void
cstring_output(
struct cstring_data *data,
struct merged_section *ms)
{
    long offset;
    struct cstring_block **p, *cstring_block;

	/*
	 * Copy the blocks into the output file.
	 */
	offset = ms->s.offset;
	for(p = &(data->cstring_blocks); *p ;){
	    cstring_block = *p;
	    memcpy(output_addr + offset,
		   cstring_block->cstrings,
		   cstring_block->used);
	    offset += cstring_block->used;
	    p = &(cstring_block->next);
	}
#ifndef RLD
	output_flush(ms->s.offset, offset - ms->s.offset);
#endif !defined(RLD)
	cstring_free(data);
}

/*
 * cstring_free() free()'s up all space used by this cstring_data block except
 * the data block itself.
 */
void
cstring_free(
struct cstring_data *data)
{
    long i;
    struct cstring_bucket *bp, *next_bp;
    struct cstring_block *cstring_block, *next_cstring_block;

	/*
	 * Free all data for this block.
	 */
	if(data->hashtable != NULL){
	    for(i = 0; i < CSTRING_HASHSIZE; i++){
		for(bp = data->hashtable[i]; bp; ){
		    next_bp = bp->next;
		    free(bp);
		    bp = next_bp;
		}
	    }
	    free(data->hashtable);
	    data->hashtable = NULL;
	}
	for(cstring_block = data->cstring_blocks; cstring_block ;){
	    next_cstring_block = cstring_block->next;
	    free(cstring_block->cstrings);
	    free(cstring_block);
	    cstring_block = next_cstring_block;
	}
	data->cstring_blocks = NULL;
}

#ifdef DEBUG
/*
 * print_cstring_data() prints a cstring_data.  Used for debugging.
 */
void
print_cstring_data(
struct cstring_data *data,
char *indent)
{
    char *s;
    struct cstring_block **p, *cstring_block;
/*
    long i;
    struct cstring_bucket *bp;
*/

	print("%sC string data at 0x%x\n", indent, data);
	if(data == NULL)
	    return;
	print("%s    hashtable 0x%x\n", indent, data->hashtable);
/*
	if(data->hashtable != NULL){
	    for(i = 0; i < CSTRING_HASHSIZE; i++){
		print("%s    %-3d [0x%x]\n", indent, i, data->hashtable[i]);
		for(bp = data->hashtable[i]; bp; bp = bp->next){
		    print("%s\tcstring %s\n", indent, bp->cstring);
		    print("%s\toffset  %d\n", indent, bp->offset);
		    print("%s\tnext    0x%x\n", indent, bp->next);
		}
	    }
	}
*/
	print("%s   cstring_blocks 0x%x\n", indent, data->cstring_blocks);
	for(p = &(data->cstring_blocks); *p ; p = &(cstring_block->next)){
	    cstring_block = *p;
	    print("%s\tsize %d\n", indent, cstring_block->size);
	    print("%s\tused %d\n", indent, cstring_block->used);
	    if(cstring_block->full)
		print("%s\tfull TRUE\n", indent);
	    else
		print("%s\tfull FALSE\n", indent);
	    print("%s\tnext 0x%x\n", indent, cstring_block->next);
	    print("%s\tcstrings\n", indent);
	    for(s = cstring_block->cstrings;
	        s < cstring_block->cstrings + cstring_block->used;
	        s += strlen(s) + 1){
		print("%s\t    %s\n", indent, s);
	    }
	}
}

/*
 * cstring_data_stats() prints the cstring_data stats.  Used for tuning.
 */
void
cstring_data_stats(
struct cstring_data *data,
struct merged_section *ms)
{
	if(data == NULL)
	    return;
	print("literal cstring section (%0.16s,%0.16s) contains:\n",
	      ms->s.segname, ms->s.sectname);
	print("    %d bytes of merged strings\n", ms->s.size);
	print("    from %d files and %d total bytes from those "
	      "files\n", data->nfiles, data->nbytes);
	print("    average number of bytes per file %g\n",
	      (double)((double)data->nbytes / (double)(data->nfiles)));
	print("    %d merged strings\n", data->noutput_strings);
	print("    from %d files and %d total strings from those "
	      "files\n", data->nfiles, data->ninput_strings);
	print("    average number of strings per file %g\n",
	      (double)((double)data->ninput_strings / (double)(data->nfiles)));
	if(data->nprobes != 0){
	    print("    number of hash probes %d\n", data->nprobes);
	    print("    average number of hash probes %g\n",
	    (double)((double)(data->nprobes) / (double)(data->ninput_strings)));
	}
}
#endif DEBUG
