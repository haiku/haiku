/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <Url.h>

#include <ctype.h>
#include <cstdio>
#include <cstdlib>
#include <new>

#include <MimeType.h>
#include <Roster.h>

#include <ICUWrapper.h>
#include <RegExp.h>

#include <unicode/idna.h>
#include <unicode/stringpiece.h>


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
			if (HasAuthority())
				fUrlString << "//";
		}

		fUrlString << Authority();
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
	// TODO: Implement for real!
	return fHasProtocol && (fHasHost || fHasPath);
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


// #pragma mark - utility functionality


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


BString
BUrl::PreferredApplication() const
{
	BString appSignature;
	BMimeType mime(_UrlMimeType().String());
	mime.GetPreferredApp(appSignature.LockBuffer(B_MIME_TYPE_LENGTH));
	appSignature.UnlockBuffer();

	return BString(appSignature);
}


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


void
BUrl::_ExplodeUrlString(const BString& url)
{
	// The regexp is provided in RFC3986 (URI generic syntax), Appendix B
	static RegExp urlMatcher(
		"^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?");

	_ResetFields();

	RegExp::MatchResult match = urlMatcher.Match(url.String());

	if (!match.HasMatched())
		return; // TODO error reporting

	// Scheme/Protocol
	url.CopyInto(fProtocol, match.GroupStartOffsetAt(1),
		match.GroupEndOffsetAt(1) - match.GroupStartOffsetAt(1));
	if (!_IsProtocolValid()) {
		fHasProtocol = false;
		fProtocol.Truncate(0);
	} else
		fHasProtocol = true;

	// Authority (including user credentials, host, and port
	if (match.GroupEndOffsetAt(2) - match.GroupStartOffsetAt(2) > 0)
	{
		url.CopyInto(fAuthority, match.GroupStartOffsetAt(3),
			match.GroupEndOffsetAt(3) - match.GroupStartOffsetAt(3));
		SetAuthority(fAuthority);
	} else {
		fHasHost = false;
		fHasPort = false;
		fHasUserName = false;
		fHasPassword = false;
	}

	// Path
	url.CopyInto(fPath, match.GroupStartOffsetAt(4),
		match.GroupEndOffsetAt(4) - match.GroupStartOffsetAt(4));
	if (!fPath.IsEmpty())
		fHasPath = true;

	// Query
	if (match.GroupEndOffsetAt(5) - match.GroupStartOffsetAt(5) > 0)
	{
		url.CopyInto(fRequest, match.GroupStartOffsetAt(6),
			match.GroupEndOffsetAt(6) - match.GroupStartOffsetAt(6));
		fHasRequest = true;
	} else {
		fRequest = "";
		fHasRequest = false;
	}

	// Fragment
	if (match.GroupEndOffsetAt(7) - match.GroupStartOffsetAt(7) > 0)
	{
		url.CopyInto(fFragment, match.GroupStartOffsetAt(8),
			match.GroupEndOffsetAt(8) - match.GroupStartOffsetAt(8));
		fHasFragment = true;
	} else {
		fFragment = "";
		fHasFragment = false;
	}
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


void
BUrl::SetAuthority(const BString& authority)
{
	fAuthority = authority;

	fHasPort = false;
	fHasUserName = false;
	fHasPassword = false;

	// An empty authority is still an authority, making it possible to have
	// URLs such as file:///path/to/file.
	// TODO however, there is no way to unset the authority once it is set...
	// We may want to take a const char* parameter and allow NULL.
	fHasHost = true;

	if (fAuthority.IsEmpty())
		return;

	int32 userInfoEnd = fAuthority.FindFirst('@');

	// URL contains userinfo field
	if (userInfoEnd != -1) {
		BString userInfo;
		fAuthority.CopyInto(userInfo, 0, userInfoEnd);

		int16 colonDelimiter = userInfo.FindFirst(':', 0);

		if (colonDelimiter == 0) {
			SetPassword(userInfo);
		} else if (colonDelimiter != -1) {
			userInfo.CopyInto(fUser, 0, colonDelimiter);
			userInfo.CopyInto(fPassword, colonDelimiter + 1,
				userInfo.Length() - colonDelimiter);
			SetUserName(fUser);
			SetPassword(fPassword);
		} else {
			SetUserName(fUser);
		}
	}


	// Extract the host part
	int16 hostEnd = fAuthority.FindFirst(':', userInfoEnd);
	userInfoEnd++;

	if (hostEnd < 0) {
		// no ':' found, the host extends to the end of the URL
		hostEnd = fAuthority.Length() + 1;
	}

	// The host is likely to be present if an authority is
	// defined, but in some weird cases, it's not.
	if (hostEnd != userInfoEnd) {
		fAuthority.CopyInto(fHost, userInfoEnd, hostEnd - userInfoEnd);
		SetHost(fHost);
	}

	// Extract the port part
	fPort = 0;
	if (fAuthority.ByteAt(hostEnd) == ':') {
		hostEnd++;
		int16 portEnd = fAuthority.Length();

		BString portString;
		fAuthority.CopyInto(portString, hostEnd, portEnd - hostEnd);
		fPort = atoi(portString.String());

		//  Even if the port is invalid, the URL is considered to
		// have a port.
		fHasPort = portString.Length() > 0;
	}
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
