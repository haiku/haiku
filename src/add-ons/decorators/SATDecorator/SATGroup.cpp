/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include "SATGroup.h"

#include <vector>

#include <Debug.h>
#include <Message.h>

#include "SATWindow.h"
#include "StackAndTile.h"
#include "Window.h"


using namespace std;


WindowArea::WindowArea(Crossing* leftTop, Crossing* rightTop,
	Crossing* leftBottom, Crossing* rightBottom)
	:
	fGroup(NULL),

	fLeftTopCrossing(leftTop),
	fRightTopCrossing(rightTop),
	fLeftBottomCrossing(leftBottom),
	fRightBottomCrossing(rightBottom)
{

}


bool
WindowArea::SetGroup(SATGroup* group)
{
	if (group && !group->fWindowAreaList.AddItem(this))
		return false;

	if (fGroup)
		fGroup->fWindowAreaList.RemoveItem(this);

	fGroup = group;
	return true;
}


WindowArea::~WindowArea()
{
	if (fGroup)
		fGroup->WindowAreaRemoved(this);

	_CleanupCorners();
	SetGroup(NULL);
}


bool
WindowArea::MoveWindowToPosition(SATWindow* window, int32 index)
{
	int32 oldIndex = fWindowList.IndexOf(window);
	if (oldIndex < 0)
		return false;
	ASSERT(oldIndex != index);
	if (oldIndex < index)
		index++;
	else
		oldIndex++;
	if (!fWindowList.AddItem(window, index))
		return false;
	fWindowList.RemoveItemAt(oldIndex);
	return true;
}


bool
WindowArea::_AddWindow(SATWindow* window, SATWindow* after)
{
	if (after) {
		int32 indexAfter = fWindowList.IndexOf(after);
		if (!fWindowList.AddItem(window, indexAfter + 1))
			return false;
	}
	else if (!fWindowList.AddItem(window))
		return false;

	AcquireReference();

	if (fWindowList.CountItems() <= 1)
		_InitCorners();

	fWindowLayerOrder.AddItem(window);
	return true;
}


bool
WindowArea::_RemoveWindow(SATWindow* window)
{
	if (!fWindowList.RemoveItem(window))
		return false;

	fWindowLayerOrder.RemoveItem(window);
	window->RemovedFromArea(this);
	ReleaseReference();
	return true;
}


Tab*
WindowArea::LeftTab()
{
	return fLeftTopCrossing->VerticalTab();
}


Tab*
WindowArea::RightTab()
{
	return fRightBottomCrossing->VerticalTab();
}


Tab*
WindowArea::TopTab()
{
	return fLeftTopCrossing->HorizontalTab();
}


Tab*
WindowArea::BottomTab()
{
	return fRightBottomCrossing->HorizontalTab();
}


BRect
WindowArea::Frame()
{
	return BRect(fLeftTopCrossing->VerticalTab()->Position(),
		fLeftTopCrossing->HorizontalTab()->Position(),
		fRightBottomCrossing->VerticalTab()->Position(),
		fRightBottomCrossing->HorizontalTab()->Position());
}


bool
WindowArea::PropagateToGroup(SATGroup* group)
{
	BReference<Crossing> newLeftTop = _CrossingByPosition(fLeftTopCrossing,
		group);
	BReference<Crossing> newRightTop = _CrossingByPosition(fRightTopCrossing,
		group);
	BReference<Crossing> newLeftBottom = _CrossingByPosition(
		fLeftBottomCrossing, group);
	BReference<Crossing> newRightBottom = _CrossingByPosition(
		fRightBottomCrossing, group);

	if (!newLeftTop || !newRightTop || !newLeftBottom || !newRightBottom)
		return false;

	fLeftTopCrossing = newLeftTop;
	fRightTopCrossing = newRightTop;
	fLeftBottomCrossing = newLeftBottom;
	fRightBottomCrossing = newRightBottom;

	for (int i = 0; i < fWindowList.CountItems(); i++)
		fWindowList.ItemAt(i)->PropagateToGroup(group, this);

	return true;
}


bool
WindowArea::MoveToTopLayer(SATWindow* window)
{
	if (!fWindowLayerOrder.RemoveItem(window))
		return false;
	return fWindowLayerOrder.AddItem(window);
}


