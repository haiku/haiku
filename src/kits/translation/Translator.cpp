/*****************************************************************************/
//               File: Translator.cpp
//              Class: BTranslator
//   Reimplimented by: Michael Wilber, Translation Kit Team
//   Reimplimentation: 2002-06-15
//
// Description: This file contains the BTranslator class, the base class for
//              all translators that don't use the BeOS R4/R4.5 add-on method.
//
//
// Copyright (c) 2002 OpenBeOS Project
//
// Original Version: Copyright 1998, Be Incorporated, All Rights Reserved.
//                   Copyright 1995-1997, Jon Watte
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

#include <Translator.h>

// Set refcount to 1
// ---------------------------------------------------------------
// Constructor
//
// Sets refcount to 1 and creates a semaphore for locking the
// object.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
BTranslator::BTranslator()
{
	fRefCount = 1;
	fSem = create_sem(1, "BTranslator Lock");
}

// ---------------------------------------------------------------
// Destructor
//
// Deletes the semaphore and resets it.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
BTranslator::~BTranslator()
{
	delete_sem(fSem);
	fSem = 0;
}

// ---------------------------------------------------------------
// Acquire
//
// Increments the refcount and returns a pointer to this object.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: NULL, if failed to acquire the sempaphore
//          pointer to this object if successful
// ---------------------------------------------------------------
BTranslator *BTranslator::Acquire()
{
	if (fSem > 0 && acquire_sem(fSem) == B_OK) {
		fRefCount++;
		release_sem(fSem);
		return this;
	} else
		return NULL;
}

// ---------------------------------------------------------------
// Release
//
// Decrements the refcount and returns a pointer to this object.
// When the refcount hits zero, the object is destroyed. This is
// so multiple objects can own the BTranslator and it won't get
// deleted until all of them are done with it.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: NULL, if failed to acquire the sempaphore or the
//                object was just deleted
//          pointer to this object if successful
// ---------------------------------------------------------------
BTranslator *BTranslator::Release()
{
	if (fSem > 0 && acquire_sem(fSem) == B_OK) {
		fRefCount--;
		if (fRefCount > 0) {
			release_sem(fSem);
			return this;
		} else {
			delete this;
			return NULL;
		}
	} else
		return NULL;
}

// ---------------------------------------------------------------
// ReferenceCount
//
// This function returns the current
// refcount. Notice that it is not thread safe.
// This function is only meant for fun/debugging.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: the current refcount
// ---------------------------------------------------------------
int32 BTranslator::ReferenceCount()
{
	return fRefCount;
}

// ---------------------------------------------------------------
// MakeConfigurationView
//
// This virtual function is for creating a configuration view
// for the translator so the user can change its settings. This
// base class version does nothing. Not all BTranslator derived
// object are required to support this function.
//
// Preconditions:
//
// Parameters: ioExtension, configuration data for the translator
//             outView, where the created view is stored
//             outText, the bounds of the view are stored here
//
// Postconditions:
//
// Returns: B_ERROR
// ---------------------------------------------------------------
status_t BTranslator::MakeConfigurationView(BMessage *ioExtension,
	BView **outView, BRect *outExtent)
{
	return B_ERROR;
}

// ---------------------------------------------------------------
// GetConfigurationMessage
//
// Puts the current configuration for the translator into
// ioExtension. Not all translators are required to support
// this function
//
// Preconditions:
//
// Parameters: ioExtension, where the configuration data is stored
//
// Postconditions:
//
// Returns: B_ERROR
// ---------------------------------------------------------------
status_t BTranslator::GetConfigurationMessage(BMessage *ioExtension)
{
	return B_ERROR;
}

// ---------------------------------------------------------------
// _Reserved_Translator_0
//
// It does nothing! :) Its only here for past/future binary
// compatiblity.
//
// Preconditions:
//
// Parameters: n, not used
//             p, not used
//
// Postconditions:
//
// Returns: B_ERROR
// ---------------------------------------------------------------
status_t BTranslator::_Reserved_Translator_0(int32 n, void *p)
{
	return B_ERROR;
}

// ---------------------------------------------------------------
// _Reserved_Translator_1
//
// It does nothing! :) Its only here for past/future binary
// compatiblity.
//
// Preconditions:
//
// Parameters: n, not used
//             p, not used
//
// Postconditions:
//
// Returns: B_ERROR
// ---------------------------------------------------------------
status_t BTranslator::_Reserved_Translator_1(int32 n, void *p)
{
	return B_ERROR;
}

// ---------------------------------------------------------------
// _Reserved_Translator_2
//
// It does nothing! :) Its only here for past/future binary
// compatiblity.
//
// Preconditions:
//
// Parameters: n, not used
//             p, not used
//
// Postconditions:
//
// Returns: B_ERROR
// ---------------------------------------------------------------
status_t BTranslator::_Reserved_Translator_2(int32 n, void *p)
{
	return B_ERROR;
}

// ---------------------------------------------------------------
// _Reserved_Translator_3
//
// It does nothing! :) Its only here for past/future binary
// compatiblity.
//
// Preconditions:
//
// Parameters: n, not used
//             p, not used
//
// Postconditions:
//
// Returns: B_ERROR
// ---------------------------------------------------------------
status_t BTranslator::_Reserved_Translator_3(int32 n, void *p)
{
	return B_ERROR;
}

// ---------------------------------------------------------------
// _Reserved_Translator_4
//
// It does nothing! :) Its only here for past/future binary
// compatiblity.
//
// Preconditions:
//
// Parameters: n, not used
//             p, not used
//
// Postconditions:
//
// Returns: B_ERROR
// ---------------------------------------------------------------
status_t BTranslator::_Reserved_Translator_4(int32 n, void *p)
{
	return B_ERROR;
}

// ---------------------------------------------------------------
// _Reserved_Translator_5
//
// It does nothing! :) Its only here for past/future binary
// compatiblity.
//
// Preconditions:
//
// Parameters: n, not used
//             p, not used
//
// Postconditions:
//
// Returns: B_ERROR
// ---------------------------------------------------------------
status_t BTranslator::_Reserved_Translator_5(int32 n, void *p)
{
	return B_ERROR;
}

// ---------------------------------------------------------------
// _Reserved_Translator_6
//
// It does nothing! :) Its only here for past/future binary
// compatiblity.
//
// Preconditions:
//
// Parameters: n, not used
//             p, not used
//
// Postconditions:
//
// Returns: B_ERROR
// ---------------------------------------------------------------
status_t BTranslator::_Reserved_Translator_6(int32 n, void *p)
{
	return B_ERROR;
}

// ---------------------------------------------------------------
// _Reserved_Translator_7
//
// It does nothing! :) Its only here for past/future binary
// compatiblity.
//
// Preconditions:
//
// Parameters: n, not used
//             p, not used
//
// Postconditions:
//
// Returns: B_ERROR
// ---------------------------------------------------------------
status_t BTranslator::_Reserved_Translator_7(int32 n, void *p)
{
	return B_ERROR;
}
