/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "WebAppInterface.h"

#include <stdio.h>

#include <AppFileInfo.h>
#include <Application.h>
#include <Autolock.h>
#include <File.h>
#include <HttpHeaders.h>
#include <HttpRequest.h>
#include <Json.h>
#include <Message.h>
#include <Roster.h>
#include <Url.h>
#include <UrlContext.h>
#include <UrlProtocolListener.h>
#include <UrlProtocolRoster.h>

#include "AutoLocker.h"
#include "List.h"
#include "PackageInfo.h"


#define CODE_REPOSITORY_DEFAULT "haikuports"
#define BASEURL_DEFAULT "https://depot.haiku-os.org"
#define USERAGENT_FALLBACK_VERSION "0.0.0"


class JsonBuilder {
public:
	JsonBuilder()
		:
		fString("{"),
		fInList(false)
	{
	}

	JsonBuilder& AddObject()
	{
		fString << '{';
		fInList = false;
		return *this;
	}

	JsonBuilder& AddObject(const char* name)
	{
		_StartName(name);
		fString << '{';
		fInList = false;
		return *this;
	}

	JsonBuilder& EndObject()
	{
		fString << '}';
		fInList = true;
		return *this;
	}

	JsonBuilder& AddArray(const char* name)
	{
		_StartName(name);
		fString << '[';
		fInList = false;
		return *this;
	}

	JsonBuilder& EndArray()
	{
		fString << ']';
		fInList = true;
		return *this;
	}

	JsonBuilder& AddStrings(const StringList& strings)
	{
		for (int i = 0; i < strings.CountItems(); i++)
			AddItem(strings.ItemAtFast(i));
		return *this;
	}

	JsonBuilder& AddItem(const char* item)
	{
		return AddItem(item, false);
	}

	JsonBuilder& AddItem(const char* item, bool nullIfEmpty)
	{
		if (item == NULL || (nullIfEmpty && strlen(item) == 0)) {
			if (fInList)
				fString << ",null";
			else
				fString << "null";
		} else {
			if (fInList)
				fString << ",\"";
			else
				fString << '"';
			fString << _EscapeString(item);
			fString << '"';
		}
		fInList = true;
		return *this;
	}

	JsonBuilder& AddValue(const char* name, const char* value)
	{
		return AddValue(name, value, false);
	}

	JsonBuilder& AddValue(const char* name, const char* value,
		bool nullIfEmpty)
	{
		_StartName(name);
		if (value == NULL || (nullIfEmpty && strlen(value) == 0)) {
			fString << "null";
		} else {
			fString << '"';
			fString << _EscapeString(value);
			fString << '"';
		}
		fInList = true;
		return *this;
	}

	JsonBuilder& AddValue(const char* name, int value)
	{
		_StartName(name);
		fString << value;
		fInList = true;
		return *this;
	}

	JsonBuilder& AddValue(const char* name, bool value)
	{
		_StartName(name);
		if (value)
			fString << "true";
		else
			fString << "false";
		fInList = true;
		return *this;
	}

	const BString& End()
	{
		fString << "}\n";
		return fString;
	}

private:
	void _StartName(const char* name)
	{
		if (fInList)
			fString << ",\"";
		else
			fString << '"';
		fString << _EscapeString(name);
		fString << "\":";
	}

	BString _EscapeString(const char* original) const
	{
		BString string(original);
		string.ReplaceAll("\\", "\\\\");
		string.ReplaceAll("\"", "\\\"");
		string.ReplaceAll("/", "\\/");
		string.ReplaceAll("\b", "\\b");
		string.ReplaceAll("\f", "\\f");
		string.ReplaceAll("\n", "\\n");
		string.ReplaceAll("\r", "\\r");
		string.ReplaceAll("\t", "\\t");
		return string;
	}

private:
	BString		fString;
	bool		fInList;
};


class ProtocolListener : public BUrlProtocolListener {
public:
	ProtocolListener()
		:
		fDownloadIO(NULL),
		fDebug(false)
	{
	}

	virtual	void ConnectionOpened(BUrlRequest* caller)
	{
//		printf("ConnectionOpened(%p)\n", caller);
	}

	virtual void HostnameResolved(BUrlRequest* caller, const char* ip)
	{
//		printf("HostnameResolved(%p): %s\n", caller, ip);
	}

	virtual void ResponseStarted(BUrlRequest* caller)
	{
		if (fDebug)
			printf("ResponseStarted(%p)\n", caller);
	}

	virtual void HeadersReceived(BUrlRequest* caller)
	{
		if (fDebug)
			printf("HeadersReceived(%p)\n", caller);
	}

	virtual void DataReceived(BUrlRequest* caller, const char* data,
		off_t position, ssize_t size)
	{
		if (fDebug) {
			printf("DataReceived(%p): %ld bytes\n", caller, size);
			printf("%.*s", (int)size, data);
		}

		if (fDownloadIO != NULL)
			fDownloadIO->Write(data, size);
	}

	virtual	void DownloadProgress(BUrlRequest* caller, ssize_t bytesReceived,
		ssize_t bytesTotal)
	{
//		printf("DownloadProgress(%p): %ld/%ld\n", caller, bytesReceived,
//			bytesTotal);
	}

	virtual void UploadProgress(BUrlRequest* caller, ssize_t bytesSent,
		ssize_t bytesTotal)
	{
		if (fDebug)
			printf("UploadProgress(%p): %ld/%ld\n", caller, bytesSent, bytesTotal);
	}

	virtual void RequestCompleted(BUrlRequest* caller, bool success)
	{
		if (fDebug)
			printf("RequestCompleted(%p): %d\n", caller, success);
	}

	virtual void DebugMessage(BUrlRequest* caller,
		BUrlProtocolDebugMessage type, const char* text)
	{
//		printf("DebugMessage(%p): %s\n", caller, text);
	}

	void SetDownloadIO(BDataIO* downloadIO)
	{
		fDownloadIO = downloadIO;
	}

	void SetDebug(bool debug)
	{
		fDebug = debug;
	}

private:
	BDataIO*		fDownloadIO;
	bool			fDebug;
};


int
WebAppInterface::fRequestIndex = 0;


enum {
	NEEDS_AUTHORIZATION = 1 << 0,
	ENABLE_DEBUG		= 1 << 1,
};


BString WebAppInterface::fBaseUrl = BString(BASEURL_DEFAULT);
BString WebAppInterface::fUserAgent = BString();
BLocker WebAppInterface::fUserAgentLocker = BLocker();

WebAppInterface::WebAppInterface()
	:
	fLanguage("en")
{
}


WebAppInterface::WebAppInterface(const WebAppInterface& other)
	:
	fUsername(other.fUsername),
	fPassword(other.fPassword),
	fLanguage(other.fLanguage)
{
}


WebAppInterface::~WebAppInterface()
{
}


WebAppInterface&
WebAppInterface::operator=(const WebAppInterface& other)
{
	if (this == &other)
		return *this;

	fUsername = other.fUsername;
	fPassword = other.fPassword;
	fLanguage = other.fLanguage;

	return *this;
}


void
WebAppInterface::SetAuthorization(const BString& username,
	const BString& password)
{
	fUsername = username;
	fPassword = password;
}


static bool
arguments_is_url_valid(const BString& value)
{
	if (value.Length() < 8) {
		fprintf(stderr, "the url is less than 8 characters in length\n");
		return false;
	}

	int32 schemeEnd = value.FindFirst("://");

	if (schemeEnd < B_OK) {
		fprintf(stderr, "the url does not contain the '://' string\n");
		return false;
	}

	BString scheme;
	value.CopyInto(scheme, 0, schemeEnd);

	if (scheme != "http" && scheme != "https") {
		fprintf(stderr, "the url scheme should be 'http' or 'https'\n");
		return false;
	}

	if (value.Length() - 1 == value.FindLast("/")) {
		fprintf(stderr, "the url should be be terminated with a '/'\n");
		return false;
	}

	return true;
}


/*! This method will set the web app base URL, returning a status to
    indicate if the URL was acceptable.
    \return B_OK if the base URL was valid and B_BAD_VALUE if not.
 */
status_t
WebAppInterface::SetBaseUrl(const BString& url)
{
	if (!arguments_is_url_valid(url))
		return B_BAD_VALUE;

	fBaseUrl.SetTo(url);

	return B_OK;
}


const BString
WebAppInterface::_GetUserAgentVersionString()
{
	app_info info;

	if (be_app->GetAppInfo(&info) != B_OK) {
		fprintf(stderr, "Unable to get the application info\n");
		be_app->Quit();
		return BString(USERAGENT_FALLBACK_VERSION);
	}

	BFile file(&info.ref, B_READ_ONLY);

	if (file.InitCheck() != B_OK) {
		fprintf(stderr, "Unable to access the application info file\n");
		be_app->Quit();
		return BString(USERAGENT_FALLBACK_VERSION);
	}

	BAppFileInfo appFileInfo(&file);
	version_info versionInfo;

	if (appFileInfo.GetVersionInfo(
		&versionInfo, B_APP_VERSION_KIND) != B_OK) {
		fprintf(stderr, "Unable to establish the application version\n");
		be_app->Quit();
		return BString(USERAGENT_FALLBACK_VERSION);
	}

	BString result;
	result.SetToFormat("%" B_PRId32 ".%" B_PRId32 ".%" B_PRId32,
		versionInfo.major, versionInfo.middle, versionInfo.minor);
	return result;
}


/*! This method will devise a suitable User-Agent header value that
	can be transmitted with HTTP requests to the server in order
	to identify this client.
 */
const BString
WebAppInterface::_GetUserAgent()
{
	AutoLocker<BLocker> lock(&fUserAgentLocker);

	if (fUserAgent.IsEmpty()) {
		fUserAgent.SetTo("HaikuDepot/");
		fUserAgent.Append(_GetUserAgentVersionString());
	}

	return fUserAgent;
}


void
WebAppInterface::SetPreferredLanguage(const BString& language)
{
	fLanguage = language;
}


status_t
WebAppInterface::RetrievePackageInfo(const BString& packageName,
	const BString& architecture, BMessage& message)
{
	BString jsonString = JsonBuilder()
		.AddValue("jsonrpc", "2.0")
		.AddValue("id", ++fRequestIndex)
		.AddValue("method", "getPkg")
		.AddArray("params")
			.AddObject()
				.AddValue("name", packageName)
				.AddValue("architectureCode", architecture)
				.AddValue("naturalLanguageCode", fLanguage)
				.AddValue("repositoryCode", CODE_REPOSITORY_DEFAULT)
				.AddValue("versionType", "NONE")
			.EndObject()
		.EndArray()
	.End();

	return _SendJsonRequest("pkg", jsonString, 0, message);
}


status_t
WebAppInterface::RetrieveBulkPackageInfo(const StringList& packageNames,
	const StringList& packageArchitectures, BMessage& message)
{
	BString jsonString = JsonBuilder()
		.AddValue("jsonrpc", "2.0")
		.AddValue("id", ++fRequestIndex)
		.AddValue("method", "getBulkPkg")
		.AddArray("params")
			.AddObject()
				.AddArray("pkgNames")
					.AddStrings(packageNames)
				.EndArray()
				.AddArray("architectureCodes")
					.AddStrings(packageArchitectures)
				.EndArray()
				.AddArray("repositoryCodes")
					.AddItem(CODE_REPOSITORY_DEFAULT)
				.EndArray()
				.AddValue("naturalLanguageCode", fLanguage)
				.AddValue("versionType", "LATEST")
				.AddArray("filter")
					.AddItem("PKGCATEGORIES")
					.AddItem("PKGSCREENSHOTS")
					.AddItem("PKGICONS")
					.AddItem("PKGVERSIONLOCALIZATIONDESCRIPTIONS")
					.AddItem("PKGCHANGELOG")
				.EndArray()
			.EndObject()
		.EndArray()
	.End();

	return _SendJsonRequest("pkg", jsonString, 0, message);
}


status_t
WebAppInterface::RetrievePackageIcon(const BString& packageName,
	BDataIO* stream)
{
	BString urlString = _FormFullUrl(BString("/pkgicon/") << packageName
		<< ".hvif");
	bool isSecure = 0 == urlString.FindFirst("https://");

	BUrl url(urlString);

	ProtocolListener listener;
	listener.SetDownloadIO(stream);

	BHttpRequest request(url, isSecure, "HTTP", &listener);
	request.SetMethod(B_HTTP_GET);

	thread_id thread = request.Run();
	wait_for_thread(thread, NULL);

	const BHttpResult& result = dynamic_cast<const BHttpResult&>(
		request.Result());

	int32 statusCode = result.StatusCode();

	if (statusCode == 200)
		return B_OK;

	return B_ERROR;
}


status_t
WebAppInterface::RetrieveUserRatings(const BString& packageName,
	const BString& architecture, int resultOffset, int maxResults,
	BMessage& message)
{
	BString jsonString = JsonBuilder()
		.AddValue("jsonrpc", "2.0")
		.AddValue("id", ++fRequestIndex)
		.AddValue("method", "searchUserRatings")
		.AddArray("params")
			.AddObject()
				.AddValue("pkgName", packageName)
				.AddValue("pkgVersionArchitectureCode", architecture)
				.AddValue("offset", resultOffset)
				.AddValue("limit", maxResults)
			.EndObject()
		.EndArray()
	.End();

	return _SendJsonRequest("userrating", jsonString, 0, message);
}


