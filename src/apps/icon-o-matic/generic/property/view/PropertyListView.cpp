/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "PropertyListView.h"

#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <Clipboard.h>
#ifdef __HAIKU__
#  include <LayoutUtils.h>
#endif
#include <Locale.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Window.h>

#include "CommonPropertyIDs.h"
//#include "LanguageManager.h"
#include "Property.h"
#include "PropertyItemView.h"
#include "PropertyObject.h"
#include "Scrollable.h"
#include "Scroller.h"
#include "ScrollView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-Properties"


enum {
	MSG_COPY_PROPERTIES		= 'cppr',
	MSG_PASTE_PROPERTIES	= 'pspr',

	MSG_ADD_KEYFRAME		= 'adkf',

	MSG_SELECT_ALL			= B_SELECT_ALL,
	MSG_SELECT_NONE			= 'slnn',
	MSG_INVERT_SELECTION	= 'invs',
};

// TabFilter class

class TabFilter : public BMessageFilter {
 public:
	TabFilter(PropertyListView* target)
		: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
		  fTarget(target)
		{
		}
	virtual	~TabFilter()
		{
		}
	virtual	filter_result	Filter(BMessage* message, BHandler** target)
		{
			filter_result result = B_DISPATCH_MESSAGE;
			switch (message->what) {
				case B_UNMAPPED_KEY_DOWN:
				case B_KEY_DOWN: {
					uint32 key;
					uint32 modifiers;
					if (message->FindInt32("raw_char", (int32*)&key) >= B_OK
						&& message->FindInt32("modifiers", (int32*)&modifiers) >= B_OK)
						if (key == B_TAB && fTarget->TabFocus(modifiers & B_SHIFT_KEY))
							result = B_SKIP_MESSAGE;
					break;
				}
				default:
					break;
			}
			return result;
		}
 private:
 	PropertyListView*		fTarget;
};


// constructor
PropertyListView::PropertyListView()
	: BView(BRect(0.0, 0.0, 100.0, 100.0), NULL, B_FOLLOW_NONE,
			B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
	  Scrollable(),
	  BList(20),
	  fClipboard(new BClipboard("icon-o-matic properties")),

	  fPropertyM(NULL),

	  fPropertyObject(NULL),
	  fSavedProperties(new PropertyObject()),

	  fLastClickedItem(NULL),
	  fSuspendUpdates(false),

	  fMouseWheelFilter(new MouseWheelFilter(this)),
	  fTabFilter(new TabFilter(this))
{
	SetLowColor(255, 255, 255, 255);
	SetHighColor(0, 0, 0, 255);
	SetViewColor(B_TRANSPARENT_32_BIT);
}

// destructor
PropertyListView::~PropertyListView()
{
	delete fClipboard;

	delete fPropertyObject;
	delete fSavedProperties;

	delete fMouseWheelFilter;
	delete fTabFilter;
}

// AttachedToWindow
void
PropertyListView::AttachedToWindow()
{
	Window()->AddCommonFilter(fMouseWheelFilter);
	Window()->AddCommonFilter(fTabFilter);
}

// DetachedFromWindow
void
PropertyListView::DetachedFromWindow()
{
	Window()->RemoveCommonFilter(fTabFilter);
	Window()->RemoveCommonFilter(fMouseWheelFilter);
}

// FrameResized
void
PropertyListView::FrameResized(float width, float height)
{
	SetVisibleSize(width, height);
	Invalidate();
}

// Draw
void
PropertyListView::Draw(BRect updateRect)
{
	if (!fSuspendUpdates)
		FillRect(updateRect, B_SOLID_LOW);
}

// MakeFocus
void
PropertyListView::MakeFocus(bool focus)
{
	if (focus == IsFocus())
		return;

	BView::MakeFocus(focus);
	if (::ScrollView* scrollView = dynamic_cast< ::ScrollView*>(Parent()))
		scrollView->ChildFocusChanged(focus);
}

// MouseDown
void
PropertyListView::MouseDown(BPoint where)
{
	if (!(modifiers() & B_SHIFT_KEY)) {
		DeselectAll();
	}
	MakeFocus(true);
}

// MessageReceived
void
PropertyListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PASTE_PROPERTIES: {
			if (!fPropertyObject || !fClipboard->Lock())
				break;

			BMessage* data = fClipboard->Data();
			if (!data) {
				fClipboard->Unlock();
				break;
			}

			PropertyObject propertyObject;
			BMessage archive;
			for (int32 i = 0;
				 data->FindMessage("property", i, &archive) >= B_OK;
				 i++) {
				BArchivable* archivable = instantiate_object(&archive);
				if (!archivable)
					continue;
				// see if this is actually a property
				Property* property = dynamic_cast<Property*>(archivable);
				if (property == NULL || !propertyObject.AddProperty(property))
					delete archivable;
			}
			if (propertyObject.CountProperties() > 0)
				PasteProperties(&propertyObject);
			fClipboard->Unlock();
			break;
		}
		case MSG_COPY_PROPERTIES: {
			if (!fPropertyObject || !fClipboard->Lock())
				break;

			BMessage* data = fClipboard->Data();
			if (!data) {
				fClipboard->Unlock();
				break;
			}

			fClipboard->Clear();
			for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
				if (!item->IsSelected())
					continue;
				const Property* property = item->GetProperty();
				if (property) {
					BMessage archive;
					if (property->Archive(&archive) >= B_OK) {
						data->AddMessage("property", &archive);
					}
				}
			}
			fClipboard->Commit();
			fClipboard->Unlock();
			_CheckMenuStatus();
			break;
		}

		// property selection
		case MSG_SELECT_ALL:
			for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
				item->SetSelected(true);
			}
			_CheckMenuStatus();
			break;
		case MSG_SELECT_NONE:
			for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
				item->SetSelected(false);
			}
			_CheckMenuStatus();
			break;
		case MSG_INVERT_SELECTION:
			for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
				item->SetSelected(!item->IsSelected());
			}
			_CheckMenuStatus();
			break;

		default:
			BView::MessageReceived(message);
	}
}

