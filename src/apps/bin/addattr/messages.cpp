// Author:      Sebastian Nozzi
// Created:     3 may 2002

// Modifications:
// (please include author, date, and description)

// mmu_man@sf.net: note the original one doesn't link to libbe

#include <stdio.h>

// Keeps the complete name of this binary at run-time
// To be set at the beginning of the programm
char *completeToolName;

// Predefined command line messages for the user

void usageMsg() {
	fprintf( stderr, "usage: %s [-t type] attr value file1 [file2...]\n", completeToolName);
	fprintf( stderr, "\tType is one of:\n");
	fprintf( stderr, "\t\tstring, mime, int, llong, float, double, bool,\n");
	fprintf( stderr, "\t\tor a numeric value\n");
	fprintf( stderr, "\tThe default is `string\'\n");
}

void invalidAttrMsg( const char *attrTypeName ) {
	fprintf( stderr, "%s: attribute type %s is not valid\n",completeToolName, attrTypeName);
	fprintf( stderr, "\tTry one of: string, mime, int, llong, float, double,\n");
	fprintf( stderr, "\t\tbool, or a <number>\n");
}

void problemsWithFileMsg( const char *file )
{
	fprintf( stderr, "%s: can\'t open file %s to add attribute\n", completeToolName, file );
}
