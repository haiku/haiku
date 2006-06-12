/*
 * Copyright (c) 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:	
 *		Ulrich Wimboeck
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Assmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <ListView.h>

#include <Autolock.h>
#include <PropertyInfo.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <Window.h>

#include <stdio.h>


struct track_data {
	BPoint		drag_start;
	int32		item_index;
	bool		was_selected;
	bool		try_drag;
};

static property_info sProperties[] = {
	{ "Item", { B_COUNT_PROPERTIES, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Returns the number of BListItems currently in the list.", 0, { B_INT32_TYPE } 
	},

	{ "Item", { B_EXECUTE_PROPERTY, 0 }, { B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER,
		B_RANGE_SPECIFIER, B_REVERSE_RANGE_SPECIFIER, 0 },
		"Select and invoke the specified items, first removing any existing selection." 
	},

	{ "Selection", { B_COUNT_PROPERTIES, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Returns int32 count of items in the selection.", 0, { B_INT32_TYPE } 
	},

	{ "Selection", { B_EXECUTE_PROPERTY, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Invoke items in selection." 
	},

	{ "Selection", { B_GET_PROPERTY, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Returns int32 indices of all items in the selection.", 0, { B_INT32_TYPE } 
	},

	{ "Selection", { B_SET_PROPERTY, 0 }, { B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER,
		B_RANGE_SPECIFIER, B_REVERSE_RANGE_SPECIFIER, 0 },
		"Extends current selection or deselects specified items. Boolean field \"data\" "
		"chooses selection or deselection.", 0, { B_BOOL_TYPE } 
	},

	{ "Selection", { B_SET_PROPERTY, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Select or deselect all items in the selection. Boolean field \"data\" chooses "
		"selection or deselection.", 0, { B_BOOL_TYPE } 
	},
};


BListView::BListView(BRect frame, const char* name, list_view_type type,
		uint32 resizingMode, uint32 flags)
	: BView(frame, name, resizingMode, flags)
{
	_InitObject(type);
}


BListView::BListView(BMessage* archive)
	: BView(archive)
{
	int32 listType;

	archive->FindInt32("_lv_type", &listType);
	fListType = (list_view_type)listType;

	fFirstSelected = -1;
	fLastSelected = -1;
	fAnchorIndex = -1;

	fSelectMessage = NULL;
	fScrollView = NULL;
	fTrack = NULL;

	fWidth = Bounds().Width();
	
	int32 i = 0;
	BMessage subData;
	
	while (archive->FindMessage("_l_items", i++, &subData)) {
		BArchivable *object = instantiate_object(&subData);
		if (!object)
			continue;

		BListItem *item = dynamic_cast<BListItem*>(object);
		if (!item)
			continue;

		AddItem(item);
	}

	if (archive->HasMessage("_msg")) {
		BMessage *invokationMessage = new BMessage;

		archive->FindMessage("_msg", invokationMessage);
		SetInvocationMessage(invokationMessage);
	}

	if (archive->HasMessage("_2nd_msg")) {
		BMessage *selectionMessage = new BMessage;

		archive->FindMessage("_2nd_msg", selectionMessage);
		SetSelectionMessage(selectionMessage);
	}
}


BListView::~BListView()
{
	SetSelectionMessage(NULL);
}


BArchivable *
BListView::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BListView"))
		return new BListView(archive);

	return NULL;
}


status_t
BListView::Archive(BMessage *archive, bool deep) const
{
	status_t status = BView::Archive(archive, deep);
	if (status < B_OK)
		return status;

	status = archive->AddInt32("_lv_type", fListType);
	if (status == B_OK && deep) {
		BListItem *item;
		int32 i = 0;

		while ((item = ItemAt(i++))) {
			BMessage subData;
			status = item->Archive(&subData, true);
			if (status >= B_OK)
				status = archive->AddMessage("_l_items", &subData);

			if (status < B_OK)
				break;
		}
	}

	if (status >= B_OK && InvocationMessage() != NULL)
		status = archive->AddMessage("_msg", InvocationMessage());

	if (status == B_OK && fSelectMessage != NULL)
		status = archive->AddMessage("_2nd_msg", fSelectMessage);

	return status;
}


void
BListView::Draw(BRect updateRect)
{
	for (int i = 0; i < CountItems(); i++) {
		BRect itemFrame = ItemFrame(i);

		if (itemFrame.Intersects(updateRect))
			DrawItem(((BListItem*)ItemAt(i)), itemFrame);
	}
}


void
BListView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_COUNT_PROPERTIES:
		case B_EXECUTE_PROPERTY:
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		{
			BPropertyInfo propInfo(sProperties);
			BMessage specifier;
			const char *property;

			if (msg->GetCurrentSpecifier(NULL, &specifier) != B_OK
				|| specifier.FindString("property", &property) != B_OK)
				return;

			switch (propInfo.FindMatch(msg, 0, &specifier, msg->what, property)) {
				case B_ERROR:
					BView::MessageReceived(msg);
					break;

				case 0:
				{
					BMessage reply(B_REPLY);
					reply.AddInt32("result", CountItems());
					reply.AddInt32("error", B_OK);

					msg->SendReply(&reply);
					break;
				}

				case 1:
					break;

				case 2:
				{
					int32 count = 0;

					for (int32 i = 0; i < CountItems(); i++) {
						if (ItemAt(i)->IsSelected())
							count++;
					}

					BMessage reply(B_REPLY);
					reply.AddInt32("result", count);
					reply.AddInt32("error", B_OK);

					msg->SendReply(&reply);
					break;
				}

				case 3:
					break;

				case 4:
				{
					BMessage reply (B_REPLY);

					for (int32 i = 0; i < CountItems(); i++) {
						if (ItemAt(i)->IsSelected())
							reply.AddInt32("result", i);
					}

					reply.AddInt32("error", B_OK);

					msg->SendReply(&reply);
					break;
				}

				case 5:
					break;

				case 6:
				{
					BMessage reply(B_REPLY);

					bool select;
					if (msg->FindBool("data", &select) == B_OK && select)
						Select(0, CountItems() - 1, false);
					else
						DeselectAll();

					reply.AddInt32("error", B_OK);

					msg->SendReply(&reply);
					break;
				}
			}
			break;
		}

		case B_SELECT_ALL:
			Select(0, CountItems() - 1, false);
			break;

		default:
			BView::MessageReceived(msg);
	}
}


void
BListView::MouseDown(BPoint point)
{
	if (!IsFocus()) {
		MakeFocus();
		Sync();
		Window()->UpdateIfNeeded();
	}

	BMessage *message = Looper()->CurrentMessage();
	int32 index = IndexOf(point);

	// If the user double (or more) clicked within the current selection,
	// we don't change the selection but invoke the selection.
	int32 clicks;
	if (message->FindInt32("clicks", &clicks) == B_OK && clicks > 1
		&& index >= fFirstSelected && index <= fLastSelected) {
		Invoke();
		return;
	}

	int32 modifiers;
	message->FindInt32("modifiers", &modifiers);

	fTrack->drag_start = point;
	fTrack->item_index = index;
	fTrack->was_selected = index >= 0 ? ItemAt(index)->IsSelected() : false;
	fTrack->try_drag = true;

	if (index > -1) {
		if (fListType == B_MULTIPLE_SELECTION_LIST) {
			if (modifiers & B_CONTROL_KEY) {
				// select entire block
				// TODO: maybe review if we want it like in Tracker (anchor item)
				Select(min_c(index, fFirstSelected), max_c(index, fLastSelected));
			} else {
				if (modifiers & B_SHIFT_KEY) {
					// toggle selection state of clicked item (like in Tracker)
					// toggle selection state of clicked item
					if (ItemAt(index)->IsSelected())
						Deselect(index);
					else
						Select(index, true);
				} else {
					Select(index);
				}
			}
		} else {
			// toggle selection state of clicked item
			if ((modifiers & B_SHIFT_KEY) && ItemAt(index)->IsSelected())
				Deselect(index);
			else
				Select(index);
		}
	} else {
		if (!(modifiers & B_SHIFT_KEY))
			DeselectAll();
	}
}

// MouseUp
void
BListView::MouseUp(BPoint pt)
{
	fTrack->item_index = -1;
	fTrack->try_drag = false;
}

// MouseMoved
void
BListView::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	if (fTrack->item_index == -1) {
		// mouse was not clicked above any item
		// or no mouse button pressed
		return;
	}

	if (_TryInitiateDrag(pt))
		return;
}

// KeyDown
void
BListView::KeyDown(const char *bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_UP_ARROW:
		{
			if (fFirstSelected == -1)
				break;
				
			bool extend = false;

			if (fListType == B_MULTIPLE_SELECTION_LIST && (modifiers() & B_SHIFT_KEY))
				extend = true;

			Select (fFirstSelected - 1, extend);

			ScrollToSelection ();

			break;
		}
		case B_DOWN_ARROW:
		{
			if (fFirstSelected == -1)
				break;
				
			bool extend = false;

			if (fListType == B_MULTIPLE_SELECTION_LIST && (modifiers() & B_SHIFT_KEY))
				extend = true;

			Select (fLastSelected + 1, extend);

			ScrollToSelection ();

			break;
		}
		case B_HOME:
		{
			Select ( 0, fListType == B_MULTIPLE_SELECTION_LIST );

			ScrollToSelection ();

			break;
		}
		case B_END:
		{
			Select ( CountItems () - 1, fListType == B_MULTIPLE_SELECTION_LIST );

			ScrollToSelection ();

			break;
		}
		case B_RETURN:
		case B_SPACE:
		{
			Invoke ();

			break;
		}
		default:
			BView::KeyDown ( bytes, numBytes );
	}
}

// MakeFocus
void
BListView::MakeFocus(bool focused)
{
	if (IsFocus() == focused)
		return;

	BView::MakeFocus(focused);

	if (fScrollView)
		fScrollView->SetBorderHighlighted(focused);
}

// FrameResized
void
BListView::FrameResized(float width, float height)
{
	fWidth = Bounds().right;
	_FixupScrollBar();
}

// TargetedByScrollView
void
BListView::TargetedByScrollView(BScrollView *view)
{
	fScrollView = view;
}


void
BListView::ScrollTo(BPoint point)
{
	BView::ScrollTo(point);
	fWidth = Bounds().right;
}


bool
BListView::AddItem(BListItem *item, int32 index)
{
	if (!fList.AddItem(item, index))
		return false;

	if (fFirstSelected != -1 && index <= fFirstSelected)
		fFirstSelected++;

	if (fLastSelected != -1 && index <= fLastSelected)
		fLastSelected++;

	if (Window()) {
		BFont font;
		GetFont(&font);

		item->Update(this, &font);

		_FixupScrollBar();
		_InvalidateFrom(index);
	}

	return true;
}


bool
BListView::AddItem(BListItem* item)
{
	if (!fList.AddItem(item))
		return false;

	// No need to adapt selection, as this item is the last in the list

	if (Window()) {
		BFont font;
		GetFont(&font);

		item->Update(this, &font);

		_FixupScrollBar();
		InvalidateItem(CountItems() - 1);
	}

	return true;
}

// AddList
bool
BListView::AddList(BList* list, int32 index)
{
	if (!fList.AddList(list, index))
		return false;

	int32 count = fList.CountItems();

	if (fFirstSelected != -1 && index < fFirstSelected)
		fFirstSelected += count;

	if (fLastSelected != -1 && index < fLastSelected)
		fLastSelected += count;

	if (Window()) {
		BFont font;
		GetFont(&font);

		int32 i = 0;
		while(BListItem *item = (BListItem*)list->ItemAt(i)) {
			item->Update(this, &font);
			i++;
		}
		
		_FixupScrollBar();
		Invalidate(); // TODO
	}

	return true;
}

// AddList
bool
BListView::AddList(BList* list)
{
	return AddList(list, CountItems());
}

// RemoveItem
BListItem*
BListView::RemoveItem(int32 index)
{
	BListItem *item = ItemAt(index);
	if (!item)
		return NULL;

	if (item->IsSelected())
		Deselect(index);

	if (!fList.RemoveItem(item))
		return NULL;

	if (fFirstSelected != -1 && index < fFirstSelected)
		fFirstSelected--;

	if (fLastSelected != -1 && index < fLastSelected)
		fLastSelected--;

	_InvalidateFrom(index);
	_FixupScrollBar();

	return item;
}

// RemoveItem
bool
BListView::RemoveItem(BListItem *item)
{
	return BListView::RemoveItem(IndexOf(item)) != NULL;
}

// RemoveItems
bool
BListView::RemoveItems(int32 index, int32 count)
{
	if (index >= CountItems())
		index = -1;

	if (index < 0)
		return false;

	// TODO: very bad for performance!!
	while (count--)
		BListView::RemoveItem(index);

	return true;
}

// SetSelectionMessage
void
BListView::SetSelectionMessage(BMessage* message)
{
	delete fSelectMessage;
	fSelectMessage = message;
}

// SetInvocationMessage
void
BListView::SetInvocationMessage(BMessage* message)
{
	BInvoker::SetMessage(message);
}

// InvocationMessage
BMessage*
BListView::InvocationMessage() const
{
	return BInvoker::Message();
}

// InvocationCommand
uint32
BListView::InvocationCommand() const
{
	return BInvoker::Command();
}

// SelectionMessage
BMessage*
BListView::SelectionMessage() const
{
	return fSelectMessage;
}

// SelectionCommand
uint32
BListView::SelectionCommand() const
{
	if (fSelectMessage)
		return fSelectMessage->what;
	else
		return 0;
}

// SetListType
void
BListView::SetListType(list_view_type type)
{
	if (fListType == B_MULTIPLE_SELECTION_LIST &&
		type == B_SINGLE_SELECTION_LIST)
		Select(CurrentSelection(0));

	fListType = type;
}
//------------------------------------------------------------------------------
list_view_type BListView::ListType() const
{
	return fListType;
}

// ItemAt
BListItem*
BListView::ItemAt(int32 index) const
{
	return (BListItem*)fList.ItemAt(index);
}

// IndexOf
int32
BListView::IndexOf(BListItem *item) const
{
	return fList.IndexOf(item);
}

// IndexOf
int32
BListView::IndexOf(BPoint point) const
{
	float y = 0.0f;

	// TODO: somehow binary search, but items don't know their frame
	for (int i = 0; i < fList.CountItems(); i++) {
		y += ItemAt(i)->Height();

		if (point.y < y)
			return i;
	}

	return -1;
}

// FirstItem
BListItem*
BListView::FirstItem() const
{
	return (BListItem*)fList.FirstItem();
}

// LastItem
BListItem*
BListView::LastItem() const
{
	return (BListItem*)fList.LastItem();
}

// HasItem
bool
BListView::HasItem(BListItem *item) const
{
	return fList.HasItem(item);
}

// CountItems
int32
BListView::CountItems() const
{
	return fList.CountItems();
}

// MakeEmpty
void
BListView::MakeEmpty()
{
	_DeselectAll(-1, -1);
	fList.MakeEmpty();

	Invalidate();
}

// IsEmpty
bool
BListView::IsEmpty() const
{
	return fList.IsEmpty();
}

// DoForEach
void
BListView::DoForEach(bool (*func)(BListItem*))
{
	fList.DoForEach(reinterpret_cast<bool (*)(void*)>(func));
}

// DoForEach
void
BListView::DoForEach(bool (*func)(BListItem*, void*), void* arg )
{
	fList.DoForEach(reinterpret_cast<bool (*)(void*, void*)>(func), arg);
}

// Items
const BListItem**
BListView::Items() const
{
	return (const BListItem**)fList.Items();
}

// InvalidateItem
void
BListView::InvalidateItem(int32 index)
{
	Invalidate(ItemFrame(index));
}

// ScrollToSelection
void
BListView::ScrollToSelection()
{
	BRect itemFrame = ItemFrame(CurrentSelection(0));

	if (Bounds().Intersects(itemFrame.InsetByCopy(0.0f, 2.0f)))
		return;

	if (itemFrame.top < Bounds().top)
		ScrollTo(0, itemFrame.top);
	else
		ScrollTo(0, itemFrame.bottom - Bounds().Height());
}


void
BListView::Select(int32 index, bool extend)
{
	if (_Select(index, extend)) {
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}
}


void
BListView::Select(int32 start, int32 finish, bool extend)
{
	if (_Select(start, finish, extend)) {
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}
}


bool
BListView::IsItemSelected(int32 index) const
{
	BListItem *item = ItemAt(index);
	if (item)
		return item->IsSelected();

	return false;
}

// CurrentSelection
int32
BListView::CurrentSelection(int32 index) const
{
	if (fFirstSelected == -1)
		return -1;

	if (index == 0)
		return fFirstSelected;

	for (int32 i = fFirstSelected; i <= fLastSelected; i++) {
		if (ItemAt(i)->IsSelected()) {
			if (index == 0)
				return i;

			index--;
		}
	}

	return -1;
}

// Invoke
status_t
BListView::Invoke(BMessage *message)
{
	// Note, this is more or less a copy of BControl::Invoke() and should
	// stay that way (ie. changes done there should be adopted here)

	bool notify = false;
	uint32 kind = InvokeKind(&notify);

	BMessage clone(kind);
	status_t err = B_BAD_VALUE;

	if (!message && !notify)
		message = Message();
		
	if (!message) {
		if (!IsWatched())
			return err;
	} else
		clone = *message;

	clone.AddInt64("when", (int64)system_time());
	clone.AddPointer("source", this);
	clone.AddMessenger("be:sender", BMessenger(this));

	if (fListType == B_SINGLE_SELECTION_LIST)
		clone.AddInt32("index", fFirstSelected);
	else {
		if (fFirstSelected >= 0) {
			for (int32 i = fFirstSelected; i <= fLastSelected; i++) {
				if (ItemAt(i)->IsSelected())
					clone.AddInt32("index", i);
			}
		}
	}

	if (message)
		err = BInvoker::Invoke(&clone);

	SendNotices(kind, &clone);

	return err;
}


void
BListView::DeselectAll()
{
	if (_DeselectAll(-1, -1)) {
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}
}


void
BListView::DeselectExcept(int32 exceptFrom, int32 exceptTo)
{
	if (exceptFrom > exceptTo || exceptFrom < 0 || exceptTo < 0)
		return;

	if (_DeselectAll(exceptFrom, exceptTo)) {
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}
}


void
BListView::Deselect(int32 index)
{
	if (_Deselect(index)) {
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}
}


void
BListView::SelectionChanged()
{
	// Hook method to be implemented by subclasses
}


void
BListView::SortItems(int (*cmp)(const void *, const void *)) 
{
	if (_DeselectAll(-1, -1)) {
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}

	fList.SortItems(cmp);
	Invalidate();
}

// SwapItems
bool
BListView::SwapItems(int32 a, int32 b)
{
	MiscData data;

	data.swap.a = a;
	data.swap.b = b;

	return DoMiscellaneous(B_SWAP_OP, &data);
}

// MoveItem
bool
BListView::MoveItem(int32 from, int32 to)
{
	MiscData data;

	data.move.from = from;
	data.move.to = to;

	return DoMiscellaneous(B_MOVE_OP, &data);
}

// ReplaceItem
bool
BListView::ReplaceItem(int32 index, BListItem *item)
{
	MiscData data;

	data.replace.index = index;
	data.replace.item = item;

	return DoMiscellaneous(B_REPLACE_OP, &data);
}

// AttachedToWindow
void
BListView::AttachedToWindow()
{
	BView::AttachedToWindow();
	_FontChanged();

	if (!Messenger().IsValid())
		SetTarget(Window(), NULL);

	_FixupScrollBar();
}

// FrameMoved
void
BListView::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}

// ItemFrame
BRect
BListView::ItemFrame(int32 index)
{
	BRect frame(0, 0, Bounds().Width(), -1);

	if (index < 0 || index >= CountItems())
		return frame;

	// TODO: this is very expensive, the (last) offsets could be cached
	for (int32 i = 0; i <= index; i++) {
		frame.top = frame.bottom + 1;
		frame.bottom += (float)ceil(ItemAt(i)->Height());
	}

	return frame;
}


BHandler*
BListView::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 form, const char* property)
{
	BPropertyInfo propInfo(sProperties);

	if (propInfo.FindMatch(message, 0, specifier, form, property) < 0)
		return BView::ResolveSpecifier(message, index, specifier, form, property);

	// TODO: msg->AddInt32("_match_code_", );

	return this;
}


status_t
BListView::GetSupportedSuites(BMessage* data)
{
	if (data == NULL)
		return B_BAD_VALUE;
	
	status_t err = data->AddString("suites", "suite/vnd.Be-list-view");

	BPropertyInfo propertyInfo(sProperties);
	if (err == B_OK)
		err = data->AddFlat("messages", &propertyInfo);
	
	if (err == B_OK)
		return BView::GetSupportedSuites(data);
	return err;
}

// Perform
status_t
BListView::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}

// WindowActivated
void
BListView::WindowActivated(bool state)
{
	BView::WindowActivated(state);
}

// DetachedFromWindow
void
BListView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}

// InitiateDrag
bool
BListView::InitiateDrag(BPoint point, int32 index, bool wasSelected)
{
	return false;
}

// ResizeToPreferred
void
BListView::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}

// GetPreferredSize
void
BListView::GetPreferredSize(float *width, float *height)
{
	BView::GetPreferredSize(width, height);
}

// AllAttached
void
BListView::AllAttached()
{
	BView::AllAttached();
}

// AllDetached
void
BListView::AllDetached()
{
	BView::AllDetached();
}

// DoMiscellaneous
bool
BListView::DoMiscellaneous(MiscCode code, MiscData *data)
{
	if (code > B_SWAP_OP)
		return false;

	switch (code) {
		case B_NO_OP:
			break;

		case B_REPLACE_OP:
			return _ReplaceItem(data->replace.index, data->replace.item);

		case B_MOVE_OP:
			return _MoveItem(data->move.from, data->move.to);

		case B_SWAP_OP:
			return _SwapItems(data->swap.a, data->swap.b);
	}

	return false;
}

void BListView::_ReservedListView2() {}
void BListView::_ReservedListView3() {}
void BListView::_ReservedListView4() {}

BListView &BListView::operator=(const BListView &)
{
	return *this;
}

// _InitObject
void
BListView::_InitObject(list_view_type type)
{
	fListType = type;
	fFirstSelected = -1;
	fLastSelected = -1;
	fAnchorIndex = -1;
	fWidth = Bounds().Width();
	fSelectMessage = NULL;
	fScrollView = NULL;
	fTrack = new track_data;
	fTrack->try_drag = false;
	fTrack->item_index = -1;
}

// _FixupScrollBar
void
BListView::_FixupScrollBar()
{
	BScrollBar* vertScroller = ScrollBar(B_VERTICAL);

	if (!vertScroller)
		return;

	BRect bounds = Bounds();
	int32 count = CountItems();

	float itemHeight = 0;
	for (int32 i = 0; BListItem* item = ItemAt(i); i++) {
		itemHeight += item->Height();
	}

	if (bounds.Height() > itemHeight) {
		// no scrolling
		vertScroller->SetRange(0.0, 0.0);
		vertScroller->SetValue(0.0);
			// also scrolls ListView to the top
	} else {
		vertScroller->SetRange(0.0, itemHeight - bounds.Height() - 1.0);
		vertScroller->SetProportion(bounds.Height () / itemHeight);
		// scroll up if there is empty room on bottom
		if (itemHeight < bounds.bottom) {
			ScrollBy(0.0, bounds.bottom - itemHeight);
		}
	}

	if (count != 0) {
		vertScroller->SetSteps((float)ceil(FirstItem()->Height()),
			bounds.Height());
	}
}

// _InvalidateFrom
void
BListView::_InvalidateFrom(int32 index)
{
	// make sure index is behind last valid index
	int32 count = CountItems();
	if (index >= count) {
		index = count;
	}
	// take the item before the wanted one,
	// because that might already be removed
	index--;
	BRect dirty = Bounds();
	if (index >= 0) {
		dirty.top = ItemFrame(index).bottom + 1;
	}
	Invalidate(dirty);
}

// _FontChanged
void
BListView::_FontChanged()
{
	BFont font;
	GetFont(&font);

	for (int i = 0; i < CountItems (); i ++)
		ItemAt(i)->Update(this, &font);
}


/*!
	Selects the item at the specified \a index, and returns \c true in
	case the selection was changed because of this method.
	If \a extend is \c false, all previously selected items are deselected.
*/
bool
BListView::_Select(int32 index, bool extend)
{
	if (index < 0 || index >= CountItems())
		return false;

	// only lock the window when there is one
	BAutolock locker(Window());
	if (Window() && !locker.IsLocked())
		return false;

	bool changed = false;

	if (fFirstSelected != -1 && !extend)
		changed = _DeselectAll(index, index);

	BListItem* item = ItemAt(index);
	if (!item->IsEnabled() || item->IsSelected()) {
		// if the item is already selected, or can't be selected,
		// we're done here
		return changed;
	}

	// keep track of first and last selected item
	if (fFirstSelected == -1) {
		// no previous selection
		fFirstSelected = index;
		fLastSelected = index;
	} else if (index < fFirstSelected) {
		fFirstSelected = index;
	} else if (index > fLastSelected) {
		fLastSelected = index;
	}

	ItemAt(index)->Select();
	if (Window())
		InvalidateItem(index);

	return true;
}


