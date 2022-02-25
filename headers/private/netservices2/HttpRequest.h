/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_HTTP_REQUEST_H_
#define _B_HTTP_REQUEST_H_

#include <memory>
#include <string_view>
#include <variant>

#include <ErrorsExt.h>
#include <String.h>

class BUrl;


namespace BPrivate {

namespace Network {


class BHttpMethod {
public:
	// Constants for default methods in RFC 7230 section 4.2
	enum Verb {
		Get,
		Head,
		Post,
		Put,
		Delete,
		Connect,
		Options,
		Trace
	};

	// Error type when constructing with a custom method
	class InvalidMethod : public BError {
	public:
								InvalidMethod(const char* origin, BString input);

		virtual	const char*		Message() const noexcept override;
		virtual	BString			DebugMessage() const override;

		BString					input;
	};

	// Constructors & Destructor
								BHttpMethod(Verb verb) noexcept;
								BHttpMethod(const std::string_view& method);
								BHttpMethod(const BHttpMethod& other);
								BHttpMethod(BHttpMethod&& other) noexcept;
								~BHttpMethod();

	// Assignment operators
			BHttpMethod&		operator=(const BHttpMethod& other);
			BHttpMethod&		operator=(BHttpMethod&& other) noexcept;

	// Get the method as a string
	const	std::string_view	Method() const noexcept;

private:
	std::variant<Verb, BString>	fMethod;
};


class BHttpRequest {
public:
	// Constructors and Destructor
	BHttpRequest();
	BHttpRequest(const BUrl& url);
	BHttpRequest(const BHttpRequest& other) = delete;
	BHttpRequest(BHttpRequest&& other) noexcept;
	~BHttpRequest();

	// Assignment operators
			BHttpRequest&	operator=(const BHttpRequest& other) = delete;
			BHttpRequest&	operator=(BHttpRequest&&) noexcept;

	// Access
			bool			IsEmpty() const noexcept;
	const	BHttpMethod&	Method() const noexcept;
	const	BUrl&			Url() const noexcept;

	// Named Setters
			void			SetMethod(const BHttpMethod& method);
			void			SetUrl(const BUrl& url);

private:
	struct Impl;
	std::unique_ptr<Impl>	fData;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_REQUEST_H_
