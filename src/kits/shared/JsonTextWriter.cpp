/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */


#include "JsonTextWriter.h"

#include <stdio.h>
#include <stdlib.h>

#include <UnicodeChar.h>


namespace BPrivate {


static bool
b_json_is_7bit_clean(uint8 c)
{
	return c >= 0x20 && c < 0x7f;
}


static bool
b_json_is_illegal(uint8 c)
{
	return c < 0x20 || c == 0x7f;
}


static const char*
b_json_simple_esc_sequence(char c)
{
	switch (c) {
		case '"':
			return "\\\"";
		case '\\':
			return "\\\\";
		case '/':
			return "\\/";
		case '\b':
			return "\\b";
		case '\f':
			return "\\f";
		case '\n':
			return "\\n";
		case '\r':
			return "\\r";
		case '\t':
			return "\\t";
		default:
			return NULL;
	}
}


static size_t
b_json_len_7bit_clean_non_esc(uint8* c, size_t length) {
	size_t result = 0;

	while (result < length
		&& b_json_is_7bit_clean(c[result])
		&& b_json_simple_esc_sequence(c[result]) == NULL) {
		result++;
	}

	return result;
}


/*! The class and sub-classes of it are used as a stack internal to the
    BJsonTextWriter class.  As the JSON is parsed, the stack of these
    internal listeners follows the stack of the JSON parsing in terms of
    containers; arrays and objects.
*/

class BJsonTextWriterStackedEventListener : public BJsonEventListener {
public:
								BJsonTextWriterStackedEventListener(
									BJsonTextWriter* writer,
									BJsonTextWriterStackedEventListener* parent);
								~BJsonTextWriterStackedEventListener();

				bool			Handle(const BJsonEvent& event);
				void			HandleError(status_t status, int32 line,
									const char* message);
				void			Complete();

				status_t		ErrorStatus();

				BJsonTextWriterStackedEventListener*
								Parent();

protected:

			status_t			StreamNumberNode(const BJsonEvent& event);

			status_t			StreamStringVerbatim(const char* string);
			status_t			StreamStringVerbatim(const char* string,
									off_t offset, size_t length);

			status_t			StreamStringEncoded(const char* string);
			status_t			StreamStringEncoded(const char* string,
									off_t offset, size_t length);

			status_t			StreamQuotedEncodedString(const char* string);
			status_t			StreamQuotedEncodedString(const char* string,
									off_t offset, size_t length);

			status_t			StreamChar(char c);

		virtual	bool			WillAdd();
		virtual void			DidAdd();

			void				SetStackedListenerOnWriter(
									BJsonTextWriterStackedEventListener*
									stackedListener);

			BJsonTextWriter*
								fWriter;
			BJsonTextWriterStackedEventListener*
								fParent;
				uint32			fCount;

};


class BJsonTextWriterArrayStackedEventListener
	: public BJsonTextWriterStackedEventListener {
public:
								BJsonTextWriterArrayStackedEventListener(
									BJsonTextWriter* writer,
									BJsonTextWriterStackedEventListener* parent);
								~BJsonTextWriterArrayStackedEventListener();

				bool			Handle(const BJsonEvent& event);

protected:
				bool			WillAdd();
};


class BJsonTextWriterObjectStackedEventListener
	: public BJsonTextWriterStackedEventListener {
public:
								BJsonTextWriterObjectStackedEventListener(
									BJsonTextWriter* writer,
									BJsonTextWriterStackedEventListener* parent);
								~BJsonTextWriterObjectStackedEventListener();

				bool			Handle(const BJsonEvent& event);
};

} // namespace BPrivate


using BPrivate::BJsonTextWriterStackedEventListener;
using BPrivate::BJsonTextWriterArrayStackedEventListener;
using BPrivate::BJsonTextWriterObjectStackedEventListener;


// #pragma mark - BJsonTextWriterStackedEventListener


BJsonTextWriterStackedEventListener::BJsonTextWriterStackedEventListener(
	BJsonTextWriter* writer,
	BJsonTextWriterStackedEventListener* parent)
{
	fWriter = writer;
	fParent = parent;
	fCount = 0;
}


BJsonTextWriterStackedEventListener::~BJsonTextWriterStackedEventListener()
{
}


bool
BJsonTextWriterStackedEventListener::Handle(const BJsonEvent& event)
{
	status_t writeResult = B_OK;

	if (fWriter->ErrorStatus() != B_OK)
		return false;

	switch (event.EventType()) {

		case B_JSON_NUMBER:
		case B_JSON_STRING:
		case B_JSON_TRUE:
		case B_JSON_FALSE:
		case B_JSON_NULL:
		case B_JSON_OBJECT_START:
		case B_JSON_ARRAY_START:
			if (!WillAdd())
				return false;
			break;

		default:
			break;
	}

	switch (event.EventType()) {

		case B_JSON_NUMBER:
			writeResult = StreamNumberNode(event);
			break;

		case B_JSON_STRING:
			writeResult = StreamQuotedEncodedString(event.Content());
			break;

		case B_JSON_TRUE:
			writeResult = StreamStringVerbatim("true", 0, 4);
			break;

		case B_JSON_FALSE:
			writeResult = StreamStringVerbatim("false", 0, 5);
			break;

		case B_JSON_NULL:
			writeResult = StreamStringVerbatim("null", 0, 4);
			break;

		case B_JSON_OBJECT_START:
		{
			writeResult = StreamChar('{');

			if (writeResult == B_OK) {
				SetStackedListenerOnWriter(
					new BJsonTextWriterObjectStackedEventListener(
						fWriter, this));
			}
			break;
		}

		case B_JSON_ARRAY_START:
		{
			writeResult = StreamChar('[');

			if (writeResult == B_OK) {
				SetStackedListenerOnWriter(
					new BJsonTextWriterArrayStackedEventListener(
						fWriter, this));
			}
			break;
		}

		default:
		{
			HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
				"unexpected type of json item to add to container");
			return false;
		}
	}

	if (writeResult == B_OK)
		DidAdd();
	else {
		HandleError(writeResult, JSON_EVENT_LISTENER_ANY_LINE,
			"error writing output");
	}

	return ErrorStatus() == B_OK;
}


void
BJsonTextWriterStackedEventListener::HandleError(status_t status, int32 line,
	const char* message)
{
	fWriter->HandleError(status, line, message);
}


void
BJsonTextWriterStackedEventListener::Complete()
{
		// illegal state.
	HandleError(JSON_EVENT_LISTENER_ANY_LINE, B_NOT_ALLOWED,
		"Complete() called on stacked message listener");
}


status_t
BJsonTextWriterStackedEventListener::ErrorStatus()
{
	return fWriter->ErrorStatus();
}


BJsonTextWriterStackedEventListener*
BJsonTextWriterStackedEventListener::Parent()
{
	return fParent;
}


status_t
BJsonTextWriterStackedEventListener::StreamNumberNode(const BJsonEvent& event)
{
	return fWriter->StreamNumberNode(event);
}


status_t
BJsonTextWriterStackedEventListener::StreamStringVerbatim(const char* string)
{
	return fWriter->StreamStringVerbatim(string);
}


status_t
BJsonTextWriterStackedEventListener::StreamStringVerbatim(const char* string,
	off_t offset, size_t length)
{
	return fWriter->StreamStringVerbatim(string, offset, length);
}


status_t
BJsonTextWriterStackedEventListener::StreamStringEncoded(const char* string)
{
	return fWriter->StreamStringEncoded(string);
}


status_t
BJsonTextWriterStackedEventListener::StreamStringEncoded(const char* string,
	off_t offset, size_t length)
{
	return fWriter->StreamStringEncoded(string, offset, length);
}


status_t
BJsonTextWriterStackedEventListener::StreamQuotedEncodedString(
	const char* string)
{
	return fWriter->StreamQuotedEncodedString(string);
}


status_t
BJsonTextWriterStackedEventListener::StreamQuotedEncodedString(
	const char* string, off_t offset, size_t length)
{
	return fWriter->StreamQuotedEncodedString(string, offset, length);
}


status_t
BJsonTextWriterStackedEventListener::StreamChar(char c)
{
	return fWriter->StreamChar(c);
}


bool
BJsonTextWriterStackedEventListener::WillAdd()
{
	return true; // carry on
}


void
BJsonTextWriterStackedEventListener::DidAdd()
{
	fCount++;
}


void
BJsonTextWriterStackedEventListener::SetStackedListenerOnWriter(
	BJsonTextWriterStackedEventListener* stackedListener)
{
	fWriter->SetStackedListener(stackedListener);
}


// #pragma mark - BJsonTextWriterArrayStackedEventListener


BJsonTextWriterArrayStackedEventListener::BJsonTextWriterArrayStackedEventListener(
	BJsonTextWriter* writer,
	BJsonTextWriterStackedEventListener* parent)
	:
	BJsonTextWriterStackedEventListener(writer, parent)
{
}


BJsonTextWriterArrayStackedEventListener
	::~BJsonTextWriterArrayStackedEventListener()
{
}


bool
BJsonTextWriterArrayStackedEventListener::Handle(const BJsonEvent& event)
{
	status_t writeResult = B_OK;

	if (fWriter->ErrorStatus() != B_OK)
		return false;

	switch (event.EventType()) {
		case B_JSON_ARRAY_END:
		{
			writeResult = StreamChar(']');

			if (writeResult == B_OK) {
				SetStackedListenerOnWriter(fParent);
				delete this;
				return true; // must exit immediately after delete this.
			}
			break;
		}

		default:
			return BJsonTextWriterStackedEventListener::Handle(event);
	}

	if(writeResult != B_OK) {
		HandleError(writeResult, JSON_EVENT_LISTENER_ANY_LINE,
			"error writing output");
	}

	return ErrorStatus() == B_OK;
}


bool
BJsonTextWriterArrayStackedEventListener::WillAdd()
{
	status_t writeResult = B_OK;

	if (writeResult == B_OK && fCount > 0)
		writeResult = StreamChar(',');

	if (writeResult != B_OK) {
		HandleError(B_IO_ERROR, JSON_EVENT_LISTENER_ANY_LINE,
			"error writing data");
		return false;
	}

	return BJsonTextWriterStackedEventListener::WillAdd();
}


// #pragma mark - BJsonTextWriterObjectStackedEventListener


BJsonTextWriterObjectStackedEventListener::BJsonTextWriterObjectStackedEventListener(
	BJsonTextWriter* writer,
	BJsonTextWriterStackedEventListener* parent)
	:
	BJsonTextWriterStackedEventListener(writer, parent)
{
}


BJsonTextWriterObjectStackedEventListener
	::~BJsonTextWriterObjectStackedEventListener()
{
}


bool
BJsonTextWriterObjectStackedEventListener::Handle(const BJsonEvent& event)
{
	status_t writeResult = B_OK;

	if (fWriter->ErrorStatus() != B_OK)
		return false;

	switch (event.EventType()) {
		case B_JSON_OBJECT_END:
		{
			writeResult = StreamChar('}');

			if (writeResult == B_OK) {
				SetStackedListenerOnWriter(fParent);
				delete this;
				return true; // just exit after delete this.
			}
			break;
		}

		case B_JSON_OBJECT_NAME:
		{
			if (writeResult == B_OK && fCount > 0)
				writeResult = StreamChar(',');

			if (writeResult == B_OK)
				writeResult = StreamQuotedEncodedString(event.Content());

			if (writeResult == B_OK)
				writeResult = StreamChar(':');

			break;
		}

		default:
			return BJsonTextWriterStackedEventListener::Handle(event);
	}

	if (writeResult != B_OK) {
		HandleError(writeResult, JSON_EVENT_LISTENER_ANY_LINE,
			"error writing data");
	}

	return ErrorStatus() == B_OK;
}


// #pragma mark - BJsonTextWriter


BJsonTextWriter::BJsonTextWriter(
	BDataIO* dataIO)
	:
	fDataIO(dataIO)
{

		// this is a preparation for this buffer to easily be used later
		// to efficiently output encoded unicode characters.

	fUnicodeAssemblyBuffer[0] = '\\';
	fUnicodeAssemblyBuffer[1] = 'u';

	fStackedListener = new BJsonTextWriterStackedEventListener(this, NULL);
}


BJsonTextWriter::~BJsonTextWriter()
{
	BJsonTextWriterStackedEventListener* listener = fStackedListener;

	while (listener != NULL) {
		BJsonTextWriterStackedEventListener* nextListener = listener->Parent();
		delete listener;
		listener = nextListener;
	}

	fStackedListener = NULL;
}


bool
BJsonTextWriter::Handle(const BJsonEvent& event)
{
	return fStackedListener->Handle(event);
}


void
BJsonTextWriter::Complete()
{
		// upon construction, this object will add one listener to the
		// stack.  On complete, this listener should still be there;
		// otherwise this implies an unterminated structure such as array
		// / object.

	if (fStackedListener->Parent() != NULL) {
		HandleError(B_BAD_DATA, JSON_EVENT_LISTENER_ANY_LINE,
			"unexpected end of input data");
	}
}


void
BJsonTextWriter::SetStackedListener(
	BJsonTextWriterStackedEventListener* stackedListener)
{
	fStackedListener = stackedListener;
}


status_t
BJsonTextWriter::StreamNumberNode(const BJsonEvent& event)
{
	return StreamStringVerbatim(event.Content());
}


status_t
BJsonTextWriter::StreamStringVerbatim(const char* string)
{
	return StreamStringVerbatim(string, 0, strlen(string));
}


status_t
BJsonTextWriter::StreamStringVerbatim(const char* string,
	off_t offset, size_t length)
{
	return fDataIO->WriteExactly(&string[offset], length);
}


status_t
BJsonTextWriter::StreamStringEncoded(const char* string)
{
	return StreamStringEncoded(string, 0, strlen(string));
}


status_t
BJsonTextWriter::StreamStringUnicodeCharacter(uint32 c)
{
	sprintf(&fUnicodeAssemblyBuffer[2], "%04" B_PRIx32, c);
		// note that the buffer's first two bytes are populated with the JSON
		// prefix for a unicode char.
	return StreamStringVerbatim(fUnicodeAssemblyBuffer, 0, 6);
}


/*! Note that this method will expect a UTF-8 encoded string. */

status_t
BJsonTextWriter::StreamStringEncoded(const char* string,
	off_t offset, size_t length)
{
	status_t writeResult = B_OK;
	uint8* string8bit = (uint8*)string;
	size_t i = 0;

	while (i < length && writeResult == B_OK) {
		uint8 c = string8bit[offset + i];
		const char* simpleEsc = b_json_simple_esc_sequence(c);

		if (simpleEsc != NULL) {
			// here the character to emit is something like a tab or a quote
			// in this case the output JSON should escape it so that it looks
			// like \t or \n in the output.
			writeResult = StreamStringVerbatim(simpleEsc, 0, 2);
			i++;
		} else {
			if (b_json_is_7bit_clean(c)) {
				// in this case the first character is a simple one that can be
				// output without any special handling.  Find the sequence of
				// such characters and output them as a sequence so that it's
				// included as one write operation.
				size_t l = 1 + b_json_len_7bit_clean_non_esc(
					&string8bit[offset + i + 1], length - (offset + i + 1));
				writeResult = StreamStringVerbatim(&string[offset + i], 0, l);
				i += static_cast<size_t>(l);
			} else {
				if (b_json_is_illegal(c)) {
					fprintf(stderr, "! string encoding error - illegal "
						"character [%" B_PRIu32 "]\n", static_cast<uint32>(c));
					i++;
				} else {
					// now we have a UTF-8 sequence.  Read the UTF-8 sequence
					// to get the unicode character and then encode that as
					// JSON.
					const char* unicodeStr = &string[offset + i];
					uint32 unicodeCharacter = BUnicodeChar::FromUTF8(
						&unicodeStr);
					writeResult = StreamStringUnicodeCharacter(
						unicodeCharacter);
					i += static_cast<size_t>(unicodeStr - &string[offset + i]);
				}
			}
		}
	}

	return writeResult;
}


status_t
BJsonTextWriter::StreamQuotedEncodedString(const char* string)
{
	return StreamQuotedEncodedString(string, 0, strlen(string));
}


status_t
BJsonTextWriter::StreamQuotedEncodedString(const char* string,
	off_t offset, size_t length)
{
	status_t write_result = B_OK;

	if (write_result == B_OK)
		write_result = StreamChar('\"');

	if (write_result == B_OK)
		write_result = StreamStringEncoded(string, offset, length);

	if (write_result == B_OK)
		write_result = StreamChar('\"');

	return write_result;
}


status_t
BJsonTextWriter::StreamChar(char c)
{
	return fDataIO->WriteExactly(&c, 1);
}
