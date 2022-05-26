/*
 * Copyright 2014-2020 Haiku, inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef HTTP_TEST_H
#define HTTP_TEST_H


#include <Url.h>

#include <TestCase.h>
#include <TestSuite.h>

#include <cppunit/TestSuite.h>
#include <tools/cppunit/ThreadedTestCase.h>

#include "TestServer.h"


class HttpTest: public BThreadedTestCase {
public:
						HttpTest(TestServerMode mode = TEST_SERVER_MODE_HTTP);
	virtual				~HttpTest();

	virtual	void		setUp();

			void		GetTest();
			void		HeadTest();
			void		NoContentTest();
			void		UploadTest();
			void		AuthBasicTest();
			void		AuthDigestTest();
			void		ProxyTest();
			void		AutoRedirectTest();

	static	void		AddTests(BTestSuite& suite);

private:
			void		_GetTest(const BString& path);
private:
			TestServer	fTestServer;
};


class HttpsTest: public HttpTest {
public:
						HttpsTest();
};


#endif
