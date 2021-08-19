/* NXVersion.c: implements the software release version module */

#import <stdio.h>
#import <libc.h>

#define MAX_DESC_SIZE 512
static char external_version[MAX_DESC_SIZE];
static char internal_version[MAX_DESC_SIZE];
static char version_file[] = "/private/adm/software_version";

static void
GetVersions()
{
	/*
	 * read the string from the file each time, so that changes to the
	 * file contents are reflected immediately to clients.
	 */
	
	FILE* v_file = fopen(version_file, "r");
	
	if (v_file != NULL) {
		char* nl;
		if (fgets(external_version, MAX_DESC_SIZE, v_file) == NULL)
			goto error;
		else {
			nl = index(external_version, '\n');
			if (nl)
				*nl = '\0';
		}
		if (fgets(internal_version, MAX_DESC_SIZE, v_file) == NULL)
			goto error;
		else {
			nl = index(internal_version, '\n');
			if (nl)
				*nl = '\0';
		}
		goto done;
	}
	
error:
	internal_version[0] = '\0';
	external_version[0] = '\0';
done:
	if (v_file)
		fclose(v_file);
}

const char*
NXExternalSoftwareRelease()
{
	GetVersions();
	return external_version;
}

const char*
NXInternalSoftwareRelease()
{
	GetVersions();
	return internal_version;
}

