/*****************************************************************************/
// OpenBeOS Translation Kit Test
// Author: Brian Matzon <brian@matzon.dk>
// Version: 0.1.0
//
// This is the Test application for BBitmapStream
//
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2002 OpenBeOS Project
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
#include "BitmapStreamTest.h"
#include <stdio.h>
#include <TranslatorRoster.h>
#include <Application.h>

/**
 * Default constructor - no work
 */
BitmapStreamTest::BitmapStreamTest() {
}

/**
 * Default destructor - no work
 */
BitmapStreamTest::~BitmapStreamTest() {
}

/**
 * Initializes the test
 *
 * @return B_OK if initialization went ok, B_ERROR if not
 */
status_t BitmapStreamTest::Initialize() {
	//aquire default roster
	roster = BTranslatorRoster::Default();
	if(roster == NULL) {
		Debug("Failed to create aquire default TranslatorRoster\n");
		return B_ERROR;		
	}
	
	//print version information
	if(verbose && roster != NULL) {
		int32 outCurVersion;
		int32 outMinVersion;
		long inAppVersion;
		const char* info = roster->Version(&outCurVersion, &outMinVersion, inAppVersion);
		printf("Default TranslatorRoster aquired. Version: %s\n", info);
	}

	return B_OK;
}

/**
 * Initializes the tests.
 * This will test ALL methods in BTranslatorRoster.
 *
 * @return B_OK if the whole test went ok, B_ERROR if not
 */
status_t BitmapStreamTest::Perform() {
	if(ConstructorTest() != B_OK) {
		Debug("ERROR: ConstructorTest did not complete successfully\n");
		return B_ERROR;
	}
	if(DetachBitmap() != B_OK) {
		Debug("ERROR: DetachBitmap did not complete successfully\n");
		return B_ERROR;
	}
	if(Position() != B_OK) {
		Debug("ERROR: Position did not complete successfully\n");
		return B_ERROR;
	}
	if(ReadAt() != B_OK) {
		Debug("ERROR: ReadAt did not complete successfully\n");
		return B_ERROR;
	}
	if(Seek() != B_OK) {
		Debug("ERROR: Seek did not complete successfully\n");
		return B_ERROR;
	}
	if(SetSize() != B_OK) {
		Debug("ERROR: SetSize did not complete successfully\n");
		return B_ERROR;
	}	
	if(WriteAt() != B_OK) {
		Debug("ERROR: WriteAt did not complete successfully\n");
		return B_ERROR;
	}
	if(Size() != B_OK) {
		Debug("ERROR: Size did not complete successfully\n");
		return B_ERROR;
	}
	
	return B_OK;
}
/**
 * Tests:
 * BBitmapStream(BBitmap *map = NULL)
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t BitmapStreamTest::ConstructorTest() {
	return B_OK;
}

/**
 * Tests:
 * status_t DetachBitmap(BBitmap **outMap)
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t BitmapStreamTest::DetachBitmap() {
	BFile file("./data/images/image.gif", B_READ_ONLY);
	if(file.InitCheck() != B_OK) {
		return B_ERROR;
	}

	BBitmapStream stream;
	BBitmap* result = NULL;
	if (roster->Translate(&file, NULL, NULL, &stream, B_TRANSLATOR_BITMAP) < B_OK) {
		return B_ERROR;
	}
	stream.DetachBitmap(&result);
	
	if(result != NULL) {
		return B_OK;
	}
	
	return B_ERROR;
}

/**
 * Tests:
 * off_t Position() const
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t BitmapStreamTest::Position() {
	return B_OK;
}

/**
 * Tests:
 * ssize_t ReadAt(off_t pos, void *buffer, size_t *size)
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t BitmapStreamTest::ReadAt() {
	return B_OK;
}

/**
 * Tests:
 * off_t Seek(off_t position, uint32 whence)
 * 
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t BitmapStreamTest::Seek() {
	return B_OK;
}

/**
 * Tests:
 * status_t SetSize(off_t size) const
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t BitmapStreamTest::SetSize() {
	return B_OK;	
}

/**
 * Tests:
 * ssize_t WriteAt(off_t pos, const void *data, size_t *size)
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t BitmapStreamTest::WriteAt() {
	return B_OK;
}

/**
 * Tests:
 * off_t Size() const
 *
 * @return B_OK if everything went ok, B_ERROR if not
 */
status_t BitmapStreamTest::Size() {
	return B_OK;
}
/**
 * Prints debug information to stdout, if verbose is set to true
 */
void BitmapStreamTest::Debug(char* string) {
	if(verbose) {
		printf(string);
	}
}

int main() {
	BApplication app("application/x-vnd.OpenBeOS-translationkit_bitmapstreamtest");
	BitmapStreamTest test;
	test.setVerbose(true);
	test.Initialize();
	if(test.Perform() != B_OK) {
		printf("Tests did not complete successfully\n");
	} else {
		printf("All tests completed without errors\n");
	}
}