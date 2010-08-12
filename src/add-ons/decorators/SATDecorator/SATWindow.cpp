/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "SATWindow.h"

#include <Debug.h>

#include "Window.h"


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
	fDecorator = dynamic_cast<SATDecorator*>(fWindow->Decorator());
	fDesktop = fWindow->Desktop();

	fGroupCookie = &fOwnGroupCookie;
	_InitGroup();

	fSATSnappingBehaviourList.AddItem(&fSATStacking);
	fSATSnappingBehaviourList.AddItem(&fSATTiling);
}


SATWindow::~SATWindow()
{
	fShutdown = true;

	if (fForeignGroupCookie.GetGroup())
		fForeignGroupCookie.GetGroup()->RemoveWindow(this);
	if (fOwnGroupCookie.GetGroup())
		fOwnGroupCookie.GetGroup()->RemoveWindow(this);
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

	return true;
}


bool
SATWindow::RemovedFromGroup(SATGroup* group)
{
	STRACE_SAT("SATWindow::RemovedFromGroup group: %p window %s\n", group,
		fWindow->Title());

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
	GroupIterator groupIterator(fStackAndTile, GetWindow()->Desktop());
	for (SATGroup* group = groupIterator.NextGroup(); group;
		group = groupIterator.NextGroup()) {
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
SATWindow::DoGroupLayout()
{
	if (!PositionManagedBySAT())
		return;

	fGroupCookie->DoGroupLayout(this);

	for (int i = 0; i < fSATSnappingBehaviourList.CountItems(); i++)
		fSATSnappingBehaviourList.ItemAt(i)->DoGroupLayout();
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

	// TODO get this values from the decorator
	frame.left -= 5.;
	frame.right += 6.;
	frame.top -= 27;
	frame.bottom += 5;		

	return frame;
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
	if (!fDecorator)
		return false;

	if (IsTabHighlighted() == active)
		return false;

	BRegion dirty;
	fDecorator->HighlightTab(active, &dirty);
	fWindow->ProcessDirtyRegion(dirty);

	return true;
}


bool
SATWindow::HighlightBorders(bool active)
{
	if (!fDecorator)
		return false;

	if (IsBordersHighlighted() == active)
		return false;

	BRegion dirty;
	fDecorator->HighlightBorders(active, &dirty);
	fWindow->ProcessDirtyRegion(dirty);
	return true;
}


bool
SATWindow::IsTabHighlighted()
{
	if (fDecorator)
		return fDecorator->IsTabHighlighted();
	return false;
}


bool
SATWindow::IsBordersHighlighted()
{
	if (fDecorator)
		return fDecorator->IsBordersHighlighted();
	return false;
}


bool
SATWindow::SetStackedMode(bool stacked)
{
	if (!fDecorator)
		return false;
	BRegion dirty;
	fDecorator->SetStackedMode(stacked, &dirty);
	fDesktop->RebuildAndRedrawAfterWindowChange(fWindow, dirty);
	return true;
}


bool
SATWindow::SetStackedTabLength(float length, bool drawZoom)
{
	if (!fDecorator)
		return false;
	BRegion dirty;
	fDecorator->SetStackedTabLength(length, drawZoom, &dirty);
	fDesktop->RebuildAndRedrawAfterWindowChange(fWindow, dirty);
	return true;
}


bool
SATWindow::SetStackedTabMoving(bool moving)
{
	if (!fDecorator)
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


SATWindow::GroupCookie::GroupCookie(SATWindow* satWindow)
	:
	fSATWindow(satWindow),

	windowArea(NULL),

	leftBorder(NULL),
	topBorder(NULL),
	rightBorder(NULL),
	bottomBorder(NULL),

	leftBorderConstraint(NULL),
	topBorderConstraint(NULL),
	rightBorderConstraint(NULL),
	bottomBorderConstraint(NULL),

	leftConstraint(NULL),
	topConstraint(NULL),
	minWidthConstraint(NULL),
	minHeightConstraint(NULL),
	widthConstraint(NULL),
	heightConstraint(NULL)
{
	
}


SATWindow::GroupCookie::~GroupCookie()
{
	Uninit();
}


void
SATWindow::GroupCookie::DoGroupLayout(SATWindow* triggerWindow)
{
	if (!fSATGroup.Get())
		return;

	BRect frame = triggerWindow->CompleteWindowFrame();

	// adjust window size soft constraints
	widthConstraint->SetRightSide(frame.Width());
	heightConstraint->SetRightSide(frame.Height());
	
	// adjust window position soft constraints
	// (a bit more penalty for them so they take precedence)
	leftConstraint->SetRightSide(frame.left);
	topConstraint->SetRightSide(frame.top);


	widthConstraint->SetPenaltyNeg(110);
	widthConstraint->SetPenaltyPos(110);
	heightConstraint->SetPenaltyNeg(110);
	heightConstraint->SetPenaltyPos(110);

	leftConstraint->SetPenaltyNeg(100);
	leftConstraint->SetPenaltyPos(100);
	topConstraint->SetPenaltyNeg(100);
	topConstraint->SetPenaltyPos(100);

	// After we set the new parameter solve and apply the new layout.
	fSATGroup->SolveSATAndAdjustWindows(triggerWindow);

	// set penalties back to normal
	widthConstraint->SetPenaltyNeg(10);
	widthConstraint->SetPenaltyPos(10);
	heightConstraint->SetPenaltyNeg(10);
	heightConstraint->SetPenaltyPos(10);

	leftConstraint->SetPenaltyNeg(1);
	leftConstraint->SetPenaltyPos(1);
	topConstraint->SetPenaltyNeg(1);
	topConstraint->SetPenaltyPos(1);
}


void
SATWindow::GroupCookie::MoveWindow(int32 workspace)
{
	Window* window = fSATWindow->GetWindow();
	Desktop* desktop = window->Desktop();

	BRect frame = fSATWindow->CompleteWindowFrame();
	desktop->MoveWindowBy(window, leftBorder->Value() - frame.left,
		topBorder->Value() - frame.top, workspace);

	// Update frame to the new position
	frame.OffsetBy(leftBorder->Value() - frame.left,
		topBorder->Value() - frame.top);
	desktop->ResizeWindowBy(window, rightBorder->Value() - frame.right,
		bottomBorder->Value() - frame.bottom);
}


bool
SATWindow::GroupCookie::Init(SATGroup* group, WindowArea* area)
{
	ASSERT(fSATGroup.Get() == NULL);

	Window* window = fSATWindow->GetWindow();
	fSATGroup.SetTo(group);
	windowArea = area;

	LinearSpec* linearSpec = group->GetLinearSpec();
	// create variables
	leftBorder = linearSpec->AddVariable();
	topBorder = linearSpec->AddVariable();
	rightBorder = linearSpec->AddVariable();
	bottomBorder = linearSpec->AddVariable();

	leftBorder->SetRange(-DBL_MAX, DBL_MAX);
	topBorder->SetRange(-DBL_MAX, DBL_MAX);
	rightBorder->SetRange(-DBL_MAX, DBL_MAX);
	bottomBorder->SetRange(-DBL_MAX, DBL_MAX);

	if (!leftBorder || !topBorder || !rightBorder || !bottomBorder) {
		// clean up
		Uninit();
		return false;
	}

	// create constraints
	BRect frame = fSATWindow->CompleteWindowFrame();
	leftConstraint = linearSpec->AddConstraint(1.0, leftBorder,
		OperatorType(EQ), frame.left, 1, 1);
	topConstraint  = linearSpec->AddConstraint(1.0, topBorder,
		OperatorType(EQ), frame.top, 1, 1);

	int32 minWidth, maxWidth, minHeight, maxHeight;
	window->GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	minWidthConstraint = linearSpec->AddConstraint(1.0, leftBorder, -1.0,
		rightBorder, OperatorType(LE), -minWidth);
	minHeightConstraint = linearSpec->AddConstraint(1.0, topBorder, -1.0,
		bottomBorder, OperatorType(LE), -minHeight);

	// The width and height constraints have higher penalties than the
	// position constraints (left, top), so a window will keep its size
	// unless explicitly resized.
	widthConstraint = linearSpec->AddConstraint(-1.0, leftBorder, 1.0,
		rightBorder, OperatorType(EQ), frame.Width(), 10, 10);
	heightConstraint = linearSpec->AddConstraint(-1.0, topBorder, 1.0,
		bottomBorder, OperatorType(EQ), frame.Height(), 10, 10);
	
	if (!leftConstraint || !topConstraint || !minWidthConstraint
		|| !minHeightConstraint || !widthConstraint || !heightConstraint) {
		// clean up
		Uninit();
		return false;
	}
	
	leftBorderConstraint = area->LeftTab()->Connect(leftBorder);
	topBorderConstraint = area->TopTab()->Connect(topBorder);
	rightBorderConstraint = area->RightTab()->Connect(rightBorder);
	bottomBorderConstraint = area->BottomTab()->Connect(bottomBorder);

	if (!leftBorderConstraint || !topBorderConstraint
		|| !rightBorderConstraint || !bottomBorderConstraint) {
		Uninit();
		return false;
	}

	return true;
}


void
SATWindow::GroupCookie::Uninit()
{
	delete leftBorder;
	delete topBorder;
	delete rightBorder;
	delete bottomBorder;
	leftBorder = NULL;
	topBorder = NULL;
	rightBorder = NULL;
	bottomBorder = NULL;
	
	delete leftBorderConstraint;
	delete topBorderConstraint;
	delete rightBorderConstraint;
	delete bottomBorderConstraint;
	leftBorderConstraint = NULL;
	topBorderConstraint = NULL;
	rightBorderConstraint = NULL;
	bottomBorderConstraint = NULL;

	delete leftConstraint;
	delete topConstraint;
	delete minWidthConstraint;
	delete minHeightConstraint;
	delete widthConstraint;
	delete heightConstraint;
	leftConstraint = NULL;
	topConstraint = NULL;
	minWidthConstraint = NULL;
	minHeightConstraint = NULL;
	widthConstraint = NULL;
	heightConstraint = NULL;

	fSATGroup.Unset();
	windowArea = NULL;
}


bool
SATWindow::GroupCookie::PropagateToGroup(SATGroup* group, WindowArea* area)
{
	if (!fSATGroup->fSATWindowList.RemoveItem(fSATWindow))
		return false;
	Uninit();

	if (!Init(group, area))
		return false;

	if (!group->fSATWindowList.AddItem(fSATWindow)) {
		Uninit();
		return false;
	}

	return true;
}
