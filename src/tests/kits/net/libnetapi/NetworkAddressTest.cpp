/*
 * Copyright 2010-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "NetworkAddressTest.h"

#include <netinet6/in6.h>

#include <NetworkAddress.h>
#include <TypeConstants.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


NetworkAddressTest::NetworkAddressTest()
{
}


NetworkAddressTest::~NetworkAddressTest()
{
}


void
NetworkAddressTest::TestUnset()
{
	BNetworkAddress unset;

	CPPUNIT_ASSERT(unset.Family() == AF_UNSPEC);
	CPPUNIT_ASSERT(unset.Port() == 0);

	BNetworkAddress set(AF_INET, NULL);
	CPPUNIT_ASSERT(set.Family() == AF_INET);
	CPPUNIT_ASSERT(unset != set);

	set.Unset();
	CPPUNIT_ASSERT(unset == set);
}


void
NetworkAddressTest::TestSetTo()
{
	BNetworkAddress address;

	CPPUNIT_ASSERT(address.SetTo("127.0.0.1") == B_OK);
	CPPUNIT_ASSERT(address.Family() == AF_INET);
	CPPUNIT_ASSERT(address == BNetworkAddress(htonl(INADDR_LOOPBACK)));

	CPPUNIT_ASSERT(address.SetTo("::1") == B_OK);
	CPPUNIT_ASSERT(address.Family() == AF_INET6);
	CPPUNIT_ASSERT(address == BNetworkAddress(in6addr_loopback));

	CPPUNIT_ASSERT(address.SetTo(AF_INET, "::1") != B_OK);
	CPPUNIT_ASSERT(address.SetTo(AF_INET6, "127.0.0.1") != B_OK);
	CPPUNIT_ASSERT(address.SetTo(AF_INET, "127.0.0.1") == B_OK);
	CPPUNIT_ASSERT(address.SetTo(AF_INET6, "::1") == B_OK);
}


void
NetworkAddressTest::TestWildcard()
{
	BNetworkAddress wildcard;
	wildcard.SetToWildcard(AF_INET);

	CPPUNIT_ASSERT(wildcard.Family() == AF_INET);
	CPPUNIT_ASSERT(wildcard.Length() == sizeof(sockaddr_in));
	CPPUNIT_ASSERT(wildcard.Port() == 0);
	CPPUNIT_ASSERT(((sockaddr_in&)wildcard.SockAddr()).sin_addr.s_addr
		== INADDR_ANY);

	BNetworkAddress null(AF_INET, NULL);
	CPPUNIT_ASSERT(wildcard == null);
}


void
NetworkAddressTest::TestIsLocal()
{
	BNetworkAddress local(AF_INET, "localhost");
	CPPUNIT_ASSERT(local.IsLocal());

	BNetworkAddress google(AF_INET, "google.com");
	CPPUNIT_ASSERT(!google.IsLocal());
}


void
NetworkAddressTest::TestFlatten()
{
	// IPv4

	BNetworkAddress ipv4(AF_INET, "localhost", 9999);

	char buffer[256];
	CPPUNIT_ASSERT(ipv4.Flatten(buffer, sizeof(buffer)) == B_OK);

	BNetworkAddress unflattened;
	CPPUNIT_ASSERT(unflattened.Unflatten(B_NETWORK_ADDRESS_TYPE, buffer,
		sizeof(buffer)) == B_OK);

	// unflatten buffer too small
	CPPUNIT_ASSERT(unflattened.Unflatten(B_NETWORK_ADDRESS_TYPE, buffer, 0)
		!= B_OK);
	CPPUNIT_ASSERT(unflattened.Unflatten(B_NETWORK_ADDRESS_TYPE, buffer, 3)
		!= B_OK);
	CPPUNIT_ASSERT(unflattened.Unflatten(B_NETWORK_ADDRESS_TYPE, buffer, 16)
		!= B_OK);

	CPPUNIT_ASSERT(ipv4 == unflattened);
}


/*static*/ void
NetworkAddressTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("NetworkAddressTest");

	suite.addTest(new CppUnit::TestCaller<NetworkAddressTest>(
		"NetworkAddressTest::TestUnset", &NetworkAddressTest::TestUnset));
	suite.addTest(new CppUnit::TestCaller<NetworkAddressTest>(
		"NetworkAddressTest::TestSetTo", &NetworkAddressTest::TestSetTo));
	suite.addTest(new CppUnit::TestCaller<NetworkAddressTest>(
		"NetworkAddressTest::TestWildcard", &NetworkAddressTest::TestWildcard));
	suite.addTest(new CppUnit::TestCaller<NetworkAddressTest>(
		"NetworkAddressTest::TestIsLocal", &NetworkAddressTest::TestIsLocal));
	suite.addTest(new CppUnit::TestCaller<NetworkAddressTest>(
		"NetworkAddressTest::TestFlatten", &NetworkAddressTest::TestFlatten));

	parent.addTest("NetworkAddressTest", &suite);
}
