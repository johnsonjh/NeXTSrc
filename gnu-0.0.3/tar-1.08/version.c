/* Version info for tar.
   Copyright (C) 1989, Free Software Foundation.

This file is part of GNU Tar.

GNU Tar is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 1, or (at your option)
any later version.

GNU Tar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Tar; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */
   
char version_string[] = "GNU tar version 1.08";

/* Version 1.00:  This file added.  -version option added */

/*		Installed new version of the remote-tape library */
/*		Added -help option */
/*	1.01	Fixed typoes in tar.texinfo */
/*		Fixed a bug in the #define for rmtcreat() */
/*		Fixed the -X option to not call realloc() of 0. */

/*	1.02	Fixed tar.c so 'tar -h' and 'tar -v' don't cause core dump */
/*		Also fixed the 'usage' message to be more up-to-date. */
/*		Fixed diffarch.c so verify should compile without MTIOCTOP
			defined */

/*	1.03	Fixed buffer.c so 'tar tzf NON_EXISTENT_FILE' returns an error
			message instead of hanging forever */

/*		More fixes to tar.texinfo */
/*	1.04	Added functions msg() and msg_perror()  Modified all the
			files to call them.  Also checked that all (I hope)
			calls to msg_perror() have a valid errno value
			(modified anno() to leave errno alone), etc
		Re-fixed the -X option.  This
 		time for sure. . . */	
/*		re-modified the msg stuff.  flushed anno() completely */
/*		Modified the directory stuff so it should work on sysV boxes */
/*		added ftime() to getdate.y */
/*		Fixed un_quote_string() so it won't wedge on \" Also fixed
		\ddd (like \123, etc) */
/*		More fixes to tar.texinfo */

/*	1.05	A fix to make confirm() work when the archive is on stdin
		include 'extern FILE *msg_file;' in pr_mkdir(), and fix
		tar.h to work with __STDC__ */

/*		Added to port.c: mkdir() ftruncate()  Removed: lstat() */
/*		Fixed -G to work with -X */
/*		Another fix to tar.texinfo */
/*		Changed tar.c to say argv[0]":you must specify exactly ... */
/*		buffer.c: modified child_open() to keep tar from hanging when
		it is done reading/writing a compressed archive */
/*		added fflush(msg_file) before printing error messages */
/*		create.c: fixed to make link_names non-absolute */
/*	1.06	Use STDC_MSG if __STDC__ defined
		ENXIO meand end-of-volume in archive (for the UNIX PC)
		Added break after volume-header case (line 440) extract.c
 		Added patch from arnold@unix.cc.emory.edu to rtape_lib.c
		Added f_absolute_paths option.
		Deleted refereces to UN*X manual sections (dump(8), etc)
 		Fixed to not core-dump on illegal options
		Modified msg_perror to call perror("") instead of perror(0)
		patch so -X - works
		Fixed tar.c so 'tar cf - -C dir' doesn't core-dump
		tar.c (name_match): Fixed to chdir() to the appropriate
		directory if the matching name's change_dir is set.  This
		makes tar xv -C foo {files} work. */
/*	1.07	New version to go on beta tape with GCC 1.35
		Better USG support.  Also support for __builtin_alloca
		if we're compiling with GCC.
		diffarch.c: Include the correct header files so MTIOCTOP
		is defined.
		tar.c:  Don't print the verbose list of options unless
		given -help.  The list of options is *way* too long.

	1.08	Sparse file support added.  Also various other features.
		See ChangeLog for details.
*/
