/*
 * Copyright 2016, Andrew Lindesay, apl@lindesay.co.nz
 * Distributed under the terms of the MIT License.
 */
#ifndef NETWORK_URL_TEST_H
#define NETWORK_URL_TEST_H


#include <TestCase.h>
#include <TestSuite.h>


class NetworkUrlTest : public CppUnit::TestCase {
public:
								NetworkUrlTest();
	virtual						~NetworkUrlTest();

	virtual	void				setUp();
	virtual	void				tearDown();

			void				TestValidFullUrl();

			void				TestFileUrl();

			void				TestWithUserNameAndPasswordNoHostAndPort();
			void				TestHostAndPortWithNoUserNameAndPassword();
			void				TestHostWithNoPortOrUserNameAndPassword();
			void				TestHostWithNoPortNoPath();
			void				TestHostWithPortNoPath();
			void				TestHostWithEmptyPort();
			void				TestHostWithPathAndFragment();
			void				TestHostWithFragment();
			void				TestIpv6HostPortPathAndRequest();
			void				TestProtocol();
			void				TestMailTo();
			void				TestDataUrl();

			void				TestAuthorityNoUserName();
			void				TestAuthorityWithCredentialsSeparatorNoPassword();
			void				TestAuthorityWithoutCredentialsSeparatorNoPassword();
			void				TestAuthorityBadPort();

			void				TestWhitespaceBefore();
			void				TestWhitespaceAfter();
			void				TestWhitespaceMiddle();
			void				TestHttpNoHost();
			void				TestEmpty();
			void				TestBadHosts();

	static	void				AddTests(BTestSuite& suite);

};


#endif	// NETWORK_URL_TEST_H
