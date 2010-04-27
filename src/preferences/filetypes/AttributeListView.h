/*
 * Copyright 2006-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATTRIBUTE_LIST_VIEW_H
#define ATTRIBUTE_LIST_VIEW_H


#include <ListView.h>
#include <Mime.h>
#include <String.h>


class AttributeItem : public BStringItem {
	public:
		AttributeItem(const char* name, const char* publicName, type_code type,
			const char* displayAs, int32 alignment, int32 width, bool visible,
			bool editable);
		AttributeItem();
		AttributeItem(const AttributeItem& other);
		virtual ~AttributeItem();

		virtual void DrawItem(BView* owner, BRect itemRect,
			bool drawEverything = false);

		const char* Name() const { return fName.String(); }
		const char* PublicName() const { return Text(); }

		type_code Type() const { return fType; }
		const char* DisplayAs() const { return fDisplayAs.String(); }
		int32 Alignment() const { return fAlignment; }
		int32 Width() const { return fWidth; }
		bool Visible() const { return fVisible; }
		bool Editable() const { return fEditable; }

		AttributeItem& operator=(const AttributeItem& other);

		bool operator==(const AttributeItem& other) const;
		bool operator!=(const AttributeItem& other) const;

	private:
		BString		fName;
		type_code	fType;
		BString		fDisplayAs;
		int32		fAlignment;
		int32		fWidth;
		bool		fVisible;
		bool		fEditable;
};

class AttributeListView : public BListView {
	public:
		AttributeListView(const char* name);
		virtual ~AttributeListView();

		void SetTo(BMimeType* type);

		virtual void Draw(BRect updateRect);

	private:
		void _DeleteItems();
};

struct type_map {
	const char*	name;
	type_code	type;
};

extern const struct type_map kTypeMap[];

struct display_as_map {
	const char* name;
	const char* identifier;
	type_code	supported[8];
};

extern const struct display_as_map kDisplayAsMap[];

AttributeItem* create_attribute_item(BMessage& attributes, int32 index);

#endif	// ATTRIBUTE_LIST_VIEW_H
