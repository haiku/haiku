/*
 * Copyright 2001-2015 Haiku, Inc. All rights resrerved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Assmus, superstippi@gmx.de
 *		Axel Dörfler, axeld@pinc-software.de
 *		Marc Flerackers, mflerackers@androme.be
 *		Rene Gollent, rene@gollent.com
 *		Ulrich Wimboeck
 */


#include <ListView.h>

#include <algorithm>

#include <stdio.h>

#include <Autolock.h>
#include <LayoutUtils.h>
#include <PropertyInfo.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <Thread.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>


struct track_data {
	BPoint		drag_start;
	int32		item_index;
	bool		was_selected;
	bool		try_drag;
	bool		is_dragging;
	bigtime_t	last_click_time;
};


const float kDoubleClickThreshold = 6.0f;


static property_info sProperties[] = {
	{ "Item", { B_COUNT_PROPERTIES, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Returns the number of BListItems currently in the list.", 0,
		{ B_INT32_TYPE }
	},

	{ "Item", { B_EXECUTE_PROPERTY, 0 }, { B_INDEX_SPECIFIER,
		B_REVERSE_INDEX_SPECIFIER, B_RANGE_SPECIFIER,
		B_REVERSE_RANGE_SPECIFIER, 0 },
		"Select and invoke the specified items, first removing any existing "
		"selection."
	},

	{ "Selection", { B_COUNT_PROPERTIES, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Returns int32 count of items in the selection.", 0, { B_INT32_TYPE }
	},

	{ "Selection", { B_EXECUTE_PROPERTY, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Invoke items in selection."
	},

	{ "Selection", { B_GET_PROPERTY, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Returns int32 indices of all items in the selection.", 0,
		{ B_INT32_TYPE }
	},

	{ "Selection", { B_SET_PROPERTY, 0 }, { B_INDEX_SPECIFIER,
		B_REVERSE_INDEX_SPECIFIER, B_RANGE_SPECIFIER,
		B_REVERSE_RANGE_SPECIFIER, 0 },
		"Extends current selection or deselects specified items. Boolean field "
		"\"data\" chooses selection or deselection.", 0, { B_BOOL_TYPE }
	},

	{ "Selection", { B_SET_PROPERTY, 0 }, { B_DIRECT_SPECIFIER, 0 },
		"Select or deselect all items in the selection. Boolean field \"data\" "
		"chooses selection or deselection.", 0, { B_BOOL_TYPE }
	},

	{ 0 }
};


BListView::BListView(BRect frame, const char* name, list_view_type type,
	uint32 resizingMode, uint32 flags)
	:
	BView(frame, name, resizingMode, flags | B_SCROLL_VIEW_AWARE)
{
	_InitObject(type);
}


BListView::BListView(const char* name, list_view_type type, uint32 flags)
	:
	BView(name, flags | B_SCROLL_VIEW_AWARE)
{
	_InitObject(type);
}


BListView::BListView(list_view_type type)
	:
	BView(NULL, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE
		| B_SCROLL_VIEW_AWARE)
{
	_InitObject(type);
}


BListView::BListView(BMessage* archive)
	:
	BView(archive)
{
	int32 listType;
	archive->FindInt32("_lv_type", &listType);
	_InitObject((list_view_type)listType);

	int32 i = 0;
	BMessage subData;
	while (archive->FindMessage("_l_items", i++, &subData) == B_OK) {
		BArchivable* object = instantiate_object(&subData);
		if (object == NULL)
			continue;

		BListItem* item = dynamic_cast<BListItem*>(object);
		if (item != NULL)
			AddItem(item);
	}

	if (archive->HasMessage("_msg")) {
		BMessage* invokationMessage = new BMessage;

		archive->FindMessage("_msg", invokationMessage);
		SetInvocationMessage(invokationMessage);
	}

	if (archive->HasMessage("_2nd_msg")) {
		BMessage* selectionMessage = new BMessage;

		archive->FindMessage("_2nd_msg", selectionMessage);
		SetSelectionMessage(selectionMessage);
	}
}


BListView::~BListView()
{
	// NOTE: According to BeBook, BListView does not free the items itself.
	delete fTrack;
	SetSelectionMessage(NULL);
}


// #pragma mark -


BArchivable*
BListView::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BListView"))
		return new BListView(archive);

	return NULL;
}


status_t
BListView::Archive(BMessage* data, bool deep) const
{
	status_t status = BView::Archive(data, deep);
	if (status < B_OK)
		return status;

	status = data->AddInt32("_lv_type", fListType);
	if (status == B_OK && deep) {
		BListItem* item;
		int32 i = 0;

		while ((item = ItemAt(i++)) != NULL) {
			BMessage subData;
			status = item->Archive(&subData, true);
			if (status >= B_OK)
				status = data->AddMessage("_l_items", &subData);

			if (status < B_OK)
				break;
		}
	}

	if (status >= B_OK && InvocationMessage() != NULL)
		status = data->AddMessage("_msg", InvocationMessage());

	if (status == B_OK && fSelectMessage != NULL)
		status = data->AddMessage("_2nd_msg", fSelectMessage);

	return status;
}


// #pragma mark -


void
BListView::Draw(BRect updateRect)
{
	int32 count = CountItems();
	if (count == 0)
		return;

	BRect itemFrame(0, 0, Bounds().right, -1);
	for (int i = 0; i < count; i++) {
		BListItem* item = ItemAt(i);
		itemFrame.bottom = itemFrame.top + ceilf(item->Height()) - 1;

		if (itemFrame.Intersects(updateRect))
			DrawItem(item, itemFrame);

		itemFrame.top = itemFrame.bottom + 1;
	}
}


void
BListView::AttachedToWindow()
{
	BView::AttachedToWindow();
	_UpdateItems();

	if (!Messenger().IsValid())
		SetTarget(Window(), NULL);

	_FixupScrollBar();
}


void
BListView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


void
BListView::AllAttached()
{
	BView::AllAttached();
}


void
BListView::AllDetached()
{
	BView::AllDetached();
}


void
BListView::FrameResized(float newWidth, float newHeight)
{
	_FixupScrollBar();

	// notify items of new width.
	_UpdateItems();
}


void
BListView::FrameMoved(BPoint newPosition)
{
	BView::FrameMoved(newPosition);
}


void
BListView::TargetedByScrollView(BScrollView* view)
{
	fScrollView = view;
	// TODO: We could SetFlags(Flags() | B_FRAME_EVENTS) here, but that
	// may mess up application code which manages this by some other means
	// and doesn't want us to be messing with flags.
}


void
BListView::WindowActivated(bool active)
{
	BView::WindowActivated(active);
}


// #pragma mark -


void
BListView::MessageReceived(BMessage* message)
{
	if (message->HasSpecifiers()) {
		BMessage reply(B_REPLY);
		status_t err = B_BAD_SCRIPT_SYNTAX;
		int32 index;
		BMessage specifier;
		int32 what;
		const char* property;

		if (message->GetCurrentSpecifier(&index, &specifier, &what, &property)
				!= B_OK) {
			return BView::MessageReceived(message);
		}

		BPropertyInfo propInfo(sProperties);
		switch (propInfo.FindMatch(message, index, &specifier, what,
			property)) {
			case 0: // Item: Count
				err = reply.AddInt32("result", CountItems());
				break;

			case 1: { // Item: EXECUTE
				switch (what) {
					case B_INDEX_SPECIFIER:
					case B_REVERSE_INDEX_SPECIFIER: {
						int32 index;
						err = specifier.FindInt32("index", &index);
						if (err >= B_OK) {
							if (what == B_REVERSE_INDEX_SPECIFIER)
								index = CountItems() - index;
							if (index < 0 || index >= CountItems())
								err = B_BAD_INDEX;
						}
						if (err >= B_OK) {
							Select(index, false);
							Invoke();
						}
						break;
					}
					case B_RANGE_SPECIFIER: {
					case B_REVERSE_RANGE_SPECIFIER:
						int32 beg, end, range;
						err = specifier.FindInt32("index", &beg);
						if (err >= B_OK)
							err = specifier.FindInt32("range", &range);
						if (err >= B_OK) {
							if (what == B_REVERSE_RANGE_SPECIFIER)
								beg = CountItems() - beg;
							end = beg + range;
							if (!(beg >= 0 && beg <= end && end < CountItems()))
								err = B_BAD_INDEX;
							if (err >= B_OK) {
								if (fListType != B_MULTIPLE_SELECTION_LIST
									&& end - beg > 1)
									err = B_BAD_VALUE;
								if (err >= B_OK) {
									Select(beg, end - 1, false);
									Invoke();
								}
							}
						}
						break;
					}
				}
				break;
			}
			case 2: { // Selection: COUNT
					int32 count = 0;

					for (int32 i = 0; i < CountItems(); i++) {
						if (ItemAt(i)->IsSelected())
							count++;
					}

				err = reply.AddInt32("result", count);
				break;
			}
			case 3: // Selection: EXECUTE
				err = Invoke();
				break;

			case 4: // Selection: GET
				err = B_OK;
				for (int32 i = 0; err >= B_OK && i < CountItems(); i++) {
					if (ItemAt(i)->IsSelected())
						err = reply.AddInt32("result", i);
				}
				break;

			case 5: { // Selection: SET
				bool doSelect;
				err = message->FindBool("data", &doSelect);
				if (err >= B_OK) {
					switch (what) {
						case B_INDEX_SPECIFIER:
						case B_REVERSE_INDEX_SPECIFIER: {
							int32 index;
							err = specifier.FindInt32("index", &index);
							if (err >= B_OK) {
								if (what == B_REVERSE_INDEX_SPECIFIER)
									index = CountItems() - index;
								if (index < 0 || index >= CountItems())
									err = B_BAD_INDEX;
							}
							if (err >= B_OK) {
								if (doSelect)
									Select(index,
										fListType == B_MULTIPLE_SELECTION_LIST);
								else
									Deselect(index);
							}
							break;
						}
						case B_RANGE_SPECIFIER: {
						case B_REVERSE_RANGE_SPECIFIER:
							int32 beg, end, range;
							err = specifier.FindInt32("index", &beg);
							if (err >= B_OK)
								err = specifier.FindInt32("range", &range);
							if (err >= B_OK) {
								if (what == B_REVERSE_RANGE_SPECIFIER)
									beg = CountItems() - beg;
								end = beg + range;
								if (!(beg >= 0 && beg <= end
									&& end < CountItems()))
									err = B_BAD_INDEX;
								if (err >= B_OK) {
									if (fListType != B_MULTIPLE_SELECTION_LIST
										&& end - beg > 1)
										err = B_BAD_VALUE;
									if (doSelect)
										Select(beg, end - 1, fListType
											== B_MULTIPLE_SELECTION_LIST);
									else {
										for (int32 i = beg; i < end; i++)
											Deselect(i);
									}
								}
							}
							break;
						}
					}
				}
				break;
			}
			case 6: // Selection: SET (select/deselect all)
				bool doSelect;
				err = message->FindBool("data", &doSelect);
				if (err >= B_OK) {
					if (doSelect)
						Select(0, CountItems() - 1, true);
					else
						DeselectAll();
				}
				break;

			default:
				return BView::MessageReceived(message);
		}

		if (err != B_OK) {
			reply.what = B_MESSAGE_NOT_UNDERSTOOD;
			reply.AddString("message", strerror(err));
		}

		reply.AddInt32("error", err);
		message->SendReply(&reply);
		return;
	}

	switch (message->what) {
		case B_MOUSE_WHEEL_CHANGED:
			if (!fTrack->is_dragging)
				BView::MessageReceived(message);
			break;

		case B_SELECT_ALL:
			if (fListType == B_MULTIPLE_SELECTION_LIST)
				Select(0, CountItems() - 1, false);
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
BListView::KeyDown(const char* bytes, int32 numBytes)
{
	bool extend = fListType == B_MULTIPLE_SELECTION_LIST
		&& (modifiers() & B_SHIFT_KEY) != 0;

	if (fFirstSelected == -1
		&& (bytes[0] == B_UP_ARROW || bytes[0] == B_DOWN_ARROW)) {
		// nothing is selected yet, select the first enabled item
		int32 lastItem = CountItems() - 1;
		for (int32 i = 0; i <= lastItem; i++) {
			if (ItemAt(i)->IsEnabled()) {
				Select(i);
				break;
			}
		}
		return;
	}

	switch (bytes[0]) {
		case B_UP_ARROW:
		{
			if (fAnchorIndex > 0) {
				if (!extend || fAnchorIndex <= fFirstSelected) {
					for (int32 i = 1; fAnchorIndex - i >= 0; i++) {
						if (ItemAt(fAnchorIndex - i)->IsEnabled()) {
							// Select the previous enabled item
							Select(fAnchorIndex - i, extend);
							break;
						}
					}
				} else {
					Deselect(fAnchorIndex);
					do
						fAnchorIndex--;
					while (fAnchorIndex > 0
						&& !ItemAt(fAnchorIndex)->IsEnabled());
				}
			}

			ScrollToSelection();
			break;
		}

		case B_DOWN_ARROW:
		{
			int32 lastItem = CountItems() - 1;
			if (fAnchorIndex < lastItem) {
				if (!extend || fAnchorIndex >= fLastSelected) {
					for (int32 i = 1; fAnchorIndex + i <= lastItem; i++) {
						if (ItemAt(fAnchorIndex + i)->IsEnabled()) {
							// Select the next enabled item
							Select(fAnchorIndex + i, extend);
							break;
						}
					}
				} else {
					Deselect(fAnchorIndex);
					do
						fAnchorIndex++;
					while (fAnchorIndex < lastItem
						&& !ItemAt(fAnchorIndex)->IsEnabled());
				}
			}

			ScrollToSelection();
			break;
		}

		case B_HOME:
			if (extend) {
				Select(0, fAnchorIndex, true);
				fAnchorIndex = 0;
			} else {
				// select the first enabled item
				int32 lastItem = CountItems() - 1;
				for (int32 i = 0; i <= lastItem; i++) {
					if (ItemAt(i)->IsEnabled()) {
						Select(i, false);
						break;
					}
				}
			}

			ScrollToSelection();
			break;

		case B_END:
			if (extend) {
				Select(fAnchorIndex, CountItems() - 1, true);
				fAnchorIndex = CountItems() - 1;
			} else {
				// select the last enabled item
				for (int32 i = CountItems() - 1; i >= 0; i--) {
					if (ItemAt(i)->IsEnabled()) {
						Select(i, false);
						break;
					}
				}
			}

			ScrollToSelection();
			break;

		case B_PAGE_UP:
		{
			BPoint scrollOffset(LeftTop());
			scrollOffset.y = std::max(0.0f, scrollOffset.y - Bounds().Height());
			ScrollTo(scrollOffset);
			break;
		}

		case B_PAGE_DOWN:
		{
			BPoint scrollOffset(LeftTop());
			if (BListItem* item = LastItem()) {
				scrollOffset.y += Bounds().Height();
				scrollOffset.y = std::min(item->Bottom() - Bounds().Height(),
					scrollOffset.y);
			}
			ScrollTo(scrollOffset);
			break;
		}

		case B_RETURN:
		case B_SPACE:
			Invoke();
			break;

		default:
			BView::KeyDown(bytes, numBytes);
	}
}


void
BListView::MouseDown(BPoint where)
{
	if (!IsFocus()) {
		MakeFocus();
		Sync();
		Window()->UpdateIfNeeded();
	}

	int32 index = IndexOf(where);
	int32 modifiers = 0;

	BMessage* message = Looper()->CurrentMessage();
	if (message != NULL)
		message->FindInt32("modifiers", &modifiers);

	// If the user double (or more) clicked within the current selection,
	// we don't change the selection but invoke the selection.
	// TODO: move this code someplace where it can be shared everywhere
	// instead of every class having to reimplement it, once some sane
	// API for it is decided.
	BPoint delta = where - fTrack->drag_start;
	bigtime_t sysTime;
	Window()->CurrentMessage()->FindInt64("when", &sysTime);
	bigtime_t timeDelta = sysTime - fTrack->last_click_time;
	bigtime_t doubleClickSpeed;
	get_click_speed(&doubleClickSpeed);
	bool doubleClick = false;

	if (timeDelta < doubleClickSpeed
		&& fabs(delta.x) < kDoubleClickThreshold
		&& fabs(delta.y) < kDoubleClickThreshold
		&& fTrack->item_index == index) {
		doubleClick = true;
	}

	if (doubleClick && index >= fFirstSelected && index <= fLastSelected) {
		fTrack->drag_start.Set(INT32_MAX, INT32_MAX);
		Invoke();
		return BView::MouseDown(where);
	}

	if (!doubleClick) {
		fTrack->drag_start = where;
		fTrack->last_click_time = system_time();
		fTrack->item_index = index;
		fTrack->was_selected = index >= 0 ? ItemAt(index)->IsSelected() : false;
		fTrack->try_drag = true;

		SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	}

	if (index >= 0) {
		if (fListType == B_MULTIPLE_SELECTION_LIST) {
			if ((modifiers & B_SHIFT_KEY) != 0) {
				// select entire block
				if (index >= fFirstSelected && index < fLastSelected)
					// clicked inside of selected items block, deselect all
					// but from the first selected item to the clicked item
					DeselectExcept(fFirstSelected, index);
				else
					Select(std::min(index, fFirstSelected), std::max(index,
						fLastSelected));
			} else {
				if ((modifiers & B_COMMAND_KEY) != 0) {
					// toggle selection state of clicked item (like in Tracker)
					if (ItemAt(index)->IsSelected())
						Deselect(index);
					else
						Select(index, true);
				} else if (!ItemAt(index)->IsSelected())
					// To enable multi-select drag and drop, we only
					// exclusively select a single item if it's not one of the
					// already selected items. This behavior gives the mouse
					// tracking thread the opportunity to initiate the
					// multi-selection drag with all the items still selected.
					Select(index);
			}
		} else {
			// toggle selection state of clicked item (except drag & drop)
			if ((modifiers & B_COMMAND_KEY) != 0 && ItemAt(index)->IsSelected())
				Deselect(index);
			else
				Select(index);
		}
	} else if ((modifiers & B_COMMAND_KEY) == 0)
		DeselectAll();

	BView::MouseDown(where);
}


void
BListView::MouseUp(BPoint where)
{
	BView::MouseUp(where);

	uint32* buttons = 0;
	GetMouse(&where, buttons);

	if (buttons == 0) {
		fTrack->try_drag = false;
		fTrack->is_dragging = false;
	}
}


void
BListView::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	BView::MouseMoved(where, code, dragMessage);

	if (fTrack->item_index >= 0 && fTrack->try_drag) {
		// initiate a drag if the mouse was moved far enough
		BPoint offset = where - fTrack->drag_start;
		float dragDistance = sqrtf(offset.x * offset.x + offset.y * offset.y);
		if (dragDistance >= 5.0f) {
			fTrack->try_drag = false;
			fTrack->is_dragging = InitiateDrag(fTrack->drag_start,
				fTrack->item_index, fTrack->was_selected);
		}
	}
}


bool
BListView::InitiateDrag(BPoint where, int32 index, bool wasSelected)
{
	return false;
}


// #pragma mark -


void
BListView::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}


void
BListView::GetPreferredSize(float *_width, float *_height)
{
	int32 count = CountItems();

	if (count > 0) {
		float maxWidth = 0.0;
		for (int32 i = 0; i < count; i++) {
			float itemWidth = ItemAt(i)->Width();
			if (itemWidth > maxWidth)
				maxWidth = itemWidth;
		}

		if (_width != NULL)
			*_width = maxWidth;
		if (_height != NULL)
			*_height = ItemAt(count - 1)->Bottom();
	} else
		BView::GetPreferredSize(_width, _height);
}


BSize
BListView::MinSize()
{
	// We need a stable min size: the BView implementation uses
	// GetPreferredSize(), which by default just returns the current size.
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(10, 10));
}


BSize
BListView::MaxSize()
{
	return BView::MaxSize();
}


BSize
BListView::PreferredSize()
{
	// We need a stable preferred size: the BView implementation uses
	// GetPreferredSize(), which by default just returns the current size.
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), BSize(100, 50));
}


