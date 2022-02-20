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


/*
	\brief Case insensitively compare two string_views.

	Inspired by https://stackoverflow.com/a/4119881
*/
static inline bool
iequals(const std::string_view& a, const std::string_view& b)
{
	return std::equal(
		a.begin(), a.end(),
		b.begin(), b.end(),
		[](char a, char b) {
			return tolower(a) == tolower(b);
	});
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


BHttpFields::FieldName::FieldName(const std::string_view& name)
	: fName(name)
{

}


BHttpFields::FieldName::FieldName(BString name)
	: fName(std::move(name))
{

}


/*!
	\brief Copy constructor; any borrowed field is copied into an owned field
*/
BHttpFields::FieldName::FieldName(const FieldName& other)
{
	BString otherName = other;
	fName = std::move(otherName);
}


/*!
	\brief Move constructor

	Moving leaves the other object in the empty state. It is implemented to satisfy the internal
	requirements of BHttpFields and std::list<Field>. Once an object is moved from it must no
	longer be used as an entry in a BHttpFields object.
*/
BHttpFields::FieldName::FieldName(FieldName&& other) noexcept
	: fName(std::move(other.fName))
{
	other.fName = std::string_view();
}


/*!
	\brief Copy assignment; the copy is always owned
*/
BHttpFields::FieldName&
BHttpFields::FieldName::operator=(const BHttpFields::FieldName& other)
{
	BString otherName = other;
	fName = std::move(otherName);
	return *this;
}


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


/*!
	\brief Unchecked assignment of owned name

	This should only be used interally when the name is known to be valid!
*/
BHttpFields::FieldName&
BHttpFields::FieldName::operator=(BString name)
{
	fName = std::move(name);
	return *this;
}


bool
BHttpFields::FieldName::operator==(const BString& other) const noexcept
{
	if (std::holds_alternative<std::string_view>(fName)) {
		return iequals(std::get<std::string_view>(fName), std::string_view(other.String()));
	} else {
		return std::get<BString>(fName).ICompare(other) == 0;
	}
}


bool
BHttpFields::FieldName::operator==(const std::string_view& other) const noexcept
{
	if (std::holds_alternative<std::string_view>(fName)) {
		return iequals(std::get<std::string_view>(fName), other);
	} else {
		return std::get<BString>(fName).ICompare(other.data(), other.size()) == 0;
	}
}


bool
BHttpFields::FieldName::operator==(const BHttpFields::FieldName& other) const noexcept
{
	if (std::holds_alternative<std::string_view>(other.fName)) {
		return *this == std::get<std::string_view>(other.fName);
	} else {
		return *this == std::get<BString>(other.fName);
	}
}



BHttpFields::FieldName::operator BString() const
{
	if (std::holds_alternative<std::string_view>(fName)) {
		const auto& name = std::get<std::string_view>(fName);
		return BString(name.data(), name.size());
	} else {
		return std::get<BString>(fName);
	}
}


BHttpFields::FieldName::operator std::string_view() const
{
	if (std::holds_alternative<std::string_view>(fName)) {
		return std::get<std::string_view>(fName);
	} else {
		return std::string_view(std::get<BString>(fName).String());
	}
}


// #pragma mark -- BHttpFields::Field


BHttpFields::Field::Field() noexcept
	: fName(std::string_view()), fValue(std::string_view())
{

}


BHttpFields::Field::Field(const std::string_view& name, const std::string_view& value)
	: Field(name, value, false)
{

}


/*!
	\brief Internal constructor that has the option to create an instance of a 'borrowed' item.
*/
BHttpFields::Field::Field(const std::string_view& name, const std::string_view& value, bool borrowed)
	: fName(name), fValue(value)
{
	if (name.length() == 0 || !validate_http_token_string(name))
		throw BHttpFields::InvalidInput(__PRETTY_FUNCTION__, fName);
	if (value.length() == 0 || !validate_value_string(value))
		throw BHttpFields::InvalidInput(__PRETTY_FUNCTION__, BString(value.data(), value.length()));

	if (!borrowed) {
		// set as owned
		fName = BHttpFields::FieldName(BString(name.data(), name.length()));
		fValue = BString(value.data(), value.length());
	}
}


BHttpFields::Field::Field(const BHttpFields::Field& other)
	: fName(std::string_view()), fValue(std::string_view())
{
	if (!other.IsEmpty()) {
		BString name = other.fName;
		fName = std::move(name);
		std::string_view otherValue = other.Value();
		fValue = BString(otherValue.data(), otherValue.length());
	}
}


BHttpFields::Field::Field(BHttpFields::Field&& other) noexcept
	: fName(std::move(other.fName)), fValue(std::move(other.fValue))
{
	other.fName.fName = std::string_view();
	other.fValue = std::string_view();
}


BHttpFields::Field&
BHttpFields::Field::operator=(const BHttpFields::Field& other)
{
	if (other.IsEmpty()) {
		fName = std::string_view();
		fValue = std::string_view();
	} else {
		BString name = other.fName;
		fName = std::move(name);
		std::string_view otherValue = other.Value();
		fValue = BString(otherValue.data(), otherValue.length());
	}
	return *this;
}


BHttpFields::Field&
BHttpFields::Field::operator=(BHttpFields::Field&& other) noexcept
{
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
	if (std::holds_alternative<std::string_view>(fValue)) {
		return std::get<std::string_view>(fValue);
	} else {
		return std::string_view(std::get<BString>(fValue).String());
	}
}


bool
BHttpFields::Field::IsEmpty() const noexcept
{
	// The object is either fully empty, or it has data, so we only have to check fValue.
	if (std::holds_alternative<std::string_view>(fValue))
		return std::get<std::string_view>(fValue).length() == 0;
	return false;
}


// #pragma mark -- BHttpFields


BHttpFields::BHttpFields()
{

}


BHttpFields::BHttpFields(std::initializer_list<BHttpFields::Field> fields)
{
	for (auto& field: fields) {
		if (!field.IsEmpty())
			_AddField(Field(field));
	}
}


BHttpFields::BHttpFields(const BHttpFields& other) = default;


BHttpFields::BHttpFields(BHttpFields&& other)
	: fFields(std::move(other.fFields))
{
	// Explicitly clear the other list, as the C++ standard does not specify that the other list
	// will be empty.
	other.fFields.clear();
}


BHttpFields::~BHttpFields() noexcept
{

}


BHttpFields&
BHttpFields::operator=(const BHttpFields& other) = default;


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
	_AddField(BHttpFields::Field(name, value));
}


void
BHttpFields::RemoveField(const std::string_view& name) noexcept
{
	for(auto it = FindField(name); it != end(); it = FindField(name)) {
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


void
BHttpFields::_AddField(Field&& field)
{
	// This could be made more efficient bay adding a set of existing keys to quickly check against
	auto rIterator = std::find_if(fFields.rbegin(), fFields.rend(), [&, field](const Field& f){
		return f.Name() == field.Name();
	});

	if (rIterator == fFields.rend())
		fFields.push_back(field);
	else
		fFields.insert(rIterator.base(), field);
}
