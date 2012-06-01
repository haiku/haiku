/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */

/*
 * Pass an AMD CPUID in hex, and get out a CPUID for OS.h
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define AMD_VENDOR 0x110000

#define EXT_FAMILY_MASK 0xF00000
#define EXT_MODEL_MASK	0x0F0000
#define FAMILY_MASK		0x000F00
#define MODEL_MASK		0x0000F0
#define STEPPING_MASK	0x00000F


// Converts a hexadecimal string to integer
static int xtoi(const char* xs, unsigned int* result)
{
	size_t szlen = strlen(xs);
	int i, xv, fact;

	if (szlen > 0) {
		// Converting more than 32bit hexadecimal value?
		if (szlen>8) return 2; // exit

		// Begin conversion here
		*result = 0;
		fact = 1;

		// Run until no more character to convert
		for (i = szlen - 1; i>=0 ;i--) {
			if (isxdigit(*(xs+i))) {
				if (*(xs+i)>=97) {
					xv = ( *(xs+i) - 97) + 10;
				} else if ( *(xs+i) >= 65) {
					xv = (*(xs+i) - 65) + 10;
				} else {
					xv = *(xs+i) - 48;
				}
				*result += (xv * fact);
				fact *= 16;
			} else {
				// Conversion was abnormally terminated
				// by non hexadecimal digit, hence
				// returning only the converted with
				// an error value 4 (illegal hex character)
				return 4;
			}
		}
	}

	// Nothing to convert
	return 1;
}


int
main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Provide the AMD cpuid in hex, and you will get how we id it\n");
		printf("usage: amdcpuid <cpuid_hex>\n");
		return 1;
	}

	unsigned int cpuid;
	xtoi(argv[1], &cpuid);

	printf("cpuid: 0x%X\n", cpuid);

	unsigned int extFam = (cpuid & EXT_FAMILY_MASK) >> 20;
	unsigned int extMod = (cpuid & EXT_MODEL_MASK) >> 16;
	unsigned int family = (cpuid & FAMILY_MASK) >> 8;
	unsigned int model = (cpuid & MODEL_MASK) >> 4;
	unsigned int stepping = (cpuid & STEPPING_MASK);

	unsigned int amdFamily = 0;
	unsigned int amdModel = 0;
	if (family == 0xF) {
		amdFamily = extFam + family;
		amdModel = (extMod << 4) + model;
	} else {
		amdFamily = family;
		amdModel = model;
	}

	// Haiku AMD cpuid format: VVFFMM
	unsigned int amdHaiku
		= AMD_VENDOR + (amdFamily << 8) + amdModel;

	printf("family: 0x%lX\n", amdFamily);
	printf("model: 0x%lX\n", amdModel);
	printf("stepping: 0x%lX\n", stepping);
	printf("Haiku CPUID: 0x%lX\n", amdHaiku);

	return 0;
}