/*!
	Selects the items between \a from and \a to, and returns \c true in
	case the selection was changed because of this method.
	If \a extend is \c false, all previously selected items are deselected.
*/
bool
BListView::_Select(int32 from, int32 to, bool extend)
{
	if (to < from)
		return false;

	BAutolock locker(Window());
	if (Window() && !locker.IsLocked())
		return false;

	bool changed = false;

	if (fFirstSelected != -1 && !extend)
		changed = _DeselectAll(from, to);

	if (fFirstSelected == -1) {
		fFirstSelected = from;
		fLastSelected = to;
	} else if (from < fFirstSelected)
		fFirstSelected = from;
	else if (to > fLastSelected)
		fLastSelected = to;

	for (int32 i = from; i <= to; ++i) {
		BListItem *item = ItemAt(i);
		if (item && !item->IsSelected()) {
			item->Select();
			if (Window())
				InvalidateItem(i);
			changed = true;
		}
	}

	return changed;
}


bool
BListView::_Deselect(int32 index)
{
	if (index < 0 || index >= CountItems())
		return false;

	BAutolock locker(Window());
	if (Window() && !locker.IsLocked())
		return false;

	BListItem *item = ItemAt(index);

	if (item && item->IsSelected()) {
		BRect frame(ItemFrame(index));
		BRect bounds(Bounds());

		item->Deselect();

		if (fFirstSelected == index && fLastSelected == index) {
			fFirstSelected = -1;
			fLastSelected = -1;
		} else {
			if (fFirstSelected == index)
				fFirstSelected = _CalcFirstSelected(index);

			if (fLastSelected == index)
				fLastSelected = _CalcLastSelected(index);
		}

		if (bounds.Intersects(frame))
			DrawItem(ItemAt(index), frame, true);
	}

	return true;
}


