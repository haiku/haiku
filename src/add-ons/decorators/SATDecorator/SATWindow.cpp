/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include "SATWindow.h"

#include <Debug.h>

#include "StackAndTilePrivate.h"

#include "SATGroup.h"
#include "ServerApp.h"
#include "Window.h"


using namespace BPrivate;
using namespace LinearProgramming;


const uint32 kExtentPenalty = 10;


GroupCookie::GroupCookie(SATWindow* satWindow)
	:
	fSATWindow(satWindow),

	fWindowArea(NULL),

	fLeftBorder(NULL),
	fTopBorder(NULL),
	fRightBorder(NULL),
	fBottomBorder(NULL),

	fLeftBorderConstraint(NULL),
	fTopBorderConstraint(NULL),
	fRightBorderConstraint(NULL),
	fBottomBorderConstraint(NULL),

	fLeftConstraint(NULL),
	fTopConstraint(NULL),
	fMinWidthConstraint(NULL),
	fMinHeightConstraint(NULL),
	fWidthConstraint(NULL),
	fHeightConstraint(NULL)
{
}


GroupCookie::~GroupCookie()
{
	Uninit();
}


void
GroupCookie::DoGroupLayout(SATWindow* triggerWindow)
{
	if (!fSATGroup.Get())
		return;

	BRect frame = triggerWindow->CompleteWindowFrame();
	// Make it also work for solver which don't support negative variables
	frame.OffsetBy(kMakePositiveOffset, kMakePositiveOffset);

	// adjust window size soft constraints
	fWidthConstraint->SetRightSide(frame.Width());
	fHeightConstraint->SetRightSide(frame.Height());

	LinearSpec* linearSpec = fSATGroup->GetLinearSpec();
	fLeftConstraint = linearSpec->AddConstraint(1.0, fLeftBorder, kEQ,
		frame.left);
	fTopConstraint  = linearSpec->AddConstraint(1.0, fTopBorder, kEQ,
		frame.top);

	// adjust window position soft constraints
	// (a bit more penalty for them so they take precedence)
	fWidthConstraint->SetPenaltyNeg(-1);
	fWidthConstraint->SetPenaltyPos(-1);
	fHeightConstraint->SetPenaltyNeg(-1);
	fHeightConstraint->SetPenaltyPos(-1);

	// After we set the new parameter solve and apply the new layout.
	ResultType result;
	for (int32 tries = 0; tries < 15; tries++) {
		result = fSATGroup->GetLinearSpec()->Solve();
		if (result == kInfeasible) {
			debug_printf("can't solve constraints!\n");
			break;
		}
		if (result == kOptimal) {
			fSATGroup->AdjustWindows(triggerWindow);
			_UpdateWindowSize(frame);
			break;
		}
	}

	// set penalties back to normal
	fWidthConstraint->SetPenaltyNeg(kExtentPenalty);
	fWidthConstraint->SetPenaltyPos(kExtentPenalty);
	fHeightConstraint->SetPenaltyNeg(kExtentPenalty);
	fHeightConstraint->SetPenaltyPos(kExtentPenalty);

	linearSpec->RemoveConstraint(fLeftConstraint);
	fLeftConstraint = NULL;
	linearSpec->RemoveConstraint(fTopConstraint);
	fTopConstraint = NULL;
}


void
GroupCookie::MoveWindow(int32 workspace)
{
	Window* window = fSATWindow->GetWindow();
	Desktop* desktop = window->Desktop();

	BRect frame = fSATWindow->CompleteWindowFrame();
	BRect frameSAT(fLeftBorder->Value() - kMakePositiveOffset,
		fTopBorder->Value() - kMakePositiveOffset,
		fRightBorder->Value() - kMakePositiveOffset,
		fBottomBorder->Value() - kMakePositiveOffset);

	desktop->MoveWindowBy(window, round(frameSAT.left - frame.left),
		round(frameSAT.top - frame.top), workspace);

	// Update frame to the new position
	frame.OffsetBy(round(frameSAT.left - frame.left),
		round(frameSAT.top - frame.top));
	desktop->ResizeWindowBy(window, round(frameSAT.right - frame.right),
		round(frameSAT.bottom - frame.bottom));

	_UpdateWindowSize(frame);
}


