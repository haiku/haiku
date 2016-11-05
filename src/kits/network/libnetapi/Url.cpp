/*
 * Copyright 2010-2016 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Andrew Lindesay, apl@lindesay.co.nz
 */


#include <Url.h>

#include <ctype.h>
#include <cstdio>
#include <cstdlib>
#include <new>

#include <MimeType.h>
#include <Roster.h>

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	#include <ICUWrapper.h>
#endif

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	#include <unicode/idna.h>
	#include <unicode/stringpiece.h>
#endif


static const char* kArchivedUrl = "be:url string";


BUrl::BUrl(const char* url)
	:
	fUrlString(),
	fProtocol(),
	fUser(),
	fPassword(),
	fHost(),
	fPort(0),
	fPath(),
	fRequest(),
	fHasHost(false),
	fHasFragment(false)
{
	SetUrlString(url);
}


BUrl::BUrl(BMessage* archive)
	:
	fUrlString(),
	fProtocol(),
	fUser(),
	fPassword(),
	fHost(),
	fPort(0),
	fPath(),
	fRequest(),
	fHasHost(false),
	fHasFragment(false)
{
	BString url;

	if (archive->FindString(kArchivedUrl, &url) == B_OK)
		SetUrlString(url);
	else
		_ResetFields();
}


BUrl::BUrl(const BUrl& other)
	:
	BArchivable(),
	fUrlString(),
	fProtocol(other.fProtocol),
	fUser(other.fUser),
	fPassword(other.fPassword),
	fHost(other.fHost),
	fPort(other.fPort),
	fPath(other.fPath),
	fRequest(other.fRequest),
	fFragment(other.fFragment),
	fUrlStringValid(other.fUrlStringValid),
	fAuthorityValid(other.fAuthorityValid),
	fUserInfoValid(other.fUserInfoValid),
	fHasProtocol(other.fHasProtocol),
	fHasUserName(other.fHasUserName),
	fHasPassword(other.fHasPassword),
	fHasHost(other.fHasHost),
	fHasPort(other.fHasPort),
	fHasPath(other.fHasPath),
	fHasRequest(other.fHasRequest),
	fHasFragment(other.fHasFragment)
{
	if (fUrlStringValid)
		fUrlString = other.fUrlString;

	if (fAuthorityValid)
		fAuthority = other.fAuthority;

	if (fUserInfoValid)
		fUserInfo = other.fUserInfo;

}


BUrl::BUrl(const BUrl& base, const BString& location)
	:
	fUrlString(),
	fProtocol(),
	fUser(),
	fPassword(),
	fHost(),
	fPort(0),
	fPath(),
	fRequest(),
	fAuthorityValid(false),
	fUserInfoValid(false),
	fHasUserName(false),
	fHasPassword(false),
	fHasHost(false),
	fHasPort(false),
	fHasFragment(false)
{
	// This implements the algorithm in RFC3986, Section 5.2.

	BUrl relative(location);
	if (relative.HasProtocol()) {
		SetProtocol(relative.Protocol());
		if (relative.HasAuthority())
			SetAuthority(relative.Authority());
		SetPath(relative.Path());
		SetRequest(relative.Request());
	} else {
		if (relative.HasAuthority()) {
			SetAuthority(relative.Authority());
			SetPath(relative.Path());
			SetRequest(relative.Request());
		} else {
			if (relative.Path().IsEmpty()) {
				_SetPathUnsafe(base.Path());
				if (relative.HasRequest())
					SetRequest(relative.Request());
				else
					SetRequest(base.Request());
			} else {
				if (relative.Path()[0] == '/')
					SetPath(relative.Path());
				else {
					BString path = base._MergePath(relative.Path());
					SetPath(path);
				}
				SetRequest(relative.Request());
			}

			if (base.HasAuthority())
				SetAuthority(base.Authority());
		}
		SetProtocol(base.Protocol());
	}

	if (relative.HasFragment())
		SetFragment(relative.Fragment());
}


BUrl::BUrl()
	:
	fUrlString(),
	fProtocol(),
	fUser(),
	fPassword(),
	fHost(),
	fPort(0),
	fPath(),
	fRequest(),
	fHasHost(false),
	fHasFragment(false)
{
	_ResetFields();
}


