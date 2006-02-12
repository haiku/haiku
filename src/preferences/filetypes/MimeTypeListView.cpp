/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "MimeTypeListView.h"

#include <Mime.h>


// TODO: lazy type collecting (super types only at startup)


bool
mimetype_is_application_signature(BMimeType& type)
{
	char preferredApp[B_MIME_TYPE_LENGTH];

	// The preferred application of an application is the same
	// as its signature.

	return type.GetPreferredApp(preferredApp) == B_OK
		&& !strcasecmp(type.Type(), preferredApp);
}


//	#pragma mark -


MimeTypeItem::MimeTypeItem(BMimeType& type, bool flat)
	: BStringItem(type.Type(), !flat && type.IsSupertypeOnly() ? 1 : 0, false),
	fType(type.Type())
{
	_SetTo(type);
}


MimeTypeItem::MimeTypeItem(const char* type, bool flat)
	: BStringItem(type, !flat && strchr(type, '/') != NULL ? 1 : 0, false),
	fType(type)
{
	BMimeType mimeType(type);
	_SetTo(mimeType);
}


MimeTypeItem::~MimeTypeItem()
{
}


void
MimeTypeItem::DrawItem(BView* owner, BRect itemRect, bool drawEverything)
{
	BFont font;

	if (IsSupertypeOnly()) {
		owner->GetFont(&font);
		BFont boldFont(font);
		boldFont.SetFace(B_BOLD_FACE);
		owner->SetFont(&boldFont);
	}

	BStringItem::DrawItem(owner, itemRect, drawEverything);

	if (IsSupertypeOnly())
		owner->SetFont(&font);
}


void
MimeTypeItem::_SetTo(BMimeType& type)
{
	fIsSupertype = type.IsSupertypeOnly();

	if (IsSupertypeOnly()) {
		// this is a super type
		fSupertype = type.Type();
		fDescription = type.Type();
		return;
	}

	const char* subType = strchr(type.Type(), '/');
	fSupertype.SetTo(type.Type(), subType - type.Type());
	fSubtype.SetTo(subType + 1);
		// omit the slash

	Update();
}


void
MimeTypeItem::Update()
{
	BMimeType type(fType.String());

	char description[B_MIME_TYPE_LENGTH];
	if (type.GetShortDescription(description) == B_OK)
		SetText(description);
	else
		SetText(Subtype());

	fDescription = Text();
}


void
MimeTypeItem::AddSubtype()
{
	if (fSubtype == Text())
		return;

	BString text = Description();
	text.Append(" (");
	text.Append(fSubtype);
	text.Append(")");

	SetText(text.String());
}


int
MimeTypeItem::Compare(const BListItem* a, const BListItem* b)
{
	const MimeTypeItem* typeA = dynamic_cast<const MimeTypeItem*>(a);
	const MimeTypeItem* typeB = dynamic_cast<const MimeTypeItem*>(b);

	if (typeA != NULL && typeB != NULL) {
		int compare = strcasecmp(typeA->Supertype(), typeB->Supertype());
		if (compare != 0)
			return compare;
	}

	const BStringItem* stringA = dynamic_cast<const BStringItem*>(a);
	const BStringItem* stringB = dynamic_cast<const BStringItem*>(b);

	if (stringA != NULL && stringB != NULL)
		return strcasecmp(stringA->Text(), stringB->Text());

	return (int)(a - b);
}


int
MimeTypeItem::CompareLabels(const BListItem* a, const BListItem* b)
{
	const MimeTypeItem* typeA = dynamic_cast<const MimeTypeItem*>(a);
	const MimeTypeItem* typeB = dynamic_cast<const MimeTypeItem*>(b);

	if (typeA != NULL && typeB != NULL) {
		int compare = strcasecmp(typeA->Description(), typeB->Description());
		if (compare != 0)
			return compare;
	}

	const BStringItem* stringA = dynamic_cast<const BStringItem*>(a);
	const BStringItem* stringB = dynamic_cast<const BStringItem*>(b);

	if (stringA != NULL && stringB != NULL)
		return strcasecmp(stringA->Text(), stringB->Text());

	return (int)(a - b);
}


//	#pragma mark -


MimeTypeListView::MimeTypeListView(BRect rect, const char* name,
		uint32 resizingMode)
	: BOutlineListView(rect, name, B_SINGLE_SELECTION_LIST, resizingMode)
{
	_CollectTypes();
}


MimeTypeListView::~MimeTypeListView()
{
	// free items, stupid BOutlineListView doesn't do this itself

	for (int32 i = FullListCountItems(); i-- > 0;) {
		delete FullListItemAt(i);
	}
}


void
MimeTypeListView::_CollectTypes()
{
	BMessage superTypes;
	if (BMimeType::GetInstalledSupertypes(&superTypes) != B_OK)
		return;

	const char* superType;
	int32 index = 0;
	while (superTypes.FindString("super_types", index++, &superType) == B_OK) {
		BStringItem* superTypeItem = new MimeTypeItem(superType);
		AddItem(superTypeItem);

		BMessage types;
		if (BMimeType::GetInstalledTypes(superType, &types) != B_OK)
			continue;

		const char* type;
		int32 typeIndex = 0;
		while (types.FindString("types", typeIndex++, &type) == B_OK) {
			BMimeType mimeType(type);

			if (mimetype_is_application_signature(mimeType))
				continue;

			BStringItem* typeItem = new MimeTypeItem(mimeType);
			AddUnder(typeItem, superTypeItem);
		}
	}

	_MakeTypesUnique();
}


void
MimeTypeListView::_MakeTypesUnique(MimeTypeItem* underItem)
{
	SortItemsUnder(underItem, false, &MimeTypeItem::Compare);

	bool lastItemSame = false;
	MimeTypeItem* last = NULL;

	int32 index = 0;
	uint32 level = 0;
	if (underItem != NULL) {
		index = FullListIndexOf(underItem) + 1;
		level = underItem->OutlineLevel() + 1;
	}

	for (; index < FullListCountItems(); index++) {
		MimeTypeItem* item = dynamic_cast<MimeTypeItem*>(FullListItemAt(index));
		if (item == NULL)
			continue;

		if (item->OutlineLevel() < level) {
			// left sub-tree
			break;
		}

		item->SetText(item->Description());

		if (last == NULL || MimeTypeItem::CompareLabels(last, item)) {
			if (lastItemSame) {
				last->AddSubtype();
				if (Window())
					InvalidateItem(IndexOf(last));
			}

			lastItemSame = false;
			last = item;
			continue;
		}

		lastItemSame = true;
		last->AddSubtype();
		if (Window())
			InvalidateItem(IndexOf(last));
		last = item;
	}

	if (lastItemSame) {
		last->AddSubtype();
		if (Window())
			InvalidateItem(IndexOf(last));
	}
}


void
MimeTypeListView::UpdateItem(MimeTypeItem* item)
{
	int32 selected = -1;
	if (IndexOf(item) == CurrentSelection())
		selected = CurrentSelection();

	item->Update();
	_MakeTypesUnique(dynamic_cast<MimeTypeItem*>(Superitem(item)));

	if (selected != -1) {
		int32 index = IndexOf(item);
		if (index != selected) {
			Select(index);
			ScrollToSelection();
		}
	}
	if (Window())
		InvalidateItem(IndexOf(item));
}


void
MimeTypeListView::RemoveItem(MimeTypeItem* item)
{
}

