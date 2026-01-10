/*
 * Copyright 2016-2018, Andrew Lindesay, apl@lindesay.co.nz.
 * Distributed under the terms of the MIT License.
 */


#include "NetworkUrlTest.h"

#include <Url.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


NetworkUrlTest::NetworkUrlTest()
{
}


NetworkUrlTest::~NetworkUrlTest()
{
}


void
NetworkUrlTest::setUp()
{
}


void
NetworkUrlTest::tearDown()
{
}


// #pragma mark - General Tests

/*
This is the "happy days" tests that checks that a URL featuring all of the
parsed elements successfully processes and the elements are present.
*/

void NetworkUrlTest::TestValidFullUrl()
{
	BUrl url("http://ewe:pea@www.something.co.nz:8888/some/path?key1=value1#fragment", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "http");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(url.UserName() == "ewe");
	CPPUNIT_ASSERT(url.HasUserName());
	CPPUNIT_ASSERT(url.Password() == "pea");
	CPPUNIT_ASSERT(url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "www.something.co.nz");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Port() == 8888);
	CPPUNIT_ASSERT(url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "/some/path");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(url.Request() == "key1=value1");
	CPPUNIT_ASSERT(url.HasRequest());
	CPPUNIT_ASSERT(url.Fragment() == "fragment");
	CPPUNIT_ASSERT(url.HasFragment());
}


void NetworkUrlTest::TestHostWithPathAndFragment()
{
	BUrl url("http://1.2.3.4/some/path#frag/ment", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "http");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "1.2.3.4");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "/some/path");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(url.Fragment() == "frag/ment");
	CPPUNIT_ASSERT(url.HasFragment());
}


void NetworkUrlTest::TestHostWithFragment()
{
	BUrl url("http://1.2.3.4#frag/ment", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "http");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "1.2.3.4");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(url.HasPath()); // see Url.cpp - evidently an empty path is still a path?
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(url.Fragment() == "frag/ment");
	CPPUNIT_ASSERT(url.HasFragment());
}


