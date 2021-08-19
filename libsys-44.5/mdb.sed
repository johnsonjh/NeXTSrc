#
# sed file for changing spec_sys in to spec_sys_mdb
#

/MALLOC_DEBUG/s/^##//
/NON-MALLOC_DEBUG/s/^/##/
