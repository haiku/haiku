/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "FilterConfigView.h"

#include <Alert.h>
#include <Bitmap.h>
#include <Box.h>
#include <Catalog.h>
#include <Locale.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <ScrollView.h>

#include "CenterContainer.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Config Views"


// FiltersConfigView
const uint32 kMsgFilterMoved = 'flmv';
const uint32 kMsgChainSelected = 'chsl';
const uint32 kMsgAddFilter = 'addf';
const uint32 kMsgRemoveFilter = 'rmfi';
const uint32 kMsgFilterSelected = 'fsel';

const uint32 kMsgItemDragged = 'itdr';


class DragListView : public BListView {
public:
	DragListView(BRect frame, const char *name,
			list_view_type type = B_SINGLE_SELECTION_LIST,
			 uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			 BMessage *itemMovedMsg = NULL)
		:
		BListView(frame, name, type, resizingMode),
		fDragging(false),
		fItemMovedMessage(itemMovedMsg)
	{
	}

	virtual bool InitiateDrag(BPoint point, int32 index, bool wasSelected)
	{
		BRect frame(ItemFrame(index));
		BBitmap *bitmap = new BBitmap(frame.OffsetToCopy(B_ORIGIN), B_RGBA32,
			true);
		BView *view = new BView(bitmap->Bounds(), NULL, 0, 0);
		bitmap->AddChild(view);

		if (view->LockLooper()) {
			BListItem *item = ItemAt(index);
			bool selected = item->IsSelected();

			view->SetLowColor(225, 225, 225, 128);
			view->FillRect(view->Bounds());

			if (selected)
				item->Deselect();
			ItemAt(index)->DrawItem(view, view->Bounds(), true);
			if (selected)
				item->Select();

			view->UnlockLooper();
		}
		fLastDragTarget = -1;
		fDragIndex = index;
		fDragging = true;

		BMessage drag(kMsgItemDragged);
		drag.AddInt32("index", index);
		DragMessage(&drag, bitmap, B_OP_ALPHA, point - frame.LeftTop(), this);

		return true;
	}

	void DrawDragTargetIndicator(int32 target)
	{
		PushState();
		SetDrawingMode(B_OP_INVERT);

		bool last = false;
		if (target >= CountItems())
			target = CountItems() - 1, last = true;

		BRect frame = ItemFrame(target);
		if (last)
			frame.OffsetBy(0,frame.Height());
		frame.bottom = frame.top + 1;

		FillRect(frame);

		PopState();
	}

	virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *msg)
	{
		BListView::MouseMoved(point, transit, msg);

		if ((transit != B_ENTERED_VIEW && transit != B_INSIDE_VIEW)
			|| !fDragging)
			return;

		int32 target = IndexOf(point);
		if (target == -1)
			target = CountItems();

		// correct the target insertion index
		if (target == fDragIndex || target == fDragIndex + 1)
			target = -1;

		if (target == fLastDragTarget)
			return;

		// remove old target indicator
		if (fLastDragTarget != -1)
			DrawDragTargetIndicator(fLastDragTarget);

		// draw new one
		fLastDragTarget = target;
		if (target != -1)
			DrawDragTargetIndicator(target);
	}

	virtual void MouseUp(BPoint point)
	{
		if (fDragging) {
			fDragging = false;
			if (fLastDragTarget != -1)
				DrawDragTargetIndicator(fLastDragTarget);
		}
		BListView::MouseUp(point);
	}

	virtual void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
			case kMsgItemDragged:
			{
				int32 source = msg->FindInt32("index");
				BPoint point = msg->FindPoint("_drop_point_");
				ConvertFromScreen(&point);
				int32 to = IndexOf(point);
				if (to > fDragIndex)
					to--;
				if (to == -1)
					to = CountItems() - 1;

				if (source != to) {
					MoveItem(source,to);

					if (fItemMovedMessage != NULL) {
						BMessage msg(fItemMovedMessage->what);
						msg.AddInt32("from",source);
						msg.AddInt32("to",to);
						Messenger().SendMessage(&msg);
					}
				}
				break;
			}
		}
		BListView::MessageReceived(msg);
	}

private:
	bool		fDragging;
	int32		fLastDragTarget,fDragIndex;
	BMessage	*fItemMovedMessage;
};


//	#pragma mark -


class FilterConfigBox : public BBox {
public:
	FilterConfigBox(BString& label, BView* child)
		:
		BBox(BRect(0,0,100,100)),
		fChild(child)
	{
		SetLabel(label);
		float w = child->Bounds().Width();
		float h = child->Bounds().Height();
		child->MoveTo(3, 13);
		ResizeTo(w + 6, h + 16);
		AddChild(child);
	}

