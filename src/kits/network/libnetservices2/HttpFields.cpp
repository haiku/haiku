/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <HttpFields.h>

#include <algorithm>
#include <ctype.h>
#include <utility>

#include "HttpPrivate.h"

using namespace BPrivate::Network;


// #pragma mark -- utilities


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
validate_value_string(const std::string_view& string)
{
	for (auto it = string.cbegin(); it < string.cend(); it++) {
		if ((*it >= 0 && *it < 32) || *it == 127 || *it == '\t')
			return false;
	}
	return true;
}


/*!
	\brief Case insensitively compare two string_views.

	Inspired by https://stackoverflow.com/a/4119881
*/
static inline bool
iequals(const std::string_view& a, const std::string_view& b)
{
	return std::equal(a.begin(), a.end(), b.begin(), b.end(),
		[](char a, char b) { return tolower(a) == tolower(b); });
}


/*!
	\brief Trim whitespace from the beginning and end of a string_view

	Inspired by:
		https://terrislinenbach.medium.com/trimming-whitespace-from-a-string-view-6795e18b108f
*/
static inline std::string_view
trim(std::string_view in)
{
	auto left = in.begin();
	for (;; ++left) {
		if (left == in.end())
			return std::string_view();
		if (!isspace(*left))
			break;
	}

	auto right = in.end() - 1;
	for (; right > left && isspace(*right); --right)
		;

	return std::string_view(left, std::distance(left, right) + 1);
}


// #pragma mark -- BHttpFields::InvalidHeader


BHttpFields::InvalidInput::InvalidInput(const char* origin, BString input)
	:
	BError(origin),
	input(std::move(input))
{
}


const char*
BHttpFields::InvalidInput::Message() const noexcept
{
	return "Invalid format or unsupported characters in input";
}


BString
BHttpFields::InvalidInput::DebugMessage() const
{
	BString output = BError::DebugMessage();
	output << "\t " << input << "\n";
	return output;
}


// #pragma mark -- BHttpFields::Name


BHttpFields::FieldName::FieldName() noexcept
	:
	fName(std::string_view())
{
}


BHttpFields::FieldName::FieldName(const std::string_view& name) noexcept
	:
	fName(name)
{
}


/*!
	\brief Copy constructor;
*/
BHttpFields::FieldName::FieldName(const FieldName& other) noexcept = default;


/*!
	\brief Move constructor

	Moving leaves the other object in the empty state. It is implemented to satisfy the internal
	requirements of BHttpFields and std::list<Field>. Once an object is moved from it must no
	longer be used as an entry in a BHttpFields object.
*/
BHttpFields::FieldName::FieldName(FieldName&& other) noexcept
	:
	fName(std::move(other.fName))
{
	other.fName = std::string_view();
}


/*!
	\brief Copy assignment;
*/
BHttpFields::FieldName& BHttpFields::FieldName::operator=(
	const BHttpFields::FieldName& other) noexcept = default;


/*!
	\brief Move assignment

	Moving leaves the other object in the empty state. It is implemented to satisfy the internal
	requirements of BHttpFields and std::list<Field>. Once an object is moved from it must no
	longer be used as an entry in a BHttpFields object.
*/
BHttpFields::FieldName&
BHttpFields::FieldName::operator=(BHttpFields::FieldName&& other) noexcept
{
	fName = std::move(other.fName);
	other.fName = std::string_view();
	return *this;
}


bool
BHttpFields::FieldName::operator==(const BString& other) const noexcept
{
	return iequals(fName, std::string_view(other.String()));
}


bool
BHttpFields::FieldName::operator==(const std::string_view& other) const noexcept
{
	return iequals(fName, other);
}


bool
BHttpFields::FieldName::operator==(const BHttpFields::FieldName& other) const noexcept
{
	return iequals(fName, other.fName);
}


BHttpFields::FieldName::operator std::string_view() const
{
	return fName;
}


// #pragma mark -- BHttpFields::Field


BHttpFields::Field::Field() noexcept
	:
	fName(std::string_view()),
	fValue(std::string_view())
{
}


BHttpFields::Field::Field(const std::string_view& name, const std::string_view& value)
{
	if (name.length() == 0 || !validate_http_token_string(name))
		throw BHttpFields::InvalidInput(__PRETTY_FUNCTION__, BString(name.data(), name.size()));
	if (value.length() == 0 || !validate_value_string(value))
		throw BHttpFields::InvalidInput(__PRETTY_FUNCTION__, BString(value.data(), value.length()));

	BString rawField(name.data(), name.size());
	rawField << ": ";
	rawField.Append(value.data(), value.size());

	fName = std::string_view(rawField.String(), name.size());
	fValue = std::string_view(rawField.String() + name.size() + 2, value.size());
	fRawField = std::move(rawField);
}


BHttpFields::Field::Field(BString& field)
{
	// Check if the input contains a key, a separator and a value.
	auto separatorIndex = field.FindFirst(':');
	if (separatorIndex <= 0)
		throw BHttpFields::InvalidInput(__PRETTY_FUNCTION__, field);

	// Get the name and the value. Remove whitespace around the value.
	auto name = std::string_view(field.String(), separatorIndex);
	auto value = trim(std::string_view(field.String() + separatorIndex + 1));

	if (name.length() == 0 || !validate_http_token_string(name))
		throw BHttpFields::InvalidInput(__PRETTY_FUNCTION__, BString(name.data(), name.size()));
	if (value.length() == 0 || !validate_value_string(value))
		throw BHttpFields::InvalidInput(__PRETTY_FUNCTION__, BString(value.data(), value.length()));

	fRawField = std::move(field);
	fName = name;
	fValue = value;
}


