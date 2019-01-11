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


void
GetAddrInfoTest::AddrConfigTest()
{
	struct addrinfo *info = NULL, hints = {0};
	hints.ai_flags = AI_ADDRCONFIG;

	// localhost is guaranteed to have an IPv6 address.
	int result = getaddrinfo("localhost", NULL, &hints, &info);
	CPPUNIT_ASSERT(result == 0);
	CPPUNIT_ASSERT(info != NULL);

	bool check;
	for (struct addrinfo* iter = info; iter != NULL; iter = iter->ai_next) {
		// only IPv4 addresses should be returned as we don't have IPv6 routing.
		check = iter->ai_family == AF_INET;
		if (!check) break;
	}

	freeaddrinfo(info);
	CPPUNIT_ASSERT(check);
}


/* static */ void
GetAddrInfoTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("GetAddrInfoTest");

	suite.addTest(new CppUnit::TestCaller<GetAddrInfoTest>(
		"GetAddrInfoTest::EmptyTest", &GetAddrInfoTest::EmptyTest));
	suite.addTest(new CppUnit::TestCaller<GetAddrInfoTest>(
		"GetAddrInfoTest::AddrConfigTest", &GetAddrInfoTest::AddrConfigTest));

	parent.addTest("GetAddrInfoTest", &suite);
}
