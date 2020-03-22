/*
 * Copyright 2010-2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "NetworkAddressTest.h"

#include <arpa/inet.h>
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

	CPPUNIT_ASSERT(unset.InitCheck() == B_OK);
	CPPUNIT_ASSERT(unset.Family() == AF_UNSPEC);
	CPPUNIT_ASSERT(unset.Port() == 0);

	BNetworkAddress set(AF_INET, "127.0.0.1");
	CPPUNIT_ASSERT(set.InitCheck() == B_OK);
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

	CPPUNIT_ASSERT(address.SetTo("::1", (uint16)0,
		B_UNCONFIGURED_ADDRESS_FAMILIES) == B_OK);
	CPPUNIT_ASSERT(address.Family() == AF_INET6);
	CPPUNIT_ASSERT(address == BNetworkAddress(in6addr_loopback));

	CPPUNIT_ASSERT(address.SetTo(AF_INET, "::1") != B_OK);
	CPPUNIT_ASSERT(address.SetTo(AF_INET6, "127.0.0.1") != B_OK);
	CPPUNIT_ASSERT(address.SetTo(AF_INET, "127.0.0.1") == B_OK);
	CPPUNIT_ASSERT(address.SetTo(AF_INET6, "::1", (uint16)0,
		B_UNCONFIGURED_ADDRESS_FAMILIES) == B_OK);
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
	CPPUNIT_ASSERT(wildcard.IsEmpty());

	BNetworkAddress null(AF_INET, NULL);
	CPPUNIT_ASSERT(wildcard == null);

	wildcard.SetPort(555);
	CPPUNIT_ASSERT(!wildcard.IsEmpty());
}


void
NetworkAddressTest::TestNullAddr()
{
	BNetworkAddress nullAddr(AF_INET, NULL, 555);
	CPPUNIT_ASSERT(nullAddr.InitCheck() == B_OK);

	CPPUNIT_ASSERT(nullAddr.Family() == AF_INET);
	CPPUNIT_ASSERT(nullAddr.Length() == sizeof(sockaddr_in));
	CPPUNIT_ASSERT(nullAddr.Port() == 555);
	CPPUNIT_ASSERT(!nullAddr.IsEmpty());
}


void
NetworkAddressTest::TestSetAddressFromFamilyPort()
{
	BString addressStr = "192.168.1.1";
	BNetworkAddress nullAddr(AF_INET, NULL, 555);
	in_addr_t inetAddress = inet_addr(addressStr.String());
	CPPUNIT_ASSERT(nullAddr.SetAddress(inetAddress) == B_OK);
	CPPUNIT_ASSERT(nullAddr.InitCheck() == B_OK);

	CPPUNIT_ASSERT(nullAddr.Family() == AF_INET);
	CPPUNIT_ASSERT(nullAddr.Length() == sizeof(sockaddr_in));

	sockaddr_in& sin = (sockaddr_in&)nullAddr.SockAddr();
	CPPUNIT_ASSERT(addressStr == inet_ntoa(sin.sin_addr));
	CPPUNIT_ASSERT(nullAddr.Port() == 555);
	CPPUNIT_ASSERT(!nullAddr.IsEmpty());
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


void
NetworkAddressTest::TestEquals()
{
	BNetworkAddress v4AddressA("192.168.1.100");
	BNetworkAddress v4AddressB("192.168.1.100");
	BNetworkAddress v6AddressA("feed::dead:beef", (uint16)0,
		B_UNCONFIGURED_ADDRESS_FAMILIES);
	BNetworkAddress v6AddressB("feed::dead:beef", (uint16)0,
		B_UNCONFIGURED_ADDRESS_FAMILIES);

	CPPUNIT_ASSERT(v4AddressA.Equals(v4AddressB));
	CPPUNIT_ASSERT(v6AddressA.Equals(v6AddressB));
	CPPUNIT_ASSERT(!v4AddressA.Equals(v6AddressA));
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
		"NetworkAddressTest::TestNullAddr", &NetworkAddressTest::TestNullAddr));
	suite.addTest(new CppUnit::TestCaller<NetworkAddressTest>(
		"NetworkAddressTest::TestSetAddressFromFamilyPort",
		&NetworkAddressTest::TestSetAddressFromFamilyPort));
	suite.addTest(new CppUnit::TestCaller<NetworkAddressTest>(
		"NetworkAddressTest::TestIsLocal", &NetworkAddressTest::TestIsLocal));
	suite.addTest(new CppUnit::TestCaller<NetworkAddressTest>(
		"NetworkAddressTest::TestFlatten", &NetworkAddressTest::TestFlatten));
	suite.addTest(new CppUnit::TestCaller<NetworkAddressTest>(
		"NetworkAddressTest::TestEquals", &NetworkAddressTest::TestEquals));

	parent.addTest("NetworkAddressTest", &suite);
}
