//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file Pattern.cpp
	MIME sniffer pattern implementation
*/

#include <sniffer/Err.h>
#include <sniffer/Pattern.h>
#include <DataIO.h>
#include <stdio.h>	// for SEEK_* defines
#include <new>

#include <AutoDeleter.h>

using namespace BPrivate::Storage::Sniffer;

Pattern::Pattern(const std::string &string, const std::string &mask)
	: fCStatus(B_NO_INIT)
	, fErrorMessage(NULL)
{
	SetTo(string, mask);
}

Pattern::Pattern(const std::string &string)
	: fCStatus(B_NO_INIT)
	, fErrorMessage(NULL)
{
	// Build a mask with all bits turned on of the
	// appropriate length
	std::string mask = "";
	for (uint i = 0; i < string.length(); i++)
		mask += (char)0xFF;
	SetTo(string, mask);
}

Pattern::~Pattern() {
	delete fErrorMessage;
}

status_t
Pattern::InitCheck() const {
	return fCStatus;
}

Err*
Pattern::GetErr() const {
	if (fCStatus == B_OK)
		return NULL;
	else
		return new(std::nothrow) Err(*fErrorMessage);
}

void dumpStr(const std::string &string, const char *label = NULL) {
	if (label)
		printf("%s: ", label);
	for (uint i = 0; i < string.length(); i++)
		printf("%x ", string[i]);
	printf("\n");
}

status_t
Pattern::SetTo(const std::string &string, const std::string &mask) {
	fString = string;
	if (fString.length() == 0) {
		SetStatus(B_BAD_VALUE, "Sniffer pattern error: illegal empty pattern");		
	} else {
		fMask = mask;
//		dumpStr(string, "data");
//		dumpStr(mask, "mask");
		if (fString.length() != fMask.length()) {
			SetStatus(B_BAD_VALUE, "Sniffer pattern error: pattern and mask lengths do not match");
		} else {
			SetStatus(B_OK);
		}		
	}
	return fCStatus;
}

/*! \brief Looks for a pattern match in the given data stream, starting from
	each offset withing the given range. Returns true is a match is found,
	false if not.
*/
bool
Pattern::Sniff(Range range, BPositionIO *data, bool caseInsensitive) const {
	int32 start = range.Start();
	int32 end = range.End();
	off_t size = data->Seek(0, SEEK_END);
	if (end >= size)
		end = size-1;	// Don't bother searching beyond the end of the stream
	for (int i = start; i <= end; i++) {
		if (Sniff(i, size, data, caseInsensitive))
			return true;
	}
	return false;
}

// BytesNeeded
/*! \brief Returns the number of bytes needed to perform a complete sniff, or an error
	code if something goes wrong.
*/
ssize_t
Pattern::BytesNeeded() const
{
	ssize_t result = InitCheck();
	if (result == B_OK)
		result = fString.length();
	return result;
}

//#define OPTIMIZATION_IS_FOR_CHUMPS
#if OPTIMIZATION_IS_FOR_CHUMPS
bool
Pattern::Sniff(off_t start, off_t size, BPositionIO *data, bool caseInsensitive) const {
	off_t len = fString.length();
	char *buffer = new(nothrow) char[len+1];
	if (buffer) {
		ArrayDeleter<char> _(buffer);
		ssize_t bytesRead = data->ReadAt(start, buffer, len);
		// \todo If there are fewer bytes left in the data stream
		// from the given position than the length of our data
		// string, should we just return false (which is what we're
		// doing now), or should we compare as many bytes as we
		// can and return true if those match?
		if (bytesRead < len)
			return false;
		else {
			bool result = true;
			if (caseInsensitive) {
				for (int i = 0; i < len; i++) {
					char secondChar;
					if ('A' <= fString[i] && fString[i] <= 'Z')
						secondChar = 'a' + (fString[i] - 'A');	// Also check lowercase
					else if ('a' <= fString[i] && fString[i] <= 'z')
						secondChar = 'A' + (fString[i] - 'a');	// Also check uppercase
					else
						secondChar = fString[i]; // Check the same char twice as punishment for doing a case insensitive search ;-)
					if (((fString[i] & fMask[i]) != (buffer[i] & fMask[i]))
					     && ((secondChar & fMask[i]) != (buffer[i] & fMask[i])))
					{
						result = false;
						break;
					}
				}
			} else {
				for (int i = 0; i < len; i++) {
					if ((fString[i] & fMask[i]) != (buffer[i] & fMask[i])) {
						result = false;
						break;
					}
				}
			}
			return result;
		}	
	} else
		return false;
}
#else
bool
Pattern::Sniff(off_t start, off_t size, BPositionIO *data, bool caseInsensitive) const {
	off_t len = fString.length();
	char *buffer = new(std::nothrow) char[len+1];
	if (buffer) {
		ArrayDeleter<char> _(buffer);
		ssize_t bytesRead = data->ReadAt(start, buffer, len);
		// \todo If there are fewer bytes left in the data stream
		// from the given position than the length of our data
		// string, should we just return false (which is what we're
		// doing now), or should we compare as many bytes as we
		// can and return true if those match?
		if (bytesRead < len)
			return false;
		else {
			bool result = true;
			if (caseInsensitive) {
				for (int i = 0; i < len; i++) {
					char secondChar;
					if ('A' <= fString[i] && fString[i] <= 'Z')
						secondChar = 'a' + (fString[i] - 'A');	// Also check lowercase
					else if ('a' <= fString[i] && fString[i] <= 'z')
						secondChar = 'A' + (fString[i] - 'a');	// Also check uppercase
					else
						secondChar = fString[i]; // Check the same char twice as punishment for doing a case insensitive search ;-)
					if (((fString[i] & fMask[i]) != (buffer[i] & fMask[i]))
					     && ((secondChar & fMask[i]) != (buffer[i] & fMask[i])))
					{
						result = false;
						break;
					}
				}
			} else {
				for (int i = 0; i < len; i++) {
					if ((fString[i] & fMask[i]) != (buffer[i] & fMask[i])) {
						result = false;
						break;
					}
				}
			}
			return result;
		}	
	} else
		return false;
}
#endif

void
Pattern::SetStatus(status_t status, const char *msg) {
	fCStatus = status;
	if (status == B_OK)
		SetErrorMessage(NULL);
	else {
		if (msg)
			SetErrorMessage(msg);
		else {
			SetErrorMessage("Sniffer parser error: Pattern::SetStatus() -- NULL msg with non-B_OK status.\n"
				"(This is officially the most helpful error message you will ever receive ;-)");
		}
	}
}

void
Pattern::SetErrorMessage(const char *msg) {
	delete fErrorMessage;
	fErrorMessage = (msg) ? (new(std::nothrow) Err(msg, -1)) : (NULL);
}



