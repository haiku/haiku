/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef RESPONSE_H
#define RESPONSE_H


#include <stdexcept>

#include <ObjectList.h>
#include <String.h>


namespace IMAP {


class Argument;
class ConnectionReader;


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
								ParseException(const char* message);

			const char*			Message() const { return fMessage; }

protected:
			const char*			fMessage;
};


class ExpectedParseException : public ParseException {
public:
								ExpectedParseException(char expected,
									char instead);

protected:
			char				fBuffer[64];
};


class LiteralHandler {
public:
								LiteralHandler();
	virtual						~LiteralHandler();

	virtual void				HandleLiteral(ConnectionReader& reader,
									off_t length) = 0;
};


class Response : public ArgumentList {
public:
								Response();
								~Response();

			void				Parse(ConnectionReader& reader,
									const char* line, LiteralHandler* handler)
										throw(ParseException);

			bool				IsUntagged() const { return fTag == 0; }
			int32				Tag() const { return fTag; }
			bool				IsCommand(const char* command) const;
			bool				IsContinuation() const { return fContinuation; }

protected:
			char				ParseLine(ArgumentList& arguments,
									const char*& line);
			void				Consume(const char*& line, char c);
			void				ParseList(ArgumentList& arguments,
									const char*& line, char start, char end);
			void				ParseQuoted(ArgumentList& arguments,
									const char*& line);
			void				ParseLiteral(ArgumentList& arguments,
									const char*& line);
			void				ParseString(ArgumentList& arguments,
									const char*& line);
			BString				ExtractString(const char*& line);

protected:
			ConnectionReader*	fReader;
			LiteralHandler*		fLiteralHandler;
			int32				fTag;
			bool				fContinuation;
};


class ResponseParser {
public:
								ResponseParser(ConnectionReader& reader);
								~ResponseParser();

			void				SetTo(ConnectionReader& reader);
			void				SetLiteralHandler(LiteralHandler* handler);

			status_t			NextResponse(Response& response,
									bigtime_t timeout) throw(ParseException);

protected:
			ConnectionReader*	fReader;
			LiteralHandler*		fLiteralHandler;
};


}	// namespace IMAP


#endif // RESPONSE_H