// #pragma mark -


void
BListView::MakeFocus(bool focused)
{
	if (IsFocus() == focused)
		return;

	BView::MakeFocus(focused);

	if (fScrollView)
		fScrollView->SetBorderHighlighted(focused);
}


void
BListView::SetFont(const BFont* font, uint32 mask)
{
	BView::SetFont(font, mask);

	if (Window() != NULL && !Window()->InViewTransaction())
		_UpdateItems();
}


void
BListView::ScrollTo(BPoint point)
{
	BView::ScrollTo(point);
}


// #pragma mark - List ops


bool
BListView::AddItem(BListItem* item, int32 index)
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
		item->SetTop((index > 0) ? ItemAt(index - 1)->Bottom() + 1.0 : 0.0);

		item->Update(this, &font);
		_RecalcItemTops(index + 1);

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
		int32 index = CountItems() - 1;
		item->SetTop((index > 0) ? ItemAt(index - 1)->Bottom() + 1.0 : 0.0);

		item->Update(this, &font);

		_FixupScrollBar();
		InvalidateItem(CountItems() - 1);
	}

	return true;
}


bool
BListView::AddList(BList* list, int32 index)
{
	if (!fList.AddList(list, index))
		return false;

	int32 count = list->CountItems();

	if (fFirstSelected != -1 && index < fFirstSelected)
		fFirstSelected += count;

	if (fLastSelected != -1 && index < fLastSelected)
		fLastSelected += count;

	if (Window()) {
		BFont font;
		GetFont(&font);

		for (int32 i = index; i <= (index + count - 1); i++) {
			ItemAt(i)->SetTop((i > 0) ? ItemAt(i - 1)->Bottom() + 1.0 : 0.0);
			ItemAt(i)->Update(this, &font);
		}

		_RecalcItemTops(index + count - 1);

		_FixupScrollBar();
		Invalidate(); // TODO
	}

	return true;
}


