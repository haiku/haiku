/*-
 * SPDX-License-Identifier: BSD-4-Clause
 *
 * Copyright (C) 1995, 1997 Wolfgang Solfrank.
 * Copyright (C) 1995, 1997 TooLs GmbH.
 * All rights reserved.
 * Original code by Paul Popelka (paulp@uts.amdahl.com) (see below).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Written by Paul Popelka (paulp@uts.amdahl.com)
 *
 * You can do anything you want with this software, just don't say you wrote
 * it, and don't remove this notice.
 *
 * This software is provided "as is".
 *
 * The author supplies this software to be publicly redistributed on the
 * understanding that the author is not responsible for the correct
 * functioning of this software in any circumstances and is not liable for
 * any damages caused by this software.
 *
 * October 1992
 */


#include "encodings.h"

#ifndef FS_SHELL
#include <stdlib.h>

#include <util/convertutf.h>
#endif // !FS_SHELL

#define _KERNEL
extern "C"
{
#include "sys/namei.h"
#include "sys/param.h"

#include "fs/msdosfs/bootsect.h"
#include "fs/msdosfs/bpb.h"
#include "fs/msdosfs/direntry.h"
#include "fs/msdosfs/fat.h"
#include "fs/msdosfs/msdosfsmount.h"
}
#include "debug.h"
#include "support.h"


// FreeBSD driver functions revised to extend range of supported filename characters.


/*! Revision of the FreeBSD unix2winfn, adding support for characters with two-unit
	UTF-16 representations. Designed to be called inside a loop to create as many winentries as
	needed to hold a given long filename, the entries being written in order of decreasing disk
	offset.
	@param utf8Str The filename being parsed. If multiple winentries are needed, the same values of
	\a utf8Str and \a utf8Len should be passed in each iteration.
	@param utf8Len The entire length of the filename being parsed.
	@param entry A pre-allocated struct to which the UTF-16 filename, or a portion thereof, will
	be written.
	@param count The 1-based index to write to winentry::weCnt.
	@param unpaired A hostendian UTF-16 surrogate that didn't fit in the previous winentry,
	zeroed if there	was no unpaired surrogate to carry over.
	@param totalParsed Number of bytes of \a utf8Str that have been handled so far,
	including any character that had conversion output spill into \a unpaired.
	@return 0 if \a utf8str has been completely converted, or 1 if another winentry is needed.
	@post \a unpaired and \a totalParsed are updated.
*/
int
utf8_to_winentry(const u_char* utf8Str, size_t utf8Len, struct winentry* entry, int count,
	int checkSum, struct msdosfsmount* fatVolume, uint16_t* unpaired, size_t* totalParsed)
{
	// drop trailing blanks and dots
	utf8Len = winLenFixup(utf8Str, utf8Len);

	// Advance the input past the characters already stored in a previous winentry.
	// We use the totalParsed argument instead of calling mbsadjpos as the original
	// unix2winfn did. The no-libiconv fallback code in mbsadjpos was written for
	// ISO 8859-1 to UCS-2 conversion, and would require significant revisions to
	// support UTF-8 to UTF-16 conversion.
	utf8Str += *totalParsed;
	utf8Len -= *totalParsed;

	memset(entry, 0xff, sizeof(*entry));
	entry->weCnt = count;
	entry->weAttributes = ATTR_WIN95;
	entry->weReserved1 = 0;
	entry->weChksum = checkSum;
	entry->weReserved2 = 0;

	// create a temporary UTF-16 buffer to which the initial conversion output will be written
	uint32 utf16ByteCapacity = WIN_CHARS * 2;
	uint8 utf16Buffer[utf16ByteCapacity];
	memset(utf16Buffer, 0xff, sizeof(utf16Buffer));

	// if we have a UTF-16 surrogate from the last iteration, place it at the start of the buffer
	int32 unpairedByteBias = 0;
	if (*unpaired != 0) {
		*reinterpret_cast<uint16*>(utf16Buffer) = B_HOST_TO_LENDIAN_INT16(*unpaired);
		*unpaired = 0;
		utf16ByteCapacity -= 2;
		unpairedByteBias = 2;
	}

	// Convert to UTF-16. We don't use the BSD driver's unix2winchr, which in the absence of
	// libiconv only supports ASCII characters.
	int end = 0;
	size_t utf8BytesConverted = utf8Len;
	ssize_t utf16UnitsWritten = utf8_to_utf16le(reinterpret_cast<const char*>(utf8Str),
		&utf8BytesConverted, reinterpret_cast<uint16*>(utf16Buffer + unpairedByteBias),
		utf16ByteCapacity / 2, unpaired, false);
	utf8Len -= utf8BytesConverted;
	*totalParsed += utf8BytesConverted;
	*unpaired = B_LENDIAN_TO_HOST_INT16(*unpaired);
	status_t status = utf16UnitsWritten < 0 ? utf16UnitsWritten : B_OK;
	if (status != B_OK)
		REPORT_ERROR(status);

	// null-terminate the converted name if this is the last winentry for this file and there is
	// room for another character
	size_t utf16BytesWritten = utf16UnitsWritten * 2;
	if (utf16BytesWritten + 2 <= utf16ByteCapacity) {
		memset(utf16Buffer + utf16BytesWritten + unpairedByteBias, 0, 2);
		utf16BytesWritten += 2;
	}

	// copy from the continuous temp buffer into the separate winentry name parts
	int32 bytesToCopy = utf16BytesWritten + unpairedByteBias;
	memcpy(entry->wePart1, utf16Buffer, WEPART1_BYTES);
	bytesToCopy -= WEPART1_BYTES;
	if (bytesToCopy > 0) {
		memcpy(entry->wePart2, utf16Buffer + WEPART1_BYTES, WEPART2_BYTES);
		bytesToCopy -= WEPART2_BYTES;
		if (bytesToCopy > 0)
			memcpy(entry->wePart3, utf16Buffer + WEPART1_BYTES + WEPART2_BYTES, WEPART3_BYTES);
	}

	if ((!utf8Len && *unpaired == 0) || status != B_OK)
		end = WIN_LAST;
	entry->weCnt |= end;
	return !end;
}