void
GroupCookie::SetSizeLimits(int32 minWidth, int32 maxWidth, int32 minHeight,
	int32 maxHeight)
{
	fMinWidthConstraint->SetRightSide(minWidth);
	fMinHeightConstraint->SetRightSide(minHeight);
}


bool
GroupCookie::Init(SATGroup* group, WindowArea* area)
{
	ASSERT(fSATGroup.Get() == NULL);

	fSATGroup.SetTo(group);
	fWindowArea = area;

	LinearSpec* linearSpec = group->GetLinearSpec();
	// create variables
	fLeftBorder = linearSpec->AddVariable();
	fTopBorder = linearSpec->AddVariable();
	fRightBorder = linearSpec->AddVariable();
	fBottomBorder = linearSpec->AddVariable();

	if (!fLeftBorder || !fTopBorder || !fRightBorder || !fBottomBorder) {
		// clean up
		Uninit();
		return false;
	}

	// create constraints
	BRect frame = fSATWindow->CompleteWindowFrame();

	int32 minWidth, maxWidth, minHeight, maxHeight;
	fSATWindow->GetSizeLimits(&minWidth, &maxWidth, &minHeight,
		&maxHeight);
	fMinWidthConstraint = linearSpec->AddConstraint(1.0, fRightBorder, -1.0,
		fLeftBorder, kGE, minWidth);
	fMinHeightConstraint = linearSpec->AddConstraint(1.0, fBottomBorder, -1.0,
		fTopBorder, kGE, minHeight);

	// The width and height constraints have higher penalties than the
	// position constraints (left, top), so a window will keep its size
	// unless explicitly resized.
	fWidthConstraint = linearSpec->AddConstraint(-1.0, fLeftBorder, 1.0,
		fRightBorder, kEQ, frame.Width(), kExtentPenalty,
		kExtentPenalty);
	fHeightConstraint = linearSpec->AddConstraint(-1.0, fTopBorder, 1.0,
		fBottomBorder, kEQ, frame.Height(), kExtentPenalty,
		kExtentPenalty);

	if (!fMinWidthConstraint
		|| !fMinHeightConstraint || !fWidthConstraint || !fHeightConstraint) {
		// clean up
		Uninit();
		return false;
	}

	fLeftBorderConstraint = area->LeftTab()->Connect(fLeftBorder);
	fTopBorderConstraint = area->TopTab()->Connect(fTopBorder);
	fRightBorderConstraint = area->RightTab()->Connect(fRightBorder);
	fBottomBorderConstraint = area->BottomTab()->Connect(fBottomBorder);

	if (!fLeftBorderConstraint || !fTopBorderConstraint
		|| !fRightBorderConstraint || !fBottomBorderConstraint) {
		Uninit();
		return false;
	}

	return true;
}


void
GroupCookie::Uninit()
{
	delete fLeftBorder;
	delete fTopBorder;
	delete fRightBorder;
	delete fBottomBorder;
	fLeftBorder = NULL;
	fTopBorder = NULL;
	fRightBorder = NULL;
	fBottomBorder = NULL;

	delete fLeftBorderConstraint;
	delete fTopBorderConstraint;
	delete fRightBorderConstraint;
	delete fBottomBorderConstraint;
	fLeftBorderConstraint = NULL;
	fTopBorderConstraint = NULL;
	fRightBorderConstraint = NULL;
	fBottomBorderConstraint = NULL;

	delete fLeftConstraint;
	delete fTopConstraint;
	delete fMinWidthConstraint;
	delete fMinHeightConstraint;
	delete fWidthConstraint;
	delete fHeightConstraint;
	fLeftConstraint = NULL;
	fTopConstraint = NULL;
	fMinWidthConstraint = NULL;
	fMinHeightConstraint = NULL;
	fWidthConstraint = NULL;
	fHeightConstraint = NULL;

	fSATGroup.Unset();
	fWindowArea = NULL;
}


bool
GroupCookie::PropagateToGroup(SATGroup* group, WindowArea* area)
{
	if (!fSATGroup->fSATWindowList.RemoveItem(fSATWindow))
		return false;
	Uninit();

	if (!Init(group, area))
		return false;

	if (!area->SetGroup(group) || !group->fSATWindowList.AddItem(fSATWindow)) {
		Uninit();
		return false;
	}

	return true;
}


void
GroupCookie::_UpdateWindowSize(const BRect& frame)
{
	// adjust window size soft constraints
	fWidthConstraint->SetRightSide(frame.Width());
	fHeightConstraint->SetRightSide(frame.Height());
}


// #pragma mark -


SATWindow::SATWindow(StackAndTile* sat, Window* window)
	:
	fWindow(window),
	fStackAndTile(sat),

	fOwnGroupCookie(this),
	fForeignGroupCookie(this),

	fOngoingSnapping(NULL),
	fSATStacking(this),
	fSATTiling(this),
	fShutdown(false)
{
	fId = _GenerateId();

	fDesktop = fWindow->Desktop();

	fGroupCookie = &fOwnGroupCookie;
	_InitGroup();

	fSATSnappingBehaviourList.AddItem(&fSATStacking);
	fSATSnappingBehaviourList.AddItem(&fSATTiling);

	// read initial limit values
	fWindow->GetSizeLimits(&fOriginalMinWidth, &fOriginalMaxWidth,
		&fOriginalMinHeight, &fOriginalMaxHeight);
	Resized();
}


SATWindow::~SATWindow()
{
	fShutdown = true;

	if (fForeignGroupCookie.GetGroup())
		fForeignGroupCookie.GetGroup()->RemoveWindow(this);
	if (fOwnGroupCookie.GetGroup())
		fOwnGroupCookie.GetGroup()->RemoveWindow(this);
}


SATDecorator*
SATWindow::GetDecorator() const
{
	return static_cast<SATDecorator*>(fWindow->Decorator());
}


SATGroup*
SATWindow::GetGroup()
{
	if (!fGroupCookie->GetGroup())
		_InitGroup();

	// manually set the tabs of the single window
	WindowArea* windowArea = fGroupCookie->GetWindowArea();
	if (!PositionManagedBySAT() && windowArea) {
		BRect frame = CompleteWindowFrame();
		windowArea->LeftTopCrossing()->VerticalTab()->SetPosition(frame.left);
		windowArea->LeftTopCrossing()->HorizontalTab()->SetPosition(frame.top);
		windowArea->RightBottomCrossing()->VerticalTab()->SetPosition(
			frame.right);
		windowArea->RightBottomCrossing()->HorizontalTab()->SetPosition(
			frame.bottom);
	}

	return fGroupCookie->GetGroup();
}


bool
SATWindow::HandleMessage(SATWindow* sender, BPrivate::LinkReceiver& link,
	BPrivate::LinkSender& reply)
{
	int32 target;
	link.Read<int32>(&target);
	if (target == kStacking)
		return StackingEventHandler::HandleMessage(sender, link, reply);

	return false;
}


bool
SATWindow::PropagateToGroup(SATGroup* group, WindowArea* area)
{
	return fGroupCookie->PropagateToGroup(group, area);
}


void
SATWindow::MoveWindowToSAT(int32 workspace)
{
	fGroupCookie->MoveWindow(workspace);
}


bool
SATWindow::AddedToGroup(SATGroup* group, WindowArea* area)
{
	STRACE_SAT("SATWindow::AddedToGroup group: %p window %s\n", group,
		fWindow->Title());
	if (fGroupCookie == &fForeignGroupCookie)
		return false;
	if (fOwnGroupCookie.GetGroup())
		fGroupCookie = &fForeignGroupCookie;

	if (!fGroupCookie->Init(group, area)) {
		fGroupCookie = &fOwnGroupCookie;
		return false;
	}

	if (group->CountItems() > 1)
		area->UpdateSizeLimits();

	if (group->CountItems() == 2)
		group->WindowAt(0)->_UpdateSizeLimits();
	return true;
}


bool
SATWindow::RemovedFromGroup(SATGroup* group, bool stayBelowMouse)
{
	STRACE_SAT("SATWindow::RemovedFromGroup group: %p window %s\n", group,
		fWindow->Title());

	_RestoreOriginalSize(stayBelowMouse);
	if (group->CountItems() == 1)
		group->WindowAt(0)->_RestoreOriginalSize(false);

	if (fShutdown) {
		fGroupCookie->Uninit();
		return true;
	}

	ASSERT(fGroupCookie->GetGroup() == group);
	fGroupCookie->Uninit();
	if (fGroupCookie == &fOwnGroupCookie)
		_InitGroup();
	else
		fGroupCookie = &fOwnGroupCookie;

	return true;
}


bool
SATWindow::StackWindow(SATWindow* child)
{
	SATGroup* group = GetGroup();
	WindowArea* area = GetWindowArea();
	if (!group || !area)
		return false;

	bool status = group->AddWindow(child, area, this);

	if (status) {
		area->WindowList().ItemAt(0)->SetStackedMode(true);
			// for the case we are the first added window
		child->SetStackedMode(true);
	}

	DoGroupLayout();
	return true;
}


void
SATWindow::RemovedFromArea(WindowArea* area)
{
	for (int i = 0; i < fSATSnappingBehaviourList.CountItems(); i++)
		fSATSnappingBehaviourList.ItemAt(i)->RemovedFromArea(area);
}


void
SATWindow::FindSnappingCandidates()
{
	fOngoingSnapping = NULL;

	if (fWindow->Feel() != B_NORMAL_WINDOW_FEEL)
		return;

	GroupIterator groupIterator(fStackAndTile, GetWindow()->Desktop());
	for (SATGroup* group = groupIterator.NextGroup(); group;
		group = groupIterator.NextGroup()) {
		if (group->CountItems() == 1
			&& group->WindowAt(0)->GetWindow()->Feel() != B_NORMAL_WINDOW_FEEL)
			continue;
		for (int i = 0; i < fSATSnappingBehaviourList.CountItems(); i++) {
			if (fSATSnappingBehaviourList.ItemAt(i)->FindSnappingCandidates(
				group)) {
				fOngoingSnapping = fSATSnappingBehaviourList.ItemAt(i);
				return;
			}
		}
	}
}


bool
SATWindow::JoinCandidates()
{
	if (!fOngoingSnapping)
		return false;
	bool status = fOngoingSnapping->JoinCandidates();
	fOngoingSnapping = NULL;

	return status;
}


void
SATWindow::DoWindowLayout()
{
	for (int i = 0; i < fSATSnappingBehaviourList.CountItems(); i++)
		fSATSnappingBehaviourList.ItemAt(i)->DoWindowLayout();
}


void
SATWindow::DoGroupLayout()
{
	if (!PositionManagedBySAT())
		return;

	fGroupCookie->DoGroupLayout(this);

	DoWindowLayout();
}


void
SATWindow::SetSizeLimits(int32 minWidth, int32 maxWidth, int32 minHeight,
	int32 maxHeight)
{
	_UpdateSizeLimits();
}


void
SATWindow::Resized()
{
	BRect frame = fWindow->Frame();
	fOriginalWidth = frame.Width();
	fOriginalHeight = frame.Height();
}


BRect
SATWindow::CompleteWindowFrame()
{
	BRect frame = fWindow->Frame();
	if (fDesktop
		&& fDesktop->CurrentWorkspace() != fWindow->CurrentWorkspace()) {
		window_anchor& anchor = fWindow->Anchor(fWindow->CurrentWorkspace());
		if (anchor.position != kInvalidWindowPosition)
			frame.OffsetTo(anchor.position);
	}

	SATDecorator* decorator = GetDecorator();
	if (!decorator)
		return frame;
	frame.left -= decorator->BorderWidth();
	frame.right += decorator->BorderWidth() + 1;
	frame.top -= decorator->BorderWidth() + decorator->TabHeight() + 1;
	frame.bottom += decorator->BorderWidth();

	return frame;
}


void
SATWindow::GetSizeLimits(int32* minWidth, int32* maxWidth, int32* minHeight,
	int32* maxHeight) const
{
	fWindow->GetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	SATDecorator* decorator = GetDecorator();
	if (decorator == NULL)
		return;

	*minWidth += 2 * (int32)decorator->BorderWidth() + 1;
	*minHeight += 2 * (int32)decorator->BorderWidth()
		+ (int32)decorator->TabHeight() + 1;
	*maxWidth += 2 * (int32)decorator->BorderWidth() + 1;
	*maxHeight += 2 * (int32)decorator->BorderWidth()
		+ (int32)decorator->TabHeight() + 1;
}


bool
SATWindow::PositionManagedBySAT()
{
	if (fGroupCookie->GetGroup() && fGroupCookie->GetGroup()->CountItems() == 1)
		return false;

	return true;
}


