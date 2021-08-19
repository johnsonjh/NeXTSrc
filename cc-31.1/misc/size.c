static	char *sccsid = "@(#)size.c	4.4 (Berkeley) 4/22/85";
/*
 * size
 */

#include <stdio.h>
#include <a.out.h>
#include <sys/loader.h>
#define PAGESIZE 8192

int header = 0;
int nfiles = 0;
int err = 0;
int mflag = 0;
int lflag = 0;

void
main(
    int argc,
    char **argv,
    char **envp)
{
	int i, args_left;

	for (i = 1; i < argc; i++) {
	    if(argv[i][0] == '-'){
		if(argv[i][1] == '\0'){
		    nfiles += argc - i - 1;
		    break;
		}
		if(strcmp(argv[i], "-m") == 0){
		    mflag = 1;
		    continue;
		}
		if(strcmp(argv[i], "-l") == 0){
		    lflag = 1;
		    continue;
		}
	    }
	    nfiles++;
	}

	args_left = 1;
	for (i = 1; i < argc; i++) {
	    if(args_left && argv[i][0] == '-'){
		if(argv[i][1] == '\0'){
		    args_left = 0;
		    continue;
		}
		if(strcmp(argv[i], "-m") == 0)
		    continue;
		if(strcmp(argv[i], "-l") == 0)
		    continue;
	    }
	    size(argv[i]);
	}
	if(nfiles == 0)
	    size("a.out");
	exit(err);
}

size(
    char *name)
{
	struct exec buf;
	long text, data, objc, others, sum;
	int i, j;
	FILE *f;

	int mach;
	struct mach_header mh;
	struct load_command *load_commands, *lcp;
	struct segment_command *sgp;
	struct section *sp;
	long seg_sum, sect_sum;

	if((f = fopen(name, "r")) == NULL){
	    printf("size: %s not found\n", name);
	    err++;
	    return;
	}
	if(fread((char *)&buf, sizeof(buf), 1, f) != 1){
	    printf("size: can't read header of %s\n", name);
	    err++;
	    return;
	}
	mach = *((long *)&buf) == MH_MAGIC;
	if(mach){
	    rewind(f);
	    if(fread((char *)&mh, sizeof(struct mach_header), 1, f) != 1){
		printf("size: can't read header of %s\n", name);
		err++;
		return;
	    }
	    load_commands = (struct load_command *)malloc(mh.sizeofcmds);
	    if(load_commands == NULL){
		printf("size: out of memory\n");
		exit(1);
	    }
	    if(fread((char *)load_commands, mh.sizeofcmds, 1, f) != 1){
		printf("size: can't read load commands of %s\n", name);
		err++;
		return;
	    }
	    if(mflag){
		if(nfiles > 1)
			printf("%s:\n", name);
		lcp = load_commands;
		seg_sum = 0;
		for(i = 0; i < mh.ncmds; i++){
		    if(lcp->cmd == LC_SEGMENT){
			sgp = (struct segment_command *)lcp;
			printf("Segment %0.16s: %d", sgp->segname, sgp->vmsize);
			if(sgp->flags & SG_FVMLIB)
			    printf(" (fixed vm library segment)\n");
			else
			    printf("\n");
			seg_sum += sgp->vmsize;
			sp = (struct section *)((char *)sgp +
				sizeof(struct segment_command));
			sect_sum = 0;
			for(j = 0; j < sgp->nsects; j++){
			    if(mh.filetype == MH_OBJECT)
				printf("\tSection (%0.16s, %0.16s): %d",
				       sp->segname, sp->sectname, sp->size);
			    else
				printf("\tSection %0.16s: %d", sp->sectname,
				       sp->size);
			    if(lflag)
				printf(" (addr 0x%x offset %d)\n",
					sp->addr, sp->offset);
			    else
				printf("\n");
			    sect_sum += sp->size;
			    sp++;
			}
			if(sgp->nsects > 0)
			    printf("\ttotal %d\n", sect_sum);
		    }
		    lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
		}
		printf("total %d\n", seg_sum);
		free(load_commands);
		return;
	    }
	    text = 0;
	    data = 0;
	    objc = 0;
	    others = 0;
	    lcp = load_commands;
	    for(i = 0; i < mh.ncmds; i++){
		if(lcp->cmd == LC_SEGMENT){
		    sgp = (struct segment_command *)lcp;
		    if(mh.filetype == MH_OBJECT){
			sp = (struct section *)((char *)sgp +
				sizeof(struct segment_command));
			for(j = 0; j < sgp->nsects; j++){
			    if(strcmp(sp->segname, SEG_TEXT) == 0)
				text += sp->size;
			    else if(strcmp(sp->segname, SEG_DATA) == 0)
				data += sp->size;
			    else if(strcmp(sp->segname, SEG_OBJC) == 0)
				objc += sp->size;
			    else
				others += sp->size;
			    sp++;
			}
		    }
		    else{
			if(strcmp(sgp->segname, SEG_TEXT) == 0)
			    text += sgp->vmsize;
			else if(strcmp(sgp->segname, SEG_DATA) == 0)
			    data += sgp->vmsize;
			else if(strcmp(sgp->segname, SEG_OBJC) == 0)
			    objc += sgp->vmsize;
			else
			    others += sgp->vmsize;
		    }
		}
		lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	    }
	    free(load_commands);
	}
	else if(N_BADMAG(buf) && !mach){
	    printf("size: %s not an object file\n", name);
	    fclose(f);
	    err++;
	    return;
	}
	else{
	    text = buf.a_text;
	    data = buf.a_data + buf.a_bss;
	    if(buf.a_magic == ZMAGIC)
	        data = (data + PAGESIZE - 1) & ~(PAGESIZE - 1);
	    objc = 0;
	    others = 0;
	}
	if(header == 0){
	    printf("__TEXT\t__DATA\t__OBJC\tothers\tdec\thex\n");
	    header = 1;
	}
	printf("%u\t%u\t%u\t%u\t", text, data, objc, others);
	sum = text + data + objc + others;
	printf("%ld\t%lx", sum, sum);
	if(nfiles > 1)
	    printf("\t%s", name);
	printf("\n");
	fclose(f);
}
