//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ListView.cpp
//	Author:			Ulrich Wimboeck
//					Marc Flerackers (mflerackers@androme.be)
//	Description:	BListView represents a one-dimensional list view.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <ListView.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <support/Errors.h>
#include <PropertyInfo.h>
#include <Window.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
static property_info prop_list[] =
{
	{ "Item", { B_COUNT_PROPERTIES, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Returns the number of BListItems currently in the list." },
	{ "Item", { B_EXECUTE_PROPERTY, 0 }, { B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER,
		B_RANGE_SPECIFIER, B_REVERSE_RANGE_SPECIFIER, 0 },
		"Select and invoke the specified items, first removing any existing selection." },
	{ "Selection", { B_COUNT_PROPERTIES, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Returns int32 count of items in the selection." },
	{ "Selection", { B_EXECUTE_PROPERTY, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Invoke items in selection." },
	{ "Selection", { B_GET_PROPERTY, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Returns int32 indices of all items in the selection." },
	{ "Selection", { B_SET_PROPERTY, 0 }, { B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER,
		B_RANGE_SPECIFIER, B_REVERSE_RANGE_SPECIFIER, 0 },
		"Extends current selection or deselects specified items. Boolean field \"data\" "
		"chooses selection or deselection." },
	{ "Selection", { B_SET_PROPERTY, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Select or deselect all items in the selection. Boolean field \"data\" chooses "
		"selection or deselection." },
};

//------------------------------------------------------------------------------
BListView::BListView(BRect frame, const char *name, list_view_type type,
					 uint32 resizingMode, uint32 flags)
	:	BView(frame, name, resizingMode, flags)
{
	InitObject(type);
}
//------------------------------------------------------------------------------
BListView::BListView(BMessage *archive)
	:	BView(archive)
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
	
	while (archive->FindMessage("_l_items", i++, &subData))
	{
		BArchivable *object = instantiate_object(&subData);

		if (!object)
			continue;

		BListItem *item = dynamic_cast<BListItem*>(object);

		if (!item)
			continue;
		
		AddItem(item);
	}
	
	if (archive->HasMessage("_msg"))
	{
		BMessage *invokationMessage = new BMessage;

		archive->FindMessage("_msg", invokationMessage);
		SetInvocationMessage(invokationMessage);
	}

	if (archive->HasMessage("_2nd_msg"))
	{
		BMessage *selectionMessage = new BMessage;

		archive->FindMessage("_2nd_msg", selectionMessage);
		SetSelectionMessage(selectionMessage);
	}
}
//------------------------------------------------------------------------------
BListView::~BListView()
{
	SetSelectionMessage(NULL);

	if (fSelectMessage)
		delete fSelectMessage;
}
//------------------------------------------------------------------------------
BArchivable *BListView::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BListView"))
		return new BListView(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BListView::Archive(BMessage *archive, bool deep) const
{
	BView::Archive ( archive, deep );

	archive->AddInt32("_lv_type", fListType);
	
	if (deep)
	{
		int32 i = 0;
		BListItem *item;

		while ((item = ItemAt(i++)))
		{
			BMessage subData;

			if (item->Archive(&subData, true) != B_OK)
				continue;

			archive->AddMessage("_l_items", &subData);
		}
	}

	if (InvocationMessage())
		archive->AddMessage("_msg", InvocationMessage());
	
	if (fSelectMessage)
		archive->AddMessage("_2nd_msg", fSelectMessage);

	return B_OK;
}
//------------------------------------------------------------------------------
void BListView::Draw(BRect updateRect)
{
	for (int i = 0; i < CountItems(); i++)
	{
		BRect item_frame = ItemFrame(i);

		if (item_frame.Intersects(updateRect))
			DrawItem(((BListItem*)ItemAt(i)), item_frame);
	}
}
//------------------------------------------------------------------------------
void BListView::MessageReceived ( BMessage *msg )
{
	switch ( msg->what )
	{
		case B_COUNT_PROPERTIES:
		case B_EXECUTE_PROPERTY:
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		{
			BPropertyInfo propInfo ( prop_list );
			BMessage specifier;
			const char *property;

			if ( msg->GetCurrentSpecifier ( NULL, &specifier ) != B_OK ||
				specifier.FindString ( "property", &property ) != B_OK )
				return;

			switch ( propInfo.FindMatch ( msg, 0, &specifier, msg->what, property ) )
			{
				case B_ERROR:
					BView::MessageReceived ( msg );
					break;

				case 0:
				{
					BMessage reply ( B_REPLY );

					reply.AddInt32 ( "result", CountItems () );
					reply.AddInt32 ( "error", B_OK );

					msg->SendReply ( &reply );
					break;
				}
				case 1:
					break;
				case 2:
				{
					BMessage reply ( B_REPLY );

					int32 count = 0;

					for ( int32 i = 0; i < CountItems (); i++ )
						if ( ItemAt ( i )->IsSelected () )
							count++;

					reply.AddInt32 ( "result", count );
					reply.AddInt32 ( "error", B_OK );

					msg->SendReply ( &reply );
					break;
				}
				case 3:
					break;
				case 4:
				{
					BMessage reply ( B_REPLY );

					for ( int32 i = 0; i < CountItems (); i++ )
						if ( ItemAt ( i )->IsSelected () )
							reply.AddInt32 ( "result", i );

					reply.AddInt32 ( "error", B_OK );

					msg->SendReply ( &reply );
					break;
				}
				case 5:
					break;
				case 6:
				{
					BMessage reply ( B_REPLY );

					bool select;

					msg->FindBool ( "data", &select );

					if ( select )
						Select ( 0, CountItems () - 1, false );
					else
						DeselectAll ();

					reply.AddInt32 ( "error", B_OK );

					msg->SendReply ( &reply );
					break;
				}
			}

			break;
		}
		case B_SELECT_ALL:
		{
			Select(0, CountItems() - 1, false);
			break;
		}
		default:
			BView::MessageReceived(msg);
	}

	
}
//------------------------------------------------------------------------------
void BListView::MouseDown(BPoint point)
{
	if (!IsFocus())
	{
		MakeFocus();
		Sync();
		Window()->UpdateIfNeeded();
	}

	int index = IndexOf(point);

	if (index > -1)
	{
		BMessage *message = Looper()->CurrentMessage();
		int32 clicks;
		int32 modifiers;
	
		message->FindInt32("clicks", &clicks);
		message->FindInt32("modifiers", &modifiers);

		bool extend = false;

		if (fListType == B_MULTIPLE_SELECTION_LIST && (modifiers & B_CONTROL_KEY))
				extend = true;

		if (clicks == 2)
		{
			if (!ItemAt(index)->IsSelected())
				Select(index, extend);
			Invoke();
		}
		if (!InitiateDrag(point, index, IsItemSelected(index)))
		{
			if (CurrentSelection() == -1 || (!ItemAt(index)->IsSelected() &&
				!(modifiers & B_SHIFT_KEY)))
				fAnchorIndex = index;

			if (modifiers & B_SHIFT_KEY)
				Select(fAnchorIndex, index, extend);
			else
			{
				if (!ItemAt(index)->IsSelected())
					Select(index, extend);
				else
				{
					ItemAt(index)->Deselect();
					InvalidateItem(index);
				}
			}
		}
	}

	// TODO: InitiateDrag
}
//------------------------------------------------------------------------------
void BListView::KeyDown(const char *bytes, int32 numBytes)
{
	switch ( bytes[0] )
	{
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
//------------------------------------------------------------------------------
void BListView::MakeFocus(bool focused)
{
	if (IsFocus() == focused)
		return;

	BView::MakeFocus(focused);

	if (fScrollView)
		fScrollView->SetBorderHighlighted(focused);
}
//------------------------------------------------------------------------------
void BListView::FrameResized(float width, float height)
{
	// TODO
	FixupScrollBar();
}
//------------------------------------------------------------------------------
void BListView::TargetedByScrollView(BScrollView *view)
{
	fScrollView = view;
}
//------------------------------------------------------------------------------
void BListView::ScrollTo(BPoint point)
{
	BView::ScrollTo(point);
	fWidth = Bounds().right;
}
//------------------------------------------------------------------------------
bool BListView::AddItem(BListItem *item, int32 index)
{
	if (!fList.AddItem(item, index))
		return false;

	if (fFirstSelected != -1 && index < fFirstSelected)
		fFirstSelected++;

	if (fLastSelected != -1 && index < fLastSelected)
		fLastSelected++;

	if (Window())
	{
		BFont font;
		GetFont(&font);

		item->Update(this, &font);

		FixupScrollBar();
		InvalidateFrom(index);
	}

	return true;
}
//------------------------------------------------------------------------------
bool BListView::AddItem(BListItem *item)
{
	if (!fList.AddItem(item))
		return false;

	if (Window())
	{
		BFont font;
		GetFont(&font);

		item->Update(this, &font);

		FixupScrollBar();
		InvalidateItem(CountItems() - 1);
	}

	return true;
}
//------------------------------------------------------------------------------
bool BListView::AddList(BList *list, int32 index)
{
	if (!fList.AddList(list, index))
		return false;

	int32 count = fList.CountItems();

	if (fFirstSelected != -1 && index < fFirstSelected)
		fFirstSelected += count;

	if (fLastSelected != -1 && index < fLastSelected)
		fLastSelected += count;

	if (Window())
	{
		BFont font;
		GetFont(&font);

		int32 i = 0;
		while(BListItem *item = (BListItem*)list->ItemAt(i))
		{
			item->Update(this, &font);
			i++;
		}
		
		FixupScrollBar();
		Invalidate(); // TODO
	}

	return true;
}
//------------------------------------------------------------------------------
bool BListView::AddList(BList *list)
{
	return AddList(list, CountItems());
}
//------------------------------------------------------------------------------
BListItem *BListView::RemoveItem(int32 index)
{
	BListItem *item = ItemAt(index);

	if (!item)
		return NULL;

	if (item->IsSelected())
		Deselect(index);

	if(!fList.RemoveItem(item))
		return item;

	if (fFirstSelected != -1 && index < fFirstSelected)
		fFirstSelected--;

	if (fLastSelected != -1 && index < fLastSelected)
		fLastSelected--;

	InvalidateFrom(index);
	FixupScrollBar();

	return item;
}
//------------------------------------------------------------------------------
bool BListView::RemoveItem(BListItem *item)
{
	return RemoveItem(IndexOf(item)) != NULL;
}
//------------------------------------------------------------------------------
bool BListView::RemoveItems(int32 index, int32 count)
{
	if (index >= CountItems())
		index = -1;

	if (index < 0)
		return false;

	while (count--)
		RemoveItem(index);

	return true;
}
//------------------------------------------------------------------------------
void BListView::SetSelectionMessage(BMessage *message)
{
	if (fSelectMessage)
		delete fSelectMessage;

	fSelectMessage = message;
}
//------------------------------------------------------------------------------
void BListView::SetInvocationMessage(BMessage *message)
{
	BInvoker::SetMessage(message);
}
//------------------------------------------------------------------------------
BMessage *BListView::InvocationMessage() const
{
	return BInvoker::Message();
}
//------------------------------------------------------------------------------
uint32 BListView::InvocationCommand() const
{
	return BInvoker::Command();
}
//------------------------------------------------------------------------------
BMessage *BListView::SelectionMessage() const
{
	return fSelectMessage;
}
//------------------------------------------------------------------------------
uint32 BListView::SelectionCommand() const
{
	if (fSelectMessage)
		return fSelectMessage->what;
	else
		return 0;
}
//------------------------------------------------------------------------------
void BListView::SetListType(list_view_type type)
{
	if (fListType == B_MULTIPLE_SELECTION_LIST &&
		type == B_SINGLE_SELECTION_LIST)
		DeselectAll();

	fListType = type;
}
//------------------------------------------------------------------------------
list_view_type BListView::ListType() const
{
	return fListType;
}
//------------------------------------------------------------------------------
BListItem *BListView::ItemAt(int32 index) const
{
	return (BListItem*)fList.ItemAt(index);
}
//------------------------------------------------------------------------------
int32 BListView::IndexOf(BListItem *item) const
{
	return fList.IndexOf(item);
}
//------------------------------------------------------------------------------
int32 BListView::IndexOf(BPoint point) const
{
	float y = 0.0f;

	for (int i = 0; i < fList.CountItems(); i++)
	{
		y += ItemAt(i)->Height();

		if ( point.y < y )
			return i;
	}

	return -1;
}
//------------------------------------------------------------------------------
BListItem *BListView::FirstItem() const
{
	return (BListItem*)fList.FirstItem();
}
//------------------------------------------------------------------------------
BListItem *BListView::LastItem() const
{
	return (BListItem*)fList.LastItem();
}
//------------------------------------------------------------------------------
bool BListView::HasItem(BListItem *item) const
{
	return fList.HasItem(item);
}
//------------------------------------------------------------------------------
int32 BListView::CountItems() const
{
	return fList.CountItems();
}
//------------------------------------------------------------------------------
void BListView::MakeEmpty()
{
	_DeselectAll(-1, -1);
	fList.MakeEmpty();
	//virtual(&int32[2])
	Invalidate();
}
//------------------------------------------------------------------------------
bool BListView::IsEmpty() const
{
	return fList.IsEmpty();
}
//------------------------------------------------------------------------------
void BListView::DoForEach(bool (*func)(BListItem *))
{
	fList.DoForEach(reinterpret_cast<bool (*)(void*)>(func));
}
//------------------------------------------------------------------------------
void BListView::DoForEach(bool (*func)(BListItem *, void *), void *arg )
{
	fList.DoForEach(reinterpret_cast<bool (*)(void*, void*)>(func), arg);
}
//------------------------------------------------------------------------------
const BListItem **BListView::Items() const
{
	return (const BListItem**)fList.Items();
}
//------------------------------------------------------------------------------
void BListView::InvalidateItem(int32 index)
{
	Invalidate(Bounds() & ItemFrame(index));
}
//------------------------------------------------------------------------------
void BListView::ScrollToSelection ()
{
	BRect item_frame = ItemFrame ( CurrentSelection ( 0 ) );

	if ( Bounds ().Intersects ( item_frame.InsetByCopy ( 0.0f, 2.0f ) ) )
		return;

	if ( item_frame.top < Bounds ().top )
		ScrollTo ( 0, item_frame.top );
	else
		ScrollTo ( 0, item_frame.bottom - Bounds ().Height () );
}
//------------------------------------------------------------------------------
void BListView::Select(int32 index, bool extend)
{
	_Select(index, extend);

	SelectionChanged();
	InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
}
//------------------------------------------------------------------------------
void BListView::Select(int32 start, int32 finish, bool extend)
{
	_Select(start, finish, extend);

	SelectionChanged();
	InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
}
//------------------------------------------------------------------------------
bool BListView::IsItemSelected(int32 index) const
{
	BListItem *item = ItemAt(index);

	if (item)
		return item->IsSelected();
	else
		return false;
}
//------------------------------------------------------------------------------
int32 BListView::CurrentSelection(int32 index) const
{
	if (fFirstSelected == -1)
		return -1;

	if (index == 0)
		return fFirstSelected;

	for (int32 i = fFirstSelected; i <= fLastSelected; i++)
	{
		if (ItemAt(i)->IsSelected())
		{
			if (index == 0)
				return i;

			index--;
		}
	}

	return -1;
}
//------------------------------------------------------------------------------
status_t BListView::Invoke(BMessage *message)
{
	bool notify = false;
	uint32 kind = InvokeKind(&notify);

	BMessage clone(kind);
	status_t err = B_BAD_VALUE;

	if (!message && !notify)
		message = Message();
		
	if (!message)
	{
		if (!IsWatched())
			return err;
	}
	else
		clone = *message;

	clone.AddInt64("when", (int64)system_time());
	clone.AddPointer("source", this);
	clone.AddMessenger("be:sender", BMessenger(this));

	if (fListType == B_SINGLE_SELECTION_LIST)
		clone.AddInt32("index", fFirstSelected);
	else
	{
		for (int32 i = fFirstSelected; i <= fLastSelected; i++)
		{
			if (ItemAt(i)->IsSelected())
				clone.AddInt32("index", i);
		}
	}

	if (message)
		err = BInvoker::Invoke(&clone);

//	TODO: assynchronous messaging
//	SendNotices(kind, &clone);

	return err;
}
//------------------------------------------------------------------------------
void BListView::DeselectAll()
{
	if (fFirstSelected == -1)
		return;

    for (int32 index = fFirstSelected; index <= fLastSelected; ++index)
    {
		if (!ItemAt(index)->IsSelected())
		{
			ItemAt(index)->Deselect();
			InvalidateItem(index);
		}
    }
}
//------------------------------------------------------------------------------
void BListView::DeselectExcept(int32 start, int32 finish)
{
	if (fFirstSelected == -1 || finish < start)
		return;
		
	int32 index;

    for (index = fFirstSelected; index < start; ++index)
    {
		if (!ItemAt(index)->IsSelected())
		{
			ItemAt(index)->Deselect();
			InvalidateItem(index);
		}
    }
    for (index = finish + 1; index <= fLastSelected; ++index)
    {
		if (!ItemAt(index)->IsSelected())
		{
			ItemAt(index)->Deselect();
			InvalidateItem(index);
		}
    }

    SelectionChanged();
}
//------------------------------------------------------------------------------
void BListView::Deselect(int32 index)
{
	if (_Deselect(index))
	{
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}
}
//------------------------------------------------------------------------------
void BListView::SelectionChanged()
{
}
//------------------------------------------------------------------------------
void BListView::SortItems(int (*cmp)(const void *, const void *)) 
{
	if (_DeselectAll(-1, -1))
	{
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}

	fList.SortItems(cmp);
	Invalidate();
}
//------------------------------------------------------------------------------
bool BListView::SwapItems(int32 a, int32 b)
{
	MiscData data;

	data.swap.a = a;
	data.swap.b = b;

	return DoMiscellaneous(B_SWAP_OP, &data);
}
//------------------------------------------------------------------------------
bool BListView::MoveItem(int32 from, int32 to)
{
	MiscData data;

	data.move.from = from;
	data.move.to = to;

	return DoMiscellaneous(B_MOVE_OP, &data);
}
//------------------------------------------------------------------------------
bool BListView::ReplaceItem(int32 index, BListItem *item)
{
	MiscData data;

	data.replace.index = index;
	data.replace.item = item;

	return DoMiscellaneous(B_REPLACE_OP, &data);
}
//------------------------------------------------------------------------------
void BListView::AttachedToWindow()
{
	BView::AttachedToWindow();
	FontChanged();

	if (!Messenger().IsValid())
		SetTarget(Window(), NULL);

	FixupScrollBar();
}
//------------------------------------------------------------------------------
void BListView::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}
//------------------------------------------------------------------------------
BRect BListView::ItemFrame(int32 index)
{
	BRect frame(0, 0, Bounds().Width(), -1);

	if (index < 0 || index >= CountItems())
		return frame;

	for (int32 i = 0; i <= index; i++)
	{
		frame.top = frame.bottom + 1;
		frame.bottom += (float)ceil(ItemAt(i)->Height());
	}
	
	return frame;
}
//------------------------------------------------------------------------------
BHandler *BListView::ResolveSpecifier(BMessage *msg, int32 index,
									  BMessage *specifier, int32 form,
									  const char *property)
{
	BPropertyInfo propInfo(prop_list);

	if (propInfo.FindMatch(msg, 0, specifier, form, property) < 0)
		return BView::ResolveSpecifier(msg, index, specifier, form, property);

	// TODO: msg->AddInt32("_match_code_", );

	return this;
}
//------------------------------------------------------------------------------
status_t
BListView::GetSupportedSuites(BMessage *data )
{
	BPropertyInfo propertyInfo(prop_list);
	
	data->AddString("suites", "suite/vnd.Be-list-view");
	data->AddFlat("messages", &propertyInfo);
	
	return BView::GetSupportedSuites(data);
}
//------------------------------------------------------------------------------
status_t BListView::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}
//------------------------------------------------------------------------------
void BListView::WindowActivated(bool state)
{
	BView::WindowActivated(state);

	if (IsFocus())
		Draw(Bounds());
}
//------------------------------------------------------------------------------
void BListView::MouseUp(BPoint pt)
{
	if (fWidth == 0)
		return;

	DoMouseMoved(pt);
	DoMouseUp(pt);
}
//------------------------------------------------------------------------------
void BListView::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	if (fTrack == NULL)
		return;

	if (TryInitiateDrag(pt))
		return;

	DoMouseMoved(pt);
}
//------------------------------------------------------------------------------
void BListView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}
//------------------------------------------------------------------------------
bool BListView::InitiateDrag(BPoint point, int32 index, bool wasSelected)
{
	return false;
}
//------------------------------------------------------------------------------
void BListView::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}
//------------------------------------------------------------------------------
void BListView::GetPreferredSize(float *width, float *height)
{
	BView::GetPreferredSize(width, height);
}
//------------------------------------------------------------------------------
void BListView::AllAttached()
{
	BView::AllAttached();
}
//------------------------------------------------------------------------------
void BListView::AllDetached()
{
	BView::AllDetached();
}
//------------------------------------------------------------------------------
bool BListView::DoMiscellaneous(MiscCode code, MiscData *data)
{
	if (code > B_SWAP_OP)
		return false;

	switch (code)
	{
		case B_NO_OP:
		{
			break;
		}
		case B_REPLACE_OP:
		{
			return ReplaceItem(data->replace.index, data->replace.item);
		}
		case B_MOVE_OP:
		{
			return MoveItem(data->move.from, data->move.to);
		}
		case B_SWAP_OP:
		{
			return SwapItems(data->swap.a, data->swap.b);
		}
	}

	return false;
}
//------------------------------------------------------------------------------
void BListView::_ReservedListView2() {}
void BListView::_ReservedListView3() {}
void BListView::_ReservedListView4() {}
//------------------------------------------------------------------------------
BListView &BListView::operator=(const BListView &)
{
	return *this;
}
//------------------------------------------------------------------------------
void BListView::InitObject(list_view_type type)
{
	fListType = type;
	fFirstSelected = -1;
	fLastSelected = -1;
	fAnchorIndex = -1;
	fWidth = Bounds().Width();
	fSelectMessage = NULL;
	fScrollView = NULL;
	fTrack = NULL;
}
//------------------------------------------------------------------------------
void BListView::FixupScrollBar()
{
	BRect bounds, frame;
	BScrollBar *vertScroller = ScrollBar(B_VERTICAL);

	if (!vertScroller)
		return;

	bounds = Bounds();
	int32 count = CountItems();

	float y = 0;

	for (int32 i = 0; i < count; i++)
	{
		frame = ItemFrame(i);
		y += frame.Height();
	}

	if (bounds.Height() > y)
	{
		vertScroller->SetRange(0.0f, 0.0f);
		vertScroller->SetValue(0.0f);
	}
	else
	{
		vertScroller->SetRange(0.0f, y - bounds.Height());
		vertScroller->SetProportion(bounds.Height () / y);
	}

	if (count != 0)
		vertScroller->SetSteps((float)ceil(FirstItem()->Height()),
			bounds.Height());
}
//------------------------------------------------------------------------------
void BListView::InvalidateFrom(int32 index)
{
	if (index <= fList.CountItems())
		Invalidate(Bounds() & ItemFrame(index));
}
//------------------------------------------------------------------------------
void BListView::FontChanged()
{
	BFont font;
	GetFont(&font);

	for (int i = 0; i < CountItems (); i ++)
		ItemAt(i)->Update(this, &font);
}
//------------------------------------------------------------------------------
bool BListView::_Select(int32 index, bool extend)
{
	if (index < 0 || index >= CountItems())
		return false;

    if ((fFirstSelected != -1) && (!extend))
    {
		for (int32 item = fFirstSelected; item <= fLastSelected; ++item)
		{
			if ((ItemAt(item)->IsSelected()) && (item != index))
			{
				ItemAt(item)->Deselect();
				InvalidateItem(item);
			}
		}

		fFirstSelected = -1;
	}

    if (fFirstSelected == -1)
    {
		fFirstSelected = index;
		fLastSelected = index;
	}
    else if (index < fFirstSelected)
		fFirstSelected = index;
    else if (index > fLastSelected)
		fLastSelected = index;

    if (!ItemAt(index)->IsSelected())
    {
		ItemAt(index)->Select();
		InvalidateItem(index);
    }

	return true;
}
//------------------------------------------------------------------------------
bool BListView::_Select(int32 from, int32 to, bool extend)
{
	if (to < from)
		return false;

	if ((fFirstSelected != -1) && (!extend))
    {
		for (int32 item = fFirstSelected; item <= fLastSelected; ++item)
		{
			if ((ItemAt(item)->IsSelected()) && (item < from || item > to))
			{
				ItemAt(item)->Deselect();
				InvalidateItem(item);
			}
		}

		fFirstSelected = -1;
	}

	if (fFirstSelected == -1)
    {
		fFirstSelected = from;
		fLastSelected = to;
	}
    else if (from < fFirstSelected)
		fFirstSelected = from;
    else if (to > fLastSelected)
		fLastSelected = to;

	for (int32 item = from; item <= to; ++item)
	{
		if (!ItemAt(item)->IsSelected())
		{
			ItemAt(item)->Select();
			InvalidateItem(item);
		}
	}

	return true;
}
//------------------------------------------------------------------------------
bool BListView::_Deselect(int32 index)
{
	if (index < 0 || index >= CountItems())
		return false;

	if (!Window()->Lock())
		return false;

	BListItem *item = ItemAt(index);

	if (item->IsSelected())
	{
		BRect frame(ItemFrame(index));
		BRect bounds(Bounds());

		item->Deselect();

		if (fFirstSelected == index && fLastSelected == index)
		{
			fFirstSelected = -1;
			fLastSelected = -1;
		}
		else
		{
			if (fFirstSelected == index)
				fFirstSelected = CalcFirstSelected(index);

			if (fLastSelected == index)
				fLastSelected = CalcLastSelected(index);
		}

		if (bounds.Intersects(frame))
			DrawItem(ItemAt(index), frame, true);
	}

	Window()->Unlock();

	return true;
}
//------------------------------------------------------------------------------
void BListView::Deselect(int32 from, int32 to)
{
	if (from < 0 || from >= CountItems() || to < 0 || to >= CountItems())
		return;

	bool changed = false;

	for (int32 i = from; i <= to; i++)
	{
		if (_Deselect(i))
			changed = true;
	}

	if (changed)
	{
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}
}
//------------------------------------------------------------------------------
bool BListView::_DeselectAll(int32 except_from, int32 except_to)
{
	if (fFirstSelected == -1)
		return true;

	return true;
}
//------------------------------------------------------------------------------
bool BListView::TryInitiateDrag(BPoint where)
{
	return false;
}
//------------------------------------------------------------------------------
int32 BListView::CalcFirstSelected(int32 after)
{
	if (after >= CountItems())
		return -1;

	for (int32 i = after; i < CountItems(); i++)
	{
		if (ItemAt(i)->IsSelected())
			return i;
	}

	return -1;
}
//------------------------------------------------------------------------------
int32 BListView::CalcLastSelected(int32 before)
{
	if (before < 0)
		return -1;

	for (int32 i = before; i >= 0; i--)
	{
		if (ItemAt(i)->IsSelected())
			return i;
	}

	return -1;
}
//------------------------------------------------------------------------------
void BListView::DrawItem(BListItem *item, BRect itemRect, bool complete)
{
	item->DrawItem(this, itemRect, complete);
}
//------------------------------------------------------------------------------
bool BListView::DoSwapItems(int32 a, int32 b)
{
	if (!fList.SwapItems(a, b))
		return false;

	Invalidate(ItemFrame(a));
	Invalidate(ItemFrame(b));

	if (fAnchorIndex == a)
		fAnchorIndex = b;
	else if (fAnchorIndex == b)
		fAnchorIndex = a;

	RescanSelection(a, b);

	return true;
}
//------------------------------------------------------------------------------
bool BListView::DoMoveItem(int32 from, int32 to)
{
	BRect frameFrom = ItemFrame(from);
	BRect frameTo = ItemFrame(to);

	if (!fList.MoveItem(from, to))
		return false;

	RescanSelection(from, to);

	BRect frame = frameFrom | frameTo;

	if (Bounds().Intersects(frame))
		Invalidate(Bounds() & frame);

	return true;
}
//------------------------------------------------------------------------------
bool BListView::DoReplaceItem(int32 index, BListItem *item)
{
	BRect frame = ItemFrame(index);

	if (!fList.ReplaceItem(index, item))
		return false;

	if (frame != ItemFrame(index))
		InvalidateFrom(index);
	else
		Invalidate(frame);

	return true;
}
//------------------------------------------------------------------------------
void BListView::RescanSelection(int32 from, int32 to)
{
	if (from > to)
	{
		int32 tmp = from;
		from = to;
		to = tmp;
	}

	if (fAnchorIndex != -1)
	{
		if (fAnchorIndex == from)
			fAnchorIndex = to;
		else if (fAnchorIndex == to)
			fAnchorIndex = from;
	}

/*	if (from < fFirstSelected && from < fLastSelected)
		return;

	if (to > fFirstSelected && to > fLastSelected)
		return;*/
		
	int32 i;

	for (i = from; i <= to; i++)
	{
		if (ItemAt(i)->IsSelected())
		{
			fFirstSelected = i;
			break;
		}
	}

	for (i = from; i <= to; i++)
		if (ItemAt(i)->IsSelected())
			fLastSelected = i;
}
//------------------------------------------------------------------------------
void BListView::DoMouseUp(BPoint where)
{
}
//------------------------------------------------------------------------------
void BListView::DoMouseMoved(BPoint where)
{
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
