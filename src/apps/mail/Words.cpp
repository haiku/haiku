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


#include "Words.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>


enum {
	FIND_WORD,
	GET_WORD,
	GET_FLAGS
};


/*
	MAXMETAPH is the length of the Metaphone code.

	Four is a good compromise value for English names. For comparing words
	which are not names or for some non-English names, use a longer code
	length for more precise matches.

	The default here is 5.
*/

#define MAXMETAPH 6


// Character coding array, A-Z
static char vsvfn[26] = { 1, 16, 4, 16, 9, 2, 4, 16, 9, 2, 0, 2, 2, 2, 1, 4, 0,
	2, 4, 4, 1, 0, 0, 0, 8, 0};


static const char* gCmpKey;


static int
word_cmp(BString** firstArg, BString** secondArg)
{
	return word_match(gCmpKey, (*firstArg)->String()) - word_match(gCmpKey,
		(*secondArg)->String());
}


Words::Words(bool useMetaphone)
	:
	fUseMetaphone(useMetaphone)
{

}


Words::Words(BPositionIO* thes, bool useMetaphone)
	:
	WIndex(thes),
	fUseMetaphone(useMetaphone)
{

}


Words::~Words(void)
{

}


Words::Words(const char* dataPath, const char* indexPath, bool useMetaphone)
	:
	fUseMetaphone(useMetaphone)
{
	if (!useMetaphone)
		fEntrySize = sizeof(uint32);
	SetTo(dataPath, indexPath);
}


status_t
Words::BuildIndex(void)
{
	// Parse the Words file...

	// Buffer Stuff
	char			buffer[16384];
	char			*nptr, *eptr;
	int64			blockOffset;
	int32			blockSize;

	// The Word Entry
	WIndexEntry		entry;
	char			entryName[256], *namePtr = entryName;
	char			suffixName[256];
	char			flags[32], *flagsPtr = flags;

	// State Info
	int32			state = FIND_WORD;

	// Make sure we are at start of file
	fDataFile->Seek(0, SEEK_SET);
	entry.offset = -1;

	// Read blocks from thes until eof
	while (true) {
		// Get next block
		blockOffset = fDataFile->Position();
		if ((blockSize = fDataFile->Read(buffer, 16384)) == 0)
			break;

		// parse block
		for (nptr = buffer, eptr = buffer + blockSize; nptr < eptr; nptr++) {
			// Looking for start of word?
			if (state == FIND_WORD) {
				// Is start of word?
				if (isalpha(*nptr)) {
					state = GET_WORD;
					*namePtr++ = *nptr; // copy word
					entry.offset = blockOffset + (nptr - buffer);
				} else {
					entry.offset++;
				}
			} else if ((*nptr == '\n') || (*nptr == '\r')) {
				// End of word?

				if (namePtr != entryName) {
					// Add previous entry to word index
					*namePtr = 0; // terminate word
					*flagsPtr = 0; // terminate flags
					NormalizeWord(entryName, entryName);
					// Add base word
					entry.key = GetKey(entryName);
					AddItem(&entry);

					// Add suffixed words if any
					if (flagsPtr != flags) {
						// printf("Base: %s, flags: %s\n", entryName, flags);
						for (flagsPtr = flags; *flagsPtr != 0; flagsPtr++) {
							if (suffix_word(suffixName, entryName, *flagsPtr)) {
								// printf("Suffix: %s\n", suffixName);
								entry.key = GetKey(suffixName);
								AddItem(&entry);
							}
						}
					}
				}
				// Init new entry
				state = FIND_WORD;
				namePtr = entryName;
				flagsPtr = flags;
			} else if (state == GET_WORD) {
				// Start of flags?
				if (*nptr == '/') {
					*namePtr = 0; // terminate word
					// printf("Found word: %s\n", entryName);

					state = GET_FLAGS;
				} else {
					*namePtr++ = *nptr; // copy word
				}
			} else if (state == GET_FLAGS) // Are we getting the flags?
				*flagsPtr++ = *nptr; // copy flag
		}	// End for (nptr = buffer, eptr = buffer + blockSize;
				// nptr < eptr; nptr++, entry.size++)
	} // End while (true)

	SortItems();
	return B_OK;
}


