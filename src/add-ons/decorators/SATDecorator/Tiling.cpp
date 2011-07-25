/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "Tiling.h"


#include "SATWindow.h"
#include "StackAndTile.h"
#include "Window.h"


using namespace std;


//#define DEBUG_TILEING

#ifdef DEBUG_TILEING
#	define STRACE_TILING(x...) debug_printf("SAT Tiling: "x)
#else
#	define STRACE_TILING(x...) ;
#endif


SATTiling::SATTiling(SATWindow* window)
	:
	fSATWindow(window),
	fFreeAreaGroup(NULL)
{
	_ResetSearchResults();
}


SATTiling::~SATTiling()
{

}


bool
SATTiling::FindSnappingCandidates(SATGroup* group)
{
	_ResetSearchResults();

	if (fSATWindow->GetGroup() == group)
		return false;

	if (_FindFreeAreaInGroup(group)) {
		fFreeAreaGroup = group;
		_HighlightWindows(fFreeAreaGroup, true);
		return true;
	}

	return false;
}


bool
SATTiling::JoinCandidates()
{
	if (!fFreeAreaGroup)
		return false;

	if (!fFreeAreaGroup->AddWindow(fSATWindow, fFreeAreaLeft, fFreeAreaTop,
		fFreeAreaRight, fFreeAreaBottom)) {
		_ResetSearchResults();
		return false;
	}

	fFreeAreaGroup->WindowAt(0)->DoGroupLayout();

	_ResetSearchResults();
	return true;
}


bool
SATTiling::_FindFreeAreaInGroup(SATGroup* group)
{
	if (_FindFreeAreaInGroup(group, Corner::kLeftTop))
		return true;
	if (_FindFreeAreaInGroup(group, Corner::kRightTop))
		return true;
	if (_FindFreeAreaInGroup(group, Corner::kLeftBottom))
		return true;
	if (_FindFreeAreaInGroup(group, Corner::kRightBottom))
		return true;

	return false;
}


bool
SATTiling::_FindFreeAreaInGroup(SATGroup* group, Corner::position_t cor)
{
	BRect windowFrame = fSATWindow->CompleteWindowFrame();

	const TabList* verticalTabs = group->VerticalTabs();
	for (int i = 0; i < verticalTabs->CountItems(); i++) {
		Tab* tab = verticalTabs->ItemAt(i);
		const CrossingList* crossingList = tab->GetCrossingList();
		for (int c = 0; c < crossingList->CountItems(); c++) {
			Crossing* crossing = crossingList->ItemAt(c);
			if (_InteresstingCrossing(crossing, cor, windowFrame)) {
				if (_FindFreeArea(group, crossing, cor, windowFrame)) {
					STRACE_TILING("SATTiling: free area found; corner %i\n",
						cor);
					return true;
				}
			}
		}
	}

	return false;
}


const float kNoMatch = 999.f;
const float kMaxMatchingDistance = 12.f;