BUrl::BUrl(const BPath& path)
	:
	fUrlString(),
	fProtocol(),
	fUser(),
	fPassword(),
	fHost(),
	fPort(0),
	fPath(),
	fRequest(),
	fHasHost(false),
	fHasFragment(false)
{
	SetUrlString(UrlEncode(path.Path(), true, true));
	SetProtocol("file");
}


BUrl::~BUrl()
{
}


// #pragma mark URL fields modifiers


BUrl&
BUrl::SetUrlString(const BString& url)
{
	_ExplodeUrlString(url);
	return *this;
}


BUrl&
BUrl::SetProtocol(const BString& protocol)
{
	fProtocol = protocol;
	fHasProtocol = !fProtocol.IsEmpty();
	fUrlStringValid = false;
	return *this;
}


BUrl&
BUrl::SetUserName(const BString& user)
{
	fUser = user;
	fHasUserName = !fUser.IsEmpty();
	fUrlStringValid = false;
	fAuthorityValid = false;
	fUserInfoValid = false;
	return *this;
}


BUrl&
BUrl::SetPassword(const BString& password)
{
	fPassword = password;
	fHasPassword = !fPassword.IsEmpty();
	fUrlStringValid = false;
	fAuthorityValid = false;
	fUserInfoValid = false;
	return *this;
}


BUrl&
BUrl::SetHost(const BString& host)
{
	fHost = host;
	fHasHost = !fHost.IsEmpty();
	fUrlStringValid = false;
	fAuthorityValid = false;
	return *this;
}


BUrl&
BUrl::SetPort(int port)
{
	fPort = port;
	fHasPort = (port != 0);
	fUrlStringValid = false;
	fAuthorityValid = false;
	return *this;
}


BUrl&
BUrl::SetPath(const BString& path)
{
	// Implements RFC3986 section 5.2.4, "Remove dot segments"

	// 1.
	BString output;
	BString input(path);

	// 2.
	while(!input.IsEmpty())
	{
		// 2.A.
		if (input.StartsWith("./"))
		{
			input.Remove(0, 2);
			continue;
		}

		if (input.StartsWith("../"))
		{
			input.Remove(0, 3);
			continue;
		}

		// 2.B.
		if (input.StartsWith("/./"))
		{
			input.Remove(0, 2);
			continue;
		}

		if (input == "/.")
		{
			input.Remove(1, 1);
			continue;
		}

		// 2.C.
		if (input.StartsWith("/../"))
		{
			input.Remove(0, 3);
			output.Truncate(output.FindLast('/'));
			continue;
		}

		if (input == "/..")
		{
			input.Remove(1, 2);
			output.Truncate(output.FindLast('/'));
			continue;
		}

		// 2.D.
		if (input == "." || input == "..")
		{
			break;
		}

		if (input == "/.")
		{
			input.Remove(1, 1);
			continue;
		}

		// 2.E.
		int slashpos = input.FindFirst('/', 1);
		if (slashpos > 0) {
			output.Append(input, slashpos);
			input.Remove(0, slashpos);
		} else {
			output.Append(input);
			break;
		}
	}

	_SetPathUnsafe(output);
	return *this;
}


BUrl&
BUrl::SetRequest(const BString& request)
{
	fRequest = request;
	fHasRequest = !fRequest.IsEmpty();
	fUrlStringValid = false;
	return *this;
}


BUrl&
BUrl::SetFragment(const BString& fragment)
{
	fFragment = fragment;
	fHasFragment = true;
	fUrlStringValid = false;
	return *this;
}


// #pragma mark URL fields access


const BString&
BUrl::UrlString() const
{
	if (!fUrlStringValid) {
		fUrlString.Truncate(0);

		if (HasProtocol()) {
			fUrlString << fProtocol << ':';
		}

		if (HasAuthority()) {
			fUrlString << "//";
			fUrlString << Authority();
		}
		fUrlString << Path();

		if (HasRequest())
			fUrlString << '?' << fRequest;

		if (HasFragment())
			fUrlString << '#' << fFragment;

		fUrlStringValid = true;
	}

	return fUrlString;
}


const BString&
BUrl::Protocol() const
{
	return fProtocol;
}


const BString&
BUrl::UserName() const
{
	return fUser;
}


const BString&
BUrl::Password() const
{
	return fPassword;
}