	status_t
	ArchiveAddon(BMessage* into) const
	{
		return fChild->Archive(into);
	}

private:
			BView*				fChild;
};

    
//	#pragma mark -


FiltersConfigView::FiltersConfigView(BRect rect, BMailAccountSettings& account)
	:
	BBox(rect),
	fAccount(account),
	fDirection(kIncomming),
	fInboundFilters(kIncomming, false),
	fOutboundFilters(kOutgoing, false),
	fFilterView(NULL),
	fCurrentIndex(-1)
{
	BPopUpMenu *menu = new BPopUpMenu(B_EMPTY_STRING);

	BMessage* msg;
	BMenuItem *item;
	msg = new BMessage(kMsgChainSelected);
	item = new BMenuItem(B_TRANSLATE("Incoming mail filters"), msg);
	menu->AddItem(item);
	msg->AddInt32("direction", kIncomming);
	item->SetMarked(true);

	msg = new BMessage(kMsgChainSelected);
	item = new BMenuItem(B_TRANSLATE("Outgoing mail filters"), msg);
	menu->AddItem(item);
	msg->AddInt32("direction", kOutgoing);

	fChainsField = new BMenuField(BRect(0, 0, 200, 40), NULL, NULL, menu);
	fChainsField->ResizeToPreferred();
	SetLabel(fChainsField);

	// determine font height
	font_height fontHeight;
	fChainsField->GetFontHeight(&fontHeight);
	int32 height = (int32)(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading) + 5;

	rect = Bounds().InsetByCopy(10, 10);
	rect.top += 18;
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom = rect.top + 4 * height + 2;
	fListView = new DragListView(rect, NULL, B_SINGLE_SELECTION_LIST,
		B_FOLLOW_ALL, new BMessage(kMsgFilterMoved));
	AddChild(new BScrollView(NULL, fListView, B_FOLLOW_ALL, 0, false, true));
	rect.right += B_V_SCROLL_BAR_WIDTH;

//	fListView->Select(gSettings.formats.IndexOf(format));
	fListView->SetSelectionMessage(new BMessage(kMsgFilterSelected));

	rect.top = rect.bottom + 8;
	rect.bottom = rect.top + height;
	BRect sizeRect = rect;
	sizeRect.right = sizeRect.left + 30
		+ fChainsField->StringWidth(B_TRANSLATE("Add filter"));

	menu = new BPopUpMenu(B_TRANSLATE("Add filter"));
	menu->SetRadioMode(false);

	fAddField = new BMenuField(rect, NULL, NULL, menu);
	fAddField->ResizeToPreferred();
	AddChild(fAddField);

	sizeRect.left = sizeRect.right + 5;
	sizeRect.right = sizeRect.left + 30
		+ fChainsField->StringWidth(B_TRANSLATE("Remove"));
	sizeRect.top--;
	AddChild(fRemoveButton = new BButton(sizeRect, NULL, B_TRANSLATE("Remove"),
		new BMessage(kMsgRemoveFilter), B_FOLLOW_BOTTOM));

	ResizeTo(Bounds().Width(), sizeRect.bottom + 10);

	_SetDirection(fDirection);
}


FiltersConfigView::~FiltersConfigView()
{

}


void
FiltersConfigView::_SelectFilter(int32 index)
{
	if (Parent())
		Parent()->Hide();

	// remove old config view
	if (fFilterView) {
		Parent()->RemoveChild(fFilterView);
		_SaveConfig(fCurrentIndex);
		delete fFilterView;
		fFilterView = NULL;
	}

	if (index >= 0) {
		// add new config view
		AddonSettings* filterSettings = _GetCurrentMailSettings()
			->FilterSettingsAt(index);
		if (filterSettings) {
			FilterAddonList* addons = _GetCurrentFilterAddonList();
			BView* view = addons->CreateConfigView(*filterSettings);
			if (view) {
				BString name;
				addons->GetDescriptiveName(filterSettings->AddonRef(), name);
				fFilterView = new FilterConfigBox(name, view);
				Parent()->AddChild(fFilterView);
			}
		}
	}

	fCurrentIndex = index;

	// re-layout the view containing the config view
	if (CenterContainer *container = dynamic_cast<CenterContainer *>(Parent()))
		container->Layout();

	if (Parent())
		Parent()->Show();
}


