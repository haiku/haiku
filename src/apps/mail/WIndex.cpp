/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include "WIndex.h"

#include <File.h>
#include <fs_attr.h>
#include <Message.h>
#include <Node.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>


#define IVERSION	1


static int32 kCRCTable = 0;


int32 cmp_i_entries(const WIndexEntry *e1, const WIndexEntry *e2);
void gen_crc_table();
unsigned long update_crc(unsigned long crc_accum, const char *data_blk_ptr,
	int data_blk_size);


FileEntry::FileEntry(void)
{

}


FileEntry::FileEntry(const char *entryString)
	:
	BString(entryString)
{

}


status_t
WIndex::SetTo(const char *dataPath, const char *indexPath)
{
	BFile* dataFile;
	BFile indexFile;

	dataFile = new BFile();

	if (dataFile->SetTo(dataPath, B_READ_ONLY) != B_OK) {
		delete dataFile;
		return B_ERROR;
	} else {
		bool buildIndex = true;
		SetTo(dataFile);

		time_t mtime;
		time_t modified;

		dataFile->GetModificationTime(&mtime);

		if (indexFile.SetTo(indexPath, B_READ_ONLY) == B_OK) {
			attr_info info;
			if ((indexFile.GetAttrInfo("WINDEX:version", &info) == B_OK)) {
				uint32 version = 0;
				indexFile.ReadAttr("WINDEX:version", B_UINT32_TYPE, 0,
					&version, 4);
				if (IVERSION == version) {
					if ((indexFile.GetAttrInfo("WINDEX:modified", &info)
						== B_OK)) {
						indexFile.ReadAttr("WINDEX:modified", B_UINT32_TYPE, 0,
							&modified, 4);

						if (mtime == modified) {
							if (UnflattenIndex(&indexFile) == B_OK)
								buildIndex = false;
						}
					}
				}
			}
			indexFile.Unset();
		}
		if (buildIndex) {
			InitIndex();
			BuildIndex();
			if (indexFile.SetTo(indexPath,
				B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE) == B_OK) {
				FlattenIndex(&indexFile);
				indexFile.WriteAttr("WINDEX:modified", B_UINT32_TYPE, 0,
					&mtime, 4);
				uint32 version = IVERSION;
				indexFile.WriteAttr("WINDEX:version", B_UINT32_TYPE, 0,
					&version, 4);
			}
		}
	}
	return B_OK;
}


FileEntry::~FileEntry(void)
{

}


WIndex::WIndex(int32 count)
{
	fEntryList = NULL;
	fDataFile = NULL;
	fEntriesPerBlock = count;
	fEntrySize = sizeof(WIndexEntry);
	if (!atomic_or(&kCRCTable, 1))
		gen_crc_table();
}


WIndex::WIndex(BPositionIO *dataFile, int32 count)
{
	fEntryList = NULL;
	fDataFile = dataFile;
	fEntriesPerBlock = count;
	fEntrySize = sizeof(WIndexEntry);
	if (!atomic_or(&kCRCTable, 1))
		gen_crc_table();
}


WIndex::~WIndex(void)
{
	if (fEntryList)
		free(fEntryList);
	delete fDataFile;
}


status_t
WIndex::UnflattenIndex(BPositionIO *io)
{
	if (fEntryList)
		free(fEntryList);
	WIndexHead		head;

	io->Seek(0, SEEK_SET);
	io->Read(&head, sizeof(head));
	io->Seek(head.offset, SEEK_SET);

	fEntrySize = head.entrySize;
	fEntries = head.entries;
	fMaxEntries = fEntriesPerBlock;
	fBlockSize = fEntriesPerBlock * fEntrySize;
	fBlocks = fEntries / fEntriesPerBlock + 1;;
	fIsSorted = true;

	int32 size = (head.entries + 1) * head.entrySize;
	if (!(fEntryList = (uint8 *)malloc(size)))
		return B_ERROR;

	if (fEntries)
		io->Read(fEntryList, size);

	return B_OK;
}


