/*****************************************************************************/
// OpenBeOS Translation Kit Test
//
// Version: 0.1.0
// Author: Brian Matzon <brian@matzon.dk>
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
#ifndef __BITMAP_STEAM_TEST
#define __BITMAP_STEAM_TEST

#include <BitmapStream.h>
#include <File.h>

class BitmapStreamTest {
public:
    BitmapStreamTest();
    ~BitmapStreamTest();
    
    status_t Initialize();
    status_t Perform();
    inline void setVerbose(bool verbose) { this->verbose = verbose; };
    
    //actual tests
	status_t ConstructorTest();
	status_t DetachBitmap();
	status_t Position();
	status_t ReadAt();
	status_t Seek();
	status_t SetSize();
	status_t WriteAt();
	status_t Size();
private:
	void Debug(char* string);
	
	/** default roster used when performing tests */
    BTranslatorRoster* roster;
    
    /** File to read from */
    BFile* file;

    /** Whether or not to output test info */
    bool verbose;
};
#endif