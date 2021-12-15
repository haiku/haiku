/*
 * Copyright 2021 Haiku, inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef HTTP_TEST_H
#define HTTP_TEST_H


#include <HttpSession.h>
#include <TestCase.h>
#include <TestSuite.h>

using BPrivate::Network::BHttpSession;


class HttpProtocolTest: public BTestCase {
public:
					HttpProtocolTest();

			void	HttpHeaderTest();

	static	void	AddTests(BTestSuite& suite);

private:
	BHttpSession	fSession;
};


#endif