const BString&
BUrl::UserInfo() const
{
	if (!fUserInfoValid) {
		fUserInfo = fUser;

		if (HasPassword())
			fUserInfo << ':' << fPassword;

		fUserInfoValid = true;
	}

	return fUserInfo;
}


const BString&
BUrl::Host() const
{
	return fHost;
}


int
BUrl::Port() const
{
	return fPort;
}


const BString&
BUrl::Authority() const
{
	if (!fAuthorityValid) {
		fAuthority.Truncate(0);

		if (HasUserInfo())
			fAuthority << UserInfo() << '@';
		fAuthority << Host();

		if (HasPort())
			fAuthority << ':' << fPort;

		fAuthorityValid = true;
	}
	return fAuthority;
}


const BString&
BUrl::Path() const
{
	return fPath;
}


const BString&
BUrl::Request() const
{
	return fRequest;
}


const BString&
BUrl::Fragment() const
{
	return fFragment;
}


// #pragma mark URL fields tests


bool
BUrl::IsValid() const
{
	if (!fHasProtocol)
		return false;

	if (fProtocol == "http" || fProtocol == "https" || fProtocol == "ftp"
		|| fProtocol == "ipp" || fProtocol == "afp" || fProtocol == "telnet"
		|| fProtocol == "gopher" || fProtocol == "nntp" || fProtocol == "sftp"
		|| fProtocol == "finger" || fProtocol == "pop" || fProtocol == "imap") {
		return fHasHost && !fHost.IsEmpty();
	}

	if (fProtocol == "file")
		return fHasPath;

	return true;
}


bool
BUrl::HasProtocol() const
{
	return fHasProtocol;
}


bool
BUrl::HasAuthority() const
{
	return fHasHost || fHasUserName;
}


bool
BUrl::HasUserName() const
{
	return fHasUserName;
}


bool
BUrl::HasPassword() const
{
	return fHasPassword;
}


bool
BUrl::HasUserInfo() const
{
	return fHasUserName || fHasPassword;
}


bool
BUrl::HasHost() const
{
	return fHasHost;
}


bool
BUrl::HasPort() const
{
	return fHasPort;
}


bool
BUrl::HasPath() const
{
	return fHasPath;
}


bool
BUrl::HasRequest() const
{
	return fHasRequest;
}


bool
BUrl::HasFragment() const
{
	return fHasFragment;
}


// #pragma mark URL encoding/decoding of needed fields


void
BUrl::UrlEncode(bool strict)
{
	fUser = _DoUrlEncodeChunk(fUser, strict);
	fPassword = _DoUrlEncodeChunk(fPassword, strict);
	fHost = _DoUrlEncodeChunk(fHost, strict);
	fFragment = _DoUrlEncodeChunk(fFragment, strict);
	fPath = _DoUrlEncodeChunk(fPath, strict, true);
}


void
BUrl::UrlDecode(bool strict)
{
	fUser = _DoUrlDecodeChunk(fUser, strict);
	fPassword = _DoUrlDecodeChunk(fPassword, strict);
	fHost = _DoUrlDecodeChunk(fHost, strict);
	fFragment = _DoUrlDecodeChunk(fFragment, strict);
	fPath = _DoUrlDecodeChunk(fPath, strict);
}


#ifdef HAIKU_TARGET_PLATFORM_HAIKU
status_t
BUrl::IDNAToAscii()
{
	UErrorCode err = U_ZERO_ERROR;
	icu::IDNA* converter = icu::IDNA::createUTS46Instance(0, err);
	icu::IDNAInfo info;

	BString result;
	BStringByteSink sink(&result);
	converter->nameToASCII_UTF8(icu::StringPiece(fHost.String()), sink, info,
		err);

	delete converter;

	if (U_FAILURE(err))
		return B_ERROR;

	fHost = result;
	return B_OK;
}
#endif


#ifdef HAIKU_TARGET_PLATFORM_HAIKU
status_t
BUrl::IDNAToUnicode()
{
	UErrorCode err = U_ZERO_ERROR;
	icu::IDNA* converter = icu::IDNA::createUTS46Instance(0, err);
	icu::IDNAInfo info;

	BString result;
	BStringByteSink sink(&result);
	converter->nameToUnicodeUTF8(icu::StringPiece(fHost.String()), sink, info,
		err);

	delete converter;

	if (U_FAILURE(err))
		return B_ERROR;

	fHost = result;
	return B_OK;
}
#endif


