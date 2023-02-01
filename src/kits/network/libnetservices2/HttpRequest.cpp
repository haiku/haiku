/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <HttpRequest.h>

#include <algorithm>
#include <ctype.h>
#include <sstream>
#include <utility>

#include <DataIO.h>
#include <HttpFields.h>
#include <MimeType.h>
#include <NetServicesDefs.h>
#include <Url.h>

#include "HttpBuffer.h"
#include "HttpPrivate.h"

using namespace std::literals;
using namespace BPrivate::Network;


// #pragma mark -- BHttpMethod::InvalidMethod


BHttpMethod::InvalidMethod::InvalidMethod(const char* origin, BString input)
	:
	BError(origin),
	input(std::move(input))
{
}


const char*
BHttpMethod::InvalidMethod::Message() const noexcept
{
	if (input.IsEmpty())
		return "The HTTP method cannot be empty";
	else
		return "Unsupported characters in the HTTP method";
}


BString
BHttpMethod::InvalidMethod::DebugMessage() const
{
	BString output = BError::DebugMessage();
	if (!input.IsEmpty())
		output << ":\t " << input << "\n";
	return output;
}


// #pragma mark -- BHttpMethod


BHttpMethod::BHttpMethod(Verb verb) noexcept
	:
	fMethod(verb)
{
}


BHttpMethod::BHttpMethod(const std::string_view& verb)
	:
	fMethod(BString(verb.data(), verb.length()))
{
	if (verb.size() == 0 || !validate_http_token_string(verb))
		throw BHttpMethod::InvalidMethod(
			__PRETTY_FUNCTION__, std::move(std::get<BString>(fMethod)));
}


BHttpMethod::BHttpMethod(const BHttpMethod& other) = default;


BHttpMethod::BHttpMethod(BHttpMethod&& other) noexcept
	:
	fMethod(std::move(other.fMethod))
{
	other.fMethod = Get;
}


BHttpMethod::~BHttpMethod() = default;


BHttpMethod& BHttpMethod::operator=(const BHttpMethod& other) = default;


BHttpMethod&
BHttpMethod::operator=(BHttpMethod&& other) noexcept
{
	fMethod = std::move(other.fMethod);
	other.fMethod = Get;
	return *this;
}


bool
BHttpMethod::operator==(const BHttpMethod::Verb& other) const noexcept
{
	if (std::holds_alternative<Verb>(fMethod)) {
		return std::get<Verb>(fMethod) == other;
	} else {
		BHttpMethod otherMethod(other);
		auto otherMethodSv = otherMethod.Method();
		return std::get<BString>(fMethod).Compare(otherMethodSv.data(), otherMethodSv.size()) == 0;
	}
}


bool
BHttpMethod::operator!=(const BHttpMethod::Verb& other) const noexcept
{
	return !operator==(other);
}


const std::string_view
BHttpMethod::Method() const noexcept
{
	if (std::holds_alternative<Verb>(fMethod)) {
		switch (std::get<Verb>(fMethod)) {
			case Get:
				return "GET"sv;
			case Head:
				return "HEAD"sv;
			case Post:
				return "POST"sv;
			case Put:
				return "PUT"sv;
			case Delete:
				return "DELETE"sv;
			case Connect:
				return "CONNECT"sv;
			case Options:
				return "OPTIONS"sv;
			case Trace:
				return "TRACE"sv;
			default:
				// should never be reached
				std::abort();
		}
	} else {
		const auto& methodString = std::get<BString>(fMethod);
		// the following constructor is not noexcept, but we know we pass in valid data
		return std::string_view(methodString.String());
	}
}


// #pragma mark -- BHttpRequest::Data
static const BUrl kDefaultUrl = BUrl();
static const BHttpMethod kDefaultMethod = BHttpMethod::Get;
static const BHttpFields kDefaultOptionalFields = BHttpFields();

struct BHttpRequest::Data {
	BUrl url = kDefaultUrl;
	BHttpMethod method = kDefaultMethod;
	uint8 maxRedirections = 8;
	BHttpFields optionalFields;
	std::optional<BHttpAuthentication> authentication;
	bool stopOnError = false;
	bigtime_t timeout = B_INFINITE_TIMEOUT;
	std::optional<Body> requestBody;
};


// #pragma mark -- BHttpRequest helper functions


/*!
	\brief Build basic authentication header
*/
static inline BString
build_basic_http_header(const BString& username, const BString& password)
{
	BString basicEncode, result;
	basicEncode << username << ":" << password;
	result << "Basic " << encode_to_base64(basicEncode);
	return result;
}


