/*

basename(C) -- remove directory names from pathnames

OBOS Release 1.0 -- 1st January 2003

Additional Code by Sikosis (sikosis@gravity24hr.com)

*/

#include "stdio.h"

#define USAGE "basename(C) -- remove directory names from pathnames

Syntax

basename string [ suffix ]

Description
The basename command deletes any prefix ending in ``/'' and the suffix (if present in string) from string, and prints the result on the standard output. The result is the ``base'' name of the file, that is, the filename without any preceding directory path and without an extension. It is used inside substitution marks (` `) in shell procedures to construct new filenames.

Note that the suffix is not deleted if it is identical to the resulting basename.

The related command dirname deletes the last level from string and prints the resulting path on the standard output.
Exit values
basename returns the following values:

0
    successful completion

>0
    an error occurred 

Examples
The following command displays the filename memos on the standard output:

basename /usr/johnh/memos.old .old

The following shell procedure, when invoked with the argument /usr/src/cmd/cat.c, compiles the named file and moves the output to a file named cat in the current directory:

   cc $1
   mv a.out `basename $1 .c`

See also
dirname(C), sh(C)"


main(argc, argv)
char **argv;
{
	register char *p1, *p2, *p3;
	
	p1 = argv[1];
	
	if (!strcmp(p1,"--help"))
	{
		putchar('\n');
		puts(USAGE);
		putchar('\n');
		exit(1);
	}
	
	if (argc < 2) {
		putchar('\n');
		exit(1);
	}
	
	p2 = p1;
	while (*p1) {
		if (*p1++ == '/')
			p2 = p1;
	}
	if (argc>2) {
		for(p3=argv[2]; *p3; p3++) 
			;
		while(p1>p2 && p3>argv[2])
			if(*--p3 != *--p1)
				goto output;
		*p1 = '\0';
	}
output:
	puts(p2); // original code was this:- puts(p2, stdout);
	exit(0);
}