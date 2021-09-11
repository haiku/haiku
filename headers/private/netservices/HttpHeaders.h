/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_HEADERS_H_
#define _B_HTTP_HEADERS_H_


#include <List.h>
#include <Message.h>
#include <String.h>


namespace BPrivate {

namespace Network {


class BHttpHeader {
public:
								BHttpHeader();
								BHttpHeader(const char* string);
								BHttpHeader(const char* name,
									const char* value);
								BHttpHeader(const BHttpHeader& copy);

	// Header data modification
			void				SetName(const char* name);
			void				SetValue(const char* value);
			bool				SetHeader(const char* string);

	// Header data access
			const char*			Name() const;
			const char*			Value() const;
			const char*			Header() const;

	// Header data test
			bool				NameIs(const char* name) const;

	// Overloaded members
			BHttpHeader&		operator=(const BHttpHeader& other);

private:
			BString				fName;
			BString				fValue;

	mutable	BString				fRawHeader;
	mutable	bool				fRawHeaderValid;
};


class BHttpHeaders {
public:
								BHttpHeaders();
								BHttpHeaders(const BHttpHeaders& copy);
								~BHttpHeaders();

	// Header list access
			const char*			HeaderValue(const char* name) const;
			BHttpHeader&		HeaderAt(int32 index) const;

	// Header count
			int32				CountHeaders() const;

	// Header list tests
			int32				HasHeader(const char* name) const;

	// Header add or replacement
			bool				AddHeader(const char* line);
			bool				AddHeader(const char* name,
									const char* value);
			bool				AddHeader(const char* name,
									int32 value);

	// Archiving
			void				PopulateFromArchive(BMessage*);
			void				Archive(BMessage*) const;

	// Header deletion
			void				Clear();

	// Overloaded operators
			BHttpHeaders&		operator=(const BHttpHeaders& other);
			BHttpHeader&		operator[](int32 index) const;
			const char*			operator[](const char* name) const;

private:
			void				_EraseData();
			bool				_AddOrDeleteHeader(BHttpHeader* header);

private:
			BList				fHeaderList;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_HEADERS_H_