#ifdef __HAIKU__

BSize
PropertyListView::MinSize()
{
	// We need a stable min size: the BView implementation uses
	// GetPreferredSize(), which by default just returns the current size.
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(10, 10));
}


BSize
PropertyListView::MaxSize()
{
	return BView::MaxSize();
}


BSize
PropertyListView::PreferredSize()
{
	// We need a stable preferred size: the BView implementation uses
	// GetPreferredSize(), which by default just returns the current size.
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), BSize(100, 50));
}

#endif // __HAIKU__

// #pragma mark -

// TabFocus
bool
PropertyListView::TabFocus(bool shift)
{
	bool result = false;
	PropertyItemView* item = NULL;
	if (IsFocus() && !shift) {
		item = _ItemAt(0);
	} else {
		int32 focussedIndex = -1;
		for (int32 i = 0; PropertyItemView* oldItem = _ItemAt(i); i++) {
			if (oldItem->IsFocused()) {
				focussedIndex = shift ? i - 1 : i + 1;
				break;
			}
		}
		item = _ItemAt(focussedIndex);
	}
	if (item) {
		item->MakeFocus(true);
		result = true;
	}
	return result;
}

// SetMenu
void
PropertyListView::SetMenu(BMenu* menu)
{
	fPropertyM = menu;
	if (!fPropertyM)
		return;

	fSelectM = new BMenu(B_TRANSLATE("Select"));
	fSelectAllMI = new BMenuItem(B_TRANSLATE("All"), 
		new BMessage(MSG_SELECT_ALL));
	fSelectM->AddItem(fSelectAllMI);
	fSelectNoneMI = new BMenuItem(B_TRANSLATE("None"),
		new BMessage(MSG_SELECT_NONE));
	fSelectM->AddItem(fSelectNoneMI);
	fInvertSelectionMI = new BMenuItem(B_TRANSLATE("Invert selection"),
		new BMessage(MSG_INVERT_SELECTION));
	fSelectM->AddItem(fInvertSelectionMI);
	fSelectM->SetTargetForItems(this);

	fPropertyM->AddItem(fSelectM);

	fPropertyM->AddSeparatorItem();

	fCopyMI = new BMenuItem(B_TRANSLATE("Copy"), 
		new BMessage(MSG_COPY_PROPERTIES));
	fPropertyM->AddItem(fCopyMI);
	fPasteMI = new BMenuItem(B_TRANSLATE("Paste"), 
		new BMessage(MSG_PASTE_PROPERTIES));
	fPropertyM->AddItem(fPasteMI);

	fPropertyM->SetTargetForItems(this);

	// disable menus
	_CheckMenuStatus();
}