BHttpFields::Field::Field(const BHttpFields::Field& other)
	:
	fName(std::string_view()),
	fValue(std::string_view())
{
	if (other.IsEmpty()) {
		fRawField = BString();
		fName = std::string_view();
		fValue = std::string_view();
	} else {
		fRawField = other.fRawField;
		auto nameSize = other.Name().fName.size();
		auto valueOffset = other.fValue.data() - other.fRawField.value().String();
		fName = std::string_view((*fRawField).String(), nameSize);
		fValue = std::string_view((*fRawField).String() + valueOffset, other.fValue.size());
	}
}


BHttpFields::Field::Field(BHttpFields::Field&& other) noexcept
	:
	fRawField(std::move(other.fRawField)),
	fName(std::move(other.fName)),
	fValue(std::move(other.fValue))
{
	other.fName.fName = std::string_view();
	other.fValue = std::string_view();
}


BHttpFields::Field&
BHttpFields::Field::operator=(const BHttpFields::Field& other)
{
	if (other.IsEmpty()) {
		fRawField = BString();
		fName = std::string_view();
		fValue = std::string_view();
	} else {
		fRawField = other.fRawField;
		auto nameSize = other.Name().fName.size();
		auto valueOffset = other.fValue.data() - other.fRawField.value().String();
		fName = std::string_view((*fRawField).String(), nameSize);
		fValue = std::string_view((*fRawField).String() + valueOffset, other.fValue.size());
	}
	return *this;
}


BHttpFields::Field&
BHttpFields::Field::operator=(BHttpFields::Field&& other) noexcept
{
	fRawField = std::move(other.fRawField);
	fName = std::move(other.fName);
	other.fName.fName = std::string_view();
	fValue = std::move(other.fValue);
	fValue = std::string_view();
	return *this;
}


const BHttpFields::FieldName&
BHttpFields::Field::Name() const noexcept
{
	return fName;
}


std::string_view
BHttpFields::Field::Value() const noexcept
{
	return fValue;
}


std::string_view
BHttpFields::Field::RawField() const noexcept
{
	if (fRawField)
		return std::string_view((*fRawField).String(), (*fRawField).Length());
	else
		return std::string_view();
}


bool
BHttpFields::Field::IsEmpty() const noexcept
{
	// The object is either fully empty, or it has data, so we only have to check fValue.
	return !fRawField.has_value();
}


// #pragma mark -- BHttpFields


BHttpFields::BHttpFields()
{
}


BHttpFields::BHttpFields(std::initializer_list<BHttpFields::Field> fields)
{
	AddFields(fields);
}


BHttpFields::BHttpFields(const BHttpFields& other) = default;


BHttpFields::BHttpFields(BHttpFields&& other)
	:
	fFields(std::move(other.fFields))
{
	// Explicitly clear the other list, as the C++ standard does not specify that the other list
	// will be empty.
	other.fFields.clear();
}


BHttpFields::~BHttpFields() noexcept
{
}


BHttpFields& BHttpFields::operator=(const BHttpFields& other) = default;


BHttpFields&
BHttpFields::operator=(BHttpFields&& other) noexcept
{
	fFields = std::move(other.fFields);

	// Explicitly clear the other list, as the C++ standard does not specify that the other list
	// will be empty.
	other.fFields.clear();
	return *this;
}


const BHttpFields::Field&
BHttpFields::operator[](size_t index) const
{
	if (index >= fFields.size())
		throw BRuntimeError(__PRETTY_FUNCTION__, "Index out of bounds");
	auto it = fFields.cbegin();
	std::advance(it, index);
	return *it;
}


void
BHttpFields::AddField(const std::string_view& name, const std::string_view& value)
{
	fFields.emplace_back(name, value);
}


void
BHttpFields::AddField(BString& field)
{
	fFields.emplace_back(field);
}


void
BHttpFields::AddFields(std::initializer_list<Field> fields)
{
	for (auto& field: fields) {
		if (!field.IsEmpty())
			fFields.push_back(std::move(field));
	}
}


void
BHttpFields::RemoveField(const std::string_view& name) noexcept
{
	for (auto it = FindField(name); it != end(); it = FindField(name)) {
		fFields.erase(it);
	}
}


void
BHttpFields::RemoveField(ConstIterator it) noexcept
{
	fFields.erase(it);
}


void
BHttpFields::MakeEmpty() noexcept
{
	fFields.clear();
}


BHttpFields::ConstIterator
BHttpFields::FindField(const std::string_view& name) const noexcept
{
	for (auto it = fFields.cbegin(); it != fFields.cend(); it++) {
		if ((*it).Name() == name)
			return it;
	}
	return fFields.cend();
}


size_t
BHttpFields::CountFields() const noexcept
{
	return fFields.size();
}


size_t
BHttpFields::CountFields(const std::string_view& name) const noexcept
{
	size_t count = 0;
	for (auto it = fFields.cbegin(); it != fFields.cend(); it++) {
		if ((*it).Name() == name)
			count += 1;
	}
	return count;
}


BHttpFields::ConstIterator
BHttpFields::begin() const noexcept
{
	return fFields.cbegin();
}


BHttpFields::ConstIterator
BHttpFields::end() const noexcept
{
	return fFields.cend();
}
