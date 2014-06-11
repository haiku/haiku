/*
 * Copyright 2014 Haiku, inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef COOKIE_TEST_H
#define COOKIE_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class BNetworkCookie;
class BNetworkCookieJar;
class BUrl;


class CookieTest: public BTestCase {
public:
							CookieTest();
	virtual					~CookieTest();

			void			SimpleTest();
			void			StandardTest();
			void			ExpireTest();
			void			PathTest();
			void			MaxSizeTest();
			void			MaxNumberTest();
			void			UpdateTest();
			void			HttpOnlyTest();
			void			EncodingTest();
			void			DomainTest();
			void			PersistantTest();
			void			OverwriteTest();
			void			OrderTest();

			void			ExpireParsingTest();
			void			PathMatchingTest();
			void			DomainMatchingTest();
			void			MaxAgeParsingTest();

			void			ExplodeTest();

	static	void			AddTests(BTestSuite& suite);

private:
	const	BNetworkCookie*	_GetCookie(BNetworkCookieJar& jar, const BUrl& url,
								const char* name);
};


#endif