// #pragma mark -- BHttpRequest


BHttpRequest::BHttpRequest()
	:
	fData(std::make_unique<Data>())
{
}


BHttpRequest::BHttpRequest(const BUrl& url)
	:
	fData(std::make_unique<Data>())
{
	SetUrl(url);
}


BHttpRequest::BHttpRequest(BHttpRequest&& other) noexcept = default;


BHttpRequest::~BHttpRequest() = default;


BHttpRequest& BHttpRequest::operator=(BHttpRequest&&) noexcept = default;


bool
BHttpRequest::IsEmpty() const noexcept
{
	return (!fData || !fData->url.IsValid());
}


const BHttpAuthentication*
BHttpRequest::Authentication() const noexcept
{
	if (fData && fData->authentication)
		return std::addressof(*fData->authentication);
	return nullptr;
}


const BHttpFields&
BHttpRequest::Fields() const noexcept
{
	if (!fData)
		return kDefaultOptionalFields;
	return fData->optionalFields;
}


uint8
BHttpRequest::MaxRedirections() const noexcept
{
	if (!fData)
		return 8;
	return fData->maxRedirections;
}


const BHttpMethod&
BHttpRequest::Method() const noexcept
{
	if (!fData)
		return kDefaultMethod;
	return fData->method;
}


const BHttpRequest::Body*
BHttpRequest::RequestBody() const noexcept
{
	if (fData && fData->requestBody)
		return std::addressof(*fData->requestBody);
	return nullptr;
}


bool
BHttpRequest::StopOnError() const noexcept
{
	if (!fData)
		return false;
	return fData->stopOnError;
}


bigtime_t
BHttpRequest::Timeout() const noexcept
{
	if (!fData)
		return B_INFINITE_TIMEOUT;
	return fData->timeout;
}


const BUrl&
BHttpRequest::Url() const noexcept
{
	if (!fData)
		return kDefaultUrl;
	return fData->url;
}


void
BHttpRequest::SetAuthentication(const BHttpAuthentication& authentication)
{
	if (!fData)
		fData = std::make_unique<Data>();

	fData->authentication = authentication;
}


static constexpr std::array<std::string_view, 6> fReservedOptionalFieldNames
	= {"Host"sv, "Accept-Encoding"sv, "Connection"sv, "Content-Type"sv, "Content-Length"sv};


void
BHttpRequest::SetFields(const BHttpFields& fields)
{
	if (!fData)
		fData = std::make_unique<Data>();

	for (auto& field: fields) {
		if (std::find(fReservedOptionalFieldNames.begin(), fReservedOptionalFieldNames.end(),
				field.Name())
			!= fReservedOptionalFieldNames.end()) {
			std::string_view fieldName = field.Name();
			throw BHttpFields::InvalidInput(
				__PRETTY_FUNCTION__, BString(fieldName.data(), fieldName.size()));
		}
	}
	fData->optionalFields = fields;
}


void
BHttpRequest::SetMaxRedirections(uint8 maxRedirections)
{
	if (!fData)
		fData = std::make_unique<Data>();
	fData->maxRedirections = maxRedirections;
}


void
BHttpRequest::SetMethod(const BHttpMethod& method)
{
	if (!fData)
		fData = std::make_unique<Data>();
	fData->method = method;
}


void
BHttpRequest::SetRequestBody(
	std::unique_ptr<BDataIO> input, BString mimeType, std::optional<off_t> size)
{
	if (input == nullptr)
		throw std::invalid_argument("input cannot be null");

	// TODO: support optional mimetype arguments like type/subtype;parameter=value
	if (!BMimeType::IsValid(mimeType.String()))
		throw std::invalid_argument("mimeType must be a valid mimetype");

	// TODO: review if there should be complex validation between the method and whether or not
	// there is a request body. The current implementation does the validation at the request
	// generation stage, where GET, HEAD, OPTIONS, CONNECT and TRACE will not submit a body.

	if (!fData)
		fData = std::make_unique<Data>();
	fData->requestBody = {std::move(input), std::move(mimeType), size};

	// Check if the input is a BPositionIO, and if so, store the current position, so that it can
	// be rewinded in case of a redirect.
	auto inputPositionIO = dynamic_cast<BPositionIO*>(fData->requestBody->input.get());
	if (inputPositionIO != nullptr)
		fData->requestBody->startPosition = inputPositionIO->Position();
}


void
BHttpRequest::SetStopOnError(bool stopOnError)
{
	if (!fData)
		fData = std::make_unique<Data>();
	fData->stopOnError = stopOnError;
}


