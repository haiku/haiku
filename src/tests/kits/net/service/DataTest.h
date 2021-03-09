/*
 * Copyright 2014-2021 Haiku, inc.
 * Distributed under the terms of the MIT License.
 */


#ifndef DATA_TEST_H
#define DATA_TEST_H


#include <Url.h>
#include <UrlProtocolListener.h>

#include <TestCase.h>
#include <TestSuite.h>

#include <cppunit/TestSuite.h>


using BPrivate::Network::BUrlProtocolListener;
using BPrivate::Network::BUrlRequest;


class DataTest: public BTestCase {
public:
								DataTest();
	virtual						~DataTest();

			void				SimpleTest();
			void				EmptyTest();
			void				InvalidTest();
			void				CharsetTest();
			void				Base64Test();
			void				UrlDecodeTest();

	static	void				AddTests(BTestSuite& suite);

private:
			void				_RunTest(BString url, const char* expected,
									size_t expectedLength);
};


#endif

