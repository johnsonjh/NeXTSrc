/* NXVersion.h: interface to software release versioning module */

/* For now, link with static library /usr/local/lib/libVersion.a */

/*
 * NXExternalSoftwareRelease returns a read-only string that describes the
 * external version of the NeXT software that is installed on this machine
 * (e.g "2.0"). The string does not contain any newline characters.  
 * Returns "" if any errors are encountered.
 */
 
extern const char* NXExternalSoftwareRelease(void);


/*
 * NXInternalSoftwareRelease returns a read-only string that describes the
 * internal version of the NeXT software that is installed on this machine
 * (e.g "Warp3J"). The string does not contain any newline characters.  
 * Returns "" if any errors are encountered.
 */
 
extern const char* NXInternalSoftwareRelease(void);