int32
Words::GetKey(const char* s)
{
	if (fUseMetaphone) {
		char		Metaph[12];
		const char	*sPtr;
		int32		key = 0;
		int32		offset;
		char		c;

		metaphone(s, Metaph, GENERATE);
		// Compact Metaphone from 6 bytes to 4

		// printf("%s -> %s: \n", s, Metaph);

		for (sPtr = Metaph, offset = 25; *sPtr; sPtr++, offset -= 5) {
			c = *sPtr - 'A';
			// printf("%d,", int16(c));
			key |= int32(c) << offset;
		}
		for (; offset >= 0; offset -= 5)
			key |= int32(31) << offset;
		// printf(": %ld\n", key);
		return key;
	} else {
		return WIndex::GetKey(s);
	}
}


// Macros to access the character coding array
#define vowel(x)	(vsvfn[(x) - 'A'] & 1)	// AEIOU
#define same(x)		(vsvfn[(x) - 'A'] & 2)	// FJLMNR
#define varson(x)	(vsvfn[(x) - 'A'] & 4)	// CGPST
#define frontv(x)	(vsvfn[(x) - 'A'] & 8)	// EIY
#define noghf(x)	(vsvfn[(x) - 'A'] & 16)	// BDH
#define NUL '\0'

/*
	metaphone()

	Arguments:
	1 - The word to be converted to a metaphone code.
	2 - A MAXMETAPH + 1 char field for the result.
	3 - Function flag:
	If 0: Compute the Metaphone code for the first argument,
		then compare it to the Metaphone code passed in the second argument.
	If 1: Compute the Metaphone code for the first argument,
		then store the result in the area pointed to by the second argument.

	Returns:
	If function code is 0, returns Success_ for a match, else Error_.
	If function code is 1, returns Success_.
*/


