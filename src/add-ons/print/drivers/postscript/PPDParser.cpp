/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Ithamar R. Adema <ithamar.adema@team-embedded.nl>
*/


#include "PPDParser.h"

#include <File.h>
#include <Path.h>


PPDParser::PPDParser(BFile& file)
{
	InitData(file);
}


PPDParser::PPDParser(const BDirectory& dir, const char* fname)
{
	BFile file(&dir, fname, B_READ_ONLY);
	InitData(file);
}


PPDParser::PPDParser(const BPath& path)
{
	BFile file(path.Path(), B_READ_ONLY);
	InitData(file);
}


status_t
PPDParser::InitData(BFile& file)
{
	// Check if file exists...
	if ((fInitErr = file.InitCheck()) != B_OK)
		return fInitErr;

	// Read entire file into BString
	ssize_t len;
	char buffer[1025];
	while ((len = file.Read(buffer, sizeof(buffer)-1)) > 0) {
		buffer[len] = '\0';
		fContent << buffer;
	}

	// Convert DOS/Windows newlines to UNIX ones
	fContent.ReplaceAll("\r\n", "\n");
	// Handle line continuation
	fContent.ReplaceAll("&&\n", "");
	// Make sure file ends with newline
	fContent << '\n';

	return B_OK;
}


BString
PPDParser::GetParameter(const BString& param)
{
	BString result, line;
	int32 pos = 0, next;
	BString pattern;

	pattern << "*" << param << ":";

	while ((next = fContent.FindFirst('\n', pos)) > 0) {
		// Grab line (without newline)
		fContent.CopyInto(line, pos, next - pos);
		// Found our parameter?
		if (line.Compare(pattern, pattern.Length()) == 0) {
			// Copy result
			line.CopyInto(result, pattern.Length(),
				line.Length() - pattern.Length()).Trim();
			// If result is quoted, remove quotes
			if (result[0] == '"') {
				result.Truncate(result.Length() -1);
				result.Remove(0, 1);
			}
			break;
		}
		pos = next +1;
	}

	return result;
}


PPDParser::~PPDParser()
{
}
