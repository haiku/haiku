/*
 * Copyright 2014 Haiku, inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef GET_ADDR_INFO_TEST_H
#define GET_ADDR_INFO_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class GetAddrInfoTest: public BTestCase {
public:
					GetAddrInfoTest();
	virtual			~GetAddrInfoTest();

			void	EmptyTest();
			void	AddrConfigTest();

	static	void	AddTests(BTestSuite& suite);
};


#endif

