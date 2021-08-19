/*
 * The prefix to the symbol names for branch table slots in target shared
 * libraries. The rest of the symbol name is digits.  This symbol does not
 * appear in host shared libraries and thus does not appear in executables
 * that use them.
 */
#define BRANCH_SLOT_NAME	".branch_table_slot_"
