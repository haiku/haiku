/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

// Include header file for PathBlocklist class definition
#include <boot/PathBlocklist.h>
// Include standard library for memory management functions
#include <stdlib.h>
// Include algorithm library for std::max function
#include <algorithm>

// #pragma mark - BlockedPath

// Default constructor for BlockedPath class
// Initializes all member variables to their default empty state
BlockedPath::BlockedPath()
	:
	fPath(NULL),
	fLength(0),
	fCapacity(0)
{
}

// Destructor for BlockedPath class
// Frees dynamically allocated memory for the path string
BlockedPath::~BlockedPath()
{
	free(fPath);
}

// Sets the path string to the provided path
// Removes trailing slash if present and validates the path
// Returns true on success, false on memory allocation failure
bool
BlockedPath::SetTo(const char* path)
{
	// Calculate the length of the input path string
	size_t length = strlen(path);
	
	// Remove trailing slash if present to normalize the path
	if (length > 0 && path[length - 1] == '/')
		length--;
	
	// Attempt to resize the internal buffer to accommodate the new path
	if (!_Resize(length, false))
		return false;
	
	// Copy the path data if the length is greater than zero
	if (length > 0) {
		memcpy(fPath, path, length);
		fPath[length] = '\0';
	}
	return true;
}

// Appends a path component to the existing path
// Handles path separator insertion and component validation
// Returns true on success, false on memory allocation failure
bool
BlockedPath::Append(const char* component)
{
	// Calculate the length of the component to append
	size_t componentLength = strlen(component);
	
	// Remove trailing slash from component if present
	if (componentLength > 0 && component[componentLength - 1] == '/')
		componentLength--;
	
	// If component is empty after trimming, nothing to append
	if (componentLength == 0)
		return true;
	
	// Store the current path length before modification
	size_t oldLength = fLength;
	
	// Calculate new total length including separator if needed
	size_t length = (fLength > 0 ? fLength + 1 : 0) + componentLength;
	
	// Resize the buffer to accommodate the new total length
	if (!_Resize(length, true))
		return false;
	
	// Add path separator if the existing path is not empty
	if (oldLength > 0)
		fPath[oldLength++] = '/';
	
	// Copy the component data to the path buffer
	memcpy(fPath + oldLength, component, componentLength);
	return true;
}

// Internal method to resize the path buffer
// Handles memory allocation, reallocation, and deallocation
// Returns true on success, false on memory allocation failure
bool
BlockedPath::_Resize(size_t length, bool keepData)
{
	// Handle the case where length is zero - free all memory
	if (length == 0) {
		free(fPath);
		fPath = NULL;
		fLength = 0;
		fCapacity = 0;
		return true;
	}
	
	// If requested length fits within current capacity, just update length
	if (length < fCapacity) {
		fPath[length] = '\0';
		fLength = length;
		return true;
	}
	
	// Calculate new capacity - double current capacity or use length + 1
	size_t capacity = std::max(length + 1, 2 * fCapacity);
	
	// Ensure minimum capacity of 32 bytes for efficiency
	capacity = std::max(capacity, size_t(32));
	
	char* path;
	
	// If keeping existing data and path exists, use realloc
	if (fLength > 0 && keepData) {
		path = (char*)realloc(fPath, capacity);
		if (path == NULL)
			return false;
	} else {
		// Otherwise allocate new memory and free old memory
		path = (char*)malloc(capacity);
		if (path == NULL)
			return false;
		free(fPath);
	}
	
	// Update member variables with new buffer information
	fPath = path;
	fPath[length] = '\0';
	fLength = length;
	fCapacity = capacity;
	return true;
}

// #pragma mark - PathBlocklist

// Default constructor for PathBlocklist class
// Initializes an empty list of blocked paths
PathBlocklist::PathBlocklist()
{
}

// Destructor for PathBlocklist class
// Ensures all blocked paths are properly cleaned up
PathBlocklist::~PathBlocklist()
{
	MakeEmpty();
}

// Adds a path to the blocklist
// Checks for duplicates before adding to avoid redundant entries
// Returns true on success, false on memory allocation failure
bool
PathBlocklist::Add(const char* path)
{
	// Check if the path is already in the blocklist
	BlockedPath* blockedPath = _FindPath(path);
	if (blockedPath != NULL)
		return true;
	
	// Create a new BlockedPath object using nothrow allocation
	blockedPath = new(std::nothrow) BlockedPath;
	
	// Verify allocation succeeded and path was set correctly
	if (blockedPath == NULL || !blockedPath->SetTo(path)) {
		delete blockedPath;
		return false;
	}
	
	// Add the new blocked path to the internal list
	fPaths.Add(blockedPath);
	return true;
}

// Removes a path from the blocklist
// Finds and deletes the specified path if it exists
void
PathBlocklist::Remove(const char* path)
{
	// Search for the path in the current blocklist
	BlockedPath* blockedPath = _FindPath(path);
	
	// If found, remove it from the list and delete the object
	if (blockedPath != NULL) {
		fPaths.Remove(blockedPath);
		delete blockedPath;
	}
}

// Checks whether a path is currently blocked
// Returns true if the path exists in the blocklist, false otherwise
bool
PathBlocklist::Contains(const char* path) const
{
	return _FindPath(path) != NULL;
}

// Removes all paths from the blocklist
// Iterates through all blocked paths and deletes them
void
PathBlocklist::MakeEmpty()
{
	// Remove and delete each blocked path until the list is empty
	while (BlockedPath* blockedPath = fPaths.RemoveHead())
		delete blockedPath;
}

// Internal helper method to find a specific path in the blocklist
// Iterates through all blocked paths and compares them with the target
// Returns pointer to BlockedPath if found, NULL otherwise
BlockedPath*
PathBlocklist::_FindPath(const char* path) const
{
	// Iterate through all paths in the blocklist
	for (PathList::ConstIterator it = fPaths.GetIterator(); it.HasNext();) {
		BlockedPath* blockedPath = it.Next();
		
		// Use BlockedPath's equality operator to compare paths
		if (*blockedPath == path)
			return blockedPath;
	}
	
	// Path not found in the blocklist
	return NULL;
}