bool
BListView::AddList(BList* list)
{
	return AddList(list, CountItems());
}


BListItem*
BListView::RemoveItem(int32 index)
{
	BListItem* item = ItemAt(index);
	if (item == NULL)
		return NULL;

	if (item->IsSelected())
		Deselect(index);

	if (!fList.RemoveItem(item))
		return NULL;

	if (fFirstSelected != -1 && index < fFirstSelected)
		fFirstSelected--;

	if (fLastSelected != -1 && index < fLastSelected)
		fLastSelected--;

	if (fAnchorIndex != -1 && index < fAnchorIndex)
		fAnchorIndex--;

	_RecalcItemTops(index);

	_InvalidateFrom(index);
	_FixupScrollBar();

	return item;
}


bool
BListView::RemoveItem(BListItem* item)
{
	return BListView::RemoveItem(IndexOf(item)) != NULL;
}


bool
BListView::RemoveItems(int32 index, int32 count)
{
	if (index >= fList.CountItems())
		index = -1;

	if (index < 0)
		return false;

	if (fAnchorIndex != -1 && index < fAnchorIndex)
		fAnchorIndex = index;

	fList.RemoveItems(index, count);
	if (index < fList.CountItems())
		_RecalcItemTops(index);

	Invalidate();
	return true;
}


