/*
 * Copyright 2011-2016, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef RESPONSE_H
#define RESPONSE_H


#include <stdexcept>

#include <DataIO.h>
#include <ObjectList.h>
#include <String.h>


namespace IMAP {


class Argument;
class Response;


class RFC3501Encoding {
public:
								RFC3501Encoding();
								~RFC3501Encoding();

			BString				Encode(const BString& clearText) const;
			BString				Decode(const BString& encodedText) const;

private:
			void				_ToUTF8(BString& string, uint32 c) const;
			void				_Unshift(BString& string, int32& bitsToWrite,
									int32& sextet, bool& shifted) const;
};


class ArgumentList : public BObjectList<Argument> {
public:
								ArgumentList();
								~ArgumentList();

			bool				Contains(const char* string) const;
			BString				StringAt(int32 index) const;
			bool				IsStringAt(int32 index) const;
			bool				EqualsAt(int32 index,
									const char* string) const;

			ArgumentList&		ListAt(int32 index) const;
			bool				IsListAt(int32 index) const;
			bool				IsListAt(int32 index, char kind) const;

			uint32				NumberAt(int32 index) const;
			bool				IsNumberAt(int32 index) const;

			BString				ToString() const;
};


class Argument {
public:
								Argument();
	virtual						~Argument();

	virtual	BString				ToString() const = 0;
};


class ListArgument : public Argument {
public:
								ListArgument(char kind);

			ArgumentList&		List() { return fList; }
			char				Kind() { return fKind; }

	virtual	BString				ToString() const;

private:
			ArgumentList		fList;
			char				fKind;
};


class StringArgument : public Argument {
public:
								StringArgument(const BString& string);
								StringArgument(const StringArgument& other);

			const BString&		String() { return fString; }

	virtual	BString				ToString() const;

private:
			BString				fString;
};


class ParseException : public std::exception {
public:
								ParseException();
								ParseException(const char* format, ...);

			const char*			Message() const { return fBuffer; }

protected:
			char				fBuffer[64];
};


class StreamException : public std::exception {
public:
								StreamException(status_t status);

			status_t			Status() const { return fStatus; }

private:
			status_t			fStatus;
};


class ExpectedParseException : public ParseException {
public:
								ExpectedParseException(char expected,
									char instead);

protected:
			const char*			CharToString(char* buffer, size_t size, char c);
};


class LiteralHandler {
public:
								LiteralHandler();
	virtual						~LiteralHandler();

	virtual bool				HandleLiteral(Response& response,
									ArgumentList& arguments, BDataIO& stream,
									size_t& length) = 0;
};


class Response : public ArgumentList {
public:
								Response();
								~Response();

			void				Parse(BDataIO& stream, LiteralHandler* handler);

			bool				IsUntagged() const { return fTag == 0; }
			uint32				Tag() const { return fTag; }
			bool				IsCommand(const char* command) const;
			bool				IsContinuation() const { return fContinuation; }

protected:
			char				ParseLine(ArgumentList& arguments,
									BDataIO& stream);
			void				ParseList(ArgumentList& arguments,
									BDataIO& stream, char start, char end);
			void				ParseQuoted(ArgumentList& arguments,
									BDataIO& stream);
			void				ParseLiteral(ArgumentList& arguments,
									BDataIO& stream);
			void				ParseString(ArgumentList& arguments,
									BDataIO& stream);

			BString				ExtractString(BDataIO& stream);
			size_t				ExtractNumber(BDataIO& stream);

			void				Consume(BDataIO& stream, char c);

			char				Next(BDataIO& stream);
			char				Peek(BDataIO& stream);
			char				Read(BDataIO& stream);

private:
			void				_SkipLiteral(BDataIO& stream, size_t size);

protected:
			LiteralHandler*		fLiteralHandler;
			uint32				fTag;
			bool				fContinuation;
			bool				fHasNextChar;
			char				fNextChar;
};


class ResponseParser {
public:
								ResponseParser(BDataIO& stream);
								~ResponseParser();

			void				SetTo(BDataIO& stream);
			void				SetLiteralHandler(LiteralHandler* handler);

			status_t			NextResponse(Response& response,
									bigtime_t timeout);

private:
								ResponseParser(const ResponseParser& other);

protected:
			BDataIO*			fStream;
			LiteralHandler*		fLiteralHandler;
};


}	// namespace IMAP


#endif // RESPONSE_H