status_t
WIndex::FlattenIndex(BPositionIO *io)
{
	if (fEntries && !fIsSorted)
		SortItems();
	WIndexHead		head;

	head.entries = fEntries;
	head.entrySize = fEntrySize;
	head.offset = sizeof(WIndexHead);
	io->Seek(0, SEEK_SET);
	io->Write(&head, sizeof(head));
	if (fEntries)
		io->Write(fEntryList, head.entries * head.entrySize);

	return B_OK;
}


int32
WIndex::Lookup(int32 key)
{
	if (!fEntries)
		return -1;
	if (!fIsSorted)
		SortItems();

	// Binary Search
	int32	M, Lb, Ub;
	Lb = 0;
	Ub = fEntries - 1;
	while (true) {
		M = (Lb + Ub) / 2;
		if (key < ((WIndexEntry *)(fEntryList + (M * fEntrySize)))->key)
			Ub = M - 1;
		else if (key > ((WIndexEntry *)(fEntryList + (M * fEntrySize)))->key)
			Lb = M + 1;
		else
			return M;
		if (Lb > Ub)
			return -1;
	}
}


status_t
WIndex::AddItem(WIndexEntry *entry)
{
	if (_BlockCheck() == B_ERROR)
		return B_ERROR;
	memcpy(((WIndexEntry *)(fEntryList + (fEntries * fEntrySize))), entry,
		fEntrySize);
	fEntries++;
	fIsSorted = false;
	return B_OK;
}


void
WIndex::SortItems(void)
{
	qsort(fEntryList, fEntries, fEntrySize,
		(int(*)(const void *, const void *))cmp_i_entries);

	fIsSorted = true;
}


status_t
WIndex::_BlockCheck(void)
{
	if (fEntries < fMaxEntries)
		return B_OK;
	fBlocks = fEntries / fEntriesPerBlock + 1;
	uint8* tmpEntryList = (uint8 *)realloc(fEntryList, fBlockSize * fBlocks);
	if (!tmpEntryList) {
		free(fEntryList);
		fEntryList = NULL;
		return B_ERROR;
	} else {
		fEntryList = tmpEntryList;
	}
	return B_OK;
}


status_t
WIndex::InitIndex(void)
{
	if (fEntryList)
		free(fEntryList);
	fIsSorted = 0;
	fEntries = 0;
	fMaxEntries = fEntriesPerBlock;
	fBlockSize = fEntriesPerBlock * fEntrySize;
	fBlocks = 1;
	fEntryList = (uint8 *)malloc(fBlockSize);
	if (!fEntryList)
		return B_ERROR;
	return B_OK;
}


int32
WIndex::GetKey(const char *s)
{

	int32	key = 0;
	/*int32	x;
	int32	a = 84589;
	int32	b = 45989;
	int32	m = 217728;
	while (*s) {
		x = *s++ - 'a';

		key ^= (a * x + b) % m;
		key <<= 1;
	}*/

	key = update_crc(0, s, strlen(s));

	if (key < 0) // No negative values!
		key = ~key;

	return key;
}


int32
cmp_i_entries(const WIndexEntry *e1, const WIndexEntry *e2)
{
	return e1->key - e2->key;
}


status_t
WIndex::SetTo(BPositionIO *dataFile)
{
	fDataFile = dataFile;
	return B_OK;
}


void
WIndex::Unset(void)
{
	fDataFile = NULL;
}


int32
WIndex::FindFirst(const char *word)
{
	if (!fEntries)
		return -1;

	int32			index;
	char			nword[256];
	int32			key;

	NormalizeWord(word, nword);
	key = GetKey(nword);

	if ((index = Lookup(key)) < 0)
		return -1;
	// Find first instance of key
	while ((ItemAt(index - 1))->key == key)
		index--;
	return index;
}


FileEntry*
WIndex::GetEntry(int32 index)
{
	if ((index >= fEntries)||(index < 0))
		return NULL;
	WIndexEntry		*ientry;
	FileEntry		*dentry;
	char			*buffer;

	dentry = new FileEntry();

	ientry = ItemAt(index);

	int32 size;

	fDataFile->Seek(ientry->offset, SEEK_SET);
	buffer = dentry->LockBuffer(256);
	fDataFile->Read(buffer, 256);
	size = _GetEntrySize(ientry, buffer);
	//buffer[256] = 0;
	//printf("Entry: = %s\n", buffer);
	dentry->UnlockBuffer(size);
	return dentry;
}