BReference<Crossing>
WindowArea::_CrossingByPosition(Crossing* crossing, SATGroup* group)
{
	BReference<Crossing> crossRef = NULL;

	Tab* oldHTab = crossing->HorizontalTab();
	BReference<Tab> hTab = group->FindHorizontalTab(oldHTab->Position());
	if (!hTab)
		hTab = group->_AddHorizontalTab(oldHTab->Position());
	if (!hTab)
		return crossRef;

	Tab* oldVTab = crossing->VerticalTab();
	crossRef = hTab->FindCrossing(oldHTab->Position());
	if (crossRef)
		return crossRef;

	BReference<Tab> vTab = group->FindVerticalTab(oldVTab->Position());
	if (!vTab)
		vTab = group->_AddVerticalTab(oldVTab->Position());
	if (!vTab)
		return crossRef;

	return hTab->AddCrossing(vTab);
}


void
WindowArea::_InitCorners()
{
	_SetToWindowCorner(fLeftTopCrossing->RightBottomCorner());
	_SetToNeighbourCorner(fLeftTopCrossing->LeftBottomCorner());
	_SetToNeighbourCorner(fLeftTopCrossing->RightTopCorner());

	_SetToWindowCorner(fRightTopCrossing->LeftBottomCorner());
	_SetToNeighbourCorner(fRightTopCrossing->LeftTopCorner());
	_SetToNeighbourCorner(fRightTopCrossing->RightBottomCorner());

	_SetToWindowCorner(fLeftBottomCrossing->RightTopCorner());
	_SetToNeighbourCorner(fLeftBottomCrossing->LeftTopCorner());
	_SetToNeighbourCorner(fLeftBottomCrossing->RightBottomCorner());

	_SetToWindowCorner(fRightBottomCrossing->LeftTopCorner());
	_SetToNeighbourCorner(fRightBottomCrossing->LeftBottomCorner());
	_SetToNeighbourCorner(fRightBottomCrossing->RightTopCorner());
}


void
WindowArea::_CleanupCorners()
{
	_UnsetWindowCorner(fLeftTopCrossing->RightBottomCorner());
	_UnsetNeighbourCorner(fLeftTopCrossing->LeftBottomCorner(),
		fLeftBottomCrossing->LeftTopCorner());
	_UnsetNeighbourCorner(fLeftTopCrossing->RightTopCorner(),
		fLeftBottomCrossing->LeftTopCorner());

	_UnsetWindowCorner(fRightTopCrossing->LeftBottomCorner());
	_UnsetNeighbourCorner(fRightTopCrossing->LeftTopCorner(),
		fLeftBottomCrossing->RightTopCorner());
	_UnsetNeighbourCorner(fRightTopCrossing->RightBottomCorner(),
		fLeftBottomCrossing->RightTopCorner());

	_UnsetWindowCorner(fLeftBottomCrossing->RightTopCorner());
	_UnsetNeighbourCorner(fLeftBottomCrossing->LeftTopCorner(),
		fLeftBottomCrossing->LeftBottomCorner());
	_UnsetNeighbourCorner(fLeftBottomCrossing->RightBottomCorner(),
		fLeftBottomCrossing->LeftBottomCorner());

	_UnsetWindowCorner(fRightBottomCrossing->LeftTopCorner());
	_UnsetNeighbourCorner(fRightBottomCrossing->LeftBottomCorner(),
		fRightBottomCrossing->RightBottomCorner());
	_UnsetNeighbourCorner(fRightBottomCrossing->RightTopCorner(),
		fRightBottomCrossing->RightBottomCorner());
}


void
WindowArea::_SetToWindowCorner(Corner* corner)
{
	corner->status = Corner::kUsed;
	corner->windowArea = this;
}


void
WindowArea::_SetToNeighbourCorner(Corner* neighbour)
{
	if (neighbour->status == Corner::kNotDockable)
		neighbour->status = Corner::kFree;
}


void
WindowArea::_UnsetWindowCorner(Corner* corner)
{
	corner->status = Corner::kFree;
	corner->windowArea = NULL;
}


void
WindowArea::_UnsetNeighbourCorner(Corner* neighbour, Corner* opponent)
{
	if (neighbour->status == Corner::kFree && opponent->status != Corner::kUsed)
		neighbour->status = Corner::kNotDockable;
}


Corner::Corner()
	:
	status(kNotDockable),
	windowArea(NULL)
{

}


void
Corner::Trace() const
{
	switch (status) {
		case kFree:
			STRACE_SAT("free corner\n");
			break;

		case kUsed:
		{
			STRACE_SAT("attached windows:\n");
			const SATWindowList& list = windowArea->WindowList();
			for (int i = 0; i < list.CountItems(); i++) {
				STRACE_SAT("- %s\n", list.ItemAt(i)->GetWindow()->Title());
			}
			break;
		}

		case kNotDockable:
			STRACE_SAT("not dockable\n");
			break;
	};
}


