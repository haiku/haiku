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


class ArgumentList {
public:
								ArgumentList();
								~ArgumentList();

			size_t				CountItems()
									{ return (size_t)fArguments.CountItems(); }
			Argument&			ItemAt(size_t index) const
									{ return *fArguments.ItemAt(index); }
			void				MakeEmpty()
									{ fArguments.MakeEmpty(); }
			bool				AddItem(Argument* argument)
									{ return fArguments.AddItem(argument); }
			Argument*			RemoveItem(size_t index)
									{ return fArguments.RemoveItemAt(index); }

			BString				ToString() const;
			bool				Contains(const char* string) const;

			BString				StringAt(int32 index) const;
			bool				IsStringAt(int32 index) const;
			bool				EqualsAt(int32 index,
									const char* string) const;
			const ArgumentList&	ListAt(int32 index) const;
			bool				IsListAt(int32 index) const;
			bool				IsListAt(int32 index, char kind) const;
			int32				IntegerAt(int32 index) const;
			bool				IsIntegerAt(int32 index) const;

private:
			BObjectList<Argument> fArguments;
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

			const BString&		String() { return fString; }

	virtual	BString				ToString() const;

private:
			BString				fString;
};


class ParseException : public std::exception {
public:
								ParseException();
								ParseException(const char* message);
	virtual						~ParseException();

			const char*			Message() const { return fMessage; }

protected:
			const char*			fMessage;
};


class ExpectedParseException : ParseException {
public:
								ExpectedParseException(char expected,
									char instead);

protected:
			char				fBuffer[64];
};


class Response : public ArgumentList {
public:
								Response();
								~Response();

			void				SetTo(const char* line) throw(ParseException);

			bool				IsUntagged() const { return fTag == 0; }
			int32				Tag() const { return fTag; }
			bool				IsCommand(const char* command) const;
			bool				IsContinued() const { return fContinued; }

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
			int32				fTag;
			bool				fContinued;
};


}	// namespace IMAP


#endif // RESPONSE_H