// #pragma mark - utility functionality


#ifdef HAIKU_TARGET_PLATFORM_HAIKU
bool
BUrl::HasPreferredApplication() const
{
	BString appSignature = PreferredApplication();
	BMimeType mime(appSignature.String());

	if (appSignature.IFindFirst("application/") == 0
		&& mime.IsValid())
		return true;

	return false;
}
#endif


#ifdef HAIKU_TARGET_PLATFORM_HAIKU
BString
BUrl::PreferredApplication() const
{
	BString appSignature;
	BMimeType mime(_UrlMimeType().String());
	mime.GetPreferredApp(appSignature.LockBuffer(B_MIME_TYPE_LENGTH));
	appSignature.UnlockBuffer();

	return BString(appSignature);
}
#endif


#ifdef HAIKU_TARGET_PLATFORM_HAIKU
status_t
BUrl::OpenWithPreferredApplication(bool onProblemAskUser) const
{
	if (!IsValid())
		return B_BAD_VALUE;

	BString urlString = UrlString();
	if (urlString.Length() > B_PATH_NAME_LENGTH) {
		// TODO: BAlert
		//	if (onProblemAskUser)
		//		BAlert ... Too long URL!
#if DEBUG
		fprintf(stderr, "URL too long");
#endif
		return B_NAME_TOO_LONG;
	}

	char* argv[] = {
		const_cast<char*>("BUrlInvokedApplication"),
		const_cast<char*>(urlString.String()),
		NULL
	};

#if DEBUG
	if (HasPreferredApplication())
		printf("HasPreferredApplication() == true\n");
	else
		printf("HasPreferredApplication() == false\n");
#endif

	status_t status = be_roster->Launch(_UrlMimeType().String(), 1, argv+1);
	if (status != B_OK) {
#if DEBUG
		fprintf(stderr, "Opening URL failed: %s\n", strerror(status));
#endif
	}

	return status;
}
#endif


// #pragma mark Url encoding/decoding of string


/*static*/ BString
BUrl::UrlEncode(const BString& url, bool strict, bool directory)
{
	return _DoUrlEncodeChunk(url, strict, directory);
}


/*static*/ BString
BUrl::UrlDecode(const BString& url, bool strict)
{
	return _DoUrlDecodeChunk(url, strict);
}


// #pragma mark BArchivable members


status_t
BUrl::Archive(BMessage* into, bool deep) const
{
	status_t ret = BArchivable::Archive(into, deep);

	if (ret == B_OK)
		ret = into->AddString(kArchivedUrl, UrlString());

	return ret;
}


/*static*/ BArchivable*
BUrl::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BUrl"))
		return new(std::nothrow) BUrl(archive);
	return NULL;
}


// #pragma mark URL comparison


bool
BUrl::operator==(BUrl& other) const
{
	UrlString();
	other.UrlString();

	return fUrlString == other.fUrlString;
}


bool
BUrl::operator!=(BUrl& other) const
{
	return !(*this == other);
}


// #pragma mark URL assignment


const BUrl&
BUrl::operator=(const BUrl& other)
{
	fUrlStringValid = other.fUrlStringValid;
	if (fUrlStringValid)
		fUrlString = other.fUrlString;

	fAuthorityValid = other.fAuthorityValid;
	if (fAuthorityValid)
		fAuthority = other.fAuthority;

	fUserInfoValid = other.fUserInfoValid;
	if (fUserInfoValid)
		fUserInfo = other.fUserInfo;

	fProtocol = other.fProtocol;
	fUser = other.fUser;
	fPassword = other.fPassword;
	fHost = other.fHost;
	fPort = other.fPort;
	fPath = other.fPath;
	fRequest = other.fRequest;
	fFragment = other.fFragment;

	fHasProtocol = other.fHasProtocol;
	fHasUserName = other.fHasUserName;
	fHasPassword = other.fHasPassword;
	fHasHost = other.fHasHost;
	fHasPort = other.fHasPort;
	fHasPath = other.fHasPath;
	fHasRequest = other.fHasRequest;
	fHasFragment = other.fHasFragment;

	return *this;
}


const BUrl&
BUrl::operator=(const BString& string)
{
	SetUrlString(string);
	return *this;
}


const BUrl&
BUrl::operator=(const char* string)
{
	SetUrlString(string);
	return *this;
}


// #pragma mark URL to string conversion


BUrl::operator const char*() const
{
	return UrlString();
}


void
BUrl::_ResetFields()
{
	fHasProtocol = false;
	fHasUserName = false;
	fHasPassword = false;
	fHasHost = false;
	fHasPort = false;
	fHasPath = false;
	fHasRequest = false;
	fHasFragment = false;

	fProtocol.Truncate(0);
	fUser.Truncate(0);
	fPassword.Truncate(0);
	fHost.Truncate(0);
	fPort = 0;
	fPath.Truncate(0);
	fRequest.Truncate(0);
	fFragment.Truncate(0);

	// Force re-generation of these fields
	fUrlStringValid = false;
	fUserInfoValid = false;
	fAuthorityValid = false;
}


bool
BUrl::_ContainsDelimiter(const BString& url)
{
	int32 len = url.Length();

	for (int32 i = 0; i < len; i++) {
		switch (url[i]) {
			case ' ':
			case '\n':
			case '\t':
			case '\r':
			case '<':
			case '>':
			case '"':
				return true;
		}
	}

	return false;
}


enum explode_url_parse_state {
	EXPLODE_PROTOCOL,
	EXPLODE_PROTOCOLTERMINATOR,
	EXPLODE_AUTHORITYORPATH,
	EXPLODE_AUTHORITY,
	EXPLODE_PATH,
	EXPLODE_REQUEST, // query
	EXPLODE_FRAGMENT,
	EXPLODE_COMPLETE
};


typedef bool (*explode_char_match_fn)(char c);


static bool
explode_is_protocol_char(char c)
{
	return isalnum(c) || c == '+' || c == '.' || c == '-';
}


static bool
explode_is_authority_char(char c)
{
	return !(c == '/' || c == '?' || c == '#');
}


static bool
explode_is_path_char(char c)
{
	return !(c == '#' || c == '?');
}


static bool
explode_is_request_char(char c)
{
	return c != '#';
}


static int32
char_offset_until_fn_false(const char* url, int32 len, int32 offset,
	explode_char_match_fn fn)
{
	while (offset < len && fn(url[offset]))
		offset++;

	return offset;
}

/*
 * This function takes a URL in string-form and parses the components of the URL out.
 */
status_t
BUrl::_ExplodeUrlString(const BString& url)
{
	_ResetFields();

	// RFC3986, Appendix C; the URL should not contain whitespace or delimiters
	// by this point.

	if (_ContainsDelimiter(url))
		return B_BAD_VALUE;

	explode_url_parse_state state = EXPLODE_PROTOCOL;
	int32 offset = 0;
	int32 length = url.Length();
	const char *url_c = url.String();

	// The regexp is provided in RFC3986 (URI generic syntax), Appendix B
	// ^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?
	// The ensuing logic attempts to simulate the behaviour of extracting the groups
	// from the string without requiring a group-capable regex engine.

	while (offset < length) {
		switch (state) {

			case EXPLODE_PROTOCOL:
			{
				int32 end_protocol = char_offset_until_fn_false(url_c, length,
					offset, explode_is_protocol_char);

				if (end_protocol < length) {
					SetProtocol(BString(&url_c[offset], end_protocol - offset));
					state = EXPLODE_PROTOCOLTERMINATOR;
					offset = end_protocol;
				} else {
					// No protocol was found, try parsing from the string
					// start, beginning with authority or path
					SetProtocol("");
					offset = 0;
					state = EXPLODE_AUTHORITYORPATH;
				}
				break;
			}

			case EXPLODE_PROTOCOLTERMINATOR:
			{
				if (url[offset] == ':') {
					offset++;
				} else {
					// No protocol was found, try parsing from the string
					// start, beginning with authority or path
					SetProtocol("");
					offset = 0;
				}
				state = EXPLODE_AUTHORITYORPATH;
				break;
			}

			case EXPLODE_AUTHORITYORPATH:
			{
				// The authority must start with //. If it isn't there, skip
				// to parsing the path.
				if (strncmp(&url_c[offset], "//", 2) == 0) {
					state = EXPLODE_AUTHORITY;
					offset += 2;
				} else {
					state = EXPLODE_PATH;
				}
				break;
			}

			case EXPLODE_AUTHORITY:
			{
				int end_authority = char_offset_until_fn_false(url_c, length,
					offset, explode_is_authority_char);
				SetAuthority(BString(&url_c[offset], end_authority - offset));
				state = EXPLODE_PATH;
				offset = end_authority;
				break;
			}

			case EXPLODE_PATH:
			{
				int end_path = char_offset_until_fn_false(url_c, length, offset,
					explode_is_path_char);
				SetPath(BString(&url_c[offset], end_path - offset));
				state = EXPLODE_REQUEST;
				offset = end_path;
				break;
			}

			case EXPLODE_REQUEST: // query
			{
				if (url_c[offset] == '?') {
					offset++;
					int end_request = char_offset_until_fn_false(url_c, length,
						offset, explode_is_request_char);
					SetRequest(BString(&url_c[offset], end_request - offset));
					offset = end_request;
				}
				state = EXPLODE_FRAGMENT;
				break;
			}

			case EXPLODE_FRAGMENT:
			{
				if (url_c[offset] == '#') {
					offset++;
					SetFragment(BString(&url_c[offset], length - offset));
					offset = length;
				}
				state = EXPLODE_COMPLETE;
				break;
			}

			case EXPLODE_COMPLETE:
				// should never be reached - keeps the compiler happy
				break;

		}
	}

	return B_OK;
}


BString
BUrl::_MergePath(const BString& relative) const
{
	// This implements RFC3986, Section 5.2.3.
	if (HasAuthority() && fPath == "")
	{
		BString result("/");
		result << relative;
		return result;
	}

	BString result(fPath);
	result.Truncate(result.FindLast("/") + 1);
	result << relative;

	return result;
}


// This sets the path without normalizing it. If fed with a path that has . or
// .. segments, this would make the URL invalid.
void
BUrl::_SetPathUnsafe(const BString& path)
{
	fPath = path;
	fHasPath = true; // RFC says an empty path is still a path
	fUrlStringValid = false;
}


enum authority_parse_state {
	AUTHORITY_USERNAME,
	AUTHORITY_PASSWORD,
	AUTHORITY_HOST,
	AUTHORITY_PORT,
	AUTHORITY_COMPLETE
};


static bool
authority_is_username_char(char c)
{
	return !(c == ':' || c == '@');
}


static bool
authority_is_password_char(char c)
{
	return !(c == '@');
}


static bool
authority_is_ipv6_host_char(char c) {
	return (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')
		|| (c >= '0' && c <= '9') || c == ':';
}


static bool
authority_is_host_char(char c) {
	return !(c == ':' || c == '/');
}


static bool
authority_is_port_char(char c) {
	return c >= '0' && c <= '9';
}


void
BUrl::SetAuthority(const BString& authority)
{
	fAuthority = authority;

	fUser.Truncate(0);
	fPassword.Truncate(0);
	fHost.Truncate(0);
	fPort = 0;
	fHasPort = false;
	fHasUserName = false;
	fHasPassword = false;

	bool hasUsernamePassword = B_ERROR != fAuthority.FindFirst('@');
	authority_parse_state state = AUTHORITY_USERNAME;
	int32 offset = 0;
	int32 length = authority.Length();
	const char *authority_c = authority.String();

	while (AUTHORITY_COMPLETE != state && offset < length) {

		switch (state) {

			case AUTHORITY_USERNAME:
			{
				if (hasUsernamePassword) {
					int32 end_username = char_offset_until_fn_false(
						authority_c, length, offset,
						authority_is_username_char);

					SetUserName(BString(&authority_c[offset],
						end_username - offset));

					state = AUTHORITY_PASSWORD;
					offset = end_username;
				} else {
					state = AUTHORITY_HOST;
				}
				break;
			}

			case AUTHORITY_PASSWORD:
			{
				if (hasUsernamePassword && ':' == authority[offset]) {
					offset++; // move past the delimiter
					int32 end_password = char_offset_until_fn_false(
						authority_c, length, offset,
						authority_is_password_char);

					SetPassword(BString(&authority_c[offset],
						end_password - offset));

					offset = end_password;
				}

				// if the host was preceded by a username + password couple
				// then there will be an '@' delimiter to avoid.

				if (authority_c[offset] == '@') {
					offset++;
				}

				state = AUTHORITY_HOST;
				break;
			}

			case AUTHORITY_HOST:
			{

				// the host may be enclosed within brackets in order to express
				// an IPV6 address.

				if (authority_c[offset] == '[') {
					int32 end_ipv6_host = char_offset_until_fn_false(
						authority_c, length, offset + 1,
						authority_is_ipv6_host_char);

					if (authority_c[end_ipv6_host] == ']') {
						SetHost(BString(&authority_c[offset],
							(end_ipv6_host - offset) + 1));
						state = AUTHORITY_PORT;
						offset = end_ipv6_host + 1;
					}
				}

				// if an IPV6 host was not found.

				if (AUTHORITY_HOST == state) {
					int32 end_host = char_offset_until_fn_false(
						authority_c, length, offset, authority_is_host_char);

					SetHost(BString(&authority_c[offset], end_host - offset));
					state = AUTHORITY_PORT;
					offset = end_host;
				}

				break;
			}

			case AUTHORITY_PORT:
			{
				if (authority_c[offset] == ':') {
					offset++;
					int32 end_port = char_offset_until_fn_false(
						authority_c, length, offset, authority_is_port_char);
					SetPort(atoi(&authority_c[offset]));
					offset = end_port;
				}

				state = AUTHORITY_COMPLETE;

				break;
			}

			case AUTHORITY_COMPLETE:
				// should never be reached - keeps the compiler happy
				break;
		}
	}

	// An empty authority is still an authority, making it possible to have
	// URLs such as file:///path/to/file.
	// TODO however, there is no way to unset the authority once it is set...
	// We may want to take a const char* parameter and allow NULL.
	fHasHost = true;
}


/*static*/ BString
BUrl::_DoUrlEncodeChunk(const BString& chunk, bool strict, bool directory)
{
	BString result;

	for (int32 i = 0; i < chunk.Length(); i++) {
		if (_IsUnreserved(chunk[i])
				|| (directory && (chunk[i] == '/' || chunk[i] == '\\'))) {
			result << chunk[i];
		} else {
			if (chunk[i] == ' ' && !strict) {
				result << '+';
					// In non-strict mode, spaces are encoded by a plus sign
			} else {
				char hexString[5];
				snprintf(hexString, 5, "%X", chunk[i]);

				result << '%' << hexString;
			}
		}
	}

	return result;
}


/*static*/ BString
BUrl::_DoUrlDecodeChunk(const BString& chunk, bool strict)
{
	BString result;

	for (int32 i = 0; i < chunk.Length(); i++) {
		if (chunk[i] == '+' && !strict)
			result << ' ';
		else {
			char decoded = 0;
			char* out = NULL;
			char hexString[3];

			if (chunk[i] == '%' && i < chunk.Length() - 2
				&& isxdigit(chunk[i + 1]) && isxdigit(chunk[i+2])) {
				hexString[0] = chunk[i + 1];
				hexString[1] = chunk[i + 2];
				hexString[2] = 0;
				decoded = (char)strtol(hexString, &out, 16);
			}

			if (out == hexString + 2) {
				i += 2;
				result << decoded;
			} else
				result << chunk[i];
		}
	}
	return result;
}


bool
BUrl::_IsProtocolValid()
{
	for (int8 index = 0; index < fProtocol.Length(); index++) {
		char c = fProtocol[index];

		if (index == 0 && !isalpha(c))
			return false;
		else if (!isalnum(c) && c != '+' && c != '-' && c != '.')
			return false;
	}

	return fProtocol.Length() > 0;
}


bool
BUrl::_IsUnreserved(char c)
{
	return isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~';
}


bool
BUrl::_IsGenDelim(char c)
{
	return c == ':' || c == '/' || c == '?' || c == '#' || c == '['
		|| c == ']' || c == '@';
}


bool
BUrl::_IsSubDelim(char c)
{
	return c == '!' || c == '$' || c == '&' || c == '\'' || c == '('
		|| c == ')' || c == '*' || c == '+' || c == ',' || c == ';'
		|| c == '=';
}


BString
BUrl::_UrlMimeType() const
{
	BString mime;
	mime << "application/x-vnd.Be.URL." << fProtocol;

	return BString(mime);
}
