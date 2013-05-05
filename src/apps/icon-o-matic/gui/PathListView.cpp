/*
 * Copyright 2006-2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "PathListView.h"

#include <new>
#include <stdio.h>

#include <Application.h>
#include <Catalog.h>
#include <ListItem.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Mime.h>
#include <Window.h>

#include "AddPathsCommand.h"
#include "CleanUpPathCommand.h"
#include "CommandStack.h"
#include "MovePathsCommand.h"
#include "Observer.h"
#include "RemovePathsCommand.h"
#include "ReversePathCommand.h"
#include "RotatePathIndicesCommand.h"
#include "Shape.h"
#include "ShapeContainer.h"
#include "Selection.h"
#include "UnassignPathCommand.h"
#include "Util.h"
#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-PathsList"


using std::nothrow;

static const float kMarkWidth		= 14.0;
static const float kBorderOffset	= 3.0;
static const float kTextOffset		= 4.0;


class PathListItem : public SimpleItem, public Observer {
public:
	PathListItem(VectorPath* p, PathListView* listView, bool markEnabled)
		:
		SimpleItem(""),
		path(NULL),
		fListView(listView),
		fMarkEnabled(markEnabled),
		fMarked(false)
	{
		SetPath(p);
	}


	virtual ~PathListItem()
	{
		SetPath(NULL);
	}


	// SimpleItem interface
	virtual	void Draw(BView* owner, BRect itemFrame, uint32 flags)
	{
		SimpleItem::DrawBackground(owner, itemFrame, flags);

		// text
		owner->SetHighColor(0, 0, 0, 255);
		font_height fh;
		owner->GetFontHeight(&fh);
		BString truncatedString(Text());
		owner->TruncateString(&truncatedString, B_TRUNCATE_MIDDLE,
			itemFrame.Width() - kBorderOffset - kMarkWidth - kTextOffset
				- kBorderOffset);
		float height = itemFrame.Height();
		float textHeight = fh.ascent + fh.descent;
		BPoint pos;
		pos.x = itemFrame.left + kBorderOffset + kMarkWidth + kTextOffset;
		pos.y = itemFrame.top + ceilf((height - textHeight) / 2.0 + fh.ascent);
		owner->DrawString(truncatedString.String(), pos);

		if (!fMarkEnabled)
			return;

		// mark
		BRect markRect = itemFrame;
		markRect.left += kBorderOffset;
		markRect.right = markRect.left + kMarkWidth;
		markRect.top = (markRect.top + markRect.bottom - kMarkWidth) / 2.0;
		markRect.bottom = markRect.top + kMarkWidth;
		owner->SetHighColor(tint_color(owner->LowColor(), B_DARKEN_1_TINT));
		owner->StrokeRect(markRect);
		markRect.InsetBy(1, 1);
		owner->SetHighColor(tint_color(owner->LowColor(), 1.04));
		owner->FillRect(markRect);
		if (fMarked) {
			markRect.InsetBy(2, 2);
			owner->SetHighColor(tint_color(owner->LowColor(),
				B_DARKEN_4_TINT));
			owner->SetPenSize(2);
			owner->StrokeLine(markRect.LeftTop(), markRect.RightBottom());
			owner->StrokeLine(markRect.LeftBottom(), markRect.RightTop());
			owner->SetPenSize(1);
		}
	}


	// Observer interface
	virtual	void ObjectChanged(const Observable* object)
	{
		UpdateText();
	}


	// PathListItem
	void SetPath(VectorPath* p)
	{
		if (p == path)
			return;

		if (path) {
			path->RemoveObserver(this);
			path->Release();
		}

		path = p;

		if (path) {
			path->Acquire();
			path->AddObserver(this);
			UpdateText();
		}
	}


	void UpdateText()
	{
		SetText(path->Name());
		Invalidate();
	}


	void SetMarkEnabled(bool enabled)
	{
		if (fMarkEnabled == enabled)
			return;
		fMarkEnabled = enabled;
		Invalidate();
	}


	void SetMarked(bool marked)
	{
		if (fMarked == marked)
			return;
		fMarked = marked;
		Invalidate();
	}


	void Invalidate()
	{
		if (fListView->LockLooper()) {
			fListView->InvalidateItem(
				fListView->IndexOf(this));
			fListView->UnlockLooper();
		}
	}

public:
	VectorPath* 	path;

private:
	PathListView*	fListView;
	bool			fMarkEnabled;
	bool			fMarked;
};


class ShapePathListener : public PathContainerListener,
	public ShapeContainerListener {
public:
	ShapePathListener(PathListView* listView)
		:
		fListView(listView),
		fShape(NULL)
	{
	}


	virtual ~ShapePathListener()
	{
		SetShape(NULL);
	}


	// PathContainerListener interface
	virtual void PathAdded(VectorPath* path, int32 index)
	{
		fListView->_SetPathMarked(path, true);
	}


	virtual void PathRemoved(VectorPath* path)
	{
		fListView->_SetPathMarked(path, false);
	}


	// ShapeContainerListener interface
	virtual void ShapeAdded(Shape* shape, int32 index)
	{
	}


	virtual void ShapeRemoved(Shape* shape)
	{
		fListView->SetCurrentShape(NULL);
	}


	// ShapePathListener
	void SetShape(Shape* shape)
	{
		if (fShape == shape)
			return;

		if (fShape)
			fShape->Paths()->RemoveListener(this);

		fShape = shape;

		if (fShape)
			fShape->Paths()->AddListener(this);
	}


	Shape* CurrentShape() const
	{
		return fShape;
	}

private:
	PathListView*	fListView;
	Shape*			fShape;
};


// #pragma mark -


enum {
	MSG_ADD					= 'addp',

	MSG_ADD_RECT			= 'addr',
	MSG_ADD_CIRCLE			= 'addc',
	MSG_ADD_ARC				= 'adda',

	MSG_DUPLICATE			= 'dupp',

	MSG_REVERSE				= 'rvrs',
	MSG_CLEAN_UP			= 'clup',
	MSG_ROTATE_INDICES_CW	= 'ricw',
	MSG_ROTATE_INDICES_CCW	= 'ricc',

	MSG_REMOVE				= 'remp',
};


PathListView::PathListView(BRect frame, const char* name, BMessage* message,
	BHandler* target)
	:
	SimpleListView(frame, name, NULL, B_SINGLE_SELECTION_LIST),
	fMessage(message),
	fMenu(NULL),

	fPathContainer(NULL),
	fShapeContainer(NULL),
	fCommandStack(NULL),

	fCurrentShape(NULL),
	fShapePathListener(new ShapePathListener(this))
{
	SetTarget(target);
}


PathListView::~PathListView()
{
	_MakeEmpty();
	delete fMessage;

	if (fPathContainer != NULL)
		fPathContainer->RemoveListener(this);

	if (fShapeContainer != NULL)
		fShapeContainer->RemoveListener(fShapePathListener);

	delete fShapePathListener;
}


void
PathListView::SelectionChanged()
{
	SimpleListView::SelectionChanged();

	if (!fSyncingToSelection) {
		// NOTE: single selection list
		PathListItem* item
			= dynamic_cast<PathListItem*>(ItemAt(CurrentSelection(0)));
		if (fMessage != NULL) {
			BMessage message(*fMessage);
			message.AddPointer("path", item ? (void*)item->path : NULL);
			Invoke(&message);
		}
	}

	_UpdateMenu();
}


void
PathListView::MouseDown(BPoint where)
{
	if (fCurrentShape == NULL) {
		SimpleListView::MouseDown(where);
		return;
	}

	bool handled = false;
	int32 index = IndexOf(where);
	PathListItem* item = dynamic_cast<PathListItem*>(ItemAt(index));
	if (item != NULL) {
		BRect itemFrame(ItemFrame(index));
		itemFrame.right = itemFrame.left + kBorderOffset + kMarkWidth
			+ kTextOffset / 2.0;

		VectorPath* path = item->path;
		if (itemFrame.Contains(where) && fCommandStack) {
			// add or remove the path to the shape
			::Command* command;
			if (fCurrentShape->Paths()->HasPath(path)) {
				command = new UnassignPathCommand(fCurrentShape, path);
			} else {
				VectorPath* paths[1];
				paths[0] = path;
				command = new AddPathsCommand(fCurrentShape->Paths(),
					paths, 1, false, fCurrentShape->Paths()->CountPaths());
			}
			fCommandStack->Perform(command);
			handled = true;
		}
	}

	if (!handled)
		SimpleListView::MouseDown(where);
}


void
PathListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_ADD:
			if (fCommandStack != NULL) {
				VectorPath* path;
				AddPathsCommand* command;
				new_path(fPathContainer, &path, &command);
				fCommandStack->Perform(command);
			}
			break;

		case MSG_ADD_RECT:
			if (fCommandStack != NULL) {
				VectorPath* path;
				AddPathsCommand* command;
				new_path(fPathContainer, &path, &command);
				if (path != NULL) {
					path->AddPoint(BPoint(16, 16));
					path->AddPoint(BPoint(16, 48));
					path->AddPoint(BPoint(48, 48));
					path->AddPoint(BPoint(48, 16));
					path->SetClosed(true);
				}
				fCommandStack->Perform(command);
			}
			break;

		case MSG_ADD_CIRCLE:
			// TODO: ask for number of secions
			if (fCommandStack != NULL) {
				VectorPath* path;
				AddPathsCommand* command;
				new_path(fPathContainer, &path, &command);
				if (path != NULL) {
					// add four control points defining a circle:
					//   a 
					// b   d
					//   c
					BPoint a(32, 16);
					BPoint b(16, 32);
					BPoint c(32, 48);
					BPoint d(48, 32);
					
					path->AddPoint(a);
					path->AddPoint(b);
					path->AddPoint(c);
					path->AddPoint(d);
			
					path->SetClosed(true);
			
					float controlDist = 0.552284 * 16;
					path->SetPoint(0, a, a + BPoint(controlDist, 0.0),
										 a + BPoint(-controlDist, 0.0), true);
					path->SetPoint(1, b, b + BPoint(0.0, -controlDist),
										 b + BPoint(0.0, controlDist), true);
					path->SetPoint(2, c, c + BPoint(-controlDist, 0.0),
										 c + BPoint(controlDist, 0.0), true);
					path->SetPoint(3, d, d + BPoint(0.0, controlDist),
										 d + BPoint(0.0, -controlDist), true);
				}
				fCommandStack->Perform(command);
			}
			break;

		case MSG_DUPLICATE:
			if (fCommandStack != NULL) {
				PathListItem* item = dynamic_cast<PathListItem*>(
					ItemAt(CurrentSelection(0)));
				if (item == NULL)
					break;

				VectorPath* path;
				AddPathsCommand* command;
				new_path(fPathContainer, &path, &command, item->path);
				fCommandStack->Perform(command);
			}
			break;

		case MSG_REVERSE:
			if (fCommandStack != NULL) {
				PathListItem* item = dynamic_cast<PathListItem*>(
					ItemAt(CurrentSelection(0)));
				if (item == NULL)
					break;

				ReversePathCommand* command
					= new (nothrow) ReversePathCommand(item->path);
				fCommandStack->Perform(command);
			}
			break;

		case MSG_CLEAN_UP:
			if (fCommandStack != NULL) {
				PathListItem* item = dynamic_cast<PathListItem*>(
					ItemAt(CurrentSelection(0)));
				if (item == NULL)
					break;

				CleanUpPathCommand* command
					= new (nothrow) CleanUpPathCommand(item->path);
				fCommandStack->Perform(command);
			}
			break;

		case MSG_ROTATE_INDICES_CW:
		case MSG_ROTATE_INDICES_CCW:
			if (fCommandStack != NULL) {
				PathListItem* item = dynamic_cast<PathListItem*>(
					ItemAt(CurrentSelection(0)));
				if (item == NULL)
					break;

				RotatePathIndicesCommand* command
					= new (nothrow) RotatePathIndicesCommand(item->path,
					message->what == MSG_ROTATE_INDICES_CW);
				fCommandStack->Perform(command);
			}
			break;

		case MSG_REMOVE:
			RemoveSelected();
			break;

		default:
			SimpleListView::MessageReceived(message);
			break;
	}
}


void
PathListView::MakeDragMessage(BMessage* message) const
{
	SimpleListView::MakeDragMessage(message);
	message->AddPointer("container", fPathContainer);
	int32 count = CountSelectedItems();
	for (int32 i = 0; i < count; i++) {
		PathListItem* item = dynamic_cast<PathListItem*>(
			ItemAt(CurrentSelection(i)));
		if (item != NULL) {
			message->AddPointer("path", (void*)item->path);
			BMessage archive;
			if (item->path->Archive(&archive, true) == B_OK)
				message->AddMessage("path archive", &archive);
		} else
			break;
	}
}


bool
PathListView::AcceptDragMessage(const BMessage* message) const
{
	return SimpleListView::AcceptDragMessage(message);
}


void
PathListView::SetDropTargetRect(const BMessage* message, BPoint where)
{
	SimpleListView::SetDropTargetRect(message, where);
}


bool
PathListView::HandleDropMessage(const BMessage* message, int32 dropIndex)
{
	// Let SimpleListView handle drag-sorting (when drag came from ourself)
	if (SimpleListView::HandleDropMessage(message, dropIndex))
		return true;

	if (fCommandStack == NULL || fPathContainer == NULL)
		return false;

	// Drag may have come from another instance, like in another window.
	// Reconstruct the Styles from the archive and add them at the drop
	// index.
	int index = 0;
	BList paths;
	while (true) {
		BMessage archive;
		if (message->FindMessage("path archive", index, &archive) != B_OK)
			break;

		VectorPath* path = new(std::nothrow) VectorPath(&archive);
		if (path == NULL)
			break;
		
		if (!paths.AddItem(path)) {
			delete path;
			break;
		}

		index++;
	}

	int32 count = paths.CountItems();
	if (count == 0)
		return false;

	AddPathsCommand* command = new(nothrow) AddPathsCommand(fPathContainer,
		(VectorPath**)paths.Items(), count, true, dropIndex);
	if (command == NULL) {
		for (int32 i = 0; i < count; i++)
			delete (VectorPath*)paths.ItemAtFast(i);
		return false;
	}

	fCommandStack->Perform(command);

	return true;
}


void
PathListView::MoveItems(BList& items, int32 toIndex)
{
	if (fCommandStack == NULL || fPathContainer == NULL)
		return;

	int32 count = items.CountItems();
	VectorPath** paths = new (nothrow) VectorPath*[count];
	if (paths == NULL)
		return;

	for (int32 i = 0; i < count; i++) {
		PathListItem* item
			= dynamic_cast<PathListItem*>((BListItem*)items.ItemAtFast(i));
		paths[i] = item ? item->path : NULL;
	}

	MovePathsCommand* command = new (nothrow) MovePathsCommand(fPathContainer,
		paths, count, toIndex);
	if (command == NULL) {
		delete[] paths;
		return;
	}

	fCommandStack->Perform(command);
}


void
PathListView::CopyItems(BList& items, int32 toIndex)
{
	if (fCommandStack == NULL || fPathContainer == NULL)
		return;

	int32 count = items.CountItems();
	VectorPath* paths[count];

	for (int32 i = 0; i < count; i++) {
		PathListItem* item
			= dynamic_cast<PathListItem*>((BListItem*)items.ItemAtFast(i));
		paths[i] = item ? new (nothrow) VectorPath(*item->path) : NULL;
	}

	AddPathsCommand* command = new(nothrow) AddPathsCommand(fPathContainer,
		paths, count, true, toIndex);
	if (command == NULL) {
		for (int32 i = 0; i < count; i++)
			delete paths[i];
		return;
	}

	fCommandStack->Perform(command);
}


void
PathListView::RemoveItemList(BList& items)
{
	if (fCommandStack == NULL || fPathContainer == NULL)
		return;

	int32 count = items.CountItems();
	VectorPath* paths[count];
	for (int32 i = 0; i < count; i++) {
		PathListItem* item = dynamic_cast<PathListItem*>(
			(BListItem*)items.ItemAtFast(i));
		if (item != NULL)
			paths[i] = item->path;
		else
			paths[i] = NULL;
	}

	RemovePathsCommand* command = new (nothrow) RemovePathsCommand(
		fPathContainer, paths, count);

	fCommandStack->Perform(command);
}


BListItem*
PathListView::CloneItem(int32 index) const
{
	if (PathListItem* item = dynamic_cast<PathListItem*>(ItemAt(index))) {
		return new(nothrow) PathListItem(item->path,
			const_cast<PathListView*>(this), fCurrentShape != NULL);
	}
	return NULL;
}


int32
PathListView::IndexOfSelectable(Selectable* selectable) const
{
	VectorPath* path = dynamic_cast<VectorPath*>(selectable);
	if (path == NULL)
		return -1;

	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		if (SelectableFor(ItemAt(i)) == path)
			return i;
	}

	return -1;
}


Selectable*
PathListView::SelectableFor(BListItem* item) const
{
	PathListItem* pathItem = dynamic_cast<PathListItem*>(item);
	if (pathItem != NULL)
		return pathItem->path;
	return NULL;
}


// #pragma mark -


void
PathListView::PathAdded(VectorPath* path, int32 index)
{
	// NOTE: we are in the thread that messed with the
	// ShapeContainer, so no need to lock the
	// container, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	if (_AddPath(path, index))
		Select(index);

	UnlockLooper();
}


void
PathListView::PathRemoved(VectorPath* path)
{
	// NOTE: we are in the thread that messed with the
	// ShapeContainer, so no need to lock the
	// container, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	// NOTE: we're only interested in VectorPath objects
	_RemovePath(path);

	UnlockLooper();
}


// #pragma mark -


void
PathListView::SetPathContainer(PathContainer* container)
{
	if (fPathContainer == container)
		return;

	// detach from old container
	if (fPathContainer != NULL)
		fPathContainer->RemoveListener(this);

	_MakeEmpty();

	fPathContainer = container;

	if (fPathContainer == NULL)
		return;

	fPathContainer->AddListener(this);

	// sync
//	if (!fPathContainer->ReadLock())
//		return;

	int32 count = fPathContainer->CountPaths();
	for (int32 i = 0; i < count; i++)
		_AddPath(fPathContainer->PathAtFast(i), i);

//	fPathContainer->ReadUnlock();
}


void
PathListView::SetShapeContainer(ShapeContainer* container)
{
	if (fShapeContainer == container)
		return;

	// detach from old container
	if (fShapeContainer != NULL)
		fShapeContainer->RemoveListener(fShapePathListener);

	fShapeContainer = container;

	if (fShapeContainer != NULL)
		fShapeContainer->AddListener(fShapePathListener);
}


void
PathListView::SetCommandStack(CommandStack* stack)
{
	fCommandStack = stack;
}


void
PathListView::SetMenu(BMenu* menu)
{
	fMenu = menu;
	if (fMenu == NULL)
		return;

	fAddMI = new BMenuItem(B_TRANSLATE("Add"),
		new BMessage(MSG_ADD));
	fAddRectMI = new BMenuItem(B_TRANSLATE("Add rect"), 
		new BMessage(MSG_ADD_RECT));
	fAddCircleMI = new BMenuItem(B_TRANSLATE("Add circle"/*B_UTF8_ELLIPSIS*/),
		new BMessage(MSG_ADD_CIRCLE));
