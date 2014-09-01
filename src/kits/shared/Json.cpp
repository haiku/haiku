/*
 * Copyright 2014, Augustin Cavalier (waddlesplash)
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */


#include <Json.h>

#include <stdio.h>
#include <stdlib.h>

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
BJson::Parse(BMessage& message, const char* JSON)
{
	BString temp(JSON);
	return Parse(message, temp);
}


status_t
BJson::Parse(BMessage& message, BString& JSON)
{
	try {
		_Parse(message, JSON);
		return B_OK;
	} catch (ParseException e) {
		e.PrintToStream();
		return e.ReturnCode();
	}
	return B_ERROR;
}


// #pragma mark - Private methods


void
BJson::_Parse(BMessage& message, BString& JSON)
{
	BMessageBuilder builder(message);
	int32 pos = 0;
	int32 length = JSON.Length();
	
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
			}

			builder.SetWhat(JSON_TYPE_MAP);
			break;

		case '}':
			if (hierarchy.EndsWith("{") && hierarchy.Length() != 1) {
				hierarchy.Truncate(hierarchy.Length() - 1);
				builder.PopObject();
			} else if (hierarchy.Length() == 1)
				return; // End of the JSON data
			else
				throw ParseException(pos, "Unmatched closebrace }");

            break;

		case '[':
			hierarchy += "[";
			
			if (builder.What() == JSON_TYPE_ARRAY)
				builder.PushObject(builder.CountNames());
			else {
				builder.PushObject(key.String());
				key = "";
			}

			builder.SetWhat(JSON_TYPE_ARRAY);
			break;

		case ']':
			if (hierarchy.EndsWith("[")) {
				hierarchy.Truncate(hierarchy.Length() - 1);
				builder.PopObject();
			} else {
				BString error("Unmatched closebrace ] hierarchy: ");
				error << hierarchy;
				throw ParseException(pos, error);
			}

			break;

		case 't':
		{
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

		case ':':
		case ',':
		default:
			// No need to do anything here.
			break;
		}
		pos++;
	}

	throw ParseException(pos, "Unexpected end of document");
}


BString
BJson::_ParseString(BString& JSON, int32& pos)
{
	if (JSON[pos] != '"') // Verify we're at the start of a string.
		return BString("");
	pos++;
	
	BString str;
	while (JSON[pos] != '"') {
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
BJson::_ParseNumber(BString& JSON, int32& pos)
{
	BString value;

	while (true) {
		switch (JSON[pos]) {
		case '+':
		case '-':
		case 'e':
		case 'E':
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
		case '.':
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
	
	return strtod(value.String(), NULL);
}


bool
BJson::_ParseConstant(BString& JSON, int32& pos, const char* constant)
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

