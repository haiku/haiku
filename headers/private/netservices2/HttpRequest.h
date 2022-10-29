/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_HTTP_REQUEST_H_
#define _B_HTTP_REQUEST_H_

#include <memory>
#include <optional>
#include <string_view>
#include <variant>

#include <ErrorsExt.h>
#include <String.h>

class BDataIO;
class BMallocIO;
class BUrl;


namespace BPrivate {

namespace Network {


class BHttpFields;
class HttpBuffer;
class HttpSerializer;


class BHttpMethod
{
public:
	// Constants for default methods in RFC 7230 section 4.2
	enum Verb { Get, Head, Post, Put, Delete, Connect, Options, Trace };

	// Error type when constructing with a custom method
	class InvalidMethod;

	// Constructors & Destructor
								BHttpMethod(Verb verb) noexcept;
								BHttpMethod(const std::string_view& method);
								BHttpMethod(const BHttpMethod& other);
								BHttpMethod(BHttpMethod&& other) noexcept;
								~BHttpMethod();

	// Assignment operators
			BHttpMethod&		operator=(const BHttpMethod& other);
			BHttpMethod&		operator=(BHttpMethod&& other) noexcept;

	// Comparison
			bool				operator==(const Verb& other) const noexcept;
			bool				operator!=(const Verb& other) const noexcept;

	// Get the method as a string
			const std::string_view Method() const noexcept;

private:
			std::variant<Verb, BString> fMethod;
};


class BHttpMethod::InvalidMethod : public BError
{
public:
								InvalidMethod(const char* origin, BString input);

	virtual	const char*			Message() const noexcept override;
	virtual	BString				DebugMessage() const override;

			BString				input;
};


struct BHttpAuthentication {
			BString				username;
			BString				password;
};


class BHttpRequest
{
public:
	// Aggregate parameter types
	struct Body;

	// Constructors and Destructor
								BHttpRequest();
								BHttpRequest(const BUrl& url);
								BHttpRequest(const BHttpRequest& other) = delete;
								BHttpRequest(BHttpRequest&& other) noexcept;
								~BHttpRequest();

	// Assignment operators
			BHttpRequest&		operator=(const BHttpRequest& other) = delete;
			BHttpRequest&		operator=(BHttpRequest&&) noexcept;

	// Access
			bool				IsEmpty() const noexcept;
			const BHttpAuthentication* Authentication() const noexcept;
			const BHttpFields&	Fields() const noexcept;
			uint8				MaxRedirections() const noexcept;
			const BHttpMethod&	Method() const noexcept;
			const Body*			RequestBody() const noexcept;
			bool				StopOnError() const noexcept;
			bigtime_t			Timeout() const noexcept;
			const BUrl&			Url() const noexcept;

	// Named Setters
			void				SetAuthentication(const BHttpAuthentication& authentication);
			void				SetFields(const BHttpFields& fields);
			void				SetMaxRedirections(uint8 maxRedirections);
			void				SetMethod(const BHttpMethod& method);
			void				SetRequestBody(std::unique_ptr<BDataIO> input, BString mimeType,
									std::optional<off_t> size);
			void				SetStopOnError(bool stopOnError);
			void				SetTimeout(bigtime_t timeout);
			void				SetUrl(const BUrl& url);

	// Clearing Options
			void				ClearAuthentication() noexcept;
			std::unique_ptr<BDataIO> ClearRequestBody() noexcept;

	// Serialization
			BString				HeaderToString() const;

private:
	friend class BHttpSession;
	friend class HttpSerializer;
	struct Data;

			bool				RewindBody() noexcept;
			void				SerializeHeaderTo(HttpBuffer& buffer) const;

			std::unique_ptr<Data> fData;
};


struct BHttpRequest::Body {
			std::unique_ptr<BDataIO> input;
			BString				mimeType;
			std::optional<off_t> size;
			std::optional<off_t> startPosition;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_REQUEST_H_