status_t
WebAppInterface::RetrieveUserRating(const BString& packageName,
	const BPackageVersion& version, const BString& architecture,
	const BString& username, BMessage& message)
{
	BString jsonString = JsonBuilder()
		.AddValue("jsonrpc", "2.0")
		.AddValue("id", ++fRequestIndex)
		.AddValue("method", "getUserRatingByUserAndPkgVersion")
		.AddArray("params")
			.AddObject()
				.AddValue("userNickname", username)
				.AddValue("pkgName", packageName)
				.AddValue("pkgVersionArchitectureCode", architecture)
				.AddValue("pkgVersionMajor", version.Major(), true)
				.AddValue("pkgVersionMinor", version.Minor(), true)
				.AddValue("pkgVersionMicro", version.Micro(), true)
				.AddValue("pkgVersionPreRelease", version.PreRelease(), true)
				.AddValue("pkgVersionRevision", (int)version.Revision())
				.AddValue("repositoryCode", CODE_REPOSITORY_DEFAULT)
			.EndObject()
		.EndArray()
	.End();

	return _SendJsonRequest("userrating", jsonString, 0, message);
}


status_t
WebAppInterface::CreateUserRating(const BString& packageName,
	const BString& architecture, const BString& languageCode,
	const BString& comment, const BString& stability, int rating,
	BMessage& message)
{
	BString jsonString = JsonBuilder()
		.AddValue("jsonrpc", "2.0")
		.AddValue("id", ++fRequestIndex)
		.AddValue("method", "createUserRating")
		.AddArray("params")
			.AddObject()
				.AddValue("pkgName", packageName)
				.AddValue("pkgVersionArchitectureCode", architecture)
				.AddValue("pkgVersionType", "LATEST")
				.AddValue("userNickname", fUsername)
				.AddValue("rating", rating)
				.AddValue("userRatingStabilityCode", stability, true)
				.AddValue("comment", comment)
				.AddValue("repositoryCode", CODE_REPOSITORY_DEFAULT)
				.AddValue("naturalLanguageCode", languageCode)
			.EndObject()
		.EndArray()
	.End();

	return _SendJsonRequest("userrating", jsonString, NEEDS_AUTHORIZATION,
		message);
}


status_t
WebAppInterface::UpdateUserRating(const BString& ratingID,
	const BString& languageCode, const BString& comment,
	const BString& stability, int rating, bool active, BMessage& message)
{
	BString jsonString = JsonBuilder()
		.AddValue("jsonrpc", "2.0")
		.AddValue("id", ++fRequestIndex)
		.AddValue("method", "updateUserRating")
		.AddArray("params")
			.AddObject()
				.AddValue("code", ratingID)
				.AddValue("rating", rating)
				.AddValue("userRatingStabilityCode", stability, true)
				.AddValue("comment", comment)
				.AddValue("naturalLanguageCode", languageCode)
				.AddValue("active", active)
				.AddArray("filter")
					.AddItem("ACTIVE")
					.AddItem("NATURALLANGUAGE")
					.AddItem("USERRATINGSTABILITY")
					.AddItem("COMMENT")
					.AddItem("RATING")
				.EndArray()
			.EndObject()
		.EndArray()
	.End();

	return _SendJsonRequest("userrating", jsonString, NEEDS_AUTHORIZATION,
		message);
}


status_t
WebAppInterface::RetrieveScreenshot(const BString& code,
	int32 width, int32 height, BDataIO* stream)
{
	BString urlString = _FormFullUrl(BString("/pkgscreenshot/") << code
		<< ".png" << "?tw=" << width << "&th=" << height);
	bool isSecure = 0 == urlString.FindFirst("https://");

	BUrl url(urlString);

	ProtocolListener listener;
	listener.SetDownloadIO(stream);

	BHttpHeaders headers;
	headers.AddHeader("User-Agent", _GetUserAgent());

	BHttpRequest request(url, isSecure, "HTTP", &listener);
	request.SetMethod(B_HTTP_GET);
	request.SetHeaders(headers);

	thread_id thread = request.Run();
	wait_for_thread(thread, NULL);

	const BHttpResult& result = dynamic_cast<const BHttpResult&>(
		request.Result());

	int32 statusCode = result.StatusCode();

	if (statusCode == 200)
		return B_OK;

	fprintf(stderr, "failed to get screenshot from '%s': %" B_PRIi32 "\n",
		urlString.String(), statusCode);
	return B_ERROR;
}