bool
BListView::_DeselectAll(int32 exceptFrom, int32 exceptTo)
{
	if (fFirstSelected == -1)
		return false;

	BAutolock locker(Window());
	if (Window() && !locker.IsLocked())
		return false;

	bool changed = false;

	for (int32 index = fFirstSelected; index <= fLastSelected; index++) {
		// don't deselect the items we shouldn't deselect
		if (exceptFrom != -1 && exceptFrom <= index && exceptTo >= index)
			continue;

		BListItem *item = ItemAt(index);
		if (item && item->IsSelected()) {
			item->Deselect();
			InvalidateItem(index);
			changed = true;
		}
	}

	if (!changed)
		return false;

	if (exceptFrom != -1) {
		fFirstSelected = _CalcFirstSelected(exceptFrom);
		fLastSelected = _CalcLastSelected(exceptTo);
	} else
		fFirstSelected = fLastSelected = -1;

	return true;
}

// _TryInitiateDrag
bool
BListView::_TryInitiateDrag(BPoint where)
{
	if (!fTrack->try_drag | fTrack->item_index < 0)
		return false;

	BPoint offset = where - fTrack->drag_start;
	float dragDistance = sqrtf(offset.x * offset.x + offset.y * offset.y);

	if (dragDistance > 5.0) {
		fTrack->try_drag = false;
		return InitiateDrag(fTrack->drag_start, fTrack->item_index, fTrack->was_selected);;
	}
	return false;
}

