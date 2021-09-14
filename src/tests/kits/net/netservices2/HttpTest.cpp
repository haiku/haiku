/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include "HttpTest.h"

#include <cppunit/TestSuite.h>

using BPrivate::Network::BHttpSession;


HttpTest::HttpTest(BHttpSession& session)
	:fSession(session)
{

}


/* static */ void
HttpTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("HttpTest");

	BHttpSession session;

	HttpTest* httpTest = new HttpTest(session);
		// leak for now
	parent.addTest("HttpTest", &suite);
}
