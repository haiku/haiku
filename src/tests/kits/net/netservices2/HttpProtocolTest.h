/*
 * Copyright 2021 Haiku, inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef HTTP_TEST_H
#define HTTP_TEST_H


#include <HttpSession.h>
#include <TestCase.h>
#include <TestSuite.h>
#include <tools/cppunit/ThreadedTestCase.h>

#include "HttpDebugLogger.h"
#include "TestServer.h"

using BPrivate::Network::BHttpSession;


class HttpProtocolTest : public BTestCase
{
public:
								HttpProtocolTest();

			void				HttpFieldsTest();
			void				HttpMethodTest();
			void				HttpRequestTest();
			void				HttpTimeTest();

	static	void				AddTests(BTestSuite& suite);
};


class HttpIntegrationTest : public BThreadedTestCase
{
public:
								HttpIntegrationTest(TestServerMode mode);

	virtual	void				setUp() override;
	virtual	void				tearDown() override;

			void				HostAndNetworkFailTest();
			void				GetTest();
			void				GetWithBufferTest();
			void				HeadTest();
			void				NoContentTest();
			void				AutoRedirectTest();
			void				BasicAuthTest();
			void				StopOnErrorTest();
			void				RequestCancelTest();
			void				PostTest();

	static	void				AddTests(BTestSuite& suite);

private:
			TestServer			fTestServer;
			BHttpSession		fSession;
			HttpDebugLogger*	fLogger;
			BMessenger			fLoggerMessenger;
};

#endif
