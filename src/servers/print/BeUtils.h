/*****************************************************************************/
// BeUtils.cpp
//
// Version: 1.0.0d1
//
// Several utilities for writing applications for the BeOS. It are small
// very specific functions, but generally useful (could be here because of a
// lack in the APIs, or just sheer lazyness :))
//
// Authors
//   Ithamar R. Adema
//   Michael Pfeiffer
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef BEUTILS_H
#define BEUTILS_H

#include <FindDirectory.h>
#include <Path.h>
#include <SupportDefs.h>

status_t TestForAddonExistence(const char* name, directory_which which,
							   const char* section, BPath& outPath);

// Reference counted object
class Object {
private:
	volatile int32 fRefCount;

public:
	// After construction reference count is 1
	Object() : fRefCount(1) { }
	// dtor should be private, but ie. ObjectList requires a public dtor!
	virtual ~Object() { };

	// thread-safe as long as thread that calls acquire has already
	// a reference to the object
	void Acquire() { 
		atomic_add(&fRefCount, 1);
	}

	bool Release() { 
		atomic_add(&fRefCount, -1);
		if (fRefCount == 0) { 
			delete this; return true; 
		} else { 
			return false; 
		} 
	}
};

#endif
