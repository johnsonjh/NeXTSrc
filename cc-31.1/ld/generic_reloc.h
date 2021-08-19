/*
 * Global types, variables and routines declared in the file generic_reloc.c.
 *
 * The following include file need to be included before this file:
 * #include <reloc.h>
 * #include "section.h"
 */
void generic_reloc(char *contents, struct relocation_info *relocs,
		   struct section_map *map);
int undef_bsearch(const *index,
		  const struct undefined_map *undefined_map);
