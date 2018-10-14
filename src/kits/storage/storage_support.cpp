//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//----------------------------------------------------------------------
/*!
	\file storage_support.cpp
	Implementations of miscellaneous internal Storage Kit support functions.
*/

#include <new>
#include <ctype.h>
#include <string.h>

#include <StorageDefs.h>
#include <SupportDefs.h>

#include <syscalls.h>

#include "storage_support.h"

using std::nothrow;

namespace BPrivate {
namespace Storage {

/*!	\param path the path
	\return \c true, if \a path is not \c NULL and absolute, \c false otherwise
*/
bool
is_absolute_path(const char *path)
{
	return (path && path[0] == '/');
}

// parse_path
/*!	\brief Parses the supplied path and returns the position of the leaf name
		   part of the path and the length of its directory path part.

	The value returned in \a fullPath is guaranteed to be > 0, i.e. the
	function always returns a non-empty directory path part. The leaf name
	part may be empty though (i.e. \code leafStart == leafEnd \endcode), which
	will happen, if the supplied path consists only of one component.

	\param fullPath The path to be parsed.
	\param dirEnd Reference to a variable into which the end index of the
		   directory part shall be written. The index is exclusive.
	\param leafStart Reference to a variable into which the start index of
		   the leaf name part shall be written. The index is inclusive.
	\param leafEnd Reference to a variable into which the end index of
		   the leaf name part shall be written. The index is exclusive.
	\return \c B_OK, if everything went fine, B_BAD_VALUE, if the supplied
		   path is invalid.
*/
status_t
parse_path(const char *fullPath, int &dirEnd, int &leafStart, int &leafEnd)
{
	// check path and get length
	if (!fullPath)
		return B_BAD_VALUE;
	int pathLen = strlen(fullPath);
	if (pathLen == 0)
		return B_BAD_VALUE;
	// find then end of the leaf name (skip trailing '/')
	int i = pathLen - 1;
	while (i >= 0 && fullPath[i] == '/')
		i--;
	leafEnd = i + 1;
	if (leafEnd == 0) {
		// fullPath consists of slashes only
		dirEnd = leafStart = leafEnd = 1;
		return B_OK;
	}
	// find the start of the leaf name
	while (i >= 0 && fullPath[i] != '/')
		i--;
	leafStart = i + 1;
	if (leafStart == 0) {
		// fullPath contains only one component
		dirEnd = leafStart = leafEnd;
		return B_OK;
	}
	// find the end of the dir path
	while (i >= 0 && fullPath[i] == '/')
		i--;
	dirEnd = i + 1;
	if (dirEnd == 0)	// => fullPath[0] == '/' (an absolute path)
		dirEnd = 1;
	return B_OK;
}

// parse_path
/*!	\brief Parses the supplied path and returns the leaf name part of the path
		   and its directory path part.

	The value returned in \a fullPath is guaranteed to be > 0, i.e. the
	function always returns a non-empty directory path part. The leaf name
	part may be empty though (i.e. \code leafStart == leafEnd \endcode), which
	will happen, if the supplied path consists only of one component.

	\param fullPath The path to be parsed.
	\param dirPath Pointer to a character array of size \c B_PATH_NAME_LENGTH
		   or greater, into which the directory part shall be written.
		   May be \c NULL.
	\param leaf Pointer to a character array of size \c B_FILE_NAME_LENGTH
		   or greater, into which the leaf name part shall be written.
		   May be \c NULL.
	\return \c B_OK, if everything went fine, B_BAD_VALUE, if the supplied
		   path is invalid.
*/
status_t
parse_path(const char *fullPath, char *dirPath, char *leaf)
{
	// parse the path and check the lengths
	int leafStart, leafEnd, dirEnd;
	status_t error = parse_path(fullPath, dirEnd, leafStart, leafEnd);
	if (error != B_OK)
		return error;
	if (dirEnd >= B_PATH_NAME_LENGTH
		|| leafEnd - leafStart >= B_FILE_NAME_LENGTH) {
		return B_NAME_TOO_LONG;
	}
	// copy the result strings
	if (dirPath)
		strlcpy(dirPath, fullPath, dirEnd + 1);
	if (leaf)
		strlcpy(leaf, fullPath + leafStart, leafEnd - leafStart + 1);
	return B_OK;
}

// internal_parse_path
static
void
internal_parse_path(const char *fullPath, int &leafStart, int &leafEnd,
	int &pathEnd)
{
	if (fullPath == NULL)
		return;

	enum PathParserState { PPS_START, PPS_LEAF } state = PPS_START;

	int len = strlen(fullPath);
	
	leafStart = -1;
	leafEnd = -1;
	pathEnd = -2;
	
	bool loop = true;
	for (int pos = len-1; ; pos--) {
		if (pos < 0)
			break;
			
		switch (state) {
			case PPS_START:
				// Skip all trailing '/' chars, then move on to
				// reading the leaf name
				if (fullPath[pos] != '/') {
					leafEnd = pos;
					state = PPS_LEAF;
				}
				break;
			
			case PPS_LEAF:
				// Read leaf name chars until we hit a '/' char
				if (fullPath[pos] == '/') {
					leafStart = pos+1;
					pathEnd = pos-1;
					loop = false;
				}
				break;					
		}
		
		if (!loop)
			break;
	}
}

/*!	The caller is responsible for deleting the returned directory path name
	and the leaf name.
	\param fullPath the path name to be split
	\param path a variable the directory path name pointer shall
		   be written into, may be NULL
	\param leaf a variable the leaf name pointer shall be
		   written into, may be NULL
*/
status_t
split_path(const char *fullPath, char *&path, char *&leaf)
{
	return split_path(fullPath, &path, &leaf);
}

/*!	The caller is responsible for deleting the returned directory path name
	and the leaf name.
	\param fullPath the path name to be split
	\param path a pointer to a variable the directory path name pointer shall
		   be written into, may be NULL
	\param leaf a pointer to a variable the leaf name pointer shall be
		   written into, may be NULL
*/
status_t
split_path(const char *fullPath, char **path, char **leaf)
{
	if (path)
		*path = NULL;
	if (leaf)
		*leaf = NULL;

	if (fullPath == NULL)
		return B_BAD_VALUE;

	int leafStart, leafEnd, pathEnd, len;
	internal_parse_path(fullPath, leafStart, leafEnd, pathEnd);

	try {
		// Tidy up/handle special cases
		if (leafEnd == -1) {
	
			// Handle special cases 
			if (fullPath[0] == '/') {
				// Handle "/"
				if (path) {
					*path = new char[2];
					(*path)[0] = '/';
					(*path)[1] = 0;
				}
				if (leaf) {
					*leaf = new char[2];
					(*leaf)[0] = '.';
					(*leaf)[1] = 0;
				}
				return B_OK;	
			} else if (fullPath[0] == 0) {
				// Handle "", which we'll treat as "./"
				if (path) {
					*path = new char[1];
					(*path)[0] = 0;
				}
				if (leaf) {
					*leaf = new char[2];
					(*leaf)[0] = '.';
					(*leaf)[1] = 0;
				}
				return B_OK;	
			}
	
		} else if (leafStart == -1) {
			// fullPath is just an entry name, no parent directories specified
			leafStart = 0;
		} else if (pathEnd == -1) {
			// The path is '/' (since pathEnd would be -2 if we had
			// run out of characters before hitting a '/')
			pathEnd = 0;
		}
	
		// Alloc new strings and copy the path and leaf over
		if (path) {
			if (pathEnd == -2) {
				// empty path
				*path = new char[2];
				(*path)[0] = '.';
				(*path)[1] = 0;
			} else {
				// non-empty path
				len = pathEnd + 1;
				*path = new char[len+1];
				memcpy(*path, fullPath, len);
				(*path)[len] = 0;
			}
		}
		if (leaf) {
			len = leafEnd - leafStart + 1;
			*leaf = new char[len+1];
			memcpy(*leaf, fullPath + leafStart, len);
			(*leaf)[len] = 0;
		}
	} catch (std::bad_alloc& exception) {
		if (path)
			delete[] *path;
		if (leaf)
			delete[] *leaf;
		return B_NO_MEMORY;
	}
	return B_OK;
}

/*! The length of the first component is returned as well as the index at
	which the next one starts. These values are only valid, if the function
	returns \c B_OK.
	\param path the path to be parsed
	\param length the variable the length of the first component is written
		   into
	\param nextComponent the variable the index of the next component is
		   written into. \c 0 is returned, if there is no next component.
	\return \c B_OK, if \a path is not \c NULL, \c B_BAD_VALUE otherwise
*/
status_t
parse_first_path_component(const char *path, int32& length,
						   int32& nextComponent)
{
	status_t error = (path ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		int32 i = 0;
		// find first '/' or end of name
		for (; path[i] != '/' && path[i] != '\0'; i++);
		// handle special case "/..." (absolute path)
		if (i == 0 && path[i] != '\0')
			i = 1;
		length = i;
		// find last '/' or end of name
		for (; path[i] == '/' && path[i] != '\0'; i++);
		if (path[i] == '\0')	// this covers "" as well
			nextComponent = 0;
		else
			nextComponent = i;
	}
	return error;
}

/*! A string containing the first component is returned and the index, at
	which the next one starts. These values are only valid, if the function
	returns \c B_OK.
	\param path the path to be parsed
	\param component the variable the pointer to the newly allocated string
		   containing the first path component is written into. The caller
		   is responsible for delete[]'ing the string.
	\param nextComponent the variable the index of the next component is
		   written into. \c 0 is returned, if there is no next component.
	\return \c B_OK, if \a path is not \c NULL, \c B_BAD_VALUE otherwise
*/
status_t
parse_first_path_component(const char *path, char *&component,
						   int32& nextComponent)
{
	int32 length;
	status_t error = parse_first_path_component(path, length, nextComponent);
	if (error == B_OK) {
		component = new(nothrow) char[length + 1];
		if (component) {
			strncpy(component, path, length);
			component[length] = '\0';
		} else
			error = B_NO_MEMORY;
	}
	return error;
}

/*!	An entry name is considered valid, if its length doesn't exceed
	\c B_FILE_NAME_LENGTH (including the terminating null) and it doesn't
	contain any \c "/".
	\param entry the entry name
	\return
	- \c B_OK, if \a entry is valid,
	- \c B_BAD_VALUE, if \a entry is \c NULL or contains a "/",
	- \c B_NAME_TOO_LONG, if \a entry is too long
	\note \c "" is considered a valid entry name.
*/
status_t
check_entry_name(const char *entry)
{
	status_t error = (entry ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (strlen(entry) >= B_FILE_NAME_LENGTH)
			error = B_NAME_TOO_LONG;
	}
	if (error == B_OK) {
		for (int32 i = 0; error == B_OK && entry[i] != '\0'; i++) {
			if (entry[i] == '/')
				error = B_BAD_VALUE;
		}
	}
	return error;
}

/*!	An path name is considered valid, if its length doesn't exceed
	\c B_PATH_NAME_LENGTH (including the terminating null) and each of
	its components is a valid entry name.
	\param entry the entry name
	\return
	- \c B_OK, if \a path is valid,
	- \c B_BAD_VALUE, if \a path is \c NULL,
	- \c B_NAME_TOO_LONG, if \a path, or any of its components is too long
	\note \c "" is considered a valid path name.
*/
status_t
check_path_name(const char *path)
{
	// check the path is not NULL
	status_t error = (path ? B_OK : B_BAD_VALUE);
	if (error == B_BAD_VALUE)
		return error;
	// check the path components
	const char *remainder = path;
	int32 length, nextComponent;
	do {
		error = parse_first_path_component(remainder, length, nextComponent);
		if (error == B_OK) {
			if (length >= B_FILE_NAME_LENGTH)
				error = B_NAME_TOO_LONG;
			remainder += nextComponent;
		}
	} while (error == B_OK && nextComponent != 0);
	// check the length of the path
	if (error == B_OK && strlen(path) >= B_PATH_NAME_LENGTH)
		error = B_NAME_TOO_LONG;
	return error;
}

std::string
to_lower(const char *str)
{
	std::string result;
	to_lower(str, result);
	return result;
}

void
to_lower(const char *str, std::string &result)
{
	if (str) {
		result = "";
		for (int i = 0; i < (int)strlen(str); i++)
			result += tolower(str[i]);
	} else
		result = "(null)";
}

void
to_lower(const char *str, char *result)
{
	if (str && result) {
		int i;
		for (i = 0; i < (int)strlen(str); i++)
			result[i] = tolower(str[i]);
		result[i] = 0;
	}	
}

void
to_lower(char *str)
{
	to_lower(str, str);
}

void escape_path(const char *str, char *result)
{
	if (str && result) {
		int32 len = strlen(str);
		
		for (int32 i = 0; i < len; i++) {
			char ch = str[i];
			char escapeChar = 0;
			
			switch (ch) {
				case ' ':
				case '\'':
				case '"':
				case '?':
				case '\\':
				case '(':
				case ')':
				case '[':
				case ']':
				case '*':
				case '^':
					escapeChar = ch;
					break;			
			}
			
			if (escapeChar) {
				*(result++) = '\\';
				*(result++) = escapeChar;
			} else {
				*(result++) = ch;
			}
		}
		
		*result = 0;	
	}
}

void escape_path(char *str)
{
	if (str) {
		char *copy = new(nothrow) char[strlen(str)+1];
		if (copy) {
			strcpy(copy, str);
			escape_path(copy, str);
		}
		delete [] copy;
	}
}

// device_is_root_device
bool
device_is_root_device(dev_t device)
{
	return device == 1;
}

// Close
void
FDCloser::Close()
{
	if (fFD >= 0)
		_kern_close(fFD);
	fFD = -1;
}

};	// namespace Storage
};	// namespace BPrivate

