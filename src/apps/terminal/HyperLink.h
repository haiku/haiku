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
								HyperLink(const BString& address, Type type);
								HyperLink(const BString& text,
									const BString& address, Type type);

			bool				IsValid() const	{ return !fAddress.IsEmpty(); }

			const BString&		Text() const		{ return fText; }
			const BString&		Address() const		{ return fAddress; }
			Type				GetType() const		{ return fType; }

			status_t			Open();

private:
			BString				fText;
			BString				fAddress;
			Type				fType;
};


#endif	// HYPER_LINK_H
