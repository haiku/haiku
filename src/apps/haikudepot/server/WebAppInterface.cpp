/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2018, Andrew Lindesay <apl@lindesay.co.nz>.
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
#include "HaikuDepotConstants.h"
#include "List.h"
#include "Logger.h"
#include "PackageInfo.h"
#include "ServerSettings.h"
#include "ServerHelper.h"


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
	ProtocolListener(bool traceLogging)
		:
		fDownloadIO(NULL),
		fTraceLogging(traceLogging)
	{
	}

	virtual ~ProtocolListener()
	{
	}

	virtual	void ConnectionOpened(BUrlRequest* caller)
	{
	}

	virtual void HostnameResolved(BUrlRequest* caller, const char* ip)
	{
	}

	virtual void ResponseStarted(BUrlRequest* caller)
	{
	}

	virtual void HeadersReceived(BUrlRequest* caller, const BUrlResult& result)
	{
	}

	virtual void DataReceived(BUrlRequest* caller, const char* data,
		off_t position, ssize_t size)
	{
		if (fDownloadIO != NULL)
			fDownloadIO->Write(data, size);
	}

	virtual	void DownloadProgress(BUrlRequest* caller, ssize_t bytesReceived,
		ssize_t bytesTotal)
	{
	}

	virtual void UploadProgress(BUrlRequest* caller, ssize_t bytesSent,
		ssize_t bytesTotal)
	{
	}

	virtual void RequestCompleted(BUrlRequest* caller, bool success)
	{
	}

	virtual void DebugMessage(BUrlRequest* caller,
		BUrlProtocolDebugMessage type, const char* text)
	{
		if (fTraceLogging)
			printf("jrpc: %s\n", text);
	}

	void SetDownloadIO(BDataIO* downloadIO)
	{
		fDownloadIO = downloadIO;
	}

private:
	BDataIO*		fDownloadIO;
	bool			fTraceLogging;
};


int
WebAppInterface::fRequestIndex = 0;


enum {
	NEEDS_AUTHORIZATION = 1 << 0,
};


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


void
WebAppInterface::SetPreferredLanguage(const BString& language)
{
	fLanguage = language;
}


status_t
WebAppInterface::GetChangelog(const BString& packageName, BMessage& message)
{
	BString jsonString = JsonBuilder()
		.AddValue("jsonrpc", "2.0")
		.AddValue("id", ++fRequestIndex)
		.AddValue("method", "getPkgChangelog")
		.AddArray("params")
			.AddObject()
				.AddValue("pkgName", packageName)
			.EndObject()
		.EndArray()
	.End();

	return _SendJsonRequest("pkg", jsonString, 0, message);
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
	const BString &repositoryCode, const BString& username,
	BMessage& message)
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
				.AddValue("repositoryCode", repositoryCode)
			.EndObject()
		.EndArray()
	.End();

	return _SendJsonRequest("userrating", jsonString, 0, message);
}


status_t
WebAppInterface::CreateUserRating(const BString& packageName,
	const BString& architecture, const BString& repositoryCode,
	const BString& languageCode, const BString& comment,
	const BString& stability, int rating, BMessage& message)
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
				.AddValue("repositoryCode", repositoryCode)
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
	BUrl url = ServerSettings::CreateFullUrl(
		BString("/__pkgscreenshot/") << code << ".png" << "?tw="
			<< width << "&th=" << height);

	bool isSecure = url.Protocol() == "https";

	ProtocolListener listener(Logger::IsTraceEnabled());
	listener.SetDownloadIO(stream);

	BHttpHeaders headers;
	ServerSettings::AugmentHeaders(headers);

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
		url.UrlString().String(), statusCode);
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


status_t
WebAppInterface::_SendJsonRequest(const char* domain, BDataIO* requestData,
	size_t requestDataSize, uint32 flags, BMessage& reply) const
{
	if (!ServerHelper::IsNetworkAvailable()) {
		if (Logger::IsDebugEnabled()) {
			printf("dropping json-rpc request to ...[%s] as network is not "
				"available\n", domain);
		}
		return HD_NETWORK_INACCESSIBLE;
	}

	if (ServerSettings::IsClientTooOld()) {
		if (Logger::IsDebugEnabled()) {
			printf("dropping json-rpc request to ...[%s] as client is too "
				"old\n", domain);
		}
		return HD_CLIENT_TOO_OLD;
	}

	BUrl url = ServerSettings::CreateFullUrl(BString("/__api/v1/") << domain);
	bool isSecure = url.Protocol() == "https";

	if (Logger::IsDebugEnabled()) {
		printf("will make json-rpc request to [%s]\n",
			url.UrlString().String());
	}

	ProtocolListener listener(Logger::IsTraceEnabled());
	BUrlContext context;

	BHttpHeaders headers;
	headers.AddHeader("Content-Type", "application/json");
	ServerSettings::AugmentHeaders(headers);

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

	request.AdoptInputData(requestData, requestDataSize);

	BMallocIO replyData;
	listener.SetDownloadIO(&replyData);

	thread_id thread = request.Run();
	wait_for_thread(thread, NULL);

	const BHttpResult& result = dynamic_cast<const BHttpResult&>(
		request.Result());

	int32 statusCode = result.StatusCode();

	if (Logger::IsDebugEnabled()) {
		printf("did receive json-rpc response http status [%" B_PRId32 "] "
			"from [%s]\n", statusCode, url.UrlString().String());
	}

	switch (statusCode) {
		case B_HTTP_STATUS_OK:
			break;

		case B_HTTP_STATUS_PRECONDITION_FAILED:
			ServerHelper::NotifyClientTooOld(result.Headers());
			return HD_CLIENT_TOO_OLD;

		default:
			printf("json-rpc request to endpoint [.../%s] failed with http "
				"status [%" B_PRId32 "]\n", domain, statusCode);
			return B_ERROR;
	}

	status_t status = BJson::Parse(
		static_cast<const char *>(replyData.Buffer()), replyData.BufferLength(),
		reply);
	if (Logger::IsTraceEnabled() && status == B_BAD_DATA) {
		BString resultString(static_cast<const char *>(replyData.Buffer()),
			replyData.BufferLength());
		printf("Parser choked on JSON:\n%s\n", resultString.String());
	}
	return status;
}


status_t
WebAppInterface::_SendJsonRequest(const char* domain, BString jsonString,
	uint32 flags, BMessage& reply) const
{
	if (Logger::IsTraceEnabled())
		printf("_SendJsonRequest(%s)\n", jsonString.String());

	BMemoryIO* data = new BMemoryIO(
		jsonString.String(), jsonString.Length() - 1);

	return _SendJsonRequest(domain, data, jsonString.Length() - 1, flags,
		reply);
}

