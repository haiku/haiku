/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <HttpHeaders.h>

#include <ctype.h>
#include <utility>

using namespace BPrivate::Network;


// #pragma mark -- utilities


/*!
	\brief Validate whether the string conforms to a HTTP token value

	RFC 7230 section 3.2.6 determines that valid tokens for the header name are:
	!#$%&'*+=.^_`|~, any digits or alpha.

	\returns \c true if the string is valid, or \c false if it is not.
*/
static inline bool
validate_token_string(std::string_view string)
{
	for (auto it = string.cbegin(); it < string.cend(); it++) {
		if (*it <= 31 || *it == 127 || *it == '(' || *it == ')' || *it == '<' || *it == '>'
				|| *it == '@' || *it == ',' || *it == ';' || *it == '\\' || *it == '"'
				|| *it == '/' || *it == '[' || *it == ']' || *it == '?' || *it == '='
				|| *it == '{' || *it == '}' || *it == ' ')
			return false;
	}
	return true;
}


/*!
	\brief Validate whether the string is a valid HTTP header value

	RFC 7230 section 3.2.6 determines that valid tokens for the header are:
	HTAB ('\t'), SP (32), all visible ASCII characters (33-126), and all characters that
	not control characters (in the case of a char, any value < 0)

	\note When printing out the HTTP header, sometimes the string needs to be quoted and some
		characters need to be escaped. This function is not checking for whether the string can
		be transmitted as is.

	\returns \c true if the string is valid, or \c false if it is not.
*/
static inline bool
validate_value_string(std::string_view string)
{
	for (auto it = string.cbegin(); it < string.cend(); it++) {
		if ((*it >= 0 && *it < 32) || *it == 127 || *it == '\t')
			return false;
	}
	return true;
}


// #pragma mark -- BHttpHeader::InvalidHeader


BHttpHeader::InvalidInput::InvalidInput(const char* origin, BString input)
	:
	BError(origin),
	input(std::move(input))
{

}


const char*
BHttpHeader::InvalidInput::Message() const noexcept
{
	return "Invalid format or unsupported characters in input";
}


BString
BHttpHeader::InvalidInput::DebugMessage() const
{
	BString output = BError::DebugMessage();
	output << "\t " << input << "\n";
	return output;
}


// #pragma mark -- BHttpHeader::EmptyHeader


BHttpHeader::EmptyHeader::EmptyHeader(const char* origin)
	:
	BError(origin)
{

}


const char*
BHttpHeader::EmptyHeader::Message() const noexcept
{
	return "Cannot convert this object into a HTTP header string: the name or value is empty";
}


// #pragma mark -- BHttpHeader::HeaderName


BHttpHeader::HeaderName::HeaderName(std::string_view name)
{
	if (name.empty()) {
		// ignore an empty name
	} else if (!validate_token_string(name)) {
		throw InvalidInput(__PRETTY_FUNCTION__, BString(name.data(), name.size()));
	} else {
		fName.SetTo(name.data(), name.length());
		fName.CapitalizeEachWord();
	}
}


bool
BHttpHeader::HeaderName::operator==(const BString& other) const noexcept
{
	return fName.ICompare(other) == 0;
}


bool
BHttpHeader::HeaderName::operator==(const std::string_view other) const noexcept
{
	return fName.ICompare(other.data(), other.size()) == 0;
}


BHttpHeader::HeaderName::operator BString() const
{
	return fName;
}


BHttpHeader::HeaderName::operator std::string_view() const
{
	return std::string_view(fName.String());
}


// #pragma mark -- BHttpHeader


BHttpHeader::BHttpHeader()
	: fName(std::string_view())
{

}


BHttpHeader::BHttpHeader(std::string_view name, std::string_view value)
	: fName(name)
{
	SetValue(value);
}


BHttpHeader::BHttpHeader(const BHttpHeader& other) = default;


BHttpHeader::BHttpHeader(BHttpHeader&& other) noexcept = default;


BHttpHeader::~BHttpHeader() noexcept
{

}


BHttpHeader&
BHttpHeader::operator=(const BHttpHeader& other) = default;


BHttpHeader&
BHttpHeader::operator=(BHttpHeader&& other) noexcept = default;


void
BHttpHeader::SetName(std::string_view name)
{
	fName = BHttpHeader::HeaderName(name);
}


void
BHttpHeader::SetValue(std::string_view value)
{
	if (!validate_value_string(value))
		throw InvalidInput(__PRETTY_FUNCTION__, BString(value.data(), value.length()));
	fValue.SetTo(value.data(), value.length());
}


const BHttpHeader::HeaderName&
BHttpHeader::Name() noexcept
{
	return fName;
}


std::string_view
BHttpHeader::Value() noexcept
{
	return std::string_view(fValue.String());
}


bool
BHttpHeader::IsEmpty() noexcept
{
	return fName.IsEmpty() || fValue.IsEmpty();
}