Crossing::Crossing(Tab* vertical, Tab* horizontal)
	:
	fVerticalTab(vertical),
	fHorizontalTab(horizontal)
{
	fVerticalTab->AcquireReference();
	fHorizontalTab->AcquireReference();
}


Crossing::~Crossing()
{
	fVerticalTab->RemoveCrossing(this);
	fHorizontalTab->RemoveCrossing(this);

	fVerticalTab->ReleaseReference();
	fHorizontalTab->ReleaseReference();
}


Corner*
Crossing::GetCorner(Corner::position_t corner) const
{
	return _GetCorner(corner);
}


Tab*
Crossing::VerticalTab() const
{
	return fVerticalTab;
}


Tab*
Crossing::HorizontalTab() const
{
	return fHorizontalTab;
}


void
Crossing::Trace() const
{
	STRACE_SAT("left-top corner: ");
	fLeftTop.Trace();
	STRACE_SAT("right-top corner: ");
	fRightTop.Trace();
	STRACE_SAT("left-bottom corner: ");
	fLeftBottom.Trace();
	STRACE_SAT("right-bottom corner: ");
	fRightBottom.Trace();
}


Corner*
Crossing::_GetCorner(Corner::position_t corner) const
{
	switch (corner) {
		case Corner::kLeftTop:
			return const_cast<Corner*>(&fLeftTop);
		case Corner::kRightTop:
			return const_cast<Corner*>(&fRightTop);
		case Corner::kLeftBottom:
			return const_cast<Corner*>(&fLeftBottom);
		case Corner::kRightBottom:
			return const_cast<Corner*>(&fRightBottom);
	};
	return NULL;
}


Corner*
Crossing::GetOppositeCorner(Corner::position_t corner) const
{
	switch (corner) {
		case Corner::kLeftTop:
			return const_cast<Corner*>(&fRightBottom);
		case Corner::kRightTop:
			return const_cast<Corner*>(&fLeftBottom);
		case Corner::kLeftBottom:
			return const_cast<Corner*>(&fRightTop);
		case Corner::kRightBottom:
			return const_cast<Corner*>(&fLeftTop);
	};
	return NULL;
}


Tab::Tab(SATGroup* group, Variable* variable, orientation_t orientation)
	:
	fGroup(group),
	fVariable(variable),
	fOrientation(orientation)
{
	
}


Tab::~Tab()
{
	if (fOrientation == kVertical)
		fGroup->_RemoveVerticalTab(this);
	else
		fGroup->_RemoveHorizontalTab(this);

	delete fVariable;
}


float
Tab::Position() const
{
	return (float)fVariable->Value() - kMakePositiveOffset;
}


void
Tab::SetPosition(float position)
{
	fVariable->SetValue(position + kMakePositiveOffset);
}


Tab::orientation_t
Tab::Orientation() const
{
	return fOrientation;
}


Constraint*
Tab::Connect(Variable* variable)
{
	return fVariable->IsEqual(variable);
}


BReference<Crossing>
Tab::AddCrossing(Tab* tab)
{
	if (tab->Orientation() == fOrientation)
		return NULL;

	Tab* vTab = (fOrientation == kVertical) ? this : tab;
	Tab* hTab = (fOrientation == kHorizontal) ? this : tab;

	Crossing* crossing = new (std::nothrow)Crossing(vTab, hTab);
	if (!crossing)
		return NULL;

	if (!fCrossingList.AddItem(crossing)) {
		return NULL;
	}
	if (!tab->fCrossingList.AddItem(crossing)) {
		fCrossingList.RemoveItem(crossing);
		return NULL;
	}

	BReference<Crossing> crossingRef(crossing, true);
	return crossingRef;
}


bool
Tab::RemoveCrossing(Crossing* crossing)
{
	Tab* vTab = crossing->VerticalTab();
	Tab* hTab = crossing->HorizontalTab();

	if (vTab != this && hTab != this)
		return false;
	fCrossingList.RemoveItem(crossing);

	return true;
}


int32
Tab::FindCrossingIndex(Tab* tab)
{
	if (fOrientation == kVertical) {
		for (int32 i = 0; i < fCrossingList.CountItems(); i++)
			if (fCrossingList.ItemAt(i)->HorizontalTab() == tab)
				return i;
	} else {
		for (int32 i = 0; i < fCrossingList.CountItems(); i++)
			if (fCrossingList.ItemAt(i)->VerticalTab() == tab)
				return i;
	}
	return -1;
}


int32
Tab::FindCrossingIndex(float pos)
{
	if (fOrientation == kVertical) {
		for (int32 i = 0; i < fCrossingList.CountItems(); i++)
			if (fCrossingList.ItemAt(i)->HorizontalTab()->Position() == pos)
				return i;
	} else {
		for (int32 i = 0; i < fCrossingList.CountItems(); i++)
			if (fCrossingList.ItemAt(i)->VerticalTab()->Position() == pos)
				return i;
	}
	return -1;
}


