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
				TYPE_PATH_WITH_LINE_AND_COLUMN,
				TYPE_OSC_URL
			};

public:
								HyperLink();
								HyperLink(const BString& address, Type type);
								HyperLink(const BString& text,
									const BString& address, Type type);
								HyperLink(const BString& address, uint32 ref, const BString& id);

			bool				IsValid() const	{ return !fAddress.IsEmpty(); }

			const BString&		Text() const		{ return fText; }
			const BString&		Address() const		{ return fAddress; }
			Type				GetType() const		{ return fType; }
			const BString&		OSCID() const		{ return fOSCID; }
			const uint32&		OSCRef() const		{ return fOSCRef; }

			status_t			Open();

private:
			BString				fText;
			BString				fAddress;
			Type				fType;
			uint32				fOSCRef;
			BString				fOSCID;
};


#endif	// HYPER_LINK_H
