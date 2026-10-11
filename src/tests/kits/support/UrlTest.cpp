/*
 * Copyright 2010, Christophe Huriaux
 * Copyright 2014, Haiku, inc.
 * Copyright 2016-2024, Andrew Lindesay, apl@lindesay.co.nz.
 * Distributed under the terms of the MIT License.
 */

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <Url.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

typedef struct
{
	const char* url;

	struct
	{
		const char* protocol;
		const char* userName;
		const char* password;
		const char* host;
		int16		port;
		const char* path;
		const char* request;
		const char* fragment;
	} expected;
} ExplodeTest;


const ExplodeTest	kTestExplode[] =
//  Url
//	   Protocol	 User  Password  Hostname  Port	 Path	Request	  Fragment
//	   -------- --------- --------- --------- ---- ---------- ---------- ------------
{
	{ "http://user:pass@host:80/path?query#fragment",
		{ "http",   "user",   "pass",   "host",	 80,   "/path",	  "query",   "fragment" } },
	{ "http://www.host.tld/path?query#fragment",
		{ "http",   "",		"", "www.host.tld",0,   "/path",	  "query",   "fragment" } },
	{ "",
		{ "",	   "",		"", "",			0,   "",		"",		""} },
	{ "mailto:John.Doe@example.com",
		{ "mailto", "",		"", "",			0,   "John.Doe@example.com", "", "" } },
	{ "mailto:?to=addr1@an.example,addr2@an.example",
		{ "mailto", "",		"", "",			0,   "",		"to=addr1@an.example,addr2@an.example", "" } },
	{ "urn:oasis:names:specification:docbook:dtd:xml:4.1.2",
		{ "urn",	"",		"", "",			0,   "oasis:names:specification:docbook:dtd:xml:4.1.2", "", "" } },
	{ "http://www.goodsearch.com/login?return_path=/",
		{ "http",   "",		"", "www.goodsearch.com", 0, "/login", "return_path=/", "" } },
	{ "ldap://[2001:db8::7]:389/c=GB?objectClass?one",
		{ "ldap",   "",		"", "[2001:db8::7]",389,"/c=GB",   "objectClass?one", "" } },
	{ "ldap://[2001:db8::7]/c=GB?objectClass?one",
		{ "ldap",   "",		"", "[2001:db8::7]",0,  "/c=GB",   "objectClass?one", "" } },
	{ "HTTP://example.com.:80/%70a%74%68?a=%31#1%323",
		{ "HTTP",   "",		"", "example.com.",80,  "/%70a%74%68","a=%31","1%323"} },
	{ "/boot/home/Desktop/index.html",
		{ "",   "",			"", "",			 0,  "/boot/home/Desktop/index.html","",""} },
	{ "//remote.host/boot/home/Desktop",
		{ "",   "",			"", "remote.host",  0,  "/boot/home/Desktop","",""} },
	{ "tag:haiku-os.org,2020:repositories/haiku/r1beta2/x86_gcc2",
		{ "tag", "", "", "", 0, "haiku-os.org,2020:repositories/haiku/r1beta2/x86_gcc2" } }
};


class UrlTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(UrlTest);
	CPPUNIT_TEST(ParseTest);
	CPPUNIT_TEST(TestIsValid);
	CPPUNIT_TEST(TestGettersSetters);
	CPPUNIT_TEST(TestNullity);
	CPPUNIT_TEST(TestCopy);
	CPPUNIT_TEST(ExplodeImplodeTest);
	CPPUNIT_TEST(PathOnly);
	CPPUNIT_TEST(RelativeUriTest);
	CPPUNIT_TEST(IDNTest);
	CPPUNIT_TEST(TestHostAndPortWithNoUserNameAndPassword);
	CPPUNIT_TEST(TestWithUserNameAndPasswordNoHostAndPort);
	CPPUNIT_TEST(TestHostWithNoPortOrUserNameAndPassword);
	CPPUNIT_TEST(TestHostWithNoPortNoPath);
	CPPUNIT_TEST(TestHostWithPortNoPath);
	CPPUNIT_TEST(TestHostWithEmptyPort);
	CPPUNIT_TEST(TestHostWithPathAndFragment);
	CPPUNIT_TEST(TestHostWithFragment);
	CPPUNIT_TEST(TestIpv6HostPortPathAndRequest);
	CPPUNIT_TEST(TestProtocol);
	CPPUNIT_TEST(TestMailTo);
	CPPUNIT_TEST(TestDataUrl);
	CPPUNIT_TEST(TestAuthorityNoUserName);
	CPPUNIT_TEST(TestAuthorityWithCredentialsSeparatorNoPassword);
	CPPUNIT_TEST(TestAuthorityWithoutCredentialsSeparatorNoPassword);
	CPPUNIT_TEST(TestAuthorityBadPort);
	CPPUNIT_TEST(TestWhitespaceBefore);
	CPPUNIT_TEST(TestWhitespaceAfter);
	CPPUNIT_TEST(TestWhitespaceMiddle);
	CPPUNIT_TEST(TestEmpty);
	CPPUNIT_TEST(TestFileUrl);
	CPPUNIT_TEST(TestValidFullUrl);
	CPPUNIT_TEST(TestBadHosts);
	CPPUNIT_TEST_SUITE_END();