Crossing*
Tab::FindCrossing(Tab* tab)
{
	return fCrossingList.ItemAt(FindCrossingIndex(tab));
}


Crossing*
Tab::FindCrossing(float tabPosition)
{
	return fCrossingList.ItemAt(FindCrossingIndex(tabPosition));
}


const CrossingList*
Tab::GetCrossingList() const
{
	return &fCrossingList;
}


int
Tab::CompareFunction(const Tab* tab1, const Tab* tab2)
{
	if (tab1->Position() < tab2->Position())
		return -1;

	return 1;
}


SATGroup::SATGroup()
	:
	fHorizontalTabsSorted(false),
	fVerticalTabsSorted(false)
{
	
}


SATGroup::~SATGroup()
{
	ASSERT(fSATWindowList.CountItems() == 0);
	// Should be empty
	//while (fSATWindowList.CountItems() > 0)
	//	RemoveWindow(fSATWindowList.ItemAt(0));
}


bool
SATGroup::AddWindow(SATWindow* window, Tab* left, Tab* top, Tab* right,
	Tab* bottom)
{
	STRACE_SAT("SATGroup::AddWindow\n");

	// first check if we have to create tabs and missing corners.
	BReference<Tab> leftRef, rightRef, topRef, bottomRef;
	BReference<Crossing> leftTopRef, rightTopRef, leftBottomRef, rightBottomRef;

	if (left && top)
		leftTopRef = left->FindCrossing(top);
	if (right && top)
		rightTopRef = right->FindCrossing(top);
	if (left && bottom)
		leftBottomRef = left->FindCrossing(bottom);
	if (right && bottom)
		rightBottomRef = right->FindCrossing(bottom);

	if (!left) {
		leftRef = _AddVerticalTab();
		left = leftRef.Get();
	}
	if (!top) {
		topRef = _AddHorizontalTab();
		top = topRef.Get();
	}
	if (!right) {
		rightRef = _AddVerticalTab();
		right = rightRef.Get();
	}
	if (!bottom) {
		bottomRef = _AddHorizontalTab();
		bottom = bottomRef.Get();
	}
	if (!left || !top || !right || !bottom)
		return false;

	if (!leftTopRef) {
		leftTopRef = left->AddCrossing(top);
		if (!leftTopRef)
			return false;
	}
	if (!rightTopRef) {
		rightTopRef = right->AddCrossing(top);
		if (!rightTopRef)
			return false;
	}
	if (!leftBottomRef) {
		leftBottomRef = left->AddCrossing(bottom);
		if (!leftBottomRef)
			return false;
	}
	if (!rightBottomRef) {
		rightBottomRef = right->AddCrossing(bottom);
		if (!rightBottomRef)
			return false;
	}

	WindowArea* area = new(std::nothrow) WindowArea(leftTopRef, rightTopRef,
		leftBottomRef, rightBottomRef);
	if (!area)
		return false;
	// the area register itself in our area list
	if (!area->SetGroup(this)) {
		delete area;
		return false;
	}
	// delete the area if AddWindow failed / release our reference on it
	BReference<WindowArea> areaRef(area, true);

	return AddWindow(window, area);
}


bool
SATGroup::AddWindow(SATWindow* window, WindowArea* area, SATWindow* after)
{
	if (!area->_AddWindow(window, after))
		return false;

	if (!fSATWindowList.AddItem(window)) {
		area->_RemoveWindow(window);
		return false;
	}

	if (!window->AddedToGroup(this, area)) {
		area->_RemoveWindow(window);
		fSATWindowList.RemoveItem(window);
		return false;
	}

	return true;
}


bool
SATGroup::RemoveWindow(SATWindow* window, bool stayBelowMouse)
{
	if (!fSATWindowList.RemoveItem(window))
		return false;

	WindowArea* area = window->GetWindowArea();
	if (area)
		area->_RemoveWindow(window);

	int32 windowCount = CountItems();

	window->RemovedFromGroup(this, stayBelowMouse);

	if (windowCount >= 2)
		WindowAt(0)->DoGroupLayout();

	// Do nothing after removing the window from the group because this
	// could have released the last reference and destroyed ourself.
	return true;
}


int32
SATGroup::CountItems()
{
	return fSATWindowList.CountItems();
};


SATWindow*
SATGroup::WindowAt(int32 index)
{
	return fSATWindowList.ItemAt(index);
}


