/*
 * Copyright 2014 Haiku, inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef HTTP_TEST_H
#define HTTP_TEST_H


#include <Url.h>

#include <TestCase.h>
#include <TestSuite.h>


class HttpTest: public BTestCase {
public:
					HttpTest();
	virtual			~HttpTest();

			void	GetTest();
			void	UploadTest();
			void	AuthBasicTest();
			void	AuthDigestTest();
			void	ListenerTest();

	static	void	AddTests(BTestSuite& suite);

private:
			void	_AuthTest(BUrl& url);

protected:
	BUrl			fBaseUrl;
};


class HttpsTest: public HttpTest {
public:
					HttpsTest();
};


#endif
