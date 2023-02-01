/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_HTTP_RESULT_H_
#define _B_HTTP_RESULT_H_

#include <memory>
#include <optional>

#include <String.h>

class BDataIO;


namespace BPrivate {

namespace Network {

class BHttpFields;
struct HttpResultPrivate;


struct BHttpBody {
			std::optional<BString> text;
};


enum class BHttpStatusClass : int16 {
	Invalid = 000,
	Informational = 100,
	Success = 200,
	Redirection = 300,
	ClientError = 400,
	ServerError = 500
};


enum class BHttpStatusCode : int16 {
	Unknown = 0,

	// Informational status codes
	Continue = 100,
	SwitchingProtocols,

	// Success status codes
	Ok = 200,
	Created,
	Accepted,
	NonAuthoritativeInformation,
	NoContent,
	ResetContent,
	PartialContent,

	// Redirection status codes
	MultipleChoice = 300,
	MovedPermanently,
	Found,
	SeeOther,
	NotModified,
	UseProxy,
	TemporaryRedirect = 307,
	PermanentRedirect,

	// Client error status codes
	BadRequest = 400,
	Unauthorized,
	PaymentRequired,
	Forbidden,
	NotFound,
	MethodNotAllowed,
	NotAcceptable,
	ProxyAuthenticationRequired,
	RequestTimeout,
	Conflict,
	Gone,
	LengthRequired,
	PreconditionFailed,
	RequestEntityTooLarge,
	RequestUriTooLarge,
	UnsupportedMediaType,
	RequestedRangeNotSatisfiable,
	ExpectationFailed,

	// Server error status codes
	InternalServerError = 500,
	NotImplemented,
	BadGateway,
	ServiceUnavailable,
	GatewayTimeout,
};


struct BHttpStatus {
			int16				code = 0;
			BString				text;

	// Helpers
			BHttpStatusClass	StatusClass() const noexcept;
			BHttpStatusCode		StatusCode() const noexcept;
};


class BHttpResult
{
public:
	// Constructors and destructor
								BHttpResult(const BHttpResult& other) = delete;
								BHttpResult(BHttpResult&& other) noexcept;
								~BHttpResult();

	// Assignment operators
			BHttpResult&		operator=(const BHttpResult& other) = delete;
			BHttpResult&		operator=(BHttpResult&& other) noexcept;

	// Blocking Access Functions
			const BHttpStatus&	Status() const;
			const BHttpFields&	Fields() const;
			BHttpBody&			Body() const;

	// Check if data is available yet
			bool				HasStatus() const;
			bool				HasFields() const;
			bool				HasBody() const;
			bool				IsCompleted() const;

	// Identity
			int32				Identity() const;

private:
	friend class BHttpSession;
								BHttpResult(std::shared_ptr<HttpResultPrivate> data);
			std::shared_ptr<HttpResultPrivate> fData;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_RESPONSE_H_