const TabList*
SATGroup::HorizontalTabs()
{
	if (!fHorizontalTabsSorted) {
		fHorizontalTabs.SortItems(Tab::CompareFunction);
		fHorizontalTabsSorted = true;
	}
	return &fHorizontalTabs;
}


const TabList*
SATGroup::VerticalTabs()
{
	if (!fVerticalTabsSorted) {
		fVerticalTabs.SortItems(Tab::CompareFunction);
		fVerticalTabsSorted = true;
	}
	return &fVerticalTabs;
}


Tab*
SATGroup::FindHorizontalTab(float position)
{
	return _FindTab(fHorizontalTabs, position);
}


Tab*
SATGroup::FindVerticalTab(float position)
{
	return _FindTab(fVerticalTabs, position);
}


void
SATGroup::AdjustWindows(SATWindow* triggerWindow)
{
	// set window locations and sizes
	for (int i = 0; i < fSATWindowList.CountItems(); i++) {
		SATWindow* windowSAT = fSATWindowList.ItemAt(i);
		windowSAT->MoveWindowToSAT(
			triggerWindow->GetWindow()->CurrentWorkspace());
	}
}


void
SATGroup::WindowAreaRemoved(WindowArea* area)
{
	_SplitGroupIfNecessary(area);
}


status_t
SATGroup::RestoreGroup(const BMessage& archive, StackAndTile* sat)
{
	// create new group
	SATGroup* group = new (std::nothrow)SATGroup;
	if (!group)
		return B_NO_MEMORY;
	BReference<SATGroup> groupRef;
	groupRef.SetTo(group, true);

	int32 nHTabs, nVTabs;
	status_t status;
	status = archive.FindInt32("htab_count", &nHTabs);
	if (status != B_OK)
		return status;
	status = archive.FindInt32("vtab_count", &nVTabs);
	if (status != B_OK)
		return status;

	vector<BReference<Tab> > tempHTabs;
	for (int i = 0; i < nHTabs; i++) {
		BReference<Tab> tab = group->_AddHorizontalTab();
		if (!tab)
			return B_NO_MEMORY;
		tempHTabs.push_back(tab);
	}
	vector<BReference<Tab> > tempVTabs;
	for (int i = 0; i < nVTabs; i++) {
		BReference<Tab> tab = group->_AddVerticalTab();
		if (!tab)
			return B_NO_MEMORY;
		tempVTabs.push_back(tab);
	}

	BMessage areaArchive;
	for (int32 i = 0; archive.FindMessage("area", i, &areaArchive) == B_OK;
		i++) {
		uint32 leftTab, rightTab, topTab, bottomTab;
		if (areaArchive.FindInt32("left_tab", (int32*)&leftTab) != B_OK
			|| areaArchive.FindInt32("right_tab", (int32*)&rightTab) != B_OK
			|| areaArchive.FindInt32("top_tab", (int32*)&topTab) != B_OK
			|| areaArchive.FindInt32("bottom_tab", (int32*)&bottomTab) != B_OK)
			return B_ERROR;

		if (leftTab >= tempVTabs.size() || rightTab >= tempVTabs.size())
			return B_BAD_VALUE;
		if (topTab >= tempHTabs.size() || bottomTab >= tempHTabs.size())
			return B_BAD_VALUE;

		Tab* left = tempVTabs[leftTab];
		Tab* right = tempVTabs[rightTab];
		Tab* top = tempHTabs[topTab];
		Tab* bottom = tempHTabs[bottomTab];

		// adding windows to area
		uint64 windowId;
		SATWindow* prevWindow = NULL;
		for (int32 i = 0; areaArchive.FindInt64("window", i,
			(int64*)&windowId) == B_OK; i++) {
			SATWindow* window = sat->FindSATWindow(windowId);
			if (!window)
				continue;

			if (prevWindow == NULL) {
				if (!group->AddWindow(window, left, top, right, bottom))
					continue;
				prevWindow = window;
			} else {
				if (!prevWindow->StackWindow(window))
					continue;
				prevWindow = window;
			}
		}
	}
	return B_OK;
}