void
BHttpRequest::SetTimeout(bigtime_t timeout)
{
	if (!fData)
		fData = std::make_unique<Data>();
	fData->timeout = timeout;
}


void
BHttpRequest::SetUrl(const BUrl& url)
{
	if (!fData)
		fData = std::make_unique<Data>();

	if (!url.IsValid())
		throw BInvalidUrl(__PRETTY_FUNCTION__, BUrl(url));
	if (url.Protocol() != "http" && url.Protocol() != "https") {
		// TODO: optimize BStringList with modern language features
		BStringList list;
		list.Add("http");
		list.Add("https");
		throw BUnsupportedProtocol(__PRETTY_FUNCTION__, BUrl(url), list);
	}
	fData->url = url;
}


void
BHttpRequest::ClearAuthentication() noexcept
{
	if (fData)
		fData->authentication = std::nullopt;
}


std::unique_ptr<BDataIO>
BHttpRequest::ClearRequestBody() noexcept
{
	if (fData && fData->requestBody) {
		auto body = std::move(fData->requestBody->input);
		fData->requestBody = std::nullopt;
		return body;
	}
	return nullptr;
}


BString
BHttpRequest::HeaderToString() const
{
	HttpBuffer buffer;
	SerializeHeaderTo(buffer);

	return BString(static_cast<const char*>(buffer.Data().data()), buffer.RemainingBytes());
}


/*!
	\brief Private method used by BHttpSession::Request to rewind the content in case of redirect

	\retval true Content was rewinded successfully. Also the case if there is no content
	\retval false Cannot/could not rewind content.
*/
bool
BHttpRequest::RewindBody() noexcept
{
	if (fData && fData->requestBody && fData->requestBody->startPosition) {
		auto inputData = dynamic_cast<BPositionIO*>(fData->requestBody->input.get());
		return *fData->requestBody->startPosition
			== inputData->Seek(*fData->requestBody->startPosition, SEEK_SET);
	}
	return true;
}


/*!
	\brief Private method used by HttpSerializer::SetTo() to serialize the header data into a
		buffer.
*/
void
BHttpRequest::SerializeHeaderTo(HttpBuffer& buffer) const
{
	// Method & URL
	//	TODO: proxy
	buffer << fData->method.Method() << " "sv;
	if (fData->url.HasPath() && fData->url.Path().Length() > 0)
		buffer << std::string_view(fData->url.Path().String());
	else
		buffer << "/"sv;

	if (fData->url.HasRequest())
		buffer << "?"sv << Url().Request().String();

	// TODO: switch between HTTP 1.0 and 1.1 based on configuration
	buffer << " HTTP/1.1\r\n"sv;

	BHttpFields outputFields;
	if (true /* http == 1.1 */) {
		BString host = fData->url.Host();
		int defaultPort = fData->url.Protocol() == "http" ? 80 : 443;
		if (fData->url.HasPort() && fData->url.Port() != defaultPort)
			host << ':' << fData->url.Port();

		outputFields.AddFields({
			{"Host"sv, std::string_view(host.String())}, {"Accept-Encoding"sv, "gzip"sv},
			// Allows the server to compress data using the "gzip" format.
			// "deflate" is not supported, because there are two interpretations
			// of what it means (the RFC and Microsoft products), and we don't
			// want to handle this. Very few websites support only deflate,
			// and most of them will send gzip, or at worst, uncompressed data.
			{"Connection"sv, "close"sv}
			// Let the remote server close the connection after response since
			// we don't handle multiple request on a single connection
		});
	}

	if (fData->authentication) {
		// This request will add a Basic authorization header
		BString authorization = build_basic_http_header(
			fData->authentication->username, fData->authentication->password);
		outputFields.AddField("Authorization"sv, std::string_view(authorization.String()));
	}

	if (fData->requestBody) {
		outputFields.AddField(
			"Content-Type"sv, std::string_view(fData->requestBody->mimeType.String()));
		if (fData->requestBody->size)
			outputFields.AddField("Content-Length"sv, std::to_string(*fData->requestBody->size));
		else
			throw BRuntimeError(__PRETTY_FUNCTION__,
				"Transfer body with unknown content length; chunked transfer not supported");
	}

	for (const auto& field: outputFields)
		buffer << field.RawField() << "\r\n"sv;

	for (const auto& field: fData->optionalFields)
		buffer << field.RawField() << "\r\n"sv;

	buffer << "\r\n"sv;
}