bool
metaphone(const char* Word, char* Metaph, metaphlag Flag)
{
	char *n, *n_start, *n_end;			// Pointers to string
	char *metaph = NULL, *metaph_end;	// Pointers to metaph
	char ntrans[512];					// Word with uppercase letters
	char newm[MAXMETAPH + 4];			// New metaph for comparison
	int KSflag;							// State flag for X translation

	// Copy word to internal buffer, dropping non-alphabetic characters
	// and converting to upper case.

	for (n = ntrans + 1, n_end = ntrans + sizeof(ntrans) - 2;
		*Word && n < n_end; ++Word) {
		if (isalpha(*Word))
			*n++ = toupper(*Word);
	}

	if (n == ntrans + 1)
		return false;		// Return if zero characters
	else
			n_end = n;		//	Set end of string pointer

	// Pad with NULs, front and rear

	*n++ = NUL;
	*n = NUL;
	n = ntrans;
	*n++ = NUL;

	// If doing comparison, redirect pointers

	if (COMPARE == Flag) {
		metaph = Metaph;
		Metaph = newm;
	}

	// Check for PN, KN, GN, WR, WH, and X at start

	switch (*n) {
		case 'P':
		case 'K':
		case 'G':
			if ('N' == *(n + 1))
				*n++ = NUL;
			break;

		case 'A':
			if ('E' == *(n + 1))
				*n++ = NUL;
			break;

		case 'W':
			if ('R' == *(n + 1)) {
				*n++ = NUL;
			} else if ('H' == *(n + 1)) {
				*(n + 1) = *n;
				*n++ = NUL;
			}
			break;

		case 'X':
			*n = 'S';
			break;
	}

	// Now loop through the string, stopping at the end of the string
	// or when the computed Metaphone code is MAXMETAPH characters long.

	KSflag = false;	// State flag for KStranslation

	for (metaph_end = Metaph + MAXMETAPH, n_start = n;
		n <= n_end && Metaph < metaph_end; ++n) {
		if (KSflag) {
			KSflag = false;
			*Metaph++ = *n;
		} else {
			// Drop duplicates except for CC

			if (*(n - 1) == *n && *n != 'C')
				continue;

			// Check for F J L M N R or first letter vowel

			if (same(*n) || (n == n_start && vowel(*n))) {
				*Metaph++ = *n;
			} else {
				switch (*n) {
					case 'B':
						if (n < n_end || *(n - 1) != 'M')
							*Metaph++ = *n;
						break;

					case 'C':
						if (*(n - 1) != 'S' || !frontv(*(n + 1))) {
							if ('I' == *(n + 1) && 'A' == *(n + 2))
								*Metaph++ = 'X';
							else if (frontv(*(n + 1)))
								*Metaph++ = 'S';
							else if ('H' == *(n + 1)) {
								*Metaph++ = ((n == n_start && !vowel(*(n + 2)))
									|| 'S' == *(n - 1)) ? 'K' : 'X';
							} else {
								*Metaph++ = 'K';
							}
						}
						break;

					case 'D':
						*Metaph++ = ('G' == *(n + 1) && frontv(*(n + 2)))
							? 'J' : 'T';
						break;

					case 'G':
						if ((*(n + 1) != 'H' || vowel(*(n + 2)))
							&& (*(n + 1) != 'N' || ((n + 1) < n_end
								&& (*(n + 2) != 'E' || *(n + 3) != 'D')))
							&& (*(n - 1) != 'D' || !frontv(*(n + 1)))) {
							*Metaph++ = (frontv(*(n + 1))
								&& *(n + 2) != 'G') ? 'J' : 'K';
						} else if ('H' == *(n + 1) && !noghf(*(n - 3))
							&& *(n - 4) != 'H') {
							*Metaph++ = 'F';
						}
						break;

					case 'H':
						if (!varson(*(n - 1))
							&& (!vowel(*(n - 1)) || vowel(*(n + 1)))) {
							*Metaph++ = 'H';
						}
						break;

					case 'K':
						if (*(n - 1) != 'C')
							*Metaph++ = 'K';
						break;

					case 'P':
						*Metaph++ = ('H' == *(n + 1)) ? 'F' : 'P';
						break;

					case 'Q':
						*Metaph++ = 'K';
						break;

					case 'S':
						*Metaph++ = ('H' == *(n + 1) || ('I' == *(n + 1)
								&& ('O' == *(n + 2) || 'A' == *(n + 2))))
							? 'X' : 'S';
						break;

					case 'T':
						if ('I' == *(n + 1)
							&& ('O' == *(n + 2) || 'A' == *(n + 2))) {
							*Metaph++ = 'X';
						} else if ('H' == *(n + 1)) {
							*Metaph++ = 'O';
						} else if (*(n + 1) != 'C' || *(n + 2) != 'H') {
							*Metaph++ = 'T';
						}
						break;

					case 'V':
						*Metaph++ = 'F';
						break;

					case 'W':
					case 'Y':
						if (vowel(*(n + 1)))
							*Metaph++ = *n;
						break;

					case 'X':
						if (n == n_start) {
							*Metaph++ = 'S';
						} else {
							*Metaph++ = 'K';
							KSflag = true;
						}
						break;

					case 'Z':
						*Metaph++ = 'S';
						break;
				}
			}
		}

		// Compare new Metaphone code with old
		if (COMPARE == Flag
			&& *(Metaph - 1) != metaph[(Metaph - newm) - 1]) {
			return false;
		}
	}

	// If comparing, check if Metaphone codes were equal in length
	if (COMPARE == Flag && metaph[Metaph - newm])
		return false;

	*Metaph = NUL;
	return true;
}