size_t
WIndex::_GetEntrySize(WIndexEntry *entry, const char *entryData)
{
	// eliminate unused parameter warning
	(void)entry;

	return strcspn(entryData, "\n\r");
}


FileEntry*
WIndex::GetEntry(const char *word)
{
	return GetEntry(FindFirst(word));
}


char*
WIndex::NormalizeWord(const char *word, char *dest)
{
	const char 	*src;
	char		*dst;

	// remove dots and copy
	src = word;
	dst = dest;
	while (*src) {
		if (*src != '.')
			*dst++ = *src;
		src++;
	}
	*dst = 0;

	// convert to lower-case
	for (dst = dest; *dst; dst++)
		*dst = tolower(*dst);
	return dest;
}


/* crc32h.c -- package to compute 32-bit CRC one byte at a time using   */
/*             the high-bit first (Big-Endian) bit ordering convention  */
/*                                                                      */
/* Synopsis:                                                            */
/*  gen_crc_table() -- generates a 256-word table containing all CRC    */
/*                     remainders for every possible 8-bit byte.  It    */
/*                     must be executed (once) before any CRC updates.  */
/*                                                                      */
/*  unsigned update_crc(crc_accum, data_blk_ptr, data_blk_size)         */
/*           unsigned crc_accum; char *data_blk_ptr; int data_blk_size; */
/*           Returns the updated value of the CRC accumulator after     */
/*           processing each byte in the addressed block of data.       */
/*                                                                      */
/*  It is assumed that an unsigned long is at least 32 bits wide and    */
/*  that the predefined type char occupies one 8-bit byte of storage.   */
/*                                                                      */
/*  The generator polynomial used for this version of the package is    */
/*  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x^1+x^0 */
/*  as specified in the Autodin/Ethernet/ADCCP protocol standards.      */
/*  Other degree 32 polynomials may be substituted by re-defining the   */
/*  symbol POLYNOMIAL below.  Lower degree polynomials must first be    */
/*  multiplied by an appropriate power of x.  The representation used   */
/*  is that the coefficient of x^0 is stored in the LSB of the 32-bit   */
/*  word and the coefficient of x^31 is stored in the most significant  */
/*  bit.  The CRC is to be appended to the data most significant byte   */
/*  first.  For those protocols in which bytes are transmitted MSB      */
/*  first and in the same order as they are encountered in the block    */
/*  this convention results in the CRC remainder being transmitted with */
/*  the coefficient of x^31 first and with that of x^0 last (just as    */
/*  would be done by a hardware shift register mechanization).          */
/*                                                                      */
/*  The table lookup technique was adapted from the algorithm described */
/*  by Avram Perez, Byte-wise CRC Calculations, IEEE Micro 3, 40 (1983).*/


#define POLYNOMIAL 0x04c11db7L


static unsigned long crc_table[256];


void
gen_crc_table()
{
	// generate the table of CRC remainders for all possible bytes

	register int i, j;
	register unsigned long crc_accum;

	for (i = 0;  i < 256;  i++) {
		crc_accum = ((unsigned long) i << 24);
		for (j = 0;  j < 8;  j++) {
			if (crc_accum & 0x80000000L)
				crc_accum = (crc_accum << 1) ^ POLYNOMIAL;
			else
				crc_accum = (crc_accum << 1);
		}
		crc_table[i] = crc_accum;
	}

	return;
}


unsigned long
update_crc(unsigned long crc_accum, const char *data_blk_ptr, int data_blk_size)
{
	// update the CRC on the data block one byte at a time

	register int i, j;

	for (j = 0;  j < data_blk_size;  j++) {
		i = ((int) (crc_accum >> 24) ^ *data_blk_ptr++) & 0xff;
		crc_accum = (crc_accum << 8) ^ crc_table[i];
	}

	return crc_accum;
}

