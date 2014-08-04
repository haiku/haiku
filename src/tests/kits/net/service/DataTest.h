/*
 * Copyright 2014 Haiku, inc.
 * Distributed under the terms of the MIT License.
 */


#ifndef DATA_TEST_H
#define DATA_TEST_H


#include <Url.h>
#include <UrlProtocolListener.h>

#include <TestCase.h>
#include <TestSuite.h>

#include <cppunit/TestSuite.h>


class DataTest: public BTestCase, BUrlProtocolListener {
public:
								DataTest();
	virtual						~DataTest();

			void				SimpleTest();
			void				EmptyTest();
			void				InvalidTest();
			void				CharsetTest();
			void				Base64Test();
			void				UrlDecodeTest();

			void				DataReceived(BUrlRequest*, const char* data,
									off_t, ssize_t size);

	static	void				AddTests(BTestSuite& suite);

private:
			void				_RunTest(BString url, const char* expected,
									size_t expectedLength);
private:
			std::vector<char>	fReceivedData;
};


#endif