status_t
WebAppInterface::RequestCaptcha(BMessage& message)
{
	BString jsonString = JsonBuilder()
		.AddValue("jsonrpc", "2.0")
		.AddValue("id", ++fRequestIndex)
		.AddValue("method", "generateCaptcha")
		.AddArray("params")
			.AddObject()
			.EndObject()
		.EndArray()
	.End();

	return _SendJsonRequest("captcha", jsonString, 0, message);
}


status_t
WebAppInterface::CreateUser(const BString& nickName,
	const BString& passwordClear, const BString& email,
	const BString& captchaToken, const BString& captchaResponse,
	const BString& languageCode, BMessage& message)
{
	JsonBuilder builder;
	builder
		.AddValue("jsonrpc", "2.0")
		.AddValue("id", ++fRequestIndex)
		.AddValue("method", "createUser")
		.AddArray("params")
			.AddObject()
				.AddValue("nickname", nickName)
				.AddValue("passwordClear", passwordClear);

				if (!email.IsEmpty())
					builder.AddValue("email", email);

				builder.AddValue("captchaToken", captchaToken)
				.AddValue("captchaResponse", captchaResponse)
				.AddValue("naturalLanguageCode", languageCode)
			.EndObject()
		.EndArray()
	;

	BString jsonString = builder.End();

	return _SendJsonRequest("user", jsonString, 0, message);
}


status_t
WebAppInterface::AuthenticateUser(const BString& nickName,
	const BString& passwordClear, BMessage& message)
{
	BString jsonString = JsonBuilder()
		.AddValue("jsonrpc", "2.0")
		.AddValue("id", ++fRequestIndex)
		.AddValue("method", "authenticateUser")
		.AddArray("params")
			.AddObject()
				.AddValue("nickname", nickName)
				.AddValue("passwordClear", passwordClear)
			.EndObject()
		.EndArray()
	.End();

	return _SendJsonRequest("user", jsonString, 0, message);
}


// #pragma mark - private


BString
WebAppInterface::_FormFullUrl(const BString& suffix) const
{
	if (fBaseUrl.IsEmpty()) {
		fprintf(stderr, "illegal state - missing web app base url\n");
		exit(EXIT_FAILURE);
	}

	return BString(fBaseUrl) << suffix;
}


status_t
WebAppInterface::_SendJsonRequest(const char* domain, BString jsonString,
	uint32 flags, BMessage& reply) const
{
	if ((flags & ENABLE_DEBUG) != 0)
		printf("_SendJsonRequest(%s)\n", jsonString.String());

	BString urlString = _FormFullUrl(BString("/api/v1/") << domain);
	bool isSecure = 0 == urlString.FindFirst("https://");
	BUrl url(urlString);

	ProtocolListener listener;
	BUrlContext context;

	BHttpHeaders headers;
	headers.AddHeader("Content-Type", "application/json");
	headers.AddHeader("User-Agent", _GetUserAgent());

	BHttpRequest request(url, isSecure, "HTTP", &listener, &context);
	request.SetMethod(B_HTTP_POST);
	request.SetHeaders(headers);

	// Authentication via Basic Authentication
	// The other way would be to obtain a token and then use the Token Bearer
	// header.
	if ((flags & NEEDS_AUTHORIZATION) != 0
		&& !fUsername.IsEmpty() && !fPassword.IsEmpty()) {
		BHttpAuthentication authentication(fUsername, fPassword);
		authentication.SetMethod(B_HTTP_AUTHENTICATION_BASIC);
		context.AddAuthentication(url, authentication);
	}

	BMemoryIO* data = new BMemoryIO(
		jsonString.String(), jsonString.Length() - 1);

	request.AdoptInputData(data, jsonString.Length() - 1);

	BMallocIO replyData;
	listener.SetDownloadIO(&replyData);
	listener.SetDebug((flags & ENABLE_DEBUG) != 0);

	thread_id thread = request.Run();
	wait_for_thread(thread, NULL);

	const BHttpResult& result = dynamic_cast<const BHttpResult&>(
		request.Result());

	int32 statusCode = result.StatusCode();
	if (statusCode != 200) {
		printf("Response code: %" B_PRId32 "\n", statusCode);
		return B_ERROR;
	}

	jsonString.SetTo(static_cast<const char*>(replyData.Buffer()),
		replyData.BufferLength());
	if (jsonString.Length() == 0)
		return B_ERROR;

	BJson parser;
	status_t status = parser.Parse(reply, jsonString);
	if ((flags & ENABLE_DEBUG) != 0 && status == B_BAD_DATA) {
		printf("Parser choked on JSON:\n%s\n", jsonString.String());
	}
	return status;
}