bool
SATTiling::_InteresstingCrossing(Crossing* crossing,
	Corner::position_t cor, BRect& windowFrame)
{
	const Corner* corner = crossing->GetOppositeCorner(cor);
	if (corner->status != Corner::kFree)
		return false;

	float hTabPosition = crossing->HorizontalTab()->Position();
	float vTabPosition = crossing->VerticalTab()->Position();
	float hBorder = 0., vBorder = 0.;
	float vDistance = -1., hDistance = -1.;
	bool windowAtH = false, windowAtV = false;
	switch (cor) {
		case Corner::kLeftTop:
			if (crossing->RightBottomCorner()->status == Corner::kUsed)
				return false;
			vBorder = windowFrame.left;
			hBorder = windowFrame.top;
			if (crossing->LeftBottomCorner()->status == Corner::kUsed)
				windowAtV = true;
			if (crossing->RightTopCorner()->status == Corner::kUsed)
				windowAtH = true;
			vDistance = vTabPosition - vBorder;
			hDistance = hTabPosition - hBorder;
			break;
		case Corner::kRightTop:
			if (crossing->LeftBottomCorner()->status == Corner::kUsed)
				return false;
			vBorder = windowFrame.right;
			hBorder = windowFrame.top;
			if (crossing->RightBottomCorner()->status == Corner::kUsed)
				windowAtV = true;
			if (crossing->LeftTopCorner()->status == Corner::kUsed)
				windowAtH = true;
			vDistance = vBorder - vTabPosition;
			hDistance = hTabPosition - hBorder;
			break;
		case Corner::kLeftBottom:
			if (crossing->RightTopCorner()->status == Corner::kUsed)
				return false;
			vBorder = windowFrame.left;
			hBorder = windowFrame.bottom;
			if (crossing->LeftTopCorner()->status == Corner::kUsed)
				windowAtV = true;
			if (crossing->RightBottomCorner()->status == Corner::kUsed)
				windowAtH = true;
			vDistance = vTabPosition - vBorder;
			hDistance = hBorder - hTabPosition;
			break;
		case Corner::kRightBottom:
			if (crossing->LeftTopCorner()->status == Corner::kUsed)
				return false;
			vBorder = windowFrame.right;
			hBorder = windowFrame.bottom;
			if (crossing->RightTopCorner()->status == Corner::kUsed)
				windowAtV = true;
			if (crossing->LeftBottomCorner()->status == Corner::kUsed)
				windowAtH = true;
			vDistance = vBorder - vTabPosition;
			hDistance = hBorder - hTabPosition;
			break;
	};

	bool hValid = false;
	if (windowAtH && fabs(hDistance) < kMaxMatchingDistance
		&& vDistance  < kMaxMatchingDistance)
		hValid = true;
	bool vValid = false;
	if (windowAtV && fabs(vDistance) < kMaxMatchingDistance
		&& hDistance  < kMaxMatchingDistance)
		vValid = true;
	if (!hValid && !vValid)
		return false;

	return true;
};


const float kBigAreaError = 1E+17;


bool
SATTiling::_FindFreeArea(SATGroup* group, const Crossing* crossing,
	Corner::position_t corner, BRect& windowFrame)
{
	fFreeAreaLeft = fFreeAreaRight = fFreeAreaTop = fFreeAreaBottom = NULL;

	const TabList* hTabs = group->HorizontalTabs();
	const TabList* vTabs = group->VerticalTabs();
	int32 hIndex = hTabs->IndexOf(crossing->HorizontalTab());
	if (hIndex < 0)
		return false;
	int32 vIndex = vTabs->IndexOf(crossing->VerticalTab());
	if (vIndex < 0)
		return false;

	Tab** endHTab = NULL, **endVTab = NULL;
	int8 vSearchDirection = 1, hSearchDirection = 1;
	switch (corner) {
		case Corner::kLeftTop:
			fFreeAreaLeft = crossing->VerticalTab();
			fFreeAreaTop = crossing->HorizontalTab();
			endHTab = &fFreeAreaBottom;
			endVTab = &fFreeAreaRight;
			vSearchDirection = 1;
			hSearchDirection = 1;
			break;
		case Corner::kRightTop:
			fFreeAreaRight = crossing->VerticalTab();
			fFreeAreaTop = crossing->HorizontalTab();
			endHTab = &fFreeAreaBottom;
			endVTab = &fFreeAreaLeft;
			vSearchDirection = -1;
			hSearchDirection = 1;
			break;
		case Corner::kLeftBottom:
			fFreeAreaLeft = crossing->VerticalTab();
			fFreeAreaBottom = crossing->HorizontalTab();
			endHTab = &fFreeAreaTop;
			endVTab = &fFreeAreaRight;
			vSearchDirection = 1;
			hSearchDirection = -1;
			break;
		case Corner::kRightBottom:
			fFreeAreaRight = crossing->VerticalTab();
			fFreeAreaBottom = crossing->HorizontalTab();
			endHTab = &fFreeAreaTop;
			endVTab = &fFreeAreaLeft;
			vSearchDirection = -1;
			hSearchDirection = -1;
			break;
	};

	Tab* bestLeftTab = NULL, *bestRightTab = NULL, *bestTopTab = NULL,
		*bestBottomTab = NULL;
	float bestError = kBigAreaError;
	float error;
	bool stop = false;
	bool found = false;
	int32 v = vIndex;
	do {
		v += vSearchDirection;
		*endVTab = vTabs->ItemAt(v);
		int32 h = hIndex;
		do {
			h += hSearchDirection;
			*endHTab = hTabs->ItemAt(h);
			if (!_CheckArea(group, corner, windowFrame, error)) {
				if (h == hIndex + hSearchDirection)
					stop = true;
				break;
			}
			found = true;
			if (error < bestError) {
				bestError = error;
				bestLeftTab = fFreeAreaLeft;
				bestRightTab = fFreeAreaRight;
				bestTopTab = fFreeAreaTop;
				bestBottomTab = fFreeAreaBottom;
			}
		} while (*endHTab);
		if (stop)
			break;
	} while (*endVTab);
	if (!found)
		return false;

	fFreeAreaLeft = bestLeftTab;
	fFreeAreaRight = bestRightTab;
	fFreeAreaTop = bestTopTab;
	fFreeAreaBottom = bestBottomTab;

	return true;
}