/*!	Revision of the FreeBSD win2unixfn with support for two-unit UTF-16 characters. Reads the
	UTF-16 filename content from one winentry and stores it in UTF-8 form. Designed to be called
	inside a loop that is reading entries in order of increasing disk offset.
	@param mbnambuf Accumulates the UTF-8 output as winentries associated with a given file are
	parsed.
	@param chkSum The checksum from the previous winentry associated with the same file, or -1 if
	this is the first winentry in a series.
	@param unpaired A host-endian UTF-16 surrogate from a character that is split across two
	entries, retained from the last call to this function; zeroed if not applicable.
	@return The entry's checksum or -1 on failure.
	@post \a unpaired is updated.
*/
int
winentry_to_utf8(struct mbnambuf* mbnambuf, const struct winentry* entry, int chkSum,
	uint16* unpaired)
{
	if ((entry->weCnt & WIN_CNT) > howmany(WIN_MAXLEN, WIN_CHARS) || (entry->weCnt & WIN_CNT) == 0)
		return -1;

	// compare checksums
	if ((entry->weCnt & WIN_LAST) != 0)
		chkSum = entry->weChksum;
	else if (chkSum != entry->weChksum)
		chkSum = -1;

	if (chkSum == -1)
		return -1;

	// copy the three (little-endian) name parts from the entry into a single little-endian
	// buffer before converting, to avoid unpaired surrogates
	size_t utf16BufferBytes = (WIN_CHARS + 1) * 2;
		// enough space to hold the units from one winentry, plus one unit passed via the
		// unpaired parameter
	uint8 utf16Name[utf16BufferBytes];
	memcpy(utf16Name, entry->wePart1, WEPART1_BYTES);
	memcpy(utf16Name + WEPART1_BYTES, entry->wePart2, WEPART2_BYTES);
	memcpy(utf16Name + WEPART1_BYTES + WEPART2_BYTES, entry->wePart3, WEPART3_BYTES);

	uint8* utf16Ptr;
	size_t unitsToConvert = WIN_CHARS;

	// if there was a surrogate left over from the previous winentry, copy it to the end of the
	// combined buffer
	if (*unpaired != 0) {
		utf16Ptr = utf16Name + utf16BufferBytes - 4;
		uint16 lastUnitHE = B_LENDIAN_TO_HOST_INT16(*(reinterpret_cast<uint16*>(utf16Ptr)));
		if (!is_high_surrogate(lastUnitHE)) {
			INFORM("Expected high surrogate to pair with %" B_PRIX16 ", but found %" B_PRIX16 "\n",
				*unpaired, lastUnitHE);
			chkSum = -1;
		}
		utf16Ptr = utf16Name + utf16BufferBytes - 2;
		*reinterpret_cast<uint16*>(utf16Ptr) = B_HOST_TO_LENDIAN_INT16(*unpaired);
		++unitsToConvert;
		*unpaired = 0;
	}

	utf16Ptr = utf16Name;

	// The first unit in this winentry could be an isolated surrogate. Because a series of
	// winentries is stored on disk in reverse order (with the last part of the filename stored in
	// the entry with the lowest disk offset) this would be the second of two units, despite the
	// fact that it is the first that we encounter; the character's first unit will be found at the
	// end of the next winentry we parse.
	uint16 firstUnitHE = B_LENDIAN_TO_HOST_INT16(*(reinterpret_cast<uint16*>(utf16Ptr)));
	if (is_low_surrogate(firstUnitHE)) {
		// don't try to convert it now; wait until we have the other surrogate
		*unpaired = firstUnitHE;
		utf16Ptr += 2;
		--unitsToConvert;
	}

	// convert to UTF-8
	size_t utf8BufferBytes = utf16BufferBytes + (utf16BufferBytes / 2) + 1;
		// the UTF-8 translation will be up to 50% larger than the UTF-16 source, and we
		// may need to add a terminating NULL
	char utf8Name[utf8BufferBytes];
	ssize_t utf8Len = utf16le_to_utf8(reinterpret_cast<uint16*>(utf16Ptr), unitsToConvert, utf8Name,
		utf8BufferBytes);
	if (utf8Len < 0) {
		REPORT_ERROR(utf8Len);
		return -1;
	}

	if (utf8Len < static_cast<ssize_t>(utf8BufferBytes)) {
		utf8Name[utf8Len] = '\0';
	} else if (utf8Name[utf8Len - 1] != 0) {
		INFORM("Unable to NULL-terminate name of length %" B_PRIdSSIZE ".\n", utf8Len);
		return -1;
	}

	// add the name part held in this entry to the working filename
	status_t status = mbnambuf_write(mbnambuf, utf8Name, (entry->weCnt & WIN_CNT) - 1);
	if (status != B_OK) {
		REPORT_ERROR(status);
		return -1;
	}

	return chkSum;
}