int
word_match(const char* reference, const char* test)
{
	const char	*s1, *s2;
	int32 		x = 0;
	char		c1, c2;
	s1 = test;
	s2 = reference;

	bool a, b;

	while (*s2 || *s1) {
		c1 = tolower(*s1);
		c2 = tolower(*s2);

		if (*s2 && *s1) {
			if (c1 != c2) {
				a = (tolower(s1[1]) == c2);
				b = (tolower(s2[1]) == c1);
				// Reversed pair
				if (a && b) {
					x += 1;
					s1++;
					s2++;
				}
				// Extra character
				if (a) {
					x += 1;
					s1++;
				}
				// Missing Character
				else if (b) {
					x += 1;
					s2++;
				}
				// Equivalent Character
				else if (vsvfn[(unsigned)c1] == vsvfn[(unsigned)c2])
					x++;
				// Unrelated Character
				else
					x += 3;
			}
		} else {
			x += 1;
		}

		if (*s2)
			s2++;
		if (*s1)
			s1++;
	}

	return x;
}


int32
suffix_word(char* dst, const char* src, char flag)
{
	char* end;

	end = stpcpy(dst, src);
	flag = toupper(flag);
	switch(flag) {
		case 'V':
			switch(end[-1]) {
				case 'e':
					end = stpcpy(end - 1, "ive");
					break;
				default:
					end = stpcpy(end, "ive");
					break;
			}
			break;
		case 'N':
			switch(end[-1]) {
				case 'e':
					end = stpcpy(end - 1, "ion");
					break;
				case 'y':
					end = stpcpy(end - 1, "ication");
					break;
				default:
					end = stpcpy(end, "en");
					break;
			}
			break;
		case 'X':
			switch(end[-1]) {
				case 'e':
					end = stpcpy(end - 1, "ions");
					break;
				case 'y':
					end = stpcpy(end - 1, "ications");
					break;
				default:
					end = stpcpy(end, "ens");
					break;
			}
			break;
		case 'H':
			switch(end[-1]) {
				case 'y':
					end = stpcpy(end - 1, "ieth");
					break;
				default:
					end = stpcpy(end, "th");
					break;
			}
			break;
		case 'Y':
			end = stpcpy(end, "ly");
			break;
		case 'G':
			switch(end[-1]) {
				case 'e':
					end = stpcpy(end - 1, "ing");
					break;
				default:
					end = stpcpy(end, "ing");
					break;
			}
			break;
		case 'J':
			switch(end[-1]) {
				case 'e':
					end = stpcpy(end - 1, "ings");
					break;
				default:
					end = stpcpy(end, "ings");
					break;
			}
			break;
		case 'D':
			switch(end[-1]) {
				case 'e':
					end = stpcpy(end - 1, "ed");
					break;
				case 'y':
					if (!strchr("aeiou", end[-2])) {
						end = stpcpy(end - 1, "ied");
						break;
					}
					// Fall through
				default:
					end = stpcpy(end, "ed");
					break;
			}
			break;
		case 'T':
			switch(end[-1]) {
				case 'e':
					end = stpcpy(end - 1, "est");
					break;
				case 'y':
					if (!strchr("aeiou", end[-2])) {
						end = stpcpy(end - 1, "iest");
						break;
					}
					// Fall through
				default:
					end = stpcpy(end, "est");
					break;
			}
			break;
		case 'R':
			switch(end[-1]) {
				case 'e':
					end = stpcpy(end - 1, "er");
					break;
				case 'y':
					if (!strchr("aeiou", end[-2])) {
						end = stpcpy(end - 1, "ier");
						break;
					}
					// Fall through
				default:
					end = stpcpy(end, "er");
					break;
			}
			break;
		case 'Z':
			switch(end[-1]) {
				case 'e':
					end = stpcpy(end - 1, "ers");
					break;
				case 'y':
					if (!strchr("aeiou", end[-2])) {
						end = stpcpy(end - 1, "iers");
						break;
					}
					// Fall through
				default:
					end = stpcpy(end, "ers");
					break;
			}
			break;
		case 'S':
			switch(end[-1]) {
				case 's':
				case 'x':
				case 'z':
				case 'h':
					end = stpcpy(end, "es");
					break;
				case 'y':
					if (!strchr("aeiou", end[-2])) {
						end = stpcpy(end - 1, "ies");
						break;
					}
					// Fall through
				default:
					end = stpcpy(end, "s");
					break;
			}
			break;
		case 'P':
			switch(end[-1]) {
				case 'y':
					if (!strchr("aeiou", end[-2])) {
						end = stpcpy(end - 1, "iness");
						break;
					}
					// Fall through
				default:
					end = stpcpy(end, "ness");
					break;
			}
			break;
		case 'M':
			end = stpcpy(end, "'s");
			break;
		default:
			return 0;
	}
	return end - dst;
}