bool
SATTiling::_HasOverlapp(SATGroup* group)
{
	BRect areaRect = _FreeAreaSize();
	areaRect.InsetBy(1., 1.);

	const TabList* hTabs = group->HorizontalTabs();
	for (int32 h = 0; h < hTabs->CountItems(); h++) {
		Tab* hTab = hTabs->ItemAt(h);
		if (hTab->Position() >= areaRect.bottom)
			return false;
		const CrossingList* crossings = hTab->GetCrossingList();
		for (int32 i = 0; i <  crossings->CountItems(); i++) {
			Crossing* leftTopCrossing = crossings->ItemAt(i);
			Tab* vTab = leftTopCrossing->VerticalTab();
			if (vTab->Position() > areaRect.right)
				continue;
			Corner* corner = leftTopCrossing->RightBottomCorner();
			if (corner->status != Corner::kUsed)
				continue;
			BRect rect = corner->windowArea->Frame();
			if (areaRect.Intersects(rect))
				return true;
		}
	}
	return false;
}


bool
SATTiling::_CheckArea(SATGroup* group, Corner::position_t corner,
	BRect& windowFrame, float& error)
{
	error = kBigAreaError;
	if (!_CheckMinFreeAreaSize())
		return false;
	// check if corner is in the free area
	if (!_IsCornerInFreeArea(corner, windowFrame))
		return false;
	
	error = _FreeAreaError(windowFrame);
	if (!_HasOverlapp(group))
		return true;
	return false;
}


bool
SATTiling::_CheckMinFreeAreaSize()
{
	// check if the area has a minimum size
	if (fFreeAreaLeft && fFreeAreaRight
		&& fFreeAreaRight->Position() - fFreeAreaLeft->Position()
			< 2 * kMaxMatchingDistance)
		return false;
	if (fFreeAreaBottom && fFreeAreaTop
		&& fFreeAreaBottom->Position() - fFreeAreaTop->Position()
			< 2 * kMaxMatchingDistance)
		return false;
	return true;
}


float
SATTiling::_FreeAreaError(BRect& windowFrame)
{
	const float kEndTabError = 9999999;
	float error = 0.;
	if (fFreeAreaLeft && fFreeAreaRight)
		error += pow(fFreeAreaRight->Position() - fFreeAreaLeft->Position()
			- windowFrame.Width(), 2);
	else
		error += kEndTabError;
	if (fFreeAreaBottom && fFreeAreaTop)
		error += pow(fFreeAreaBottom->Position() - fFreeAreaTop->Position()
			- windowFrame.Height(), 2);
	else
		error += kEndTabError;
	return error;
}


bool
SATTiling::_IsCornerInFreeArea(Corner::position_t corner, BRect& frame)
{
	BRect freeArea = _FreeAreaSize();

	switch (corner) {
		case Corner::kLeftTop:
			if (freeArea.bottom - kMaxMatchingDistance > frame.top
				&& freeArea.right - kMaxMatchingDistance > frame.left)
				return true;
			break;
		case Corner::kRightTop:
			if (freeArea.bottom - kMaxMatchingDistance > frame.top
				&& freeArea.left + kMaxMatchingDistance < frame.right)
				return true;
			break;
		case Corner::kLeftBottom:
			if (freeArea.top + kMaxMatchingDistance < frame.bottom
				&& freeArea.right - kMaxMatchingDistance > frame.left)
				return true;
			break;
		case Corner::kRightBottom:
			if (freeArea.top + kMaxMatchingDistance < frame.bottom
				&& freeArea.left + kMaxMatchingDistance < frame.right)
				return true;
			break;
	}

	return false;
}


