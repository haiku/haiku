/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MIME_TYPE_LIST_VIEW_H
#define MIME_TYPE_LIST_VIEW_H


#include <OutlineListView.h>
#include <String.h>

class BMimeType;


class MimeTypeItem : public BStringItem {
	public:
		MimeTypeItem(BMimeType& type, bool flat = false);
		MimeTypeItem(const char* type, bool flat = false);
		virtual ~MimeTypeItem();

		virtual void DrawItem(BView* owner, BRect itemRect,
			bool drawEverything = false);

		const char* Type() const { return fType.String(); }
		const char* Subtype() const { return fSubtype.String(); }
		const char* Supertype() const { return fSupertype.String(); }
		bool IsSupertypeOnly() const { return fIsSupertype; }

		void AddSubtype();

		static int Compare(const BListItem* a, const BListItem* b);

	private:
		void _SetTo(BMimeType& type);

		BString		fSupertype;
		BString		fSubtype;
		BString		fType;
		bool		fIsSupertype;
};

class MimeTypeListView : public BOutlineListView {
	public:
		MimeTypeListView(BRect rect, const char* name,
			uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP);
		virtual ~MimeTypeListView();

	private:
		void _CollectTypes();
};

bool mimetype_is_application_signature(BMimeType& type);

#endif	// MIME_TYPE_LIST_VIEW_H