void
BListView::SetSelectionMessage(BMessage* message)
{
	delete fSelectMessage;
	fSelectMessage = message;
}


void
BListView::SetInvocationMessage(BMessage* message)
{
	BInvoker::SetMessage(message);
}


BMessage*
BListView::InvocationMessage() const
{
	return BInvoker::Message();
}


uint32
BListView::InvocationCommand() const
{
	return BInvoker::Command();
}


BMessage*
BListView::SelectionMessage() const
{
	return fSelectMessage;
}


uint32
BListView::SelectionCommand() const
{
	if (fSelectMessage)
		return fSelectMessage->what;

	return 0;
}


void
BListView::SetListType(list_view_type type)
{
	if (fListType == B_MULTIPLE_SELECTION_LIST
		&& type == B_SINGLE_SELECTION_LIST) {
		Select(CurrentSelection(0));
	}

	fListType = type;
}


list_view_type
BListView::ListType() const
{
	return fListType;
}


BListItem*
BListView::ItemAt(int32 index) const
{
	return (BListItem*)fList.ItemAt(index);
}


int32
BListView::IndexOf(BListItem* item) const
{
	if (Window()) {
		if (item != NULL) {
			int32 index = IndexOf(BPoint(0.0, item->Top()));
			if (index >= 0 && fList.ItemAt(index) == item)
				return index;

			return -1;
		}
	}
	return fList.IndexOf(item);
}


