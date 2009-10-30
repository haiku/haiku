/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */


#ifndef KeyInfos_h
#define KeyInfos_h

#include <SupportDefs.h>

// Returns an ASCII string for the given key index, or NULL if a bad code is 
// given.
const char* GetKeyName(uint8 keyIndex);

// Inverse of GetKeyName(). Finds the index of the given string. Returns 0 if 
// the string was not found in the set of key names.
uint8 FindKeyCode(const char* keyName);

// Returns the UTF8 value for the given key, or "\0" if none.
const char* GetKeyUTF8(uint8 keyIndex);

// Returns the number of key indices that are known. (Currently 256). Indices 
// (0...GetNumKeyIndices()-1) are valid.
int GetNumKeyIndices();

// Should be called at startup.
void InitKeyIndices();

#endif