bool
SATWindow::HighlightTab(bool active)
{
	SATDecorator* decorator = GetDecorator();
	if (!decorator)
		return false;

	BRegion dirty;
	uint8 highlight = active ?  SATDecorator::HIGHLIGHT_STACK_AND_TILE : 0;
	decorator->SetRegionHighlight(SATDecorator::REGION_TAB, highlight, &dirty);
	decorator->SetRegionHighlight(SATDecorator::REGION_CLOSE_BUTTON, highlight,
		&dirty);
	decorator->SetRegionHighlight(SATDecorator::REGION_ZOOM_BUTTON, highlight,
		&dirty);

	fWindow->ProcessDirtyRegion(dirty);
	return true;
}


bool
SATWindow::HighlightBorders(Decorator::Region region, bool active)
{
	SATDecorator* decorator = GetDecorator();
	if (!decorator)
		return false;

	BRegion dirty;
	uint8 highlight = active ? SATDecorator::HIGHLIGHT_STACK_AND_TILE : 0;
	decorator->SetRegionHighlight(region, highlight, &dirty);

	fWindow->ProcessDirtyRegion(dirty);
	return true;
}


bool
SATWindow::SetStackedMode(bool stacked)
{
	SATDecorator* decorator = GetDecorator();
	if (!decorator)
		return false;
	BRegion dirty;
	decorator->SetStackedMode(stacked, &dirty);
	fDesktop->RebuildAndRedrawAfterWindowChange(fWindow, dirty);
	return true;
}


bool
SATWindow::SetStackedTabLength(float length)
{
	SATDecorator* decorator = GetDecorator();
	if (!decorator)
		return false;
	BRegion dirty;
	decorator->SetStackedTabLength(length, &dirty);
	fDesktop->RebuildAndRedrawAfterWindowChange(fWindow, dirty);
	return true;
}


bool
SATWindow::SetStackedTabMoving(bool moving)
{
	SATDecorator* decorator = GetDecorator();
	if (!decorator)
		return false;

	if (!moving)
		DoGroupLayout();

	return true;
}


void
SATWindow::TabLocationMoved(float location, bool shifting)
{
	for (int i = 0; i < fSATSnappingBehaviourList.CountItems(); i++)
		fSATSnappingBehaviourList.ItemAt(i)->TabLocationMoved(location,
			shifting);
}


uint64
SATWindow::Id()
{
	return fId;
}


bool
SATWindow::SetSettings(const BMessage& message)
{
	uint64 id;
	if (message.FindInt64("window_id", (int64*)&id) != B_OK)
		return false;
	fId = id;
	return true;
}


void
SATWindow::GetSettings(BMessage& message)
{
	message.AddInt64("window_id", fId);
}


void
SATWindow::_InitGroup()
{
	ASSERT(fGroupCookie == &fOwnGroupCookie);
	ASSERT(fOwnGroupCookie.GetGroup() == NULL);
	STRACE_SAT("SATWindow::_InitGroup %s\n", fWindow->Title());
	SATGroup* group = new (std::nothrow)SATGroup;
	if (!group)
		return;
	BReference<SATGroup> groupRef;
	groupRef.SetTo(group, true);

	/* AddWindow also will trigger the window to hold a reference on the new
	group. */
	if (!groupRef->AddWindow(this, NULL, NULL, NULL, NULL))
		STRACE_SAT("SATWindow::_InitGroup(): adding window to group failed\n");
}