int32
BListView::IndexOf(BPoint point) const
{
	int32 low = 0;
	int32 high = fList.CountItems() - 1;
	int32 mid = -1;
	float frameTop = -1.0;
	float frameBottom = 1.0;

	// binary search the list
	while (high >= low) {
		mid = (low + high) / 2;
		frameTop = ItemAt(mid)->Top();
		frameBottom = ItemAt(mid)->Bottom();
		if (point.y < frameTop)
			high = mid - 1;
		else if (point.y > frameBottom)
			low = mid + 1;
		else
			return mid;
	}

	return -1;
}


BListItem*
BListView::FirstItem() const
{
	return (BListItem*)fList.FirstItem();
}


BListItem*
BListView::LastItem() const
{
	return (BListItem*)fList.LastItem();
}


bool
BListView::HasItem(BListItem *item) const
{
	return IndexOf(item) != -1;
}


int32
BListView::CountItems() const
{
	return fList.CountItems();
}


void
BListView::MakeEmpty()
{
	if (fList.IsEmpty())
		return;

	_DeselectAll(-1, -1);
	fList.MakeEmpty();

	if (Window()) {
		_FixupScrollBar();
		Invalidate();
	}
}


bool
BListView::IsEmpty() const
{
	return fList.IsEmpty();
}