// UpdateStrings
void
PropertyListView::UpdateStrings()
{
//	if (fSelectM) {
//		LanguageManager* m = LanguageManager::Default();
//	
//		fSelectM->Superitem()->SetLabel(m->GetString(PROPERTY_SELECTION, "Select"));
//		fSelectAllMI->SetLabel(m->GetString(SELECT_ALL_PROPERTIES, "All"));
//		fSelectNoneMI->SetLabel(m->GetString(SELECT_NO_PROPERTIES, "None"));
//		fInvertSelectionMI->SetLabel(m->GetString(INVERT_SELECTION, "Invert Selection"));
//	
//		fPropertyM->Superitem()->SetLabel(m->GetString(PROPERTY, "Property"));
//		fCopyMI->SetLabel(m->GetString(COPY, "Copy"));
//		if (IsEditingMultipleObjects())
//			fPasteMI->SetLabel(m->GetString(MULTI_PASTE, "Multi Paste"));
//		else
//			fPasteMI->SetLabel(m->GetString(PASTE, "Paste"));
//	}
}

// ScrollView
::ScrollView*
PropertyListView::ScrollView() const
{
	return dynamic_cast< ::ScrollView*>(ScrollSource());
}

// #pragma mark -

// SetTo
void
PropertyListView::SetTo(PropertyObject* object)
{
	// try to do without rebuilding the list
	// it should in fact be pretty unlikely that this does not
	// work, but we keep being defensive
	if (fPropertyObject && object &&
		fPropertyObject->ContainsSameProperties(*object)) {
		// iterate over view items and update their value views
		bool error = false;
		for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
			Property* property = object->PropertyAt(i);
			if (!item->AdoptProperty(property)) {
				// the reason for this can be that the property is
				// unkown to the PropertyEditorFactory and therefor
				// there is no editor view at this item
				fprintf(stderr, "PropertyListView::_SetTo() - "
								"property mismatch at %ld\n", i);
				error = true;
				break;
			}
			if (property)
				item->SetEnabled(property->IsEditable());
		}
		// we didn't need to make empty, but transfer ownership
		// of the object
		if (!error) {
			// if the "adopt" process went only halfway,
			// some properties of the original object
			// are still referenced, so we can only
			// delete the original object if the process
			// was successful and leak Properties otherwise,
			// but this case is only theoretical anyways...
			delete fPropertyObject;
		}
		fPropertyObject = object;
	} else {
		// remember scroll pos, selection and focused item
		BPoint scrollOffset = ScrollOffset();
		BList selection(20);
		int32 focused = -1;
		for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
			if (item->IsSelected())
				selection.AddItem((void*)i);
			if (item->IsFocused())
				focused = i;
		}
		if (Window())
			Window()->BeginViewTransaction();
		fSuspendUpdates = true;

		// rebuild list
		_MakeEmpty();
		fPropertyObject = object;

		if (fPropertyObject) {
			// fill with content
			for (int32 i = 0; Property* property = fPropertyObject->PropertyAt(i); i++) {
				PropertyItemView* item = new PropertyItemView(property);
				item->SetEnabled(property->IsEditable());
				_AddItem(item);
			}
			_LayoutItems();

			// restore scroll pos, selection and focus
			SetScrollOffset(scrollOffset);
			for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
				if (selection.HasItem((void*)i))
					item->SetSelected(true);
				if (i == focused)
					item->MakeFocus(true);
			}
		}

		if (Window())
			Window()->EndViewTransaction();
		fSuspendUpdates = false;

		SetDataRect(_ItemsRect());
	}

	_UpdateSavedProperties();
	_CheckMenuStatus();
	Invalidate();
}

