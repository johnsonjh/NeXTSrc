#pragma CC_NO_MACH_TEXT_SECTIONS
/*
 * This file contains global data and the size of the global data can NOT
 * change or otherwise it would make the shared library incompatable.
 */

/*
 * 'GMT' has been changed to const from normal data.  This change was made when
 * the switch to the new NeXT Mach-O link editor from the previous GNU based
 * link editor.  The difference is that the new link editor does NOT split the
 * data section into static data and global data and load the global data first
 * when building a shared library (this was done by requiring all global data
 * follow all static data in the file).  The way global data is loaded before
 * static data using the new link editor is to just have only global data (with
 * no static data) in the file and load these files first.  This is exactly what
 * has always been done for const data.
 *
 * So now the tricky part.  Since this was the only file that was part of a
 * shared library that had global and static data in it this file gets updated.
 * The static data 'GMT' was made const (moving it from the data section to the
 * text section).  Also const was added to the declaration of tzname to get it
 * to compile with out warnings (it still is in the data section and address
 * will not change).  So to make the new library and the old compatible the
 * 4 bytes added to the text section must be given up by the next symbol in the
 * text section.  This can be done because the next text symbol
 * (__NXStreamsTextPad) is in the file globals.c in libstreams and is a static
 * and just a pad.  So was reduced by 4 bytes in size and all is compatible
 * between the libraries built by different link editors.
 */
static const char GMT[] = "GMT";


char const * tzname[2] = {
	GMT,
	GMT
};