void
BListView::DoForEach(bool (*func)(BListItem*))
{
	fList.DoForEach(reinterpret_cast<bool (*)(void*)>(func));
}


void
BListView::DoForEach(bool (*func)(BListItem*, void*), void* arg)
{
	fList.DoForEach(reinterpret_cast<bool (*)(void*, void*)>(func), arg);
}


const BListItem**
BListView::Items() const
{
	return (const BListItem**)fList.Items();
}


void
BListView::InvalidateItem(int32 index)
{
	Invalidate(ItemFrame(index));
}


void
BListView::ScrollToSelection()
{
	BRect itemFrame = ItemFrame(CurrentSelection(0));

	if (itemFrame.top < Bounds().top
		|| itemFrame.Height() > Bounds().Height())
		ScrollBy(0, itemFrame.top - Bounds().top);
	else if (itemFrame.bottom > Bounds().bottom)
		ScrollBy(0, itemFrame.bottom - Bounds().bottom);
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
	BListItem* item = ItemAt(index);
	if (item != NULL)
		return item->IsSelected();

	return false;
}


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


status_t
BListView::Invoke(BMessage* message)
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
	_RecalcItemTops(0);
	Invalidate();
}


bool
BListView::SwapItems(int32 a, int32 b)
{
	MiscData data;

	data.swap.a = a;
	data.swap.b = b;

	return DoMiscellaneous(B_SWAP_OP, &data);
}


