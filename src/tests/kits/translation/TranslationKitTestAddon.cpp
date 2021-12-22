/*****************************************************************************/
// Haiku Translation Kit Test Addon
// Author: Brian Matzon <brian@matzon.dk>
// Version: 0.1.0
//
// This is the Test Addon which is used by the Unittester to load tests.
//
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2002 Haiku Project
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
#include <TestSuite.h>
#include <TestSuiteAddon.h>
#include "TranslatorRosterTest.h"
#include "BitmapStreamTest.h"
#include "TranslationUtilsTest.h"
#include "TranslatorTest.h"

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("Translation");
	suite->addTest("TranslatorRosterTest", TranslatorRosterTest::Suite());
	suite->addTest("BitmapStreamTest", BitmapStreamTest::Suite());
	suite->addTest("TranslationUtilsTest", TranslationUtilsTest::Suite());
	suite->addTest("TranslatorTest", TranslatorTest::Suite());
	return suite;
}
