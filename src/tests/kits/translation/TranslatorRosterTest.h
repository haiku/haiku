/*****************************************************************************/
// OpenBeOS Translation Kit Test
//
// Version: 0.1.0
//
// This is the Test application for BTranslatorRoster
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
#ifndef __TRANSLATOR_ROSTER_TEST
#define __TRANSLATOR_ROSTER_TEST

#include <TranslatorRoster.h>

class TranslatorRosterTest {
public:
    TranslatorRosterTest();
    ~TranslatorRosterTest();
    
    status_t Initialize();
    status_t Perform();
    inline void setVerbose(bool verbose) { this->verbose = verbose; };
    
    //actual tests
	status_t ConstructorTest();
	status_t DefaultTest();
	status_t InstantiateTest();
	status_t VersionTest();
	status_t AddTranslatorsTest();
	status_t ArchiveTest();
	status_t GetAllTranslatorsTest();
	status_t GetConfigurationMessageTest();
	status_t GetInputFormatsTest();
	status_t GetOutputFormatsTest();
	status_t GetTranslatorInfoTest();
	status_t GetTranslatorsTest();
	status_t IdentifyTest();
	status_t MakeConfigurationViewTest();
	status_t TranslateTest();
private:
	void Debug(char* string);

	/** default roster used when performing tests */
    BTranslatorRoster* roster;
    
    /** Whether or not to output test info */
    bool verbose;
};
#endif