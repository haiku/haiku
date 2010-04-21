/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MIME_TYPE_LIST_VIEW_H
#define MIME_TYPE_LIST_VIEW_H


#include <Mime.h>
#include <OutlineListView.h>
#include <String.h>


class MimeTypeItem : public BStringItem {
	public:
		MimeTypeItem(BMimeType& type, bool showIcon = false, bool flat = false);
		MimeTypeItem(const char* type, bool showIcon = false, bool flat = false);
		virtual ~MimeTypeItem();

		virtual void DrawItem(BView* owner, BRect itemRect,
			bool drawEverything = false);
		virtual void Update(BView* owner, const BFont* font);

		const char* Type() const { return fType.String(); }
		const char* Subtype() const { return fSubtype.String(); }
		const char* Supertype() const { return fSupertype.String(); }
		const char* Description() const { return fDescription.String(); }
		bool IsSupertypeOnly() const { return fIsSupertype; }

		void UpdateText();
		void AddSubtype();

		void ShowIcon(bool showIcon);
		void SetApplicationMode(bool applicationMode);

		static int Compare(const BListItem* a, const BListItem* b);
		static int CompareLabels(const BListItem* a, const BListItem* b);

	private:
		void _SetTo(BMimeType& type);

		BString		fSupertype;
		BString		fSubtype;
		BString		fType;
		BString		fDescription;
		float		fBaselineOffset;
		bool		fIsSupertype;
		bool		fFlat;
		bool		fShowIcon;
		bool		fApplicationMode;
};

class MimeTypeListView : public BOutlineListView {
	public:
		MimeTypeListView(const char* name,
			const char* supertype = NULL, bool showIcons = false,
			bool applicationMode = false);
		virtual ~MimeTypeListView();

		void SelectNewType(const char* type);
		bool SelectType(const char* type);

		void SelectItem(MimeTypeItem* item);
		MimeTypeItem* FindItem(const char* type);

		void UpdateItem(MimeTypeItem* item);

		void ShowIcons(bool showIcons);
		bool IsShowingIcons() const { return fShowIcons; }

	protected:
		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();

		virtual void MessageReceived(BMessage* message);

	private:
		void _CollectSubtypes(const char* supertype, MimeTypeItem* supertypeItem);
		void _CollectTypes();
		void _MakeTypesUnique(MimeTypeItem* underItem = NULL);
		void _AddNewType(const char* type);

		BMimeType	fSupertype;
		BString		fSelectNewType;
		bool		fShowIcons;
		bool		fApplicationMode;
};

extern bool mimetype_is_application_signature(BMimeType& type);

#endif	// MIME_TYPE_LIST_VIEW_H
