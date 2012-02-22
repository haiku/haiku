/*
 * Copyright 2009, Dana Burkart
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2010, Rene Gollent (anevilyak@gmail.com)
 * Distributed under the terms of the MIT License.
 */


#include <NaturalCompare.h>

#include <ctype.h>
#include <string.h>

#include <StorageDefs.h>
#include <SupportDefs.h>


namespace BPrivate {


// #pragma mark - Natural sorting


struct natural_chunk {
	enum chunk_type {
		NUMBER,
		ASCII,
		END
	};
	chunk_type	type;
	char		buffer[B_FILE_NAME_LENGTH];
	int32		length;
};


inline int32
FetchNaturalChunk(natural_chunk& chunk, const char* source)
{
	if (chunk.type == natural_chunk::ASCII) {
		// string chunk
		int32 pos = 0;
		while (!isdigit(source[pos]) && !isspace(source[pos])
			&& source[pos] != '\0') {
			pos++;
		}
		strlcpy(chunk.buffer, source, pos + 1);
		chunk.length = pos;
		return pos;
	}

	// Skip leading zeros and whitespace characters
	int32 skip = 0;
	while (source[0] == '0' || isspace(source[0])) {
		source++;
		skip++;
	}

	// Number chunk (stop at next white space)
	int32 pos = 0;
	while (isdigit(source[pos])) {
		pos++;
	}

	strlcpy(chunk.buffer, source, pos + 1);
	chunk.length = pos;

	// Skip trailing whitespace as well
	while (isspace(source[pos])) {
		source++;
		skip++;
	}

	return pos + skip;
}


//! Compares two strings naturally, as opposed to lexicographically
int
NaturalCompare(const char* stringA, const char* stringB)
{
	if (stringA == NULL)
		return stringB == NULL ? 0 : -1;
	if (stringB == NULL)
		return 1;

	natural_chunk a;
	natural_chunk b;

	uint32 indexA = 0;
	uint32 indexB = 0;

	while (true) {
		// Determine type of next chunks in each string based on first char
		if (stringA[indexA] == '\0')
			a.type = natural_chunk::END;
		else if (isdigit(stringA[indexA]) || isspace(stringA[indexA]))
			a.type = natural_chunk::NUMBER;
		else
			a.type = natural_chunk::ASCII;

		if (stringB[indexB] == '\0')
			b.type = natural_chunk::END;
		else if (isdigit(stringB[indexB]) || isspace(stringB[indexB]))
			b.type = natural_chunk::NUMBER;
		else
			b.type = natural_chunk::ASCII;

		// Check if we reached the end of either string
		if (a.type == natural_chunk::END)
			return b.type == natural_chunk::END ? 0 : -1;
		if (b.type == natural_chunk::END)
			return 1;

		if (a.type != b.type) {
			// Different chunk types, just compare the remaining strings
			return strcasecmp(&stringA[indexA], &stringB[indexB]);
		}

		// Fetch the next chunks
		indexA += FetchNaturalChunk(a, &stringA[indexA]);
		indexB += FetchNaturalChunk(b, &stringB[indexB]);

		// Compare the two chunks based on their type
		if (a.type == natural_chunk::ASCII) {
			// String chunks
			int result = strcasecmp(a.buffer, b.buffer);
			if (result != 0)
				return result;
		} else {
			// Number chunks - they are compared as strings to allow an
			// almost arbitrary number of digits.
			if (a.length > b.length)
				return 1;
			if (a.length < b.length)
				return -1;

			int result = strcmp(a.buffer, b.buffer);
			if (result != 0)
				return result;
		}

		// The chunks were equal, proceed with the next chunk
	}

	return 0;
}


} // namespace BPrivate
