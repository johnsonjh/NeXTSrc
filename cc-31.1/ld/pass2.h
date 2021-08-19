/*
 * The total size of the output file and the memory buffer for the output file.
 */
extern long output_size;
extern char *output_addr;

/*
 * This is used to setting the SG_NORELOC flag in the segment flags correctly.
 * See the comments in the file pass2.c where this is defined.
 */
extern struct merged_section **output_sections;

extern void pass2(void);
extern void output_flush(long offset, long size);