int32
Words::FindBestMatches(BList* matches, const char* s)
{
	int32		index;
	// printf("*** Looking for %s: ***\n", s);

	if ((index = FindFirst(s)) >= 0) {
		BString srcWord(s);
		FileEntry* entry;
		WIndexEntry* indexEntry;

		int32		key = (ItemAt(index))->key;
		int32		suffixLength;
		char		word[128], suffixWord[128];
		const char	*src, *testWord;
		const char 	*suffixFlags;
		char		*dst;

		gCmpKey = srcWord.String();

		uint8		hashTable[32];
		uint8		hashValue, highHash, lowHash;
		for (int32 i = 0; i < 32; i++)
			hashTable[i] = 0;

		do {
			indexEntry = ItemAt(index);
			// Hash the entry offset; we use this to make sure we don't add
			// the same word file entry twice;
			// It is possible for the same entry in the words file to have
			// multiple entries in the index.

			hashValue = indexEntry->offset % 256;
			highHash = hashValue >> 3;
			lowHash = 0x01 << (hashValue & 0x07);

			// printf("Testing Entry: %ld: hash=%d, highHash=%d, lowHash=%d\n",
			// indexEntry->offset, hashValue, (uint16)highHash,
			// (uint16)lowHash);

			// Has this entry offset been seen before?
			if (!(hashTable[highHash] & lowHash)) {
				// printf("New Entry\n");
				hashTable[highHash] |= lowHash; // Mark this offset so we don't add it twice

				entry = GetEntry(index);
				src = entry->String();
				while (*src && !isalpha(*src))
					src++;
				dst = word;
				while (*src && *src != '/')
					*dst++ = *src++;
				*dst = 0;
				if (*src == '/')
					suffixFlags = src + 1;
				else
					suffixFlags = src;

				// printf("Base Word: %s\n", word);
				// printf("Flags: %s\n", suffixFlags);
				testWord = word; // Test the base word first
				do {
					// printf("Testing: %s\n", testWord);
					// Does this word match the key
					if ((GetKey(testWord) == key)
					// And does it look close enough to the compare key?
					// word_match(gCmpKey, testWord)
					//	<= int32((strlen(gCmpKey)-1)/2))
						&& word_match(gCmpKey, testWord)
							<= int32(float(strlen(gCmpKey)-1)*.75)) {
						// printf("Added: %s\n", testWord);
						matches->AddItem(new BString(testWord));
					}

					// If suffix, transform and test
					if (*suffixFlags) {
						// Repeat until valid suffix found or end is reached
						suffixLength = 0;
						while (*suffixFlags
							&& !(suffixLength = suffix_word(suffixWord,
								word, *suffixFlags++))) {}
						if (suffixLength)
							testWord = suffixWord;
						else
							testWord = NULL;
					} else {
						testWord = NULL;
					}
				} while (testWord);
				delete entry;
			}
			// else printf("Redundant entry\n");

			index++;
		} while (key == (ItemAt(index))->key);

		return matches->CountItems();
	} else {
		return 0;
	}
}


void
sort_word_list(BList* matches, const char* reference)
{
	if (matches->CountItems() > 0) {
		BString srcWord(reference);
		gCmpKey = srcWord.String();
		matches->SortItems((int(*)(const void*, const void*))word_cmp);
	}
}

