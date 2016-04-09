/*
 * Copyright 2016, Andrew Lindesay, apl@lindesay.co.nz.
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


// General Tests ---------------------------------------------------------------

/*
This is the "happy days" tests that checks that a URL featuring all of the
parsed elements successfully processes and the elements are present.
*/

void NetworkUrlTest::TestValidFullUrl()
{
	BUrl url("http://ewe:pea@www.something.co.nz:8888/some/path?key1=value1#fragment");
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


void NetworkUrlTest::TestFileUrl()
{
	BUrl url("file:///northisland/wellington/brooklyn/windturbine");
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


// Authority Tests (UserName, Password, Host, Port) ----------------------------


void NetworkUrlTest::TestWithUserNameAndPasswordNoHostAndPort()
{
	BUrl url("wierd://tea:tree@/x");
	CPPUNIT_ASSERT(url.IsValid());
	CPPUNIT_ASSERT(url.Protocol() == "wierd");
	CPPUNIT_ASSERT(url.HasProtocol());
	CPPUNIT_ASSERT(url.UserName() == "tea");
	CPPUNIT_ASSERT(url.HasUserName());
	CPPUNIT_ASSERT(url.Password() == "tree");
	CPPUNIT_ASSERT(url.HasPassword());
	CPPUNIT_ASSERT(url.Host() == "");
	CPPUNIT_ASSERT(!url.HasHost());
	CPPUNIT_ASSERT(!url.HasPort());
	CPPUNIT_ASSERT(url.Path() == "/x");
	CPPUNIT_ASSERT(url.HasPath());
	CPPUNIT_ASSERT(!url.HasRequest());
	CPPUNIT_ASSERT(!url.HasFragment());
}


void NetworkUrlTest::TestHostAndPortWithNoUserNameAndPassword()
{
	BUrl url("https://www.something.co.nz:443/z");
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
	BUrl url("https://www.something.co.nz/z");
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
	BUrl url("https://www.something.co.nz");
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
	BUrl url("https://www.something.co.nz:1234");
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
	BUrl url("https://www.something.co.nz:");
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


// Invalid Forms ---------------------------------------------------------------


void NetworkUrlTest::TestWhitespaceBefore()
{
	BUrl url("   https://www.something.co.nz/z");
	CPPUNIT_ASSERT(!url.IsValid());
}


void NetworkUrlTest::TestWhitespaceAfter()
{
	BUrl url("https://www.something.co.nz/z\t\t ");
	CPPUNIT_ASSERT(!url.IsValid());
}


void NetworkUrlTest::TestWhitespaceMiddle()
{
	BUrl url("https://www.  something.co.nz/z");
	CPPUNIT_ASSERT(!url.IsValid());
}


void NetworkUrlTest::TestHttpNoHost()
{
	BUrl url("https:///z");
	CPPUNIT_ASSERT(!url.IsValid());
}


// Control ---------------------------------------------------------------------


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
		"NetworkUrlTest::TestWhitespaceBefore",
		&NetworkUrlTest::TestWhitespaceBefore));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestWhitespaceAfter",
		&NetworkUrlTest::TestWhitespaceAfter));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestWhitespaceMiddle",
		&NetworkUrlTest::TestWhitespaceMiddle));

	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestFileUrl",
		&NetworkUrlTest::TestFileUrl));
	suite.addTest(new CppUnit::TestCaller<NetworkUrlTest>(
		"NetworkUrlTest::TestValidFullUrl", &NetworkUrlTest::TestValidFullUrl));

	parent.addTest("NetworkUrlTest", &suite);
}