BRect
SATTiling::_FreeAreaSize()
{
	// not to big to be be able to add/sub small float values
	const float kBigValue = 9999999.;
	float left = fFreeAreaLeft ? fFreeAreaLeft->Position() : -kBigValue;
	float right = fFreeAreaRight ? fFreeAreaRight->Position() : kBigValue;
	float top = fFreeAreaTop ? fFreeAreaTop->Position() : -kBigValue;
	float bottom = fFreeAreaBottom ? fFreeAreaBottom->Position() : kBigValue;
	return BRect(left, top, right, bottom);
}


void
SATTiling::_HighlightWindows(SATGroup* group, bool highlight)
{
	const TabList* hTabs = group->HorizontalTabs();
	const TabList* vTabs = group->VerticalTabs();
	// height light windows at all four sites
	bool leftWindowsFound = _SearchHighlightWindow(fFreeAreaLeft, fFreeAreaTop, fFreeAreaBottom, hTabs,
		fFreeAreaTop ? Corner::kLeftBottom : Corner::kLeftTop,
		Decorator::REGION_RIGHT_BORDER, highlight);

	bool topWindowsFound = _SearchHighlightWindow(fFreeAreaTop, fFreeAreaLeft, fFreeAreaRight, vTabs,
		fFreeAreaLeft ? Corner::kRightTop : Corner::kLeftTop,
		Decorator::REGION_BOTTOM_BORDER, highlight);

	bool rightWindowsFound = _SearchHighlightWindow(fFreeAreaRight, fFreeAreaTop, fFreeAreaBottom, hTabs,
		fFreeAreaTop ? Corner::kRightBottom : Corner::kRightTop,
		Decorator::REGION_LEFT_BORDER, highlight);

	bool bottomWindowsFound = _SearchHighlightWindow(fFreeAreaBottom, fFreeAreaLeft, fFreeAreaRight,
		vTabs, fFreeAreaLeft ? Corner::kRightBottom : Corner::kLeftBottom,
		Decorator::REGION_TOP_BORDER, highlight);

	if (leftWindowsFound)
		fSATWindow->HighlightBorders(Decorator::REGION_LEFT_BORDER, highlight);
	if (topWindowsFound)
		fSATWindow->HighlightBorders(Decorator::REGION_TOP_BORDER, highlight);
	if (rightWindowsFound)
		fSATWindow->HighlightBorders(Decorator::REGION_RIGHT_BORDER, highlight);
	if (bottomWindowsFound) {
		fSATWindow->HighlightBorders(Decorator::REGION_BOTTOM_BORDER,
			highlight);
	}
}


bool
SATTiling::_SearchHighlightWindow(Tab* tab, Tab* firstOrthTab,
	Tab* secondOrthTab, const TabList* orthTabs, Corner::position_t areaCorner,
	Decorator::Region region, bool highlight)
{
	bool windowsFound = false;

	if (!tab)
		return false;

	int8 searchDir = 1;
	Tab* startOrthTab = NULL;
	Tab* endOrthTab = NULL;
	if (firstOrthTab) {
		searchDir = 1;
		startOrthTab = firstOrthTab;
		endOrthTab = secondOrthTab;
	}
	else if (secondOrthTab) {
		searchDir = -1;
		startOrthTab = secondOrthTab;
		endOrthTab = firstOrthTab;
	}
	else
		return false;

	int32 index = orthTabs->IndexOf(startOrthTab);
	if (index < 0)
		return false;

	for (; index < orthTabs->CountItems() && index >= 0; index += searchDir) {
		Tab* orthTab = orthTabs->ItemAt(index);
		if (orthTab == endOrthTab)
 			break;
		Crossing* crossing = tab->FindCrossing(orthTab);
		if (!crossing)
			continue;
		Corner* corner = crossing->GetCorner(areaCorner);
		if (corner->windowArea) {
			_HighlightWindows(corner->windowArea, region,  highlight);
			windowsFound = true;
		}
	}
	return windowsFound;
}


void
SATTiling::_HighlightWindows(WindowArea* area, Decorator::Region region,
	bool highlight)
{
	const SATWindowList& list = area->LayerOrder();
	SATWindow* topWindow = list.ItemAt(list.CountItems() - 1);
	if (topWindow == NULL)
		return;
	topWindow->HighlightBorders(region, highlight);
}


void
SATTiling::_ResetSearchResults()
{
	if (!fFreeAreaGroup)
		return;

	_HighlightWindows(fFreeAreaGroup, false);
	fFreeAreaGroup = NULL;
}
