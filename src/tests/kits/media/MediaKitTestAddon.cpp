/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "AreaTest.h"
#include "BufferTest.h"
#include "FormatDescriptions.h"
#include "SizeofTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("MediaKit");

	AreaTest::AddTests(*suite);
	BufferTest::AddTests(*suite);
	FormatDescriptionsTest::AddTests(*suite);
	SizeofTest::AddTests(*suite);

	return suite;
}

