/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NETWORK_INTERFACE_TEST_H
#define NETWORK_INTERFACE_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class NetworkInterfaceTest : public CppUnit::TestCase {
public:
								NetworkInterfaceTest();
	virtual						~NetworkInterfaceTest();

			void				TestUnset();
			void				TestFindAddress();
			void				TestFindFirstAddress();

	static	void				AddTests(BTestSuite& suite);
};


#endif	// NETWORK_INTERFACE_TEST_H
