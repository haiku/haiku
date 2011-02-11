/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "NetworkInterfaceTest.h"

#include <NetworkInterface.h>
#include <NetworkRoster.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


NetworkInterfaceTest::NetworkInterfaceTest()
{
}


NetworkInterfaceTest::~NetworkInterfaceTest()
{
}


void
NetworkInterfaceTest::setUp()
{
	fInterface.SetTo("loopTest");
	BNetworkRoster::Default().RemoveInterface(fInterface);
		// just in case

	CPPUNIT_ASSERT(BNetworkRoster::Default().AddInterface(fInterface) == B_OK);
	CPPUNIT_ASSERT(fInterface.CountAddresses() == 1);
		// every interface has one unspec address
}


void
NetworkInterfaceTest::tearDown()
{
	CPPUNIT_ASSERT(BNetworkRoster::Default().RemoveInterface(fInterface)
		== B_OK);
}


void
NetworkInterfaceTest::TestUnset()
{
	BNetworkInterface unset("test");
	unset.Unset();
	CPPUNIT_ASSERT(unset.Name() == NULL || unset.Name()[0] == '\0');
}


void
NetworkInterfaceTest::TestFindAddress()
{
	fInterface.RemoveAddressAt(0);
		// Remove empty address

	// Add first

	BNetworkInterfaceAddress first;
	first.SetAddress(BNetworkAddress(AF_INET, "8.8.8.8"));

	CPPUNIT_ASSERT(fInterface.FindAddress(first.Address()) < 0);
	CPPUNIT_ASSERT(fInterface.AddAddress(first) == B_OK);
	CPPUNIT_ASSERT(fInterface.FindAddress(first.Address()) == 0);

	// Add second

	BNetworkInterfaceAddress second;
	second.SetAddress(BNetworkAddress(AF_INET6, "::1"));

	CPPUNIT_ASSERT(fInterface.FindAddress(second.Address()) < 0);
	CPPUNIT_ASSERT(fInterface.AddAddress(second) == B_OK);
	CPPUNIT_ASSERT(fInterface.FindAddress(second.Address()) >= 0);

	// Remove them again

	CPPUNIT_ASSERT(fInterface.RemoveAddress(first) == B_OK);
	CPPUNIT_ASSERT(fInterface.FindAddress(first.Address()) < 0);
	CPPUNIT_ASSERT(fInterface.FindAddress(second.Address()) >= 0);

	CPPUNIT_ASSERT(fInterface.RemoveAddress(second.Address()) == B_OK);
	CPPUNIT_ASSERT(fInterface.FindAddress(first.Address()) < 0);
	CPPUNIT_ASSERT(fInterface.FindAddress(second.Address()) < 0);
}


void
NetworkInterfaceTest::TestFindFirstAddress()
{
	fInterface.RemoveAddressAt(0);
		// Remove empty address

	// Add first

	BNetworkInterfaceAddress first;
	first.SetAddress(BNetworkAddress(AF_INET, "8.8.8.8"));

	CPPUNIT_ASSERT(fInterface.FindFirstAddress(AF_INET) < 0);
	CPPUNIT_ASSERT(fInterface.FindFirstAddress(AF_INET6) < 0);
	CPPUNIT_ASSERT(fInterface.AddAddress(first) == B_OK);
	CPPUNIT_ASSERT(fInterface.FindFirstAddress(AF_INET) == 0);
	CPPUNIT_ASSERT(fInterface.FindFirstAddress(AF_INET6) < 0);

	// Add second

	BNetworkInterfaceAddress second;
	second.SetAddress(BNetworkAddress(AF_INET6, "::1"));

	CPPUNIT_ASSERT(fInterface.AddAddress(second) == B_OK);
	CPPUNIT_ASSERT(fInterface.FindFirstAddress(AF_INET) >= 0);
	CPPUNIT_ASSERT(fInterface.FindFirstAddress(AF_INET6) >= 0);
	CPPUNIT_ASSERT(fInterface.FindFirstAddress(AF_INET)
		!= fInterface.FindFirstAddress(AF_INET6));

	// Remove them again

	CPPUNIT_ASSERT(fInterface.RemoveAddress(first) == B_OK);
	CPPUNIT_ASSERT(fInterface.FindFirstAddress(AF_INET) < 0);
	CPPUNIT_ASSERT(fInterface.FindFirstAddress(AF_INET6) >= 0);

	CPPUNIT_ASSERT(fInterface.RemoveAddress(second.Address()) == B_OK);
	CPPUNIT_ASSERT(fInterface.FindFirstAddress(AF_INET) < 0);
	CPPUNIT_ASSERT(fInterface.FindFirstAddress(AF_INET6) < 0);
}


/*static*/ void
NetworkInterfaceTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("NetworkInterfaceTest");

	suite.addTest(new CppUnit::TestCaller<NetworkInterfaceTest>(
		"NetworkInterfaceTest::TestUnset", &NetworkInterfaceTest::TestUnset));
	suite.addTest(new CppUnit::TestCaller<NetworkInterfaceTest>(
		"NetworkInterfaceTest::TestFindAddress",
		&NetworkInterfaceTest::TestFindAddress));
	suite.addTest(new CppUnit::TestCaller<NetworkInterfaceTest>(
		"NetworkInterfaceTest::TestFindFirstAddress",
		&NetworkInterfaceTest::TestFindFirstAddress));

	parent.addTest("NetworkInterfaceTest", &suite);
}
