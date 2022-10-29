#!/bin/awk
# Copyright 2022, Haiku.
# Distributed under the terms of the MIT License.
#
# Authors:
#		Jérôme Duval, jerome.duval@gmail.com
#
# run as awk -f acpipnplist2h.awk pnp_id_registry.html acpi_id_registry.html
BEGIN {
	FS="</*td>"
}
NR == 1 {
	printf("/*\tHaiku" "$\t*/\n\n")
	printf("/*\n")
	printf(" This file is generated automatically. Don't edit. \n")
	printf("\n*/")
}

NF > 0 {
	if ($2) {
		n++

		ids[n, 1] = $4;
		ids[n, 2] = $2;
	}
}

END {
	printf("\n")
	printf("typedef struct { const char* VenId; const char* VenName; } idTable;\n")
	printf("idTable acpipnp_devids [] = {\n")
	for (i = 1; i <= n; i++) {
		printf("\t{\n")
		printf("\t\t\"%s\", \"%s\"\n", ids[i,1], ids[i,2])
		printf("\t},\n")
	}
	printf("\t};\n")
	printf("// Use this value for loop control during searching:\n")
	printf("#define	ACPIPNP_DEVTABLE_LEN	%i\n", n)
}