void
SATWindow::_UpdateSizeLimits()
{
	int32 minWidth, minHeight;
	int32 maxWidth, maxHeight;
	fWindow->GetSizeLimits(&fOriginalMinWidth, &maxWidth,
		&fOriginalMinHeight, &maxHeight);
	minWidth = fOriginalMinWidth;
	minHeight = fOriginalMinHeight;
	if (maxWidth != B_SIZE_UNLIMITED)
		fOriginalMaxWidth = maxWidth;
	if (maxHeight != B_SIZE_UNLIMITED)
		fOriginalMaxHeight = maxHeight;
	SATDecorator* decorator = GetDecorator();
	// if no size limit is set but the window is not resizeable choose the
	// current size as limit
	if (fWindow->Look() == B_MODAL_WINDOW_LOOK
		|| fWindow->Look() == B_BORDERED_WINDOW_LOOK
		|| fWindow->Look() == B_NO_BORDER_WINDOW_LOOK
		|| (fWindow->Flags() & B_NOT_RESIZABLE) != 0
		|| (fWindow->Flags() & B_NOT_H_RESIZABLE) != 0
		|| (fWindow->Flags() & B_NOT_V_RESIZABLE) != 0) {
		int32 minDecorWidth = 1, maxDecorWidth = 1;
		int32 minDecorHeight = 1, maxDecorHeight = 1;
		if (decorator)
			decorator->GetSizeLimits(&minDecorWidth, &minDecorHeight,
				&maxDecorWidth, &maxDecorHeight);
		BRect frame = fWindow->Frame();
		if (fOriginalMinWidth <= minDecorWidth)
			minWidth = frame.IntegerWidth();
		if (fOriginalMinHeight <= minDecorHeight)
			minHeight = frame.IntegerHeight();
	}
	fWindow->SetSizeLimits(minWidth, B_SIZE_UNLIMITED,
		minHeight, B_SIZE_UNLIMITED);


	if (decorator) {
		minWidth += 2 * (int32)decorator->BorderWidth();
		minHeight += 2 * (int32)decorator->BorderWidth()
			+ (int32)decorator->TabHeight() + 1;
	}
	fGroupCookie->SetSizeLimits(minWidth, B_SIZE_UNLIMITED, minHeight,
		B_SIZE_UNLIMITED);
}


uint64
SATWindow::_GenerateId()
{
	bigtime_t time = real_time_clock_usecs();
	srand(time);
	int16 randNumber = rand();
	return (time & ~0xFFFF) | randNumber;
}

void
SATWindow::_RestoreOriginalSize(bool stayBelowMouse)
{
	// restore size
	fWindow->SetSizeLimits(fOriginalMinWidth, fOriginalMaxWidth,
		fOriginalMinHeight, fOriginalMaxHeight);
	BRect frame = fWindow->Frame();
	float x = 0, y = 0;
	if (fWindow->Look() == B_MODAL_WINDOW_LOOK
		|| fWindow->Look() == B_BORDERED_WINDOW_LOOK
		|| fWindow->Look() == B_NO_BORDER_WINDOW_LOOK
		|| (fWindow->Flags() & B_NOT_RESIZABLE) != 0) {
		x = fOriginalWidth - frame.Width();
		y = fOriginalHeight - frame.Height();
	} else {
		if ((fWindow->Flags() & B_NOT_H_RESIZABLE) != 0)
			x = fOriginalWidth - frame.Width();
		if ((fWindow->Flags() & B_NOT_V_RESIZABLE) != 0)
			y = fOriginalHeight - frame.Height();
	}
	fDesktop->ResizeWindowBy(fWindow, x, y);

	if (!stayBelowMouse)
		return;
	// verify that the window stays below the mouse
	BPoint mousePosition;
	int32 buttons;
	fDesktop->GetLastMouseState(&mousePosition, &buttons);
	SATDecorator* decorator = GetDecorator();
	if (decorator == NULL)
		return;
	BRect tabRect = decorator->TabRect();
	if (mousePosition.y < tabRect.bottom && mousePosition.y > tabRect.top
		&& mousePosition.x <= frame.right + decorator->BorderWidth() +1
		&& mousePosition.x >= frame.left + decorator->BorderWidth()) {
		// verify mouse stays on the tab
		float deltaX = 0;
		if (tabRect.right < mousePosition.x)
			deltaX = mousePosition.x - tabRect.right + 20;
		else if (tabRect.left > mousePosition.x)
			deltaX = mousePosition.x - tabRect.left - 20;
		fDesktop->MoveWindowBy(fWindow, deltaX, 0);
	} else {
		// verify mouse stays on the border
		float deltaX = 0;
		float deltaY = 0;
		BRect newFrame = fWindow->Frame();
		if (x != 0 && mousePosition.x > frame.left
			&& mousePosition.x > newFrame.right) {
			deltaX = mousePosition.x - newFrame.right;
			if (mousePosition.x > frame.right)
				deltaX -= mousePosition.x - frame.right;
		}
		if (y != 0 && mousePosition.y > frame.top
			&& mousePosition.y > newFrame.bottom) {
			deltaY = mousePosition.y - newFrame.bottom;
			if (mousePosition.y > frame.bottom)
				deltaY -= mousePosition.y - frame.bottom;
		}
			fDesktop->MoveWindowBy(fWindow, deltaX, deltaY);
	}
}
