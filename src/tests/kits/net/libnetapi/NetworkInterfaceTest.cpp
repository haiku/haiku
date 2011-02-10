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
NetworkInterfaceTest::TestUnset()
{
	BNetworkInterface unset("test");
	unset.Unset();
	CPPUNIT_ASSERT(unset.Name() == NULL || unset.Name()[0] == '\0');
}


void
NetworkInterfaceTest::TestFindAddress()
{
	BNetworkInterface interface("testInterface");
	CPPUNIT_ASSERT(BNetworkRoster::Default().AddInterface(interface) == B_OK);
	CPPUNIT_ASSERT(interface.CountAddresses() == 0);

	// Add first

	BNetworkInterfaceAddress first;
	first.SetAddress(BNetworkAddress(AF_INET, "8.8.8.8"));

	CPPUNIT_ASSERT(interface.FindAddress(first.Address()) < 0);
	CPPUNIT_ASSERT(interface.AddAddress(first) == B_OK);
	CPPUNIT_ASSERT(interface.FindAddress(first.Address()) == 0);

	// Add second

	BNetworkInterfaceAddress second;
	second.SetAddress(BNetworkAddress(AF_INET6, "::1"));

	CPPUNIT_ASSERT(interface.FindAddress(second.Address()) < 0);
	CPPUNIT_ASSERT(interface.AddAddress(second) == B_OK);
	CPPUNIT_ASSERT(interface.FindAddress(second.Address()) >= 0);

	// Remove them again

	CPPUNIT_ASSERT(interface.RemoveAddress(first) == B_OK);
	CPPUNIT_ASSERT(interface.FindAddress(first.Address()) < 0);
	CPPUNIT_ASSERT(interface.FindAddress(second.Address()) >= 0);

	CPPUNIT_ASSERT(interface.RemoveAddress(second.Address()) == B_OK);
	CPPUNIT_ASSERT(interface.FindAddress(first.Address()) < 0);
	CPPUNIT_ASSERT(interface.FindAddress(second.Address()) < 0);

	CPPUNIT_ASSERT(BNetworkRoster::Default().RemoveInterface(interface)
		== B_OK);
}


void
NetworkInterfaceTest::TestFindFirstAddress()
{
	BNetworkInterface interface("testInterface");
	CPPUNIT_ASSERT(BNetworkRoster::Default().AddInterface(interface) == B_OK);
	CPPUNIT_ASSERT(interface.CountAddresses() == 0);

	// Add first

	BNetworkInterfaceAddress first;
	first.SetAddress(BNetworkAddress(AF_INET, "8.8.8.8"));

	CPPUNIT_ASSERT(interface.FindFirstAddress(AF_INET) < 0);
	CPPUNIT_ASSERT(interface.FindFirstAddress(AF_INET6) < 0);
	CPPUNIT_ASSERT(interface.AddAddress(first) == B_OK);
	CPPUNIT_ASSERT(interface.FindFirstAddress(AF_INET) == 0);
	CPPUNIT_ASSERT(interface.FindFirstAddress(AF_INET6) < 0);

	// Add second

	BNetworkInterfaceAddress second;
	second.SetAddress(BNetworkAddress(AF_INET6, "::1"));

	CPPUNIT_ASSERT(interface.AddAddress(second) == B_OK);
	CPPUNIT_ASSERT(interface.FindFirstAddress(AF_INET) >= 0);
	CPPUNIT_ASSERT(interface.FindFirstAddress(AF_INET6) >= 0);
	CPPUNIT_ASSERT(interface.FindFirstAddress(AF_INET)
		!= interface.FindFirstAddress(AF_INET6));

	// Remove them again

	CPPUNIT_ASSERT(interface.RemoveAddress(first) == B_OK);
	CPPUNIT_ASSERT(interface.FindFirstAddress(AF_INET) < 0);
	CPPUNIT_ASSERT(interface.FindFirstAddress(AF_INET6) >= 0);

	CPPUNIT_ASSERT(interface.RemoveAddress(second.Address()) == B_OK);
	CPPUNIT_ASSERT(interface.FindFirstAddress(AF_INET) < 0);
	CPPUNIT_ASSERT(interface.FindFirstAddress(AF_INET6) < 0);

	CPPUNIT_ASSERT(BNetworkRoster::Default().RemoveInterface(interface)
		== B_OK);
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
