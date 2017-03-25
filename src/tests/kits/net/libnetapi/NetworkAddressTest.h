/*
 * Copyright 2010-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NETWORK_ADDRESS_TEST_H
#define NETWORK_ADDRESS_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class NetworkAddressTest : public CppUnit::TestCase {
public:
								NetworkAddressTest();
	virtual						~NetworkAddressTest();

			void				TestUnset();
			void				TestSetTo();
			void				TestWildcard();
			void				TestNullAddr();
			void				TestSetAddressFromFamilyPort();
			void				TestIsLocal();
			void				TestFlatten();
			void				TestEquals();

	static	void				AddTests(BTestSuite& suite);
};


#endif	// NETWORK_ADDRESS_TEST_H