// PropertyChanged
void
PropertyListView::PropertyChanged(const Property* previous,
								  const Property* current)
{
	printf("PropertyListView::PropertyChanged(%s)\n",
		name_for_id(current->Identifier()));
}

// PasteProperties
void
PropertyListView::PasteProperties(const PropertyObject* object)
{
	if (!fPropertyObject)
		return;

	// default implementation is to adopt the pasted properties
	int32 count = object->CountProperties();
	for (int32 i = 0; i < count; i++) {
		Property* p = object->PropertyAtFast(i);
		Property* local = fPropertyObject->FindProperty(p->Identifier());
		if (local)
			local->SetValue(p);
	}
}

// IsEditingMultipleObjects
bool
PropertyListView::IsEditingMultipleObjects()
{
	return false;
}

// #pragma mark -

// UpdateObject
void
PropertyListView::UpdateObject(uint32 propertyID)
{
	Property* previous = fSavedProperties->FindProperty(propertyID);
	Property* current = fPropertyObject->FindProperty(propertyID);
	if (previous && current) {
		// call hook function
		PropertyChanged(previous, current);
		// update saved property if it is still contained
		// in the saved properties (if not, the notification
		// mechanism has caused to update the properties
		// and "previous" and "current" are toast)
		if (fSavedProperties->HasProperty(previous)
			&& fPropertyObject->HasProperty(current))
			previous->SetValue(current);
	}
}

// ScrollOffsetChanged
void
PropertyListView::ScrollOffsetChanged(BPoint oldOffset, BPoint newOffset)
{
	ScrollBy(newOffset.x - oldOffset.x,
			 newOffset.y - oldOffset.y);
}

// Select
void
PropertyListView::Select(PropertyItemView* item)
{
	if (item) {
		if (modifiers() & B_SHIFT_KEY) {
			item->SetSelected(!item->IsSelected());
		} else if (modifiers() & B_OPTION_KEY) {
			item->SetSelected(true);
			int32 firstSelected = _CountItems();
			int32 lastSelected = -1;
			for (int32 i = 0; PropertyItemView* otherItem = _ItemAt(i); i++) {
				if (otherItem->IsSelected()) {
					 if (i < firstSelected)
					 	firstSelected = i;
					 if (i > lastSelected)
					 	lastSelected = i;
				}
			}
			if (lastSelected > firstSelected) {
				for (int32 i = firstSelected; PropertyItemView* otherItem = _ItemAt(i); i++) {
					if (i > lastSelected)
						break;
					otherItem->SetSelected(true);
				}
			}
		} else {
			for (int32 i = 0; PropertyItemView* otherItem = _ItemAt(i); i++) {
				otherItem->SetSelected(otherItem == item);
			}
		}
	}
	_CheckMenuStatus();
}

// DeselectAll
void
PropertyListView::DeselectAll()
{
	for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
		item->SetSelected(false);
	}
	_CheckMenuStatus();
}

// Clicked
void
PropertyListView::Clicked(PropertyItemView* item)
{
	fLastClickedItem = item;
}

// DoubleClicked
void
PropertyListView::DoubleClicked(PropertyItemView* item)
{
	if (fLastClickedItem == item) {
		printf("implement PropertyListView::DoubleClicked()\n");
	}
	fLastClickedItem = NULL;
}

// #pragma mark -

// _UpdateSavedProperties
void
PropertyListView::_UpdateSavedProperties()
{
	fSavedProperties->DeleteProperties();

	if (!fPropertyObject)
		return;

	int32 count = fPropertyObject->CountProperties();
	for (int32 i = 0; i < count; i++) {
		const Property* p = fPropertyObject->PropertyAtFast(i);
		fSavedProperties->AddProperty(p->Clone());
	}
}