status_t
SATGroup::ArchiveGroup(BMessage& archive)
{
	archive.AddInt32("htab_count", fHorizontalTabs.CountItems());
	archive.AddInt32("vtab_count", fVerticalTabs.CountItems());

	for (int i = 0; i < fWindowAreaList.CountItems(); i++) {
		WindowArea* area = fWindowAreaList.ItemAt(i);
		int32 leftTab = fVerticalTabs.IndexOf(area->LeftTab());
		int32 rightTab = fVerticalTabs.IndexOf(area->RightTab());
		int32 topTab = fHorizontalTabs.IndexOf(area->TopTab());
		int32 bottomTab = fHorizontalTabs.IndexOf(area->BottomTab());

		BMessage areaMessage;
		areaMessage.AddInt32("left_tab", leftTab);
		areaMessage.AddInt32("right_tab", rightTab);
		areaMessage.AddInt32("top_tab", topTab);
		areaMessage.AddInt32("bottom_tab", bottomTab);

		const SATWindowList& windowList = area->WindowList();
		for (int a = 0; a < windowList.CountItems(); a++)
			areaMessage.AddInt64("window", windowList.ItemAt(a)->Id());

		archive.AddMessage("area", &areaMessage);
	}
	return B_OK;
}


BReference<Tab>
SATGroup::_AddHorizontalTab(float position)
{
	Variable* variable = fLinearSpec.AddVariable();
	if (!variable)
		return NULL;

	Tab* tab = new (std::nothrow)Tab(this, variable, Tab::kHorizontal);
	if (!tab)
		return NULL;
	BReference<Tab> tabRef(tab, true);

	if (!fHorizontalTabs.AddItem(tab))
		return NULL;

	fHorizontalTabsSorted = false;
	tabRef->SetPosition(position);
	return tabRef;
}


BReference<Tab>
SATGroup::_AddVerticalTab(float position)
{
	Variable* variable = fLinearSpec.AddVariable();
	if (!variable)
		return NULL;

	Tab* tab = new (std::nothrow)Tab(this, variable, Tab::kVertical);
	if (!tab)
		return NULL;
	BReference<Tab> tabRef(tab, true);

	if (!fVerticalTabs.AddItem(tab))
		return NULL;

	fVerticalTabsSorted = false;
	tabRef->SetPosition(position);
	return tabRef;
}


bool
SATGroup::_RemoveHorizontalTab(Tab* tab)
{
	if (!fHorizontalTabs.RemoveItem(tab))
		return false;
	fHorizontalTabsSorted = false;
	// don't delete the tab it is reference counted
	return true;
}


bool
SATGroup::_RemoveVerticalTab(Tab* tab)
{
	if (!fVerticalTabs.RemoveItem(tab))
		return false;
	fVerticalTabsSorted = false;
	// don't delete the tab it is reference counted
	return true;
}


Tab*
SATGroup::_FindTab(const TabList& list, float position)
{
	for (int i = 0; i < list.CountItems(); i++)
		if (list.ItemAt(i)->Position() == position)
			return list.ItemAt(i);

	return NULL;
}


void
SATGroup::_SplitGroupIfNecessary(WindowArea* removedArea)
{
	// if there are windows stacked in the area we don't need to split
	if (!removedArea || removedArea->WindowList().CountItems() > 1)
		return;

	WindowAreaList neighbourWindows;

	_FillNeighbourList(neighbourWindows, removedArea);

	bool ownGroupProcessed = false;
	WindowAreaList newGroup;
	while (_FindConnectedGroup(neighbourWindows, removedArea, newGroup)) {
		STRACE_SAT("Connected group found; %i window(s)\n",
			(int)newGroup.CountItems());

		if (newGroup.CountItems() == 1
			&& newGroup.ItemAt(0)->WindowList().CountItems() == 1) {
			SATWindow* window = newGroup.ItemAt(0)->WindowList().ItemAt(0);
			RemoveWindow(window);
			_EnsureGroupIsOnScreen(window->GetGroup());
		}
		else if (ownGroupProcessed)
			_SpawnNewGroup(newGroup);
		else {
			_EnsureGroupIsOnScreen(this);
			ownGroupProcessed = true;
		}

		newGroup.MakeEmpty();
	}
}


void
SATGroup::_FillNeighbourList(WindowAreaList& neighbourWindows,
	WindowArea* area)
{
	_LeftNeighbours(neighbourWindows, area);
	_RightNeighbours(neighbourWindows, area);
	_TopNeighbours(neighbourWindows, area);
	_BottomNeighbours(neighbourWindows, area);
}


void
SATGroup::_LeftNeighbours(WindowAreaList& neighbourWindows, WindowArea* parent)
{
	float startPos = parent->LeftTopCrossing()->HorizontalTab()->Position();
	float endPos = parent->LeftBottomCrossing()->HorizontalTab()->Position();

	Tab* tab = parent->LeftTopCrossing()->VerticalTab();
	const CrossingList* crossingList = tab->GetCrossingList();
	for (int i = 0; i < crossingList->CountItems(); i++) {
		Corner* corner = crossingList->ItemAt(i)->LeftTopCorner();
		if (corner->status != Corner::kUsed)
			continue;

		WindowArea* area = corner->windowArea;
		float pos1 = area->LeftTopCrossing()->HorizontalTab()->Position();
		float pos2 = area->LeftBottomCrossing()->HorizontalTab()->Position();

		if (pos1 < endPos && pos2 > startPos)
			neighbourWindows.AddItem(area);

		if (pos2 > endPos)
			break;
	}
}