bool
BListView::MoveItem(int32 from, int32 to)
{
	MiscData data;

	data.move.from = from;
	data.move.to = to;

	return DoMiscellaneous(B_MOVE_OP, &data);
}


bool
BListView::ReplaceItem(int32 index, BListItem* item)
{
	MiscData data;

	data.replace.index = index;
	data.replace.item = item;

	return DoMiscellaneous(B_REPLACE_OP, &data);
}


BRect
BListView::ItemFrame(int32 index)
{
	BRect frame = Bounds();
	if (index < 0 || index >= CountItems()) {
		frame.top = 0;
		frame.bottom = -1;
	} else {
		BListItem* item = ItemAt(index);
		frame.top = item->Top();
		frame.bottom = item->Bottom();
	}
	return frame;
}


// #pragma mark -


BHandler*
BListView::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	BPropertyInfo propInfo(sProperties);

	if (propInfo.FindMatch(message, 0, specifier, what, property) < 0) {
		return BView::ResolveSpecifier(message, index, specifier, what,
			property);
	}

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


status_t
BListView::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BListView::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BListView::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BListView::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BListView::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BListView::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BListView::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BListView::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BListView::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BListView::DoLayout();
			return B_OK;
		}
	}

	return BView::Perform(code, _data);
}


bool
BListView::DoMiscellaneous(MiscCode code, MiscData* data)
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


// #pragma mark -


void BListView::_ReservedListView2() {}
void BListView::_ReservedListView3() {}
void BListView::_ReservedListView4() {}


BListView&
BListView::operator=(const BListView& /*other*/)
{
	return *this;
}


// #pragma mark -


void
BListView::_InitObject(list_view_type type)
{
	fListType = type;
	fFirstSelected = -1;
	fLastSelected = -1;
	fAnchorIndex = -1;
	fSelectMessage = NULL;
	fScrollView = NULL;

	fTrack = new track_data;
	fTrack->drag_start = B_ORIGIN;
	fTrack->item_index = -1;
	fTrack->was_selected = false;
	fTrack->try_drag = false;
	fTrack->is_dragging = false;
	fTrack->last_click_time = 0;

	SetViewUIColor(B_LIST_BACKGROUND_COLOR);
	SetLowUIColor(B_LIST_BACKGROUND_COLOR);
}


void
BListView::_FixupScrollBar()
{

	BScrollBar* vertScroller = ScrollBar(B_VERTICAL);
	if (vertScroller != NULL) {
		BRect bounds = Bounds();
		int32 count = CountItems();

		float itemHeight = 0.0;

		if (CountItems() > 0)
			itemHeight = ItemAt(CountItems() - 1)->Bottom();

		if (bounds.Height() > itemHeight) {
			// no scrolling
			vertScroller->SetRange(0.0, 0.0);
			vertScroller->SetValue(0.0);
				// also scrolls ListView to the top
		} else {
			vertScroller->SetRange(0.0, itemHeight - bounds.Height() - 1.0);
			vertScroller->SetProportion(bounds.Height () / itemHeight);
			// scroll up if there is empty room on bottom
			if (itemHeight < bounds.bottom)
				ScrollBy(0.0, bounds.bottom - itemHeight);
		}

		if (count != 0)
			vertScroller->SetSteps(
				ceilf(FirstItem()->Height()), bounds.Height());
	}

	BScrollBar* horizontalScroller = ScrollBar(B_HORIZONTAL);
	if (horizontalScroller != NULL) {
		float w;
		GetPreferredSize(&w, NULL);
		BRect scrollBarSize = horizontalScroller->Bounds();

		if (w <= scrollBarSize.Width()) {
			// no scrolling
			horizontalScroller->SetRange(0.0, 0.0);
			horizontalScroller->SetValue(0.0);
		} else {
			horizontalScroller->SetRange(0, w - scrollBarSize.Width());
			horizontalScroller->SetProportion(scrollBarSize.Width() / w);
		}
	}
}


void
BListView::_InvalidateFrom(int32 index)
{
	// make sure index is behind last valid index
	int32 count = CountItems();
	if (index >= count)
		index = count;

	// take the item before the wanted one,
	// because that might already be removed
	index--;
	BRect dirty = Bounds();
	if (index >= 0)
		dirty.top = ItemFrame(index).bottom + 1;

	Invalidate(dirty);
}


