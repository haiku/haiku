/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "IconView.h"
#include "MimeTypeListView.h"

#include <Bitmap.h>
#include <MessageRunner.h>


// TODO: lazy type collecting (super types only at startup)


const uint32 kMsgAddType = 'adtp';


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


MimeTypeItem::MimeTypeItem(BMimeType& type, bool showIcon, bool flat)
	: BStringItem(type.Type(), !flat && !type.IsSupertypeOnly() ? 1 : 0, false),
	fType(type.Type()),
	fFlat(flat),
	fShowIcon(showIcon)
{
	_SetTo(type);
}


MimeTypeItem::MimeTypeItem(const char* type, bool showIcon, bool flat)
	: BStringItem(type, !flat && strchr(type, '/') != NULL ? 1 : 0, false),
	fType(type),
	fFlat(flat),
	fShowIcon(showIcon)
{
	BMimeType mimeType(type);
	_SetTo(mimeType);
}


MimeTypeItem::~MimeTypeItem()
{
}


void
MimeTypeItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	BFont font;

	if (IsSupertypeOnly()) {
		owner->GetFont(&font);
		BFont boldFont(font);
		boldFont.SetFace(B_BOLD_FACE);
		owner->SetFont(&boldFont);
	}

	BRect rect = frame;
	if (fFlat) {
		// This is where the latch would be - yet can freely consider this
		// as an ugly hack
		rect.left -= 11.0f;
	}

	if (fShowIcon) {
		rgb_color highColor = owner->HighColor();
		rgb_color lowColor = owner->LowColor();

		if (IsSelected() || complete) {
			if (IsSelected())
				owner->SetLowColor(tint_color(lowColor, B_DARKEN_2_TINT));

			owner->FillRect(rect, B_SOLID_LOW);
		}

		BBitmap bitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_RGBA32);
		BMimeType mimeType(fType.String());
		status_t status = icon_for_type(mimeType, bitmap, B_MINI_ICON);
		if (status < B_OK) {
			// get default generic/application icon
			BMimeType genericType(fApplicationMode
				? B_ELF_APP_MIME_TYPE : B_FILE_MIME_TYPE);
			status = icon_for_type(genericType, bitmap, B_MINI_ICON);
		}

		if (status == B_OK) {
			BPoint point(rect.left + 2.0f, 
				rect.top + (rect.Height() - B_MINI_ICON) / 2.0f);

			owner->SetDrawingMode(B_OP_ALPHA);
			owner->DrawBitmap(&bitmap, point);
		}

		owner->SetDrawingMode(B_OP_COPY);

		owner->MovePenTo(rect.left + B_MINI_ICON + 8.0f, frame.top + fBaselineOffset);
		owner->SetHighColor(0, 0, 0);
		owner->DrawString(Text());

		owner->SetHighColor(highColor);
		owner->SetLowColor(lowColor);
	} else
		BStringItem::DrawItem(owner, rect, complete);

	if (IsSupertypeOnly())
		owner->SetFont(&font);
}


void
MimeTypeItem::Update(BView* owner, const BFont* font)
{
	BStringItem::Update(owner, font);

	if (fShowIcon) {
		SetWidth(Width() + B_MINI_ICON + 2.0f);

		if (Height() < B_MINI_ICON + 4.0f)
			SetHeight(B_MINI_ICON + 4.0f);

		font_height fontHeight;
		font->GetHeight(&fontHeight);

		fBaselineOffset = fontHeight.ascent
			+ (Height() - ceilf(fontHeight.ascent + fontHeight.descent)) / 2.0f;
	}
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

	UpdateText();
}


void
MimeTypeItem::UpdateText()
{
	if (IsSupertypeOnly())
		return;

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


void
MimeTypeItem::ShowIcon(bool showIcon)
{
	fShowIcon = showIcon;
}


void
MimeTypeItem::SetApplicationMode(bool applicationMode)
{
	fApplicationMode = applicationMode;
}


/*static*/
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


/*static*/
int
MimeTypeItem::CompareLabels(const BListItem* a, const BListItem* b)
{
	if (a->OutlineLevel() != b->OutlineLevel())
		return a->OutlineLevel() - b->OutlineLevel();

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


MimeTypeListView::MimeTypeListView(const char* name,
		const char* supertype, bool showIcons, bool applicationMode)
	: BOutlineListView(name, B_SINGLE_SELECTION_LIST),
	fSupertype(supertype),
	fShowIcons(showIcons),
	fApplicationMode(applicationMode)
{
}


MimeTypeListView::~MimeTypeListView()
{
}


void
MimeTypeListView::_CollectSubtypes(const char* supertype,
	MimeTypeItem* supertypeItem)
{
	BMessage types;
	if (BMimeType::GetInstalledTypes(supertype, &types) != B_OK)
		return;

	const char* type;
	int32 index = 0;
	while (types.FindString("types", index++, &type) == B_OK) {
		BMimeType mimeType(type);

		bool isApp = mimetype_is_application_signature(mimeType);
		if (fApplicationMode ^ isApp)
			continue;

		MimeTypeItem* typeItem = new MimeTypeItem(mimeType, fShowIcons,
			supertypeItem == NULL);
		typeItem->SetApplicationMode(isApp);

		if (supertypeItem != NULL)
			AddUnder(typeItem, supertypeItem);
		else
			AddItem(typeItem);
	}
}


void
MimeTypeListView::_CollectTypes()
{
	if (fSupertype.Type() != NULL) {
		// only show MIME types that belong to this supertype
		_CollectSubtypes(fSupertype.Type(), NULL);
	} else {
		BMessage superTypes;
		if (BMimeType::GetInstalledSupertypes(&superTypes) != B_OK)
			return;

		const char* supertype;
		int32 index = 0;
		while (superTypes.FindString("super_types", index++, &supertype)
			== B_OK) {
			MimeTypeItem* supertypeItem = new MimeTypeItem(supertype);
			AddItem(supertypeItem);

			_CollectSubtypes(supertype, supertypeItem);
		}
	}

	_MakeTypesUnique();
}


void
MimeTypeListView::_MakeTypesUnique(MimeTypeItem* underItem)
{
	SortItemsUnder(underItem, underItem != NULL, &MimeTypeItem::Compare);

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
MimeTypeListView::_AddNewType(const char* type)
{
	MimeTypeItem* item = FindItem(type);

	BMimeType mimeType(type);
	bool isApp = mimetype_is_application_signature(mimeType);
	if (fApplicationMode ^ isApp || !mimeType.IsInstalled()) {
		if (item != NULL) {
			// type doesn't belong here
			RemoveItem(item);
			delete item;
		}
		return;
	}

	if (item != NULL) {
		// for some reason, the type already exists
		return;
	}

	BMimeType superType;
	MimeTypeItem* superItem = NULL;
	if (mimeType.GetSupertype(&superType) == B_OK)
		superItem = FindItem(superType.Type());

	item = new MimeTypeItem(mimeType, fShowIcons, fSupertype.Type() != NULL);

	if (item->IsSupertypeOnly())
		item->ShowIcon(false);
	item->SetApplicationMode(isApp);

	if (superItem != NULL) {
		AddUnder(item, superItem);
		InvalidateItem(IndexOf(superItem));
			// the super item is not picked up from the class (ie. bug)
	} else
		AddItem(item);

	UpdateItem(item);

	if (!fSelectNewType.ICompare(mimeType.Type())) {
		SelectItem(item);
		fSelectNewType = "";
	}
}


void
MimeTypeListView::AttachedToWindow()
{
	BOutlineListView::AttachedToWindow();

	BMimeType::StartWatching(this);
	_CollectTypes();
}


void
MimeTypeListView::DetachedFromWindow()
{
	BOutlineListView::DetachedFromWindow();
	BMimeType::StopWatching(this);

	// free all items, they will be retrieved again in AttachedToWindow()

	for (int32 i = FullListCountItems(); i-- > 0;) {
		delete FullListItemAt(i);
	}
}


void
MimeTypeListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_META_MIME_CHANGED:
		{
			const char* type;
			int32 which;
			if (message->FindString("be:type", &type) != B_OK
				|| message->FindInt32("be:which", &which) != B_OK)
				break;

			switch (which) {
				case B_SHORT_DESCRIPTION_CHANGED:
				{
					// update label

					MimeTypeItem* item = FindItem(type);
					if (item != NULL)
						UpdateItem(item);
					break;
				}
				case B_MIME_TYPE_CREATED:
				{
					// delay creation of new item a bit, until the type is fully installed

					BMessage addType(kMsgAddType);
					addType.AddString("type", type);

					if (BMessageRunner::StartSending(this, &addType, 200000ULL,
						1) != B_OK) {
						_AddNewType(type);
					}
					break;
				}
				case B_MIME_TYPE_DELETED:
				{
					// delete item
					MimeTypeItem* item = FindItem(type);
					if (item != NULL) {
						RemoveItem(item);
						delete item;
					}
					break;
				}
				case B_PREFERRED_APP_CHANGED:
				{
					// try to add or remove this type (changing the preferred
					// app might change visibility in our list)
					_AddNewType(type);

					// supposed to fall through
				}
				case B_ICON_CHANGED:
				// TODO: take B_ICON_FOR_TYPE_CHANGED into account, too
				{
					MimeTypeItem* item = FindItem(type);
					if (item != NULL && fShowIcons) {
						// refresh item
						InvalidateItem(IndexOf(item));
					}
					break;
				}

				default:
					break;
			}
			break;
		}

		case kMsgAddType:
		{
			const char* type;
			if (message->FindString("type", &type) == B_OK)
				_AddNewType(type);
			break;
		}

		default:
			BOutlineListView::MessageReceived(message);
	}
}


/*!
	\brief This method makes sure a new MIME type will be selected.
	
	If it's not in the list yet, it will be selected as soon as it's
	added.
*/
void
MimeTypeListView::SelectNewType(const char* type)
{
	if (SelectType(type))
		return;

	fSelectNewType = type;
}


bool
MimeTypeListView::SelectType(const char* type)
{
	MimeTypeItem* item = FindItem(type);
	if (item == NULL)
		return false;

	SelectItem(item);
	return true;
}


void
MimeTypeListView::SelectItem(MimeTypeItem* item)
{
	if (item == NULL) {
		Select(-1);
		return;
	}

	// Make sure the item is visible

	BListItem* superItem = item;
	while ((superItem = Superitem(superItem)) != NULL) {
		Expand(superItem);
	}

	// Select it, and make it visible

	int32 index = IndexOf(item);
	Select(index);
	ScrollToSelection();
}


MimeTypeItem*
MimeTypeListView::FindItem(const char* type)
{
	if (type == NULL)
		return NULL;

	for (int32 i = FullListCountItems(); i-- > 0;) {
		MimeTypeItem* item = dynamic_cast<MimeTypeItem*>(FullListItemAt(i));
		if (item == NULL)
			continue;

		if (!strcasecmp(item->Type(), type))
			return item;
	}

	return NULL;
}


void
MimeTypeListView::UpdateItem(MimeTypeItem* item)
{
	int32 selected = -1;
	if (IndexOf(item) == CurrentSelection())
		selected = CurrentSelection();

	item->UpdateText();
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
MimeTypeListView::ShowIcons(bool showIcons)
{
	if (showIcons == fShowIcons)
		return;

	fShowIcons = showIcons;

	if (Window() == NULL)
		return;

	// update items

	BFont font;
	GetFont(&font);

	for (int32 i = FullListCountItems(); i-- > 0;) {
		MimeTypeItem* item = dynamic_cast<MimeTypeItem*>(FullListItemAt(i));
		if (item == NULL)
			continue;

		if (!item->IsSupertypeOnly())
			item->ShowIcon(showIcons);

		item->Update(this, &font);
	}

	FrameResized(Bounds().Width(), Bounds().Height());
		// update scroller

	Invalidate();
}