void
FiltersConfigView::_SetDirection(direction direction)
{
	// remove the filter config view
	_SelectFilter(-1);

	for (int32 i = fListView->CountItems(); i-- > 0;) {
		BStringItem *item = (BStringItem *)fListView->RemoveItem(i);
		delete item;
	}

	fDirection = direction;
	MailAddonSettings* addonSettings = _GetCurrentMailSettings();
	FilterAddonList* addons = _GetCurrentFilterAddonList();
	addons->Reload();

	for (int32 i = 0; i < addonSettings->CountFilterSettings(); i++) {
		AddonSettings* filterSettings = addonSettings->FilterSettingsAt(i);
		if (addons->FindInfo(filterSettings->AddonRef()) < 0) {
			addonSettings->RemoveFilterSettings(i);
			i--;
			continue;
		}
		BString name = "Unnamed Filter";
		addons->GetDescriptiveName(filterSettings->AddonRef(), name);
		fListView->AddItem(new BStringItem(name));
	}

	// remove old filter items
	BMenu *menu = fAddField->Menu();
	for (int32 i = menu->CountItems(); i-- > 0;) {
		BMenuItem *item = menu->RemoveItem(i);
		delete item;
	}

	addons->Reload();
	for (int32 i = 0; i < addons->CountFilterAddons(); i++) {
		FilterAddonInfo& info = addons->FilterAddonAt(i);
		BString name;
		addons->GetDescriptiveName(i, name);

		BMessage* msg = new BMessage(kMsgAddFilter);
		msg->AddRef("filter", &info.ref);
		BMenuItem *item = new BMenuItem(name, msg);
		menu->AddItem(item);
	}

	menu->SetTargetForItems(this);
}


void
FiltersConfigView::AttachedToWindow()
{
	fChainsField->Menu()->SetTargetForItems(this);
	fListView->SetTarget(this);
	fAddField->Menu()->SetTargetForItems(this);
	fRemoveButton->SetTarget(this);
}


void
FiltersConfigView::DetachedFromWindow()
{
	_SaveConfig(fCurrentIndex);
}


void
FiltersConfigView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kMsgChainSelected:
		{
			direction dir;
			if (msg->FindInt32("direction", (int32*)&dir) != B_OK)
				break;

			if (fDirection == dir)
				break;

			_SetDirection(dir);
			break;
		}
		case kMsgAddFilter:
		{
			entry_ref ref;
			if (msg->FindRef("filter", &ref) < B_OK)
				break;

			FilterAddonList* filterAddons = _GetCurrentFilterAddonList();
			int32 index = filterAddons->FindInfo(ref);
			if (index < 0)
				break;
			_GetCurrentMailSettings()->AddFilterSettings(&ref);

			BString name;
			filterAddons->GetDescriptiveName(index, name);
			fListView->AddItem(new BStringItem(name));
			break;
		}
		case kMsgRemoveFilter:
		{
			int32 index = fListView->CurrentSelection();
			if (index < 0)
				break;
			BStringItem *item = (BStringItem *)fListView->RemoveItem(index);
			delete item;

			_SelectFilter(-1);

			MailAddonSettings* mailSettings = _GetCurrentMailSettings();
			mailSettings->RemoveFilterSettings(index);
			break;
		}
		case kMsgFilterSelected:
		{
			int32 index = -1;
			if (msg->FindInt32("index",&index) < B_OK)
				break;

			_SelectFilter(index);
			break;
		}
		case kMsgFilterMoved:
		{
			int32 from = msg->FindInt32("from");
			int32 to = msg->FindInt32("to");
			if (from == to)
				break;

			MailAddonSettings* mailSettings = _GetCurrentMailSettings();
			if (!mailSettings->MoveFilterSettings(from, to)) {
				BAlert* alert = new BAlert("E-mail",
					B_TRANSLATE("The filter could not be moved. Deleting "
					"filter."), B_TRANSLATE("OK"));
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go();
				fListView->RemoveItem(to);
				break;
			}    

			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}


MailAddonSettings*
FiltersConfigView::_GetCurrentMailSettings()
{
	if (fDirection == kIncomming)
		return &fAccount.InboundSettings();
	return &fAccount.OutboundSettings();
}


FilterAddonList*
FiltersConfigView::_GetCurrentFilterAddonList()
{
	if (fDirection == kIncomming)
		return &fInboundFilters;
	return &fOutboundFilters;
}


void
FiltersConfigView::_SaveConfig(int32 index)
{
	if (fFilterView) {
		AddonSettings* filterSettings = _GetCurrentMailSettings()
			->FilterSettingsAt(index);
		if (filterSettings)
			fFilterView->ArchiveAddon(&filterSettings->EditSettings());
	}
}