// _AddItem
bool
PropertyListView::_AddItem(PropertyItemView* item)
{
	if (item && BList::AddItem((void*)item)) {
//		AddChild(item);
// NOTE: for now added in _LayoutItems()
		item->SetListView(this);
		return true;
	}
	return false;
}

// _RemoveItem
PropertyItemView*
PropertyListView::_RemoveItem(int32 index)
{
	PropertyItemView* item = (PropertyItemView*)BList::RemoveItem(index);
	if (item) {
		item->SetListView(NULL);
		if (!RemoveChild(item))
			fprintf(stderr, "failed to remove view in PropertyListView::_RemoveItem()\n");
	}
	return item;
}

// _ItemAt
PropertyItemView*
PropertyListView::_ItemAt(int32 index) const
{
	return (PropertyItemView*)BList::ItemAt(index);
}

// _CountItems
int32
PropertyListView::_CountItems() const
{
	return BList::CountItems();
}

// _MakeEmpty
void
PropertyListView::_MakeEmpty()
{
	int32 count = _CountItems();
	while (PropertyItemView* item = _RemoveItem(count - 1)) {
		delete item;
		count--;
	}
	delete fPropertyObject;
	fPropertyObject = NULL;

	SetScrollOffset(BPoint(0.0, 0.0));
}

// _ItemsRect
BRect
PropertyListView::_ItemsRect() const
{
	float width = Bounds().Width();
	float height = -1.0;
	for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
		height += item->PreferredHeight() + 1.0;
	}
	if (height < 0.0)
		height = 0.0;
	return BRect(0.0, 0.0, width, height);
}

// _LayoutItems
void
PropertyListView::_LayoutItems()
{
	// figure out maximum label width
	float labelWidth = 0.0;
	for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
		if (item->PreferredLabelWidth() > labelWidth)
			labelWidth = item->PreferredLabelWidth();
	}
	labelWidth = ceilf(labelWidth);
	// layout items
	float top = 0.0;
	float width = Bounds().Width();
	for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
		item->MoveTo(BPoint(0.0, top));
		float height = item->PreferredHeight();
		item->SetLabelWidth(labelWidth);
		item->ResizeTo(width, height);
		item->FrameResized(item->Bounds().Width(),
						   item->Bounds().Height());
		top += height + 1.0;

		AddChild(item);
	}
}

// _CheckMenuStatus
void
PropertyListView::_CheckMenuStatus()
{
	if (!fPropertyM || fSuspendUpdates)
		return;

	if (!fPropertyObject) {
		fPropertyM->SetEnabled(false);
		return;
	} else
		fPropertyM->SetEnabled(false);

	bool gotSelection = false;
	for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
		if (item->IsSelected()) {
			gotSelection = true;
			break;
		}
	}
	fCopyMI->SetEnabled(gotSelection);

	bool clipboardHasData = false;
	if (fClipboard->Lock()) {
		if (BMessage* data = fClipboard->Data()) {
			clipboardHasData = data->HasMessage("property");
		}
		fClipboard->Unlock();
	}

	fPasteMI->SetEnabled(clipboardHasData);
//	LanguageManager* m = LanguageManager::Default();
	if (IsEditingMultipleObjects())
//		fPasteMI->SetLabel(m->GetString(MULTI_PASTE, "Multi paste"));
		fPasteMI->SetLabel(B_TRANSLATE("Multi paste"));
	else
//		fPasteMI->SetLabel(m->GetString(PASTE, "Paste"));
		fPasteMI->SetLabel(B_TRANSLATE("Paste"));

	bool enableMenu = fPropertyObject;
	if (fPropertyM->IsEnabled() != enableMenu)
		fPropertyM->SetEnabled(enableMenu);

	bool gotItems = _CountItems() > 0;
	fSelectM->SetEnabled(gotItems);
}