//	fAddArcMI = new BMenuItem("Add arc"B_UTF8_ELLIPSIS,
//		new BMessage(MSG_ADD_ARC));
	fDuplicateMI = new BMenuItem(B_TRANSLATE("Duplicate"), 
		new BMessage(MSG_DUPLICATE));
	fReverseMI = new BMenuItem(B_TRANSLATE("Reverse"),
		new BMessage(MSG_REVERSE));
	fCleanUpMI = new BMenuItem(B_TRANSLATE("Clean up"),
		new BMessage(MSG_CLEAN_UP));
	fRotateIndicesRightMI = new BMenuItem(B_TRANSLATE("Rotate indices forwards"),
		new BMessage(MSG_ROTATE_INDICES_CCW), 'R');
	fRotateIndicesLeftMI = new BMenuItem(B_TRANSLATE("Rotate indices backwards"),
		new BMessage(MSG_ROTATE_INDICES_CW), 'R', B_SHIFT_KEY);
	fRemoveMI = new BMenuItem(B_TRANSLATE("Remove"),
		new BMessage(MSG_REMOVE));

	fMenu->AddItem(fAddMI);
	fMenu->AddItem(fAddRectMI);
	fMenu->AddItem(fAddCircleMI);
//	fMenu->AddItem(fAddArcMI);

	fMenu->AddSeparatorItem();

	fMenu->AddItem(fDuplicateMI);
	fMenu->AddItem(fReverseMI);
	fMenu->AddItem(fCleanUpMI);

	fMenu->AddSeparatorItem();

	fMenu->AddItem(fRotateIndicesRightMI);
	fMenu->AddItem(fRotateIndicesLeftMI);

	fMenu->AddSeparatorItem();

	fMenu->AddItem(fRemoveMI);

	fMenu->SetTargetForItems(this);

	_UpdateMenu();
}


