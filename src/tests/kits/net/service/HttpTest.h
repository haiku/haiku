/*
 * Copyright 2014 Haiku, inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef HTTP_TEST_H
#define HTTP_TEST_H


#include <Url.h>

#include <TestCase.h>
#include <TestSuite.h>

#include <cppunit/TestSuite.h>


class HttpTest: public BTestCase {
public:
										HttpTest();
	virtual								~HttpTest();

								void	GetTest();
								void	PortTest();
								void	UploadTest();
								void	AuthBasicTest();
								void	AuthDigestTest();
								void	ProxyTest();

	static						void	AddTests(BTestSuite& suite);

private:
								void	_AuthTest(BUrl& url);

	template<class T> static	void	_AddCommonTests(BString prefix,
											CppUnit::TestSuite& suite);

protected:
								BUrl	fBaseUrl;
};


class HttpsTest: public HttpTest {
public:
								HttpsTest();
};


#endif