void
BListView::_UpdateItems()
{
	BFont font;
	GetFont(&font);
	for (int32 i = 0; i < CountItems(); i++) {
		ItemAt(i)->SetTop((i > 0) ? ItemAt(i - 1)->Bottom() + 1.0 : 0.0);
		ItemAt(i)->Update(this, &font);
	}
}


/*!	Selects the item at the specified \a index, and returns \c true in
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
	if (Window() != NULL && !locker.IsLocked())
		return false;

	bool changed = false;

	if (!extend && fFirstSelected != -1)
		changed = _DeselectAll(index, index);

	fAnchorIndex = index;

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

	item->Select();
	if (Window() != NULL)
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
	} else {
		if (from < fFirstSelected)
			fFirstSelected = from;
		if (to > fLastSelected)
			fLastSelected = to;
	}

	for (int32 i = from; i <= to; ++i) {
		BListItem* item = ItemAt(i);
		if (item != NULL && !item->IsSelected() && item->IsEnabled()) {
			item->Select();
			if (Window() != NULL)
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

	BWindow* window = Window();
	BAutolock locker(window);
	if (window != NULL && !locker.IsLocked())
		return false;

	BListItem* item = ItemAt(index);

	if (item != NULL && item->IsSelected()) {
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

		if (window && bounds.Intersects(frame))
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

		BListItem* item = ItemAt(index);
		if (item != NULL && item->IsSelected()) {
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

	before = std::min(CountItems() - 1, before);

	for (int32 i = before; i >= 0; i--) {
		if (ItemAt(i)->IsSelected())
			return i;
	}

	return -1;
}


void
BListView::DrawItem(BListItem* item, BRect itemRect, bool complete)
{
	if (!item->IsEnabled()) {
		rgb_color textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);
		rgb_color disabledColor;
		if (textColor.red + textColor.green + textColor.blue > 128 * 3)
			disabledColor = tint_color(textColor, B_DARKEN_2_TINT);
		else
			disabledColor = tint_color(textColor, B_LIGHTEN_2_TINT);

		SetHighColor(disabledColor);
	} else if (item->IsSelected())
		SetHighColor(ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR));
	else
		SetHighColor(ui_color(B_LIST_ITEM_TEXT_COLOR));

	item->DrawItem(this, itemRect, complete);
}


bool
BListView::_SwapItems(int32 a, int32 b)
{
	// remember frames of items before anything happens,
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
	int32 first = std::min(a, b);
	int32 last = std::max(a, b);
	if (ItemAt(a)->IsSelected() != ItemAt(b)->IsSelected()) {
		if (first < fFirstSelected || last > fLastSelected) {
			_RescanSelection(std::min(first, fFirstSelected),
				std::max(last, fLastSelected));
		}
		// though the actually selected items stayed the
		// same, the selection has still changed
		SelectionChanged();
	}

	ItemAt(a)->SetTop(aFrame.top);
	ItemAt(b)->SetTop(bFrame.top);

	// take care of invalidation
	if (Window()) {
		// NOTE: window looper is assumed to be locked!
		if (aFrame.Height() != bFrame.Height()) {
			_RecalcItemTops(first, last);
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

	_RecalcItemTops((to > from) ? from : to);

	// take care of invalidation
	if (Window()) {
		// NOTE: window looper is assumed to be locked!
		Invalidate(frameFrom | frameTo);
	}

	return true;
}


bool
BListView::_ReplaceItem(int32 index, BListItem* item)
{
	if (item == NULL)
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
		int32 start = std::min(fFirstSelected, index);
		int32 end = std::max(fLastSelected, index);
		_RescanSelection(start, end);
		SelectionChanged();
	}
	_RecalcItemTops(index);

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

	from = std::max((int32)0, from);
	to = std::min(to, CountItems() - 1);

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

	fLastSelected = fFirstSelected;
	for (int32 i = from; i <= to; i++) {
		if (ItemAt(i)->IsSelected())
			fLastSelected = i;
	}
}


void
BListView::_RecalcItemTops(int32 start, int32 end)
{
	int32 count = CountItems();
	if ((start < 0) || (start >= count))
		return;

	if (end >= 0)
		count = end + 1;

	float top = (start == 0) ? 0.0 : ItemAt(start - 1)->Bottom() + 1.0;

	for (int32 i = start; i < count; i++) {
		BListItem *item = ItemAt(i);
		item->SetTop(top);
		top += ceilf(item->Height());
	}
}