void
SATGroup::_TopNeighbours(WindowAreaList& neighbourWindows, WindowArea* parent)
{
	float startPos = parent->LeftTopCrossing()->VerticalTab()->Position();
	float endPos = parent->RightTopCrossing()->VerticalTab()->Position();

	Tab* tab = parent->LeftTopCrossing()->HorizontalTab();
	const CrossingList* crossingList = tab->GetCrossingList();
	for (int i = 0; i < crossingList->CountItems(); i++) {
		Corner* corner = crossingList->ItemAt(i)->LeftTopCorner();
		if (corner->status != Corner::kUsed)
			continue;

		WindowArea* area = corner->windowArea;
		float pos1 = area->LeftTopCrossing()->VerticalTab()->Position();
		float pos2 = area->RightTopCrossing()->VerticalTab()->Position();

		if (pos1 < endPos && pos2 > startPos)
			neighbourWindows.AddItem(area);

		if (pos2 > endPos)
			break;
	}
}


void
SATGroup::_RightNeighbours(WindowAreaList& neighbourWindows, WindowArea* parent)
{
	float startPos = parent->RightTopCrossing()->HorizontalTab()->Position();
	float endPos = parent->RightBottomCrossing()->HorizontalTab()->Position();

	Tab* tab = parent->RightTopCrossing()->VerticalTab();
	const CrossingList* crossingList = tab->GetCrossingList();
	for (int i = 0; i < crossingList->CountItems(); i++) {
		Corner* corner = crossingList->ItemAt(i)->RightTopCorner();
		if (corner->status != Corner::kUsed)
			continue;

		WindowArea* area = corner->windowArea;
		float pos1 = area->RightTopCrossing()->HorizontalTab()->Position();
		float pos2 = area->RightBottomCrossing()->HorizontalTab()->Position();

		if (pos1 < endPos && pos2 > startPos)
			neighbourWindows.AddItem(area);

		if (pos2 > endPos)
			break;
	}
}


void
SATGroup::_BottomNeighbours(WindowAreaList& neighbourWindows,
	WindowArea* parent)
{
	float startPos = parent->LeftBottomCrossing()->VerticalTab()->Position();
	float endPos = parent->RightBottomCrossing()->VerticalTab()->Position();

	Tab* tab = parent->LeftBottomCrossing()->HorizontalTab();
	const CrossingList* crossingList = tab->GetCrossingList();
	for (int i = 0; i < crossingList->CountItems(); i++) {
		Corner* corner = crossingList->ItemAt(i)->LeftBottomCorner();
		if (corner->status != Corner::kUsed)
			continue;

		WindowArea* area = corner->windowArea;
		float pos1 = area->LeftBottomCrossing()->VerticalTab()->Position();
		float pos2 = area->RightBottomCrossing()->VerticalTab()->Position();

		if (pos1 < endPos && pos2 > startPos)
			neighbourWindows.AddItem(area);

		if (pos2 > endPos)
			break;
	}
}


bool
SATGroup::_FindConnectedGroup(WindowAreaList& seedList, WindowArea* removedArea,
	WindowAreaList& newGroup)
{
	if (seedList.CountItems() == 0)
		return false;

	WindowArea* area = seedList.RemoveItemAt(0);
	newGroup.AddItem(area);

	_FollowSeed(area, removedArea, seedList, newGroup);
	return true;
}


void
SATGroup::_FollowSeed(WindowArea* area, WindowArea* veto,
	WindowAreaList& seedList, WindowAreaList& newGroup)
{
	WindowAreaList neighbours;
	_FillNeighbourList(neighbours, area);
	for (int i = 0; i < neighbours.CountItems(); i++) {
		WindowArea* currentArea = neighbours.ItemAt(i);
		if (currentArea != veto && !newGroup.HasItem(currentArea)) {
			newGroup.AddItem(currentArea);
			// if we get a area from the seed list it is not a seed any more
			seedList.RemoveItem(currentArea);
		}
		else {
			// don't _FollowSeed of invalid areas
			neighbours.RemoveItemAt(i);
			i--;
		}
	}

	for (int i = 0; i < neighbours.CountItems(); i++)
		_FollowSeed(neighbours.ItemAt(i), veto, seedList, newGroup);
}


