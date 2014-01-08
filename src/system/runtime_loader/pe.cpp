/*
 * Copyright 2013-2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	Alexander von Gluck IV, <kallisti5@unixzen.com>
 */


#include "pe.h"

#include <ctype.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static status_t
parse_mz_header(MzHeader* mzHeader, off_t* peOffset)
{
	if (memcmp(&mzHeader->magic, MZ_MAGIC, 2) != 0)
		return B_NOT_AN_EXECUTABLE;

	*peOffset = (off_t)mzHeader->lfaNew;
	return B_OK;
}


static status_t
parse_pe_header(PeHeader* peHeader)
{
	if (memcmp(&peHeader->magic, PE_MAGIC, 2) != 0)
		return B_NOT_AN_EXECUTABLE;

	// Looks like an old BeOS R3 x86 program
	if (peHeader->characteristics == 0x10E)
		return B_LEGACY_EXECUTABLE;

	return B_OK;
}


/*! Read and verify the PE header */
status_t
pe_verify_header(void *header, size_t length)
{
	if (length < sizeof(MzHeader))
		return B_NOT_AN_EXECUTABLE;

	// Verify MZ header, pull PE header offset
	off_t peOffset = 0;
	if (parse_mz_header((MzHeader*)header, &peOffset) != B_OK)
		return B_NOT_AN_EXECUTABLE;

	// MS-DOS program
	if (peOffset == 0)
		return B_UNKNOWN_EXECUTABLE;

	// Something is wrong with the binary
	if (peOffset + sizeof(PeHeader) > length)
		return B_UNKNOWN_EXECUTABLE;

	// Find the PE header based on MZ provided offset
	uint8* pePtr = (uint8*)header;
	pePtr += peOffset;

	// Win32 program or old BeOS R3 x86 program
	return parse_pe_header((PeHeader*)pePtr);
}