void
PathListView::SetCurrentShape(Shape* shape)
{
	if (fCurrentShape == shape)
		return;

	fCurrentShape = shape;
	fShapePathListener->SetShape(shape);

	_UpdateMarks();
}


// #pragma mark -


bool
PathListView::_AddPath(VectorPath* path, int32 index)
{
	if (path == NULL)
		return false;

	PathListItem* item = new(nothrow) PathListItem(path, this,
		fCurrentShape != NULL);
	if (item == NULL)
		return false;

	if (!AddItem(item, index)) {
		delete item;
		return false;
	}
	
	return true;
}


bool
PathListView::_RemovePath(VectorPath* path)
{
	PathListItem* item = _ItemForPath(path);
	if (item != NULL && RemoveItem(item)) {
		delete item;
		return true;
	}
	return false;
}


PathListItem*
PathListView::_ItemForPath(VectorPath* path) const
{
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		 PathListItem* item = dynamic_cast<PathListItem*>(ItemAt(i));
		if (item == NULL)
			continue;
		if (item->path == path)
			return item;
	}
	return NULL;
}


// #pragma mark -


void
PathListView::_UpdateMarks()
{
	int32 count = CountItems();
	if (fCurrentShape != NULL) {
		// enable display of marks and mark items whoes
		// path is contained in fCurrentShape
		for (int32 i = 0; i < count; i++) {
			PathListItem* item = dynamic_cast<PathListItem*>(ItemAt(i));
			if (item == NULL)
				continue;
			item->SetMarkEnabled(true);
			item->SetMarked(fCurrentShape->Paths()->HasPath(item->path));
		}
	} else {
		// disable display of marks
		for (int32 i = 0; i < count; i++) {
			PathListItem* item = dynamic_cast<PathListItem*>(ItemAt(i));
			if (item == NULL)
				continue;
			item->SetMarkEnabled(false);
		}
	}

	Invalidate();
}


void
PathListView::_SetPathMarked(VectorPath* path, bool marked)
{
	PathListItem* item = _ItemForPath(path);
	if (item != NULL)
		item->SetMarked(marked);
}


void
PathListView::_UpdateMenu()
{
	if (fMenu == NULL)
		return;

	bool gotSelection = CurrentSelection(0) >= 0;

	fDuplicateMI->SetEnabled(gotSelection);
	fReverseMI->SetEnabled(gotSelection);
	fCleanUpMI->SetEnabled(gotSelection);
	fRotateIndicesLeftMI->SetEnabled(gotSelection);
	fRotateIndicesRightMI->SetEnabled(gotSelection);
	fRemoveMI->SetEnabled(gotSelection);
}