void
SATGroup::_SpawnNewGroup(const WindowAreaList& newGroup)
{
	STRACE_SAT("SATGroup::_SpawnNewGroup\n");
	SATGroup* group = new (std::nothrow)SATGroup;
	if (!group)
		return;
	BReference<SATGroup> groupRef;
	groupRef.SetTo(group, true);

	for (int i = 0; i < newGroup.CountItems(); i++)
		newGroup.ItemAt(i)->PropagateToGroup(group);

	_EnsureGroupIsOnScreen(group);
}


const float kMinOverlap = 50;
const float kMoveToScreen = 75;


void
SATGroup::_EnsureGroupIsOnScreen(SATGroup* group)
{
	STRACE_SAT("SATGroup::_EnsureGroupIsOnScreen\n");
	if (!group)
		return;

	if (group->CountItems() < 1)
		return;

	SATWindow* window = group->WindowAt(0);
	Desktop* desktop = window->GetWindow()->Desktop();
	if (!desktop)
		return;

	const float kBigDistance = 1E+10;

	float minLeftDistance = kBigDistance;
	BRect leftRect;
	float minTopDistance = kBigDistance;
	BRect topRect;
	float minRightDistance = kBigDistance;
	BRect rightRect;
	float minBottomDistance = kBigDistance;
	BRect bottomRect;

	BRect screen = window->GetWindow()->Screen()->Frame();
	BRect reducedScreen = screen;
	reducedScreen.InsetBy(kMinOverlap, kMinOverlap);

	for (int i = 0; i < group->CountItems(); i++) {
		SATWindow* window = group->WindowAt(i);
		BRect frame = window->CompleteWindowFrame();
		if (reducedScreen.Intersects(frame))
			return;

		if (frame.right < screen.left + kMinOverlap) {
			float dist = fabs(screen.left - frame.right);
			if (dist < minLeftDistance) {
				minLeftDistance = dist;
				leftRect = frame;
			}
			else if (dist == minLeftDistance)
				leftRect = leftRect | frame;
		}
		if (frame.top > screen.bottom - kMinOverlap) {
			float dist = fabs(frame.top - screen.bottom);
			if (dist < minBottomDistance) {
				minBottomDistance = dist;
				bottomRect = frame;
			}
			else if (dist == minBottomDistance)
				bottomRect = bottomRect | frame;
		}
		if (frame.left > screen.right - kMinOverlap) {
			float dist = fabs(frame.left - screen.right);
			if (dist < minRightDistance) {
				minRightDistance = dist;
				rightRect = frame;
			}
			else if (dist == minRightDistance)
				rightRect = rightRect | frame;
		}
		if (frame.bottom < screen.top + kMinOverlap) {
			float dist = fabs(frame.bottom - screen.top);
			if (dist < minTopDistance) {
				minTopDistance = dist;
				topRect = frame;
			}
			else if (dist == minTopDistance)
				topRect = topRect | frame;
		}
	}

	BPoint offset;
	if (minLeftDistance < kBigDistance) {
		offset.x = screen.left - leftRect.right + kMoveToScreen;
		_CallculateYOffset(offset, leftRect, screen);
	}
	else if (minTopDistance < kBigDistance) {
		offset.y = screen.top - topRect.bottom + kMoveToScreen;
		_CallculateXOffset(offset, topRect, screen);
	}
	else if (minRightDistance < kBigDistance) {
		offset.x = screen.right - rightRect.left - kMoveToScreen;
		_CallculateYOffset(offset, rightRect, screen);
	}
	else if (minBottomDistance < kBigDistance) {
		offset.y = screen.bottom - bottomRect.top - kMoveToScreen;
		_CallculateXOffset(offset, bottomRect, screen);
	}

	if (offset.x == 0. && offset.y == 0.)
		return;
	STRACE_SAT("move group back to screen: offset x: %f offset y: %f\n",
		offset.x, offset.y);

	desktop->MoveWindowBy(window->GetWindow(), offset.x, offset.y);
	window->DoGroupLayout();
}


void
SATGroup::_CallculateXOffset(BPoint& offset, BRect& frame, BRect& screen)
{
	if (frame.right < screen.left + kMinOverlap)
		offset.x = screen.left - frame.right + kMoveToScreen;
	else if (frame.left > screen.right - kMinOverlap)
		offset.x = screen.right - frame.left - kMoveToScreen;
}


void
SATGroup::_CallculateYOffset(BPoint& offset, BRect& frame, BRect& screen)
{
	if (frame.top > screen.bottom - kMinOverlap)
		offset.y = screen.bottom - frame.top - kMoveToScreen;
	else if (frame.bottom < screen.top + kMinOverlap)
		offset.y = screen.top - frame.bottom + kMoveToScreen;
}
