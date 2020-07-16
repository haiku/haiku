/*
 * Copyright 2014-2020 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef FILE_TEST_H
#define FILE_TEST_H


#include <Url.h>

#include <TestCase.h>
#include <TestSuite.h>

#include <cppunit/TestSuite.h>
#include <tools/cppunit/ThreadedTestCase.h>

#include "TestServer.h"


class FileTest : public BThreadedTestCase {
public:
						FileTest() {};

			void		StopTest();

	static	void		AddTests(BTestSuite& suite);
};


#endif
