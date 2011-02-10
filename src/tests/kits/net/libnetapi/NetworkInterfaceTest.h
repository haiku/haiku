/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NETWORK_INTERFACE_TEST_H
#define NETWORK_INTERFACE_TEST_H


#include <NetworkInterface.h>

#include <TestCase.h>
#include <TestSuite.h>


class NetworkInterfaceTest : public CppUnit::TestCase {
public:
								NetworkInterfaceTest();
	virtual						~NetworkInterfaceTest();

	virtual	void				setUp();
	virtual	void				tearDown();

			void				TestUnset();
			void				TestFindAddress();
			void				TestFindFirstAddress();

	static	void				AddTests(BTestSuite& suite);

private:
			BNetworkInterface	fInterface;
};


#endif	// NETWORK_INTERFACE_TEST_H
