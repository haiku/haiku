/*
 * Copyright 2014, Augustin Cavalier (waddlesplash)
 * Distributed under the terms of the MIT License.
 */


#include <Json.h>

#include <stdio.h>
#include <stdlib.h>

#include <MessageBuilder.h>
#include <UnicodeChar.h>


// #pragma mark - Public methods

namespace BPrivate {

status_t
BJson::Parse(BMessage& message, const char* JSON)
{
	BString temp(JSON);
	return Parse(message, temp);
}


status_t
BJson::Parse(BMessage& message, BString& JSON)
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
			if (hierarchy.EndsWith("{") && (hierarchy.Length() != 1)) {
				hierarchy.Truncate(hierarchy.Length() - 1);
				builder.PopObject();
			} else if (hierarchy.Length() == 1)
				return B_OK; // End of the JSON data
			else
				return B_BAD_DATA; // Unmatched closebrace

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
			} else
				return B_BAD_DATA; // Unmatched closebracket

			break;

		case 't':
		{
			if (builder.What() != JSON_TYPE_ARRAY && key.Length() == 0)
				return B_BAD_DATA;
				// 'true' cannot be a key, it can only be a value
			
			if (_Parser_ParseConstant(JSON, pos, "true")) {
				if (builder.What() == JSON_TYPE_ARRAY)
					key.SetToFormat("%zu", builder.CountNames());
				builder.AddBool(key.String(), true);
				key = "";
			} else
				return B_BAD_DATA;
				// "t" out in the middle of nowhere!?

			break;
		}

		case 'f':
		{
			if (builder.What() != JSON_TYPE_ARRAY && key.Length() == 0)
				return B_BAD_DATA;
				// 'false' cannot be a key, it can only be a value
			
			if (_Parser_ParseConstant(JSON, pos, "false")) {
				if (builder.What() == JSON_TYPE_ARRAY)
					key.SetToFormat("%zu", builder.CountNames());
				builder.AddBool(key.String(), false);
				key = "";
			} else
				return B_BAD_DATA;
				// "f" out in the middle of nowhere!?

			break;
		}

        case 'n':
        {
			if (builder.What() != JSON_TYPE_ARRAY && key.Length() == 0)
				return B_BAD_DATA;
				// 'null' cannot be a key, it can only be a value
			
			if (_Parser_ParseConstant(JSON, pos, "null")) {
				if (builder.What() == JSON_TYPE_ARRAY)
					key.SetToFormat("%zu", builder.CountNames());
				builder.AddPointer(key.String(), (void*)NULL);
				key = "";
			} else
				return B_BAD_DATA;
				// "n" out in the middle of nowhere!?

			break;
		}

		case '"':
			if (builder.What() != JSON_TYPE_ARRAY && key.Length() == 0)
				key = _Parser_ParseString(JSON, pos);
			else if (builder.What() != JSON_TYPE_ARRAY && key.Length() > 0) {
				builder.AddString(key, _Parser_ParseString(JSON, pos));
				key = "";
			} else if (builder.What() == JSON_TYPE_ARRAY) {
				key << builder.CountNames();
				builder.AddString(key, _Parser_ParseString(JSON, pos));
				key = "";
			} else
				return B_BAD_DATA;
				// Pretty sure it's impossible to get here, but you never know

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
			if (builder.What() != JSON_TYPE_ARRAY && key.Length() == 0)
				return B_BAD_DATA;
				// Numbers cannot be keys, they can only be values
			
			if (builder.What() == JSON_TYPE_ARRAY)
				key << builder.CountNames();

			double number = _Parser_ParseNumber(JSON, pos);
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

	// Reached the end of the BString without reaching the end of the document
	return B_BAD_DATA;
}


// #pragma mark - Private methods


BString
BJson::_Parser_ParseString(BString& JSON, int32& pos)
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
BJson::_Parser_ParseNumber(BString& JSON, int32& pos)
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
BJson::_Parser_ParseConstant(BString& JSON, int32& pos, const char* constant)
{
	BString value;
	JSON.CopyInto(value, pos, strlen(constant));
	if (value == constant) {
		pos += strlen(constant);
		return true;
	} else
		return false;
}


} // namespace BPrivate