// _CalcFirstSelected
int32
BListView::_CalcFirstSelected(int32 after)
{
	if (after >= CountItems())
		return -1;

	int32 count = CountItems();
	for (int32 i = after; i < count; i++) {
		if (ItemAt(i)->IsSelected())
			return i;
	}

	return -1;
}


int32
BListView::_CalcLastSelected(int32 before)
{
	if (before < 0)
		return -1;

	before = min_c(CountItems() - 1, before);

	for (int32 i = before; i >= 0; i--) {
		if (ItemAt(i)->IsSelected())
			return i;
	}

	return -1;
}


void
BListView::DrawItem(BListItem *item, BRect itemRect, bool complete)
{
	item->DrawItem(this, itemRect, complete);
}


bool
BListView::_SwapItems(int32 a, int32 b)
{
	// remember frames of items before anyhing happens,
	// the tricky situation is when the two items have
	// a different height
	BRect aFrame = ItemFrame(a);
	BRect bFrame = ItemFrame(b);

	if (!fList.SwapItems(a, b))
		return false;

	if (a == b) {
		// nothing to do, but success nevertheless
		return true;
	}

	// track anchor item
	if (fAnchorIndex == a)
		fAnchorIndex = b;
	else if (fAnchorIndex == b)
		fAnchorIndex = a;

	// track selection
	// NOTE: this is only important if the selection status
	// of both items is not the same
	if (ItemAt(a)->IsSelected() != ItemAt(b)->IsSelected()) {
		int32 first = min_c(a, b);
		int32 last = max_c(a, b);
		if (first < fFirstSelected || last > fLastSelected) {
			first = min_c(first, fFirstSelected);
			last = max_c(last, fLastSelected);
			_RescanSelection(first, last);
		}
		// though the actually selected items stayed the
		// same, the selection has still changed
		SelectionChanged();
	}

	// take care of invalidation
	if (Window()) {
		// NOTE: window looper is assumed to be locked!
		if (aFrame.Height() != bFrame.Height()) {
			// items in between shifted visually
			Invalidate(aFrame | bFrame);
		} else {
			Invalidate(aFrame);
			Invalidate(bFrame);
		}
	}

	return true;
}


