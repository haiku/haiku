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
#include <utility>

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
	: fMethod(verb)
{

}


BHttpMethod::BHttpMethod(const std::string_view& verb)
	: fMethod(BString(verb.data(), verb.length()))
{
	if (verb.size() == 0 || !validate_http_token_string(verb))
		throw BHttpMethod::InvalidMethod(__PRETTY_FUNCTION__, std::move(std::get<BString>(fMethod)));
}


BHttpMethod::BHttpMethod(const BHttpMethod& other) = default;


BHttpMethod::BHttpMethod(BHttpMethod&& other) noexcept
	: fMethod(std::move(other.fMethod))
{
	other.fMethod = Get;
}


BHttpMethod::~BHttpMethod() = default;


BHttpMethod&
BHttpMethod::operator=(const BHttpMethod& other) = default;


BHttpMethod&
BHttpMethod::operator=(BHttpMethod&& other) noexcept
{
	fMethod = std::move(other.fMethod);
	other.fMethod = Get;
	return *this;
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