void NetworkUrlTest::TestIpv6HostPortPathAndRequest()
{
	BUrl url("http://[123:a3:0:E3::123]:8080/some/path?key1=value1", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "http");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "[123:a3:0:E3::123]");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Port() == 8080);
	CPPUNIT_ASSERT(url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "/some/path");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(url.Request() == "key1=value1");
	CPPUNIT_ASSERT(url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestFileUrl()
{
	BUrl url("file:///northisland/wellington/brooklyn/windturbine", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "file");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "/northisland/wellington/brooklyn/windturbine");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestDataUrl()
{
	BUrl url("data:image/png;base64,iVBORw0KGI12P4//8/w38GIErkJggg==", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "data");
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(!url.HasHost());
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(url.Path() == "image/png;base64,iVBORw0KGI12P4//8/w38GIErkJggg==");
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


// #pragma mark - Authority Tests (UserName, Password, Host, Port)


void NetworkUrlTest::TestWithUserNameAndPasswordNoHostAndPort()
{
	BUrl url("wierd://tea:tree@/x", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "wierd");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(url.UserName() == "tea");
	CPPUNIT_ASSERT(url.HasUserName());
	CPPUNIT_ASSERT(url.Password() == "tree");
	CPPUNIT_ASSERT(url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "");
	CPPUNIT_ASSERT(url.HasHost()); // any authority means there "is a host" - see SetAuthority comment.
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "/x");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestHostAndPortWithNoUserNameAndPassword()
{
	BUrl url("https://www.something.co.nz:443/z", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "https");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Port() == 443);
	CPPUNIT_ASSERT(url.HasPort());
	CPPUNIT_ASSERT(url.Host() == "www.something.co.nz");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Path() == "/z");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestHostWithNoPortOrUserNameAndPassword()
{
	BUrl url("https://www.something.co.nz/z", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "https");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "www.something.co.nz");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "/z");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestHostWithNoPortNoPath()
{
	BUrl url("https://www.something.co.nz", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "https");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "www.something.co.nz");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(!url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestHostWithPortNoPath()
{
	BUrl url("https://www.something.co.nz:1234", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "https");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "www.something.co.nz");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Port() == 1234);
	CPPUNIT_ASSERT(url.HasPort());
	CPPUNIT_ASSERT(!url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestHostWithEmptyPort()
{
	BUrl url("https://www.something.co.nz:", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "https");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "www.something.co.nz");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(!url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestProtocol()
{
	BUrl url("olala:", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "olala");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(!url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(!url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestMailTo()
{
	BUrl url("mailto:eric@example.com", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "mailto");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(!url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "eric@example.com");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


// #pragma mark - Various Authority Checks


void NetworkUrlTest::TestAuthorityNoUserName()
{
	BUrl url("anything://:pwd@host", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(url.HasPassword());
	CPPUNIT_ASSERT(url.Password() == "pwd");
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Host() == "host");
	CPPUNIT_ASSERT(!url.HasPort());
}


void NetworkUrlTest::TestAuthorityWithCredentialsSeparatorNoPassword()
{
	BUrl url("anything://unam:@host", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.HasUserName());
	CPPUNIT_ASSERT(url.UserName() == "unam");
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Host() == "host");
	CPPUNIT_ASSERT(!url.HasPort());
}


void NetworkUrlTest::TestAuthorityWithoutCredentialsSeparatorNoPassword()
{
	BUrl url("anything://unam@host", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.HasUserName());
	CPPUNIT_ASSERT(url.UserName() == "unam");
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Host() == "host");
	CPPUNIT_ASSERT(!url.HasPort());
}


void NetworkUrlTest::TestAuthorityBadPort()
{
	BUrl url("anything://host:aaa", true);
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(!url.HasUserName());
	CPPUNIT_ASSERT(!url.HasPassword());
	CPPUNIT_ASSERT(url.HasHost());
	CPPUNIT_ASSERT(url.Host() == "host");
	CPPUNIT_ASSERT(!url.HasPort());
}


// #pragma mark - Invalid Forms


void NetworkUrlTest::TestWhitespaceBefore()
{
	BUrl url("   https://www.something.co.nz/z", true);
	CPPUNIT_ASSERT(!url.IsValid());
}


void NetworkUrlTest::TestWhitespaceAfter()
{
	BUrl url("https://www.something.co.nz/z\t\t ", true);
	CPPUNIT_ASSERT(!url.IsValid());
}


void NetworkUrlTest::TestWhitespaceMiddle()
{
	BUrl url("https://www.  something.co.nz/z", true);
	CPPUNIT_ASSERT(!url.IsValid());
}


void NetworkUrlTest::TestHttpNoHost()
{
	BUrl url("https:///z", true);
	CPPUNIT_ASSERT(!url.IsValid());
}


void NetworkUrlTest::TestEmpty()
{
	BUrl url("", true);
	CPPUNIT_ASSERT(!url.IsValid());
}


// #pragma mark - Host validation


void NetworkUrlTest::TestBadHosts()
{
	CPPUNIT_ASSERT_MESSAGE("control check",
		BUrl("http://host.example.com", true).IsValid());

	CPPUNIT_ASSERT_MESSAGE("hyphen in middle",
		(BUrl("http://host.peppermint_tea.com", true).IsValid()));
	CPPUNIT_ASSERT_MESSAGE("dot at end",
		(BUrl("http://host.camomile.co.nz.", true).IsValid()));
	CPPUNIT_ASSERT_MESSAGE("simple host",
		(BUrl("http://tumeric", true).IsValid()));

	CPPUNIT_ASSERT_MESSAGE("idn domain encoded",
		(BUrl("http://xn--bcher-kva.tld", true).IsValid()));
	CPPUNIT_ASSERT_MESSAGE("idn domain unencoded",
		(BUrl("http://www.b\xc3\xbcch.at", true).IsValid()));

	CPPUNIT_ASSERT_MESSAGE("dot at start",
		!(BUrl("http://.host.example.com", true).IsValid()));
	CPPUNIT_ASSERT_MESSAGE("double dot in domain",
		!(BUrl("http://host.example..com", true).IsValid()));
	CPPUNIT_ASSERT_MESSAGE("double dot",
		!(BUrl("http://host.example..com", true).IsValid()));
	CPPUNIT_ASSERT_MESSAGE("unexpected characters",
		!(BUrl("http://<unexpected.characters>", true).IsValid()));
	CPPUNIT_ASSERT_MESSAGE("whitespace",
		!(BUrl("http://host.exa ple.com", true).IsValid()));
}


// #pragma mark - Control


/*static*/ void
NetworkUrlTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("NetworkUrlTest");

	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestHostAndPortWithNoUserNameAndPassword",
		&NetworkUrlTest::TestHostAndPortWithNoUserNameAndPassword));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestWithUserNameAndPasswordNoHostAndPort",
		&NetworkUrlTest::TestWithUserNameAndPasswordNoHostAndPort));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestHostWithNoPortOrUserNameAndPassword",
		&NetworkUrlTest::TestHostWithNoPortOrUserNameAndPassword));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestHostWithNoPortNoPath",
		&NetworkUrlTest::TestHostWithNoPortNoPath));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestHostWithPortNoPath",
		&NetworkUrlTest::TestHostWithPortNoPath));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestHostWithEmptyPort",
		&NetworkUrlTest::TestHostWithEmptyPort));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestHostWithPathAndFragment",
		&NetworkUrlTest::TestHostWithPathAndFragment));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestHostWithFragment",
		&NetworkUrlTest::TestHostWithFragment));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestIpv6HostPortPathAndRequest",
		&NetworkUrlTest::TestIpv6HostPortPathAndRequest));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestProtocol",
		&NetworkUrlTest::TestProtocol));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestMailTo",
		&NetworkUrlTest::TestMailTo));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestDataUrl",
		&NetworkUrlTest::TestDataUrl));

	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestAuthorityNoUserName",
		&NetworkUrlTest::TestAuthorityNoUserName));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
        "NetworkUrlTest::TestAuthorityWithCredentialsSeparatorNoPassword",
        &NetworkUrlTest::TestAuthorityWithCredentialsSeparatorNoPassword));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
        "NetworkUrlTest::TestAuthorityWithoutCredentialsSeparatorNoPassword",
        &NetworkUrlTest::TestAuthorityWithoutCredentialsSeparatorNoPassword));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
        "NetworkUrlTest::TestAuthorityBadPort",
        &NetworkUrlTest::TestAuthorityBadPort));

	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestWhitespaceBefore",
		&NetworkUrlTest::TestWhitespaceBefore));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestWhitespaceAfter",
		&NetworkUrlTest::TestWhitespaceAfter));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestWhitespaceMiddle",
		&NetworkUrlTest::TestWhitespaceMiddle));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestEmpty",
		&NetworkUrlTest::TestEmpty));

	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestFileUrl",
		&NetworkUrlTest::TestFileUrl));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestValidFullUrl", &NetworkUrlTest::TestValidFullUrl));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestBadHosts", &NetworkUrlTest::TestBadHosts));

	parent.addTest("NetworkUrlTest", &suite);
}
