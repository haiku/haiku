#!/bin/awk
# ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
#
#	Copyright (c) 2004, Haiku
#
#  This software is part of the Haiku distribution and is covered 
#  by the Haiku license.
#
#
#  File:        devlist2h.awk
#  Author:      Jérôme Duval
#  Description: Devices Preferences
#  Created :    July 8, 2004
# 
# ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

BEGIN {
	FS="         "
}
NR == 1 {
	printf("/*\tHaiku" "$\t*/\n\n")
	printf("/*\n")
	printf(" This file is generated automatically. Don't edit. \n")
	printf("\n*/")
}

NF > 0 {
	n++

	ids[n, 1] = $1;
	ids[n, 2] = $2;

}

END {
	printf("\n")
	printf("typedef struct { char* id; char* devname; } idTable;\n")
	printf("idTable isapnp_devids [] = {\n")
	for (i = 1; i <= n; i++) {
		printf("\t{\n")
		printf("\t\t\"%s\", \"%s\"\n", ids[i,1], ids[i,2])
		printf("\t},\n")
	}
	printf("\t};\n")
	printf("// Use this value for loop control during searching:\n")
	printf("#define	ISA_DEVTABLE_LEN	%i\n", n)
}

