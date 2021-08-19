#ifdef RLD
/*
 * The set structure that holds the information for a set of dynamicly loaded
 * object files.
 */
struct set {
    char *output_addr;		/* the output memory for this set */
    long output_size;		/* the size of the output memory for this set */
    struct object_file		/* the structures for the common symbols of */
	link_edit_common_object;/*  this set that are allocated by rld() */
    struct section_map
	link_edit_section_maps;
    struct section
	link_edit_common_section;
    long narchives;		/* the number of archives loaded in this set */
    struct archive *archives;	/* addresses and sizes of where they are */
};
struct archive {
    char *file_name;	/* name of the archive that is mapped */
    char *file_addr;	/* address the archive is mapped at */
    long file_size;	/* size that is mapped */
};
/*
 * Pointer to the array of set structures.
 */
extern struct set *sets;
/*
 * Index into the array of set structures for the current set.
 */
extern long cur_set;

extern void new_set(void);
extern void new_archive(char *file_name, char *file_addr, long file_size);
extern void remove_set(void);
extern void free_sets(void);
extern void clean_archives(void);

#endif RLD
