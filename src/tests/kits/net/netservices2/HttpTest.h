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


class HttpTest: public BTestCase {
public:
					HttpTest(BHttpSession& session);

	static	void	AddTests(BTestSuite& suite);

private:
	BHttpSession	fSession;
};


#endif
