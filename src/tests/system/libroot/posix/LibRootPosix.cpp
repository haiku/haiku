/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * Andrew Aldridge, i80and@foxquill.com
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "CryptTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("LibRootPosix");
	CryptTest::AddTests(*suite);
	return suite;
}
