/*
 * NetInfo serialization routines
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include <sys/errno.h>
#include "ni_serial.h"
#include "mm.h"
#include "system.h"
#include "clib.h"
#include "safe_stdio.h"

/*
 * Decode a NetInfo object from the given file pointer, allocating
 * memory as necessary.
 */
ni_status
ser_decode(
	   FILE *f,
	   long offset,
	   ni_object **obj
	   )
{
	XDR xdr;
	ni_status status;

	xdrstdio_create(&xdr, f, XDR_DECODE);
	if (!xdr_setpos(&xdr, offset)) {
		status = NI_SERIAL;
		goto done;
	}
	MM_ALLOC(*obj);
	MM_ZERO(*obj);
	if (!xdr_ni_object(&xdr, *obj)) {
		MM_FREE(*obj);
		status = NI_SERIAL;
	} else {
		status = NI_OK;
	}
done:
	xdr_destroy(&xdr);
	return (status);
}

/*
 * Encode the given NetInfo object into a file with the given name. 
 * Returns the size and a FILE pointer to the newly created file
 */
ni_status
ser_encode(
	   ni_object *obj,
	   char *fname,
	   long *size,
	   FILE **f
	   )
{
	XDR xdr;

	
	*f = safe_fopen(fname, "w+");
	if (*f == NULL) {
		return (NI_SERIAL);
	}
	xdrstdio_create(&xdr, *f, XDR_ENCODE);
	if (!xdr_ni_object(&xdr, obj)) {
		sys_errmsg("cannot serialize file: %m");
		xdr_destroy(&xdr);
		safe_fclose(*f);
		unlink(fname);
		return (errno == ENOSPC ? NI_NOSPACE : NI_SERIAL);
	}
	*size = xdr_getpos(&xdr);
	xdr_destroy(&xdr);
	fseek(*f, 0, 0);
	return (NI_OK);
}


/*
 * Compute the byte-size of a property.
 * This is tricky code!
 */
static long
prop_size(
	  ni_property *prop
	  )
{
	long size;
	long len;
	ni_index i;

	/*
	 * sizeof(prop->nip_name)
	 */
	len = strlen(prop->nip_name);
	size = BYTES_PER_XDR_UNIT + RNDUP(len);

	/*
	 * sizeof(prop->nip_val.ninl_len)
	 */
	size += BYTES_PER_XDR_UNIT;

	for (i = 0; i < prop->nip_val.ninl_len; i++) {
		/*
		 * sizeof(prop->nip_val.ninl_val[i])
		 */
		len = strlen(prop->nip_val.ninl_val[i]);
		size += BYTES_PER_XDR_UNIT + RNDUP(len);
	}

	/*
	 * equals the total storage used by the property
	 */
	return (size);
}

/*
 * Compute the byte-size of a NetInfo object
 * This is tricky code too! 
 */
ni_status
ser_size(
	 ni_object *obj,
	 long *size
	 )
{
	ni_index i;

	/*
	 * sizeof(obj->nio_id.nii_object) +
	 * sizeof(obj->nio_id.nii_instance) +
	 * sizeof(obj->nio_parent)
	 */
	*size = 3 * BYTES_PER_XDR_UNIT;

	/*
	 * sizeof(obj->nio_props.nipl_len)
	 */
	*size += BYTES_PER_XDR_UNIT;
	for (i = 0; i < obj->nio_props.nipl_len; i++) {
		/*
		 * sizeof(obj->nio_props.nipl_val[i])
		 */
		*size += prop_size(&obj->nio_props.nipl_val[i]);
	}

	/*
	 * sizeof(obj->nio_children.niil_len)
	 */
	*size += BYTES_PER_XDR_UNIT;

	/*
	 * sizeof(obj->nio_children.niil_val)
	 */
	*size += (BYTES_PER_XDR_UNIT * obj->nio_children.niil_len);

	/*
	 * equals the total size of the object
	 */
	return (NI_OK); /* returns a status for historical reasons */
}

/*
 * Fast-encoding does not worry about crash-recovery, it just serializes
 * directly into the file (no temp file).  Only used for database tranfers.
 */
ni_status
ser_fastencode(
	       FILE *f,
	       ni_object *obj
	       )
{
	XDR xdr;
	ni_status status;

	xdrstdio_create(&xdr, f, XDR_ENCODE);
	if (!xdr_ni_object(&xdr, obj)) {
		status = NI_NOSPACE;
	} else {
		status = NI_OK;
	}
	xdr_destroy(&xdr);
	return (status);
}

/*
 * Free a NetInfo object
 */
ni_status
ser_free(
	 ni_object *obj
	 )
{
	bool_t stat;

	stat = xdr_free(xdr_ni_object, obj);
	MM_FREE(obj);
	return (stat ? NI_OK : NI_SERIAL);
}