public:
	void IDNTest()
	{
		// http://www.w3.org/2004/04/uri-rel-test.html
		// TODO We need to decide wether to store them as UTF-8 or IDNA/punycode.

		struct Test {
			const char* escaped;
			const char* decoded;
		};

		Test tests[] = {
			{ "http://www.w%33.org", "http://www.w3.org" },
			{ "http://r%C3%A4ksm%C3%B6rg%C3%A5s.josefsson.org",
				"http://xn--rksmrgs-5wao1o.josefsson.org" },
			{ "http://%E7%B4%8D%E8%B1%86.w3.mag.keio.ac.jp",
				"http://xn--99zt52a.w3.mag.keio.ac.jp" },
			{ "http://www.%E3%81%BB%E3%82%93%E3%81%A8%E3%81%86%E3%81%AB%E3%81%AA"
				"%E3%81%8C%E3%81%84%E3%82%8F%E3%81%91%E3%81%AE%E3%82%8F%E3%81%8B%E3"
				"%82%89%E3%81%AA%E3%81%84%E3%81%A9%E3%82%81%E3%81%84%E3%82%93%E3%82"
				"%81%E3%81%84%E3%81%AE%E3%82%89%E3%81%B9%E3%82%8B%E3%81%BE%E3%81%A0"
				"%E3%81%AA%E3%81%8C%E3%81%8F%E3%81%97%E3%81%AA%E3%81%84%E3%81%A8%E3"
				"%81%9F%E3%82%8A%E3%81%AA%E3%81%84.w3.mag.keio.ac.jp/",
				"http://www.xn--n8jaaaaai5bhf7as8fsfk3jnknefdde3fg11amb5gzdb4wi9b"
				"ya3kc6lra.w3.mag.keio.ac.jp/" },

			{ "http://%E3%81%BB%E3%82%93%E3%81%A8%E3%81%86%E3%81%AB%E3%81%AA%E3"
				"%81%8C%E3%81%84%E3%82%8F%E3%81%91%E3%81%AE%E3%82%8F%E3%81%8B%E3%82"
				"%89%E3%81%AA%E3%81%84%E3%81%A9%E3%82%81%E3%81%84%E3%82%93%E3%82%81"
				"%E3%81%84%E3%81%AE%E3%82%89%E3%81%B9%E3%82%8B%E3%81%BE%E3%81%A0%E3"
				"%81%AA%E3%81%8C%E3%81%8F%E3%81%97%E3%81%AA%E3%81%84%E3%81%A8%E3%81"
				"%9F%E3%82%8A%E3%81%AA%E3%81%84.%E3%81%BB%E3%82%93%E3%81%A8%E3%81"
				"%86%E3%81%AB%E3%81%AA%E3%81%8C%E3%81%84%E3%82%8F%E3%81%91%E3%81%AE"
				"%E3%82%8F%E3%81%8B%E3%82%89%E3%81%AA%E3%81%84%E3%81%A9%E3%82%81%E3"
				"%81%84%E3%82%93%E3%82%81%E3%81%84%E3%81%AE%E3%82%89%E3%81%B9%E3%82"
				"%8B%E3%81%BE%E3%81%A0%E3%81%AA%E3%81%8C%E3%81%8F%E3%81%97%E3%81%AA"
				"%E3%81%84%E3%81%A8%E3%81%9F%E3%82%8A%E3%81%AA%E3%81%84.%E3%81%BB"
				"%E3%82%93%E3%81%A8%E3%81%86%E3%81%AB%E3%81%AA%E3%81%8C%E3%81%84%E3"
				"%82%8F%E3%81%91%E3%81%AE%E3%82%8F%E3%81%8B%E3%82%89%E3%81%AA%E3%81"
				"%84%E3%81%A9%E3%82%81%E3%81%84%E3%82%93%E3%82%81%E3%81%84%E3%81%AE"
				"%E3%82%89%E3%81%B9%E3%82%8B%E3%81%BE%E3%81%A0%E3%81%AA%E3%81%8C%E3"
				"%81%8F%E3%81%97%E3%81%AA%E3%81%84%E3%81%A8%E3%81%9F%E3%82%8A%E3%81"
				"%AA%E3%81%84.w3.mag.keio.ac.jp/",
				"http://xn--n8jaaaaai5bhf7as8fsfk3jnknefdde3fg11amb5gzdb4wi9bya3k"
				"c6lra.xn--n8jaaaaai5bhf7as8fsfk3jnknefdde3fg11amb5gzdb4wi9bya3kc6"
				"lra.xn--n8jaaaaai5bhf7as8fsfk3jnknefdde3fg11amb5gzdb4wi9bya3kc6lr"
				"a.w3.mag.keio.ac.jp/" },
			{ NULL, NULL }
		};

		for (int i = 0; tests[i].escaped != NULL; i++)
		{
			BUrl url(tests[i].escaped, false);
			BUrl idn(tests[i].decoded, false);
			status_t success = idn.IDNAToUnicode();

			CPPUNIT_ASSERT_EQUAL(B_OK, success);
			CPPUNIT_ASSERT_EQUAL(url.UrlString(), idn.UrlString());
		}
	}

	void RelativeUriTest()
	{
		// http://skew.org/uri/uri%5Ftests.html
		struct RelativeUrl {
			const char* base;
			const char* relative;
			const char* absolute;
		};

		const RelativeUrl tests[] = {
			// The port must be preserved
			{"http://host:81/",		"/path",				"http://host:81/path"},

			// Tests from http://skew.org/uri/uri_tests.html
			{"http://example.com/path?query#frag", "",
				"http://example.com/path?query"},
				// The fragment must be dropped when changing the path, but the
				// query must be preserved
			{"foo:a/b",				"../c",					"foo:/c"},
				// foo:c would be more intuitive, and is what skew.org tests.
				// However, foo:/c is what the RFC says we should get.
			{"foo:a",				"foo:.",				"foo:"},
			{"zz:abc",				"/foo/../../../bar",	"zz:/bar"},
			{"zz:abc",				"/foo/../bar",			"zz:/bar"},
			{"zz:abc",				"foo/../../../bar",		"zz:/bar"},
				// zz:bar would be more intuitive, ...
			{"zz:abc",				"zz:.",					"zz:"},
			{"http://a/b/c/d;p?q",	"/.",					"http://a/"},
			{"http://a/b/c/d;p?q",	"/.foo",				"http://a/.foo"},
			{"http://a/b/c/d;p?q",	".foo",					"http://a/b/c/.foo"},
			{"http://a/b/c/d;p?q",	"g:h",					"g:h"},

			{"http://a/b/c/d;p?q",	"g:h",					"g:h"},
			{"http://a/b/c/d;p?q",	"g",					"http://a/b/c/g"},
			{"http://a/b/c/d;p?q",	"./g",					"http://a/b/c/g"},
			{"http://a/b/c/d;p?q",	"g/",					"http://a/b/c/g/"},
			{"http://a/b/c/d;p?q",	"/g",					"http://a/g"},
			{"http://a/b/c/d;p?q",	"//g",					"http://g"},
			{"http://a/b/c/d;p?q",	"?y",					"http://a/b/c/d;p?y"},
			{"http://a/b/c/d;p?q",	"g?y",					"http://a/b/c/g?y"},
			{"http://a/b/c/d;p?q",	"#s",					"http://a/b/c/d;p?q#s"},
			{"http://a/b/c/d;p?q",	"g#s",					"http://a/b/c/g#s"},
			{"http://a/b/c/d;p?q",	"g?y#s",				"http://a/b/c/g?y#s"},
			{"http://a/b/c/d;p?q",	";x",					"http://a/b/c/;x"},
			{"http://a/b/c/d;p?q",	"g;x",					"http://a/b/c/g;x"},
			{"http://a/b/c/d;p?q",	"g;x?y#s",				"http://a/b/c/g;x?y#s"},
			{"http://a/b/c/d;p?q",	"",						"http://a/b/c/d;p?q"},
			{"http://a/b/c/d;p?q",	".",					"http://a/b/c/"},
			{"http://a/b/c/d;p?q",	"./",					"http://a/b/c/"},
			{"http://a/b/c/d;p?q",	"..",					"http://a/b/"},
			{"http://a/b/c/d;p?q",	"../",					"http://a/b/"},
			{"http://a/b/c/d;p?q",	"../g",					"http://a/b/g"},
			{"http://a/b/c/d;p?q",	"../..",				"http://a/"},
			{"http://a/b/c/d;p?q",	"../../",				"http://a/"},
			{"http://a/b/c/d;p?q",	"../../g",				"http://a/g"},

			// Parsers must be careful in handling cases where there are more
			// relative path ".." segments than there are hierarchical levels in the
			// base URI's path. Note that the ".." syntax cannot be used to change
			// the authority component of a URI.
			{"http://a/b/c/d;p?q",	"../../../g",			"http://a/g"},
			{"http://a/b/c/d;p?q",	"../../../../g",		"http://a/g"},

			// Similarly, parsers must remove the dot-segments "." and ".." when
			// they are complete components of a path, but not when they are only
			// part of a segment.
			{"http://a/b/c/d;p?q",	"/./g",					"http://a/g"},
			{"http://a/b/c/d;p?q",	"/../g",				"http://a/g"},
			{"http://a/b/c/d;p?q",	"g.",					"http://a/b/c/g."},
			{"http://a/b/c/d;p?q",	".g",					"http://a/b/c/.g"},
			{"http://a/b/c/d;p?q",	"g..",					"http://a/b/c/g.."},
			{"http://a/b/c/d;p?q",	"..g",					"http://a/b/c/..g"},

			// Less likely are cases where the relative URI reference uses
			// unnecessary or nonsensical forms of the "." and ".." complete path
			// segments.
			{"http://a/b/c/d;p?q",	"./../g",				"http://a/b/g"},
			{"http://a/b/c/d;p?q",	"./g/.",				"http://a/b/c/g/"},
			{"http://a/b/c/d;p?q",	"g/./h",				"http://a/b/c/g/h"},
			{"http://a/b/c/d;p?q",	"g/../h",				"http://a/b/c/h"},
			{"http://a/b/c/d;p?q",	"g;x=1/./y",			"http://a/b/c/g;x=1/y"},
			{"http://a/b/c/d;p?q",	"g;x=1/../y",			"http://a/b/c/y"},

			// Some applications fail to separate the reference's query and/or
			// fragment components from a relative path before merging it with the
			// base path and removing dot-segments. This error is rarely noticed,
			// since typical usage of a fragment never includes the hierarchy ("/")
			// character, and the query component is not normally used within
			// relative references.
			{"http://a/b/c/d;p?q",	"g?y/./x",			"http://a/b/c/g?y/./x"},
			{"http://a/b/c/d;p?q",	"g?y/../x",			"http://a/b/c/g?y/../x"},
			{"http://a/b/c/d;p?q",	"g#s/./x",			"http://a/b/c/g#s/./x"},
			{"http://a/b/c/d;p?q",	"g#s/../x",			"http://a/b/c/g#s/../x"},

			// Some parsers allow the scheme name to be present in a relative URI
			// reference if it is the same as the base URI scheme. This is
			// considered to be a loophole in prior specifications of partial URI
			// [RFC1630]. Its use should be avoided, but is allowed for backward
			// compatibility.
			{"http://a/b/c/d;p?q",	"http:g",			"http:g"},
			{"http://a/b/c/d;p?q",	"http:",			"http:"},

			{"http://a/b/c/d;p?q",	"./g:h",			"http://a/b/c/g:h"},
			{"http://a/b/c/d;p?q",	"/a/b/c/./../../g",	"http://a/a/g"},

			{"http://a/b/c/d;p?q=1/2",	"g",			"http://a/b/c/g"},
			{"http://a/b/c/d;p?q=1/2",	"./g",			"http://a/b/c/g"},
			{"http://a/b/c/d;p?q=1/2",	"g/",			"http://a/b/c/g/"},
			{"http://a/b/c/d;p?q=1/2",	"/g",			"http://a/g"},
			{"http://a/b/c/d;p?q=1/2",	"//g",			"http://g"},
			{"http://a/b/c/d;p?q=1/2",	"?y",			"http://a/b/c/d;p?y"},
			{"http://a/b/c/d;p?q=1/2",	"g?y",			"http://a/b/c/g?y"},
			{"http://a/b/c/d;p?q=1/2",	"g?y/./x",		"http://a/b/c/g?y/./x"},
			{"http://a/b/c/d;p?q=1/2",	"g?y/../x",		"http://a/b/c/g?y/../x"},
			{"http://a/b/c/d;p?q=1/2",	"g#s",			"http://a/b/c/g#s"},
			{"http://a/b/c/d;p?q=1/2",	"g#s/./x",		"http://a/b/c/g#s/./x"},
			{"http://a/b/c/d;p?q=1/2",	"g#s/../x",		"http://a/b/c/g#s/../x"},
			{"http://a/b/c/d;p?q=1/2",	"./",			"http://a/b/c/"},
			{"http://a/b/c/d;p?q=1/2",	"../",			"http://a/b/"},
			{"http://a/b/c/d;p?q=1/2",	"../g",			"http://a/b/g"},
			{"http://a/b/c/d;p?q=1/2",	"../../",		"http://a/"},
			{"http://a/b/c/d;p?q=1/2",	"../../g",		"http://a/g"},

			{"http://a/b/c/d;p=1/2?q",	"g",			"http://a/b/c/d;p=1/g"},
			{"http://a/b/c/d;p=1/2?q",	"./g",			"http://a/b/c/d;p=1/g"},
			{"http://a/b/c/d;p=1/2?q",	"g/",			"http://a/b/c/d;p=1/g/"},
			{"http://a/b/c/d;p=1/2?q",	"g?y",			"http://a/b/c/d;p=1/g?y"},
			{"http://a/b/c/d;p=1/2?q",	";x",			"http://a/b/c/d;p=1/;x"},
			{"http://a/b/c/d;p=1/2?q",	"g;x",			"http://a/b/c/d;p=1/g;x"},
			{"http://a/b/c/d;p=1/2?q", "g;x=1/./y", "http://a/b/c/d;p=1/g;x=1/y"},
			{"http://a/b/c/d;p=1/2?q",	"g;x=1/../y",	"http://a/b/c/d;p=1/y"},
			{"http://a/b/c/d;p=1/2?q",	"./",			"http://a/b/c/d;p=1/"},
			{"http://a/b/c/d;p=1/2?q",	"../",			"http://a/b/c/"},
			{"http://a/b/c/d;p=1/2?q",	"../g",			"http://a/b/c/g"},
			{"http://a/b/c/d;p=1/2?q",	"../../",		"http://a/b/"},
			{"http://a/b/c/d;p=1/2?q",	"../../g",		"http://a/b/g"},

			// Empty host and directory
			{"fred:///s//a/b/c",	"g:h",					"g:h"},
			{"fred:///s//a/b/c",	"g",					"fred:///s//a/b/g"},
			{"fred:///s//a/b/c",	"./g",					"fred:///s//a/b/g"},
			{"fred:///s//a/b/c",	"g/",					"fred:///s//a/b/g/"},
			{"fred:///s//a/b/c",	"/g",					"fred:///g"},
			{"fred:///s//a/b/c",	"//g",					"fred://g"},
			{"fred:///s//a/b/c",	"//g/x",				"fred://g/x"},
			{"fred:///s//a/b/c",	"///g",					"fred:///g"},
			{"fred:///s//a/b/c",	"./",					"fred:///s//a/b/"},
			{"fred:///s//a/b/c",	"../",					"fred:///s//a/"},
			{"fred:///s//a/b/c",	"../g",					"fred:///s//a/g"},
			{"fred:///s//a/b/c",	"../..",				"fred:///s//"},
			{"fred:///s//a/b/c",	"../../g",				"fred:///s//g"},
			{"fred:///s//a/b/c",	"../../../g",			"fred:///s/g"},
			{"fred:///s//a/b/c",	"../../../g",			"fred:///s/g"},

			{"http:///s//a/b/c",	"g:h",					"g:h"},
			{"http:///s//a/b/c",	"g",					"http:///s//a/b/g"},
			{"http:///s//a/b/c",	"./g",					"http:///s//a/b/g"},
			{"http:///s//a/b/c",	"g/",					"http:///s//a/b/g/"},
			{"http:///s//a/b/c",	"/g",					"http:///g"},
			{"http:///s//a/b/c",	"//g",					"http://g"},
			{"http:///s//a/b/c",	"//g/x",				"http://g/x"},
			{"http:///s//a/b/c",	"///g",					"http:///g"},
			{"http:///s//a/b/c",	"./",					"http:///s//a/b/"},
			{"http:///s//a/b/c",	"../",					"http:///s//a/"},
			{"http:///s//a/b/c",	"../g",					"http:///s//a/g"},
			{"http:///s//a/b/c",	"../..",				"http:///s//"},
			{"http:///s//a/b/c",	"../../g",				"http:///s//g"},
			{"http:///s//a/b/c",	"../../../g",			"http:///s/g"},
			{"http:///s//a/b/c",	"../../../g",			"http:///s/g"},

			{"foo:xyz",				"bar:abc",				"bar:abc"},
			{"http://example/x/y/z","../abc",				"http://example/x/abc"},
			{"http://example2/x/y/z","http://example/x/abc","http://example/x/abc"},
			{"http://ex/x/y/z",		"../r",					"http://ex/x/r"},
			{"http://ex/x/y",		"q/r",					"http://ex/x/q/r"},
			{"http://ex/x/y",		"q/r#s",				"http://ex/x/q/r#s"},
			{"http://ex/x/y",		"q/r#s/t",				"http://ex/x/q/r#s/t"},
			{"http://ex/x/y",		"ftp://ex/x/q/r",		"ftp://ex/x/q/r"},
			{"http://ex/x/y",		"",						"http://ex/x/y"},
			{"http://ex/x/y/",		"",						"http://ex/x/y/"},
			{"http://ex/x/y/pdq",	"",						"http://ex/x/y/pdq"},
			{"http://ex/x/y/",		"z/",					"http://ex/x/y/z/"},

			{"file:/swap/test/animal.rdf", "#Animal",
				"file:/swap/test/animal.rdf#Animal"},
			{"file:/e/x/y/z",		"../abc",				"file:/e/x/abc"},
			{"file:/example2/x/y/z","/example/x/abc",		"file:/example/x/abc"},
			{"file:/e/x/y/z",		"../r",					"file:/e/x/r"},
			{"file:/e/x/y/z",		"/r",					"file:/r"},
			{"file:/e/x/y",			"q/r",					"file:/e/x/q/r"},
			{"file:/e/x/y",			"q/r#s",				"file:/e/x/q/r#s"},
			{"file:/e/x/y",			"q/r#",					"file:/e/x/q/r#"},
			{"file:/e/x/y",			"q/r#s/t",				"file:/e/x/q/r#s/t"},
			{"file:/e/x/y",			"ftp://ex/x/q/r",		"ftp://ex/x/q/r"},
			{"file:/e/x/y",			"",						"file:/e/x/y"},
			{"file:/e/x/y/",		"",						"file:/e/x/y/"},
			{"file:/e/x/y/pdq",		"",						"file:/e/x/y/pdq"},
			{"file:/e/x/y/",		"z/",					"file:/e/x/y/z/"},
			{"file:/devel/WWW/2000/10/swap/test/reluri-1.n3",
				"file://meetings.example.com/cal#m1",
				"file://meetings.example.com/cal#m1"},
			{"file:/home/connolly/w3ccvs/WWW/2000/10/swap/test/reluri-1.n3",
				"file://meetings.example.com/cal#m1",
				"file://meetings.example.com/cal#m1"},
			{"file:/some/dir/foo",		"./#blort",		"file:/some/dir/#blort"},
			{"file:/some/dir/foo",		"./#",			"file:/some/dir/#"},

			{"http://example/x/abc.efg", "./",			"http://example/x/"},
			{"http://example2/x/y/z", "//example/x/abc","http://example/x/abc"},
			{"http://ex/x/y/z",			"/r",			"http://ex/r"},
			{"http://ex/x/y",			"./q:r",		"http://ex/x/q:r"},
			{"http://ex/x/y",			"./p=q:r",		"http://ex/x/p=q:r"},
			{"http://ex/x/y?pp/qq",		"?pp/rr",		"http://ex/x/y?pp/rr"},
			{"http://ex/x/y?pp/qq",		"y/z",			"http://ex/x/y/z"},

			{"mailto:local", "local/qual@domain.org#frag",
				"mailto:local/qual@domain.org#frag"},
			{"mailto:local/qual1@domain1.org", "more/qual2@domain2.org#frag",
				"mailto:local/more/qual2@domain2.org#frag"},

			{"http://ex/x/y?q",		"y?q",			"http://ex/x/y?q"},
			{"http://ex?p",			"x/y?q",		"http://ex/x/y?q"},
			{"foo:a/b",				"c/d",			"foo:a/c/d"},
			{"foo:a/b",				"/c/d",			"foo:/c/d"},
			{"foo:a/b?c#d",			"",				"foo:a/b?c"},
			{"foo:a",				"b/c",			"foo:b/c"},
			{"foo:/a/y/z",			"../b/c",		"foo:/a/b/c"},
			{"foo:a",				"./b/c",		"foo:b/c"},
			{"foo:a",				"/./b/c",		"foo:/b/c"},
			{"foo://a//b/c",		"../../d",		"foo://a/d"},
			{"foo:a",				".",			"foo:"},
			{"foo:a",				"..",			"foo:"},

			{"http://example/x/y%2Fz", "abc",			"http://example/x/abc"},
			{"http://example/a/x/y/z", "../../x%2Fabc", "http://example/a/x%2Fabc"},
			{"http://example/a/x/y%2Fz", "../x%2Fabc", "http://example/a/x%2Fabc"},
			{"http://example/x%2Fy/z", "abc", "http://example/x%2Fy/abc"},
			{"http://ex/x/y", "q%3Ar", "http://ex/x/q%3Ar"},
			{"http://example/x/y%2Fz", "/x%2Fabc", "http://example/x%2Fabc"},
			{"http://example/x/y/z", "/x%2Fabc", "http://example/x%2Fabc"},

			{"mailto:local1@domain1?query1", "local2@domain2",
				"mailto:local2@domain2"},
			{"mailto:local1@domain1", "local2@domain2?query2",
				"mailto:local2@domain2?query2"},
			{"mailto:local1@domain1?query1", "local2@domain2?query2",
				"mailto:local2@domain2?query2"},
			{"mailto:local@domain?query1", "?query2",
				"mailto:local@domain?query2"},
			{"mailto:?query1", "local2@domain2?query2",
				"mailto:local2@domain2?query2"},
			{"mailto:local1@domain1?query1", "?query2",
				"mailto:local1@domain1?query2"},

			{"foo:bar", "http://example/a/b?c/../d", "http://example/a/b?c/../d"},
			{"foo:bar", "http://example/a/b#c/../d", "http://example/a/b#c/../d"},
			{"http://example.org/base/uri", "http:this", "http:this"},
			{"http:base", "http:this", "http:this"},
			{"f:/a", ".//g", "f://g"},
			{"f://example.org/base/a", "b/c//d/e", "f://example.org/base/b/c//d/e"},
			{"mid:m@example.ord/c@example.org", "m2@example.ord/c2@example.org",
				"mid:m@example.ord/m2@example.ord/c2@example.org"},
			{"file:///C:/DEV/Haskell/lib/HXmlToolbox-3.01/examples/", "mini1.xml",
				"file:///C:/DEV/Haskell/lib/HXmlToolbox-3.01/examples/mini1.xml"},
			{"foo:a/y/z", "../b/c", "foo:a/b/c"},
			{"foo:", "b", "foo:b"},
			{"foo://a", "b", "foo://a/b"},
			{"foo://a?q", "b", "foo://a/b"},
			{"foo://a", "b?q", "foo://a/b?q"},
			{"foo://a?r", "b?q", "foo://a/b?q"},
		};

		BString message(" Base: ");
		for (unsigned int index = 0; index < sizeof(tests) / sizeof(RelativeUrl);
			index++)
		{
			BUrl baseUrl(tests[index].base, true);

			message.Truncate(7, true);
			message << tests[index].base;
			message << " Relative: ";
			message << tests[index].relative;

			CPPUNIT_ASSERT_EQUAL_MESSAGE(message.String(),
				BString(tests[index].absolute),
				BUrl(baseUrl, BString(tests[index].relative)).UrlString());
		}
	}

	void PathOnly()
	{
		BUrl test = "lol";
		CPPUNIT_ASSERT(test.HasPath());
		CPPUNIT_ASSERT_EQUAL(BString("lol"), test.Path());
		CPPUNIT_ASSERT_EQUAL(BString("lol"), test.UrlString());
	}

	void ExplodeImplodeTest()
	{
		uint8 testIndex;
		BUrl testUrl;

		for (testIndex = 0; testIndex < (sizeof(kTestExplode) / sizeof(ExplodeTest)); testIndex++)
		{
			testUrl.SetUrlString(kTestExplode[testIndex].url, true);

			CPPUNIT_ASSERT_EQUAL(BString(kTestExplode[testIndex].url),
				BString(testUrl.UrlString()));
			CPPUNIT_ASSERT_EQUAL(BString(kTestExplode[testIndex].expected.protocol),
				BString(testUrl.Protocol()));
			CPPUNIT_ASSERT_EQUAL(BString(kTestExplode[testIndex].expected.userName),
				BString(testUrl.UserName()));
			CPPUNIT_ASSERT_EQUAL(BString(kTestExplode[testIndex].expected.password),
				BString(testUrl.Password()));
			CPPUNIT_ASSERT_EQUAL(BString(kTestExplode[testIndex].expected.host),
				BString(testUrl.Host()));
			CPPUNIT_ASSERT_EQUAL(kTestExplode[testIndex].expected.port,
				testUrl.Port());
			CPPUNIT_ASSERT_EQUAL(BString(kTestExplode[testIndex].expected.path),
				BString(testUrl.Path()));
			CPPUNIT_ASSERT_EQUAL(BString(kTestExplode[testIndex].expected.request),
				BString(testUrl.Request()));
			CPPUNIT_ASSERT_EQUAL(BString(kTestExplode[testIndex].expected.fragment),
				BString(testUrl.Fragment()));
		}
	}

	void TestCopy()
	{
		BUrl url1("http://example.com", true);
		BUrl url2(url1, true);

		url2.SetHost("www.example.com");

		CPPUNIT_ASSERT_EQUAL(BString("www.example.com"), url2.Host());
		CPPUNIT_ASSERT_EQUAL(BString("http://www.example.com"), url2.UrlString());
		CPPUNIT_ASSERT_EQUAL(BString("example.com"), url1.Host());
		CPPUNIT_ASSERT_EQUAL(BString("http://example.com"), url1.UrlString());
	}

	void TestNullity()
	{
		BUrl url;
		url.SetProtocol("http");
		url.SetHost("example.com");

		CPPUNIT_ASSERT(url.HasAuthority());
		CPPUNIT_ASSERT(url.HasHost());

		CPPUNIT_ASSERT(!url.HasUserName());
		CPPUNIT_ASSERT(!url.HasPassword());
		CPPUNIT_ASSERT(!url.HasUserInfo());
		CPPUNIT_ASSERT(!url.HasPort());
		CPPUNIT_ASSERT(!url.HasPath());
		CPPUNIT_ASSERT(!url.HasRequest());
		CPPUNIT_ASSERT(!url.HasFragment());
	}

	void TestGettersSetters()
	{
		BUrl url;
		url.SetProtocol("http");
		url.SetUserName("user");
		url.SetPassword("password");
		url.SetHost("example.com");
		url.SetPort(8080);
		url.SetPath("/path");
		url.SetRequest("query=value");
		url.SetFragment("fragment");

		CPPUNIT_ASSERT_EQUAL(BString("http"), url.Protocol());
		CPPUNIT_ASSERT_EQUAL(BString("user"), url.UserName());
		CPPUNIT_ASSERT_EQUAL(BString("password"), url.Password());
		CPPUNIT_ASSERT_EQUAL(BString("user:password"), url.UserInfo());
		CPPUNIT_ASSERT_EQUAL(BString("example.com"), url.Host());
		CPPUNIT_ASSERT_EQUAL(BString("user:password@example.com:8080"),
			url.Authority());
		CPPUNIT_ASSERT_EQUAL(8080, url.Port());
		CPPUNIT_ASSERT_EQUAL(BString("/path"), url.Path());
		CPPUNIT_ASSERT_EQUAL(BString("query=value"), url.Request());
		CPPUNIT_ASSERT_EQUAL(BString("fragment"), url.Fragment());
		CPPUNIT_ASSERT_EQUAL(BString(
				"http://user:password@example.com:8080/path?query=value#fragment"),
			url.UrlString());
	}

	void TestIsValid()
	{
		BUrl url("http:", true);
		CPPUNIT_ASSERT_MESSAGE("Created with a scheme but no hierarchical segment.",
			!url.IsValid());

		url.SetHost("<invalid>");
		CPPUNIT_ASSERT_MESSAGE("Set to an invalid host", !url.IsValid());

		url.SetUrlString("", true);
		url.SetProtocol("\t \n");
		CPPUNIT_ASSERT_MESSAGE("Set a protocol with whitespace", !url.IsValid());
		url.SetProtocol("123");
		CPPUNIT_ASSERT_MESSAGE("Set an all-digits protocol", !url.IsValid());

		url.SetUserName("user");
		CPPUNIT_ASSERT_MESSAGE("Retain invalid state on user change",
			!url.IsValid());
		url.SetPassword("pass");
		CPPUNIT_ASSERT_MESSAGE("Retain invalid state on password change",
			!url.IsValid());

		url.SetProtocol("http");
		url.SetFragment("fragment");
		CPPUNIT_ASSERT_MESSAGE("Only protocol and fragment are set",
			!url.IsValid());
		url.SetFragment("fragment");
		url.SetProtocol("http");
		CPPUNIT_ASSERT_MESSAGE("Only protocol and fragment are set",
			!url.IsValid());
	}

	// Test that parsing a valid URL and converting back to string doesn't alter it
	void ParseTest()
	{
		uint8 testIndex;
		BUrl testUrl;

		const char* kTestLength[] =
		{
			"http://user:pass@www.foo.com:80/path?query#fragment",
			"http://user:pass@www.foo.com:80/path?query#",
			"http://user:pass@www.foo.com:80/path?query",
			"http://user:pass@www.foo.com:80/path?",
			"http://user:pass@www.foo.com:80/path",
			"http://user:pass@www.foo.com:80/",
			"http://user:pass@www.foo.com",
			"http://user:pass@",
			"http://www.foo.com",
			"http://",
			"http:"
		};

		for (testIndex = 0; testIndex < sizeof(kTestLength) / sizeof(const char*);
			testIndex++)
		{
			testUrl.SetUrlString(kTestLength[testIndex], true);
			CPPUNIT_ASSERT_EQUAL(BString(kTestLength[testIndex]),
				testUrl.UrlString());
		}
	}

	void TestValidFullUrl()
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

	void TestHostWithPathAndFragment()
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

	void TestHostWithFragment()
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

	void TestIpv6HostPortPathAndRequest()
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

	void TestFileUrl()
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

	void TestDataUrl()
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

	void TestWithUserNameAndPasswordNoHostAndPort()
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

	void TestHostAndPortWithNoUserNameAndPassword()
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

	void TestHostWithNoPortOrUserNameAndPassword()
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

	void TestHostWithNoPortNoPath()
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

	void TestHostWithPortNoPath()
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

	void TestHostWithEmptyPort()
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

	void TestProtocol()
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

	void TestMailTo()
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

	void TestAuthorityNoUserName()
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

	void TestAuthorityWithCredentialsSeparatorNoPassword()
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

	void TestAuthorityWithoutCredentialsSeparatorNoPassword()
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

	void TestAuthorityBadPort()
	{
		BUrl url("anything://host:aaa", true);
		CPPUNIT_ASSERT(url.IsValid());
		CPPUNIT_ASSERT(!url.HasUserName());
		CPPUNIT_ASSERT(!url.HasPassword());
		CPPUNIT_ASSERT(url.HasHost());
		CPPUNIT_ASSERT(url.Host() == "host");
		CPPUNIT_ASSERT(!url.HasPort());
	}

	void TestWhitespaceBefore()
	{
		BUrl url("   https://www.something.co.nz/z", true);
		CPPUNIT_ASSERT(!url.IsValid());
	}

	void TestWhitespaceAfter()
	{
		BUrl url("https://www.something.co.nz/z\t\t ", true);
		CPPUNIT_ASSERT(!url.IsValid());
	}

	void TestWhitespaceMiddle()
	{
		BUrl url("https://www.  something.co.nz/z", true);
		CPPUNIT_ASSERT(!url.IsValid());
	}

	void TestEmpty()
	{
		BUrl url("", true);
		CPPUNIT_ASSERT(!url.IsValid());
	}

	void TestBadHosts()
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
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(UrlTest, getTestSuiteName());
