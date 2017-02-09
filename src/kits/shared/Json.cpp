/*
 * Copyright 2014-2017, Augustin Cavalier (waddlesplash)
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */


#include <Json.h>

#include <cstdio>
#include <cstdlib>
#include <cerrno>

#include <MessageBuilder.h>
#include <UnicodeChar.h>


// #pragma mark - Public methods

namespace BPrivate {


class ParseException {
public:
	ParseException(int32 position, BString error)
		:
		fPosition(position),
		fError(error),
		fReturnCode(B_BAD_DATA)
	{
	}

	ParseException(int32 position, status_t returnCode)
		:
		fPosition(position),
		fError(""),
		fReturnCode(returnCode)
	{
	}

	void PrintToStream() const {
		const char* error;
		if (fError.Length() > 0)
			error = fError.String();
		else
			error = strerror(fReturnCode);
		printf("Parse error at %" B_PRIi32 ": %s\n", fPosition, error);
	}

	status_t ReturnCode() const
	{
		return fReturnCode;
	}

private:
	int32		fPosition;
	BString		fError;
	status_t	fReturnCode;
};


status_t
BJson::Parse(const char* JSON, BMessage& message)
{
	BString temp(JSON);
	return Parse(temp, message);
}


status_t
BJson::Parse(const BString& JSON, BMessage& message)
{
	try {
		_Parse(JSON, message);
		return B_OK;
	} catch (ParseException e) {
		e.PrintToStream();
		return e.ReturnCode();
	}
	return B_ERROR;
}


// #pragma mark - Private methods


void
BJson::_Parse(const BString& JSON, BMessage& message)
{
	BMessageBuilder builder(message);
	int32 pos = 0;
	const int32 length = JSON.Length();
	bool hadRootNode = false;

	/* Locals used by the parser. */
	// Keeps track of the hierarchy (e.g. "{[{{") that has
	// been read in. Allows the parser to verify that openbraces
	// match up to closebraces and so on and so forth.
	BString hierarchy("");
	// Stores the key that was just read by the string parser,
	// in the case that we are parsing a map.
	BString key("");

	// TODO: Check builder return codes and throw exception, or
	// change builder implementation/interface to throw exceptions
	// instead of returning errors.
	// TODO: Elimitate more duplicated code, for example by moving
	// more code into _ParseConstant().

	while (pos < length) {
		switch (JSON[pos]) {
		case '{':
			hierarchy += "{";

			if (hierarchy != "{") {
				if (builder.What() == JSON_TYPE_ARRAY)
					builder.PushObject(builder.CountNames());
				else {
					builder.PushObject(key.String());
					key = "";
				}
			} else if (hadRootNode == true) {
				throw ParseException(pos,
					"Got '{' with empty hierarchy but already had a root node");
			} else
				hadRootNode = true;

			builder.SetWhat(JSON_TYPE_MAP);
			break;

		case '}':
			if (key.Length() > 0)
				throw ParseException(pos, "Got closebrace but still have a key");
			if (hierarchy.EndsWith("{") && hierarchy.Length() != 1) {
				hierarchy.Truncate(hierarchy.Length() - 1);
				builder.PopObject();
			} else if (hierarchy.EndsWith("{") && hierarchy.Length() == 1) {
				hierarchy.Truncate(hierarchy.Length() - 1);
				break; // Should be the end of the data.
			} else
				throw ParseException(pos, "Unmatched closebrace }");

            break;

		case '[':
			hierarchy += "[";

			if (hierarchy != "[") {
				if (builder.What() == JSON_TYPE_ARRAY)
					builder.PushObject(builder.CountNames());
				else {
					builder.PushObject(key.String());
					key = "";
				}
			} else if (hadRootNode == true) {
				throw ParseException(pos,
					"Got '[' with empty hierarchy but already had a root node");
			} else
				hadRootNode = true;

			builder.SetWhat(JSON_TYPE_ARRAY);
			break;

		case ']':
			if (hierarchy.EndsWith("[") && hierarchy.Length() != 1) {
				hierarchy.Truncate(hierarchy.Length() - 1);
				builder.PopObject();
			} else if (hierarchy.EndsWith("[") && hierarchy.Length() == 1) {
				hierarchy.Truncate(hierarchy.Length() - 1);
				break; // Should be the end of the data.
			} else
				throw ParseException(pos, "Unmatched closebracket ]");

			break;


		case ':':
			if (hierarchy.Length() == 0)
				throw ParseException(pos, "Expected EOF, got ':'");
			if (builder.What() != JSON_TYPE_MAP || key.Length() == 0) {
				throw ParseException(pos, "Unexpected ':'");
			}
			break;
		case ',':
			if (builder.What() == JSON_TYPE_MAP && key.Length() != 0) {
				throw ParseException(pos, "Unexpected ',' expected ':'");
			}
			if (hierarchy.Length() == 0)
				throw ParseException(pos, "Expected EOF, got ','");
			break;

		case 't':
		{
			if (hierarchy.Length() == 0)
				throw ParseException(pos, "Expected EOF, got 't'");
			if (builder.What() != JSON_TYPE_ARRAY && key.Length() == 0) {
				throw ParseException(pos,
					"'true' cannot be a key, it can only be a value");
			}

			if (_ParseConstant(JSON, pos, "true")) {
				if (builder.What() == JSON_TYPE_ARRAY)
					key.SetToFormat("%" B_PRIu32, builder.CountNames());
				builder.AddBool(key.String(), true);
				key = "";
			} else
				throw ParseException(pos, "Unexpected 't'");

			break;
		}

		case 'f':
		{
			if (hierarchy.Length() == 0)
				throw ParseException(pos, "Expected EOF, got 'f'");
			if (builder.What() != JSON_TYPE_ARRAY && key.Length() == 0) {
				throw ParseException(pos,
					"'false' cannot be a key, it can only be a value");
			}

			if (_ParseConstant(JSON, pos, "false")) {
				if (builder.What() == JSON_TYPE_ARRAY)
					key.SetToFormat("%" B_PRIu32, builder.CountNames());
				builder.AddBool(key.String(), false);
				key = "";
			} else
				throw ParseException(pos, "Unexpected 'f'");

			break;
		}

        case 'n':
        {
			if (hierarchy.Length() == 0)
				throw ParseException(pos, "Expected EOF, got 'n'");
			if (builder.What() != JSON_TYPE_ARRAY && key.Length() == 0) {
				throw ParseException(pos,
					"'null' cannot be a key, it can only be a value");
			}

			if (_ParseConstant(JSON, pos, "null")) {
				if (builder.What() == JSON_TYPE_ARRAY)
					key.SetToFormat("%" B_PRIu32, builder.CountNames());
				builder.AddPointer(key.String(), (void*)NULL);
				key = "";
			} else
				throw ParseException(pos, "Unexpected 'n'");

			break;
		}

		case '"':
			if (hierarchy.Length() == 0)
				throw ParseException(pos, "Expected EOF, got '\"'");
			if (builder.What() != JSON_TYPE_ARRAY && key.Length() == 0)
				key = _ParseString(JSON, pos);
			else if (builder.What() != JSON_TYPE_ARRAY && key.Length() > 0) {
				builder.AddString(key, _ParseString(JSON, pos));
				key = "";
			} else if (builder.What() == JSON_TYPE_ARRAY) {
				key << builder.CountNames();
				builder.AddString(key, _ParseString(JSON, pos));
				key = "";
			} else
				throw ParseException(pos, "Internal error at encountering \"");

			break;

		case '+':
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		{
			if (hierarchy.Length() == 0)
				throw ParseException(pos, "Expected EOF, got number");
			if (builder.What() != JSON_TYPE_ARRAY && key.Length() == 0) {
				throw ParseException(pos,
					"Numbers cannot be keys, they can only be values");
			}

			if (builder.What() == JSON_TYPE_ARRAY)
				key << builder.CountNames();

			double number = _ParseNumber(JSON, pos);
			builder.AddDouble(key.String(), number);

			key = "";
			break;
		}

		case ' ':
		case '\t':
		case '\n':
		case '\r':
			// Whitespace; ignore.
			break;

		default:
			throw ParseException(pos, "Unexpected character");
		}
		pos++;
	}

	if (hierarchy.Length() != 0)
		throw ParseException(pos, "Unexpected EOF");
}


BString
BJson::_ParseString(const BString& JSON, int32& pos)
{
	if (JSON[pos] != '"') // Verify we're at the start of a string.
		return BString("");
	pos++;

	BString str;
	while (JSON[pos] != '"' && pos < JSON.Length()) {
		if (JSON[pos] == '\\') {
			pos++;
			switch (JSON[pos]) {
			case 'b':
				str += "\b";
				break;

			case 'f':
				str += "\f";
				break;

			case 'n':
				str += "\n";
				break;

			case 'r':
				str += "\r";
				break;

			case 't':
				str += "\t";
				break;

			case 'u': // 4-byte hexadecimal Unicode char (e.g. "\uffff")
			{
				uint intValue;
				BString substr;
				JSON.CopyInto(substr, pos + 1, 4);
				if (sscanf(substr.String(), "%4x", &intValue) != 1)
					return str;
					// We probably hit the end of the string.
					// This probably should be counted as an error,
					// but for now let's soft-fail instead of hard-fail.

				char character[20];
				char* ptr = character;
				BUnicodeChar::ToUTF8(intValue, &ptr);
				str.AppendChars(character, 1);
				pos += 4;
				break;
			}

			default:
				str += JSON[pos];
				break;
			}
		} else
			str += JSON[pos];
		pos++;
	}

	return str;
}


double
BJson::_ParseNumber(const BString& JSON, int32& pos)
{
	BString value;
	bool isDouble = false;

	while (true) {
		switch (JSON[pos]) {
		case '+':
		case '-':
		case 'e':
		case 'E':
		case '.':
			isDouble = true;
			// fall through
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			value += JSON[pos];
			pos++;
			continue;

		default:
			// We've reached the end of the number, so decrement the
			// "pos" value so that the parser picks up on the next char.
			pos--;
			break;
		}
		break;
	}

	errno = 0;
	double ret = 0;
	if (isDouble)
		ret = strtod(value.String(), NULL);
	else
		ret = strtoll(value.String(), NULL, 10);
	if (errno != 0)
		throw ParseException(pos, "Invalid number!");
	return ret;
}


bool
BJson::_ParseConstant(const BString& JSON, int32& pos, const char* constant)
{
	BString value;
	JSON.CopyInto(value, pos, strlen(constant));
	if (value == constant) {
		// Increase pos by the remainder of the constant, pos will be
		// increased for the first letter in the main parse loop.
		pos += strlen(constant) - 1;
		return true;
	} else
		return false;
}


} // namespace BPrivate

