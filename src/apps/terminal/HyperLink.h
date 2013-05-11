/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef HYPER_LINK_H
#define HYPER_LINK_H


#include <String.h>


class HyperLink {
public:
			enum Type {
				TYPE_URL,
				TYPE_PATH,
				TYPE_PATH_WITH_LINE,
				TYPE_PATH_WITH_LINE_AND_COLUMN
			};

public:
								HyperLink();
								HyperLink(const BString& address, Type type,
									const BString& baseAddress = BString());

			bool				IsValid() const	{ return !fAddress.IsEmpty(); }

			const BString&		Address() const		{ return fAddress; }
			const BString&		BaseAddress() const	{ return fBaseAddress; }
			Type				GetType() const		{ return fType; }

			status_t			Open();

private:
			BString				fAddress;
			BString				fBaseAddress;
			Type				fType;
};


#endif	// HYPER_LINK_H