bool
BListView::_MoveItem(int32 from, int32 to)
{
	// remember item frames before doing anything
	BRect frameFrom = ItemFrame(from);
	BRect frameTo = ItemFrame(to);

	if (!fList.MoveItem(from, to))
		return false;

	// track anchor item
	if (fAnchorIndex == from)
		fAnchorIndex = to;

	// track selection
	if (ItemAt(to)->IsSelected()) {
		_RescanSelection(from, to);
		// though the actually selected items stayed the
		// same, the selection has still changed
		SelectionChanged();
	}

	// take care of invalidation
	if (Window()) {
		// NOTE: window looper is assumed to be locked!
		Invalidate(frameFrom | frameTo);
	}

	return true;
}


bool
BListView::_ReplaceItem(int32 index, BListItem *item)
{
	if (!item)
		return false;

	BListItem* old = ItemAt(index);
	if (!old)
		return false;

	BRect frame = ItemFrame(index);

	bool selectionChanged = old->IsSelected() != item->IsSelected();

	// replace item
	if (!fList.ReplaceItem(index, item))
		return false;

	// tack selection
	if (selectionChanged) {
		int32 start = min_c(fFirstSelected, index);
		int32 end = max_c(fLastSelected, index);
		_RescanSelection(start, end);
		SelectionChanged();
	}

	bool itemHeightChanged = frame != ItemFrame(index);

	// take care of invalidation
	if (Window()) {
		// NOTE: window looper is assumed to be locked!
		if (itemHeightChanged)
			_InvalidateFrom(index);
		else
			Invalidate(frame);
	}

	if (itemHeightChanged)
		_FixupScrollBar();

	return true;
}


void
BListView::_RescanSelection(int32 from, int32 to)
{
	if (from > to) {
		int32 tmp = from;
		from = to;
		to = tmp;
	}

	from = max_c(0, from);
	to = min_c(to, CountItems() - 1);

	if (fAnchorIndex != -1) {
		if (fAnchorIndex == from)
			fAnchorIndex = to;
		else if (fAnchorIndex == to)
			fAnchorIndex = from;
	}

	for (int32 i = from; i <= to; i++) {
		if (ItemAt(i)->IsSelected()) {
			fFirstSelected = i;
			break;
		}
	}

	if (fFirstSelected > from)
		from = fFirstSelected;
	for (int32 i = from; i <= to; i++) {
		if (ItemAt(i)->IsSelected())
			fLastSelected = i;
	}
}


