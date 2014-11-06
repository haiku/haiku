/*
 * Copyright 2014, Haiku, inc.
 * Distributed under the terms of the MIT licence
 */


#include "GetAddrInfo.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


GetAddrInfoTest::GetAddrInfoTest()
{
}


GetAddrInfoTest::~GetAddrInfoTest()
{
}


void
GetAddrInfoTest::EmptyTest()
{
	struct addrinfo* info;
	addrinfo hint = {0};
	hint.ai_family = AF_INET;
	hint.ai_flags = AI_PASSIVE;

	// Trying to resolve an empty host...
	int result = getaddrinfo("", NULL, &hint, &info);

	if (info)
		freeaddrinfo(info);

	// getaddrinfo shouldn't suggest that this may work better by trying again.
	CPPUNIT_ASSERT(result != EAI_AGAIN);
}


/* static */ void
GetAddrInfoTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("GetAddrInfoTest");

	suite.addTest(new CppUnit::TestCaller<GetAddrInfoTest>(
		"GetAddrInfoTest::EmptyTest", &GetAddrInfoTest::EmptyTest));

	parent.addTest("GetAddrInfoTest", &suite);
}
