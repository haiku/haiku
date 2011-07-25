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


const float kExtentPenalty = 1;
const float kHighPenalty = 10;
const float kInequalityPenalty = 10000;


GroupCookie::GroupCookie(SATWindow* satWindow)
	:
	fSATWindow(satWindow),

	fWindowArea(NULL),

	fLeftBorder(NULL),
	fTopBorder(NULL),
	fRightBorder(NULL),
	fBottomBorder(NULL),

	fMinWidthConstraint(NULL),
	fMinHeightConstraint(NULL),
	fMaxWidthConstraint(NULL),
	fMaxHeightConstraint(NULL),
	fWidthConstraint(NULL),
	fHeightConstraint(NULL)
{
}


GroupCookie::~GroupCookie()
{
	Uninit();
}


void
GroupCookie::DoGroupLayout()
{
	if (!fSATGroup.Get())
		return;

	BRect frame = fSATWindow->CompleteWindowFrame();
	// Make it also work for solver which don't support negative variables
	frame.OffsetBy(kMakePositiveOffset, kMakePositiveOffset);

	// adjust window size soft constraints
	fWidthConstraint->SetRightSide(frame.Width());
	fHeightConstraint->SetRightSide(frame.Height());

	LinearSpec* linearSpec = fSATGroup->GetLinearSpec();
	Constraint* leftConstraint = linearSpec->AddConstraint(1.0, fLeftBorder,
		kEQ, frame.left);
	Constraint* topConstraint = linearSpec->AddConstraint(1.0, fTopBorder, kEQ,
		frame.top);

	// give soft constraints a high penalty
	fWidthConstraint->SetPenaltyNeg(kHighPenalty);
	fWidthConstraint->SetPenaltyPos(kHighPenalty);
	fHeightConstraint->SetPenaltyNeg(kHighPenalty);
	fHeightConstraint->SetPenaltyPos(kHighPenalty);

	// After we set the new parameter solve and apply the new layout.
	ResultType result;
	for (int32 tries = 0; tries < 15; tries++) {
		result = fSATGroup->GetLinearSpec()->Solve();
		if (result == kInfeasible) {
			debug_printf("can't solve constraints!\n");
			break;
		}
		if (result == kOptimal) {
			fSATGroup->AdjustWindows(fSATWindow);
			break;
		}
	}

	// set penalties back to normal
	fWidthConstraint->SetPenaltyNeg(kExtentPenalty);
	fWidthConstraint->SetPenaltyPos(kExtentPenalty);
	fHeightConstraint->SetPenaltyNeg(kExtentPenalty);
	fHeightConstraint->SetPenaltyPos(kExtentPenalty);

	linearSpec->RemoveConstraint(leftConstraint);
	linearSpec->RemoveConstraint(topConstraint);
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

	fSATWindow->AdjustSizeLimits(frameSAT);
	desktop->MoveWindowBy(window, round(frameSAT.left - frame.left),
		round(frameSAT.top - frame.top), workspace);

	// Update frame to the new position
	frame.OffsetBy(round(frameSAT.left - frame.left),
		round(frameSAT.top - frame.top));
	desktop->ResizeWindowBy(window, round(frameSAT.right - frame.right),
		round(frameSAT.bottom - frame.bottom));

	UpdateSizeConstaints(frameSAT);
}


void
GroupCookie::SetSizeLimits(int32 minWidth, int32 maxWidth, int32 minHeight,
	int32 maxHeight)
{
	fMinWidthConstraint->SetRightSide(minWidth);
	fMinHeightConstraint->SetRightSide(minHeight);
	fMaxWidthConstraint->SetRightSide(maxWidth);
	fMaxHeightConstraint->SetRightSide(maxHeight);
}


void
GroupCookie::UpdateSizeConstaints(const BRect& frame)
{
	// adjust window size soft constraints
	if (fSATWindow->IsHResizeable() == true)
		fWidthConstraint->SetRightSide(frame.Width());
	if (fSATWindow->IsVResizeable() == true)
		fHeightConstraint->SetRightSide(frame.Height());
}


bool
GroupCookie::Init(SATGroup* group, WindowArea* area)
{
	ASSERT(fSATGroup.Get() == NULL);

	fSATGroup.SetTo(group);
	fWindowArea = area;

	LinearSpec* linearSpec = group->GetLinearSpec();
	// create variables
	fLeftBorder = area->LeftTab()->Var();
	fTopBorder = area->TopTab()->Var();
	fRightBorder = area->RightTab()->Var();
	fBottomBorder = area->BottomTab()->Var();

	// size limit constraints
	int32 minWidth, maxWidth;
	int32 minHeight, maxHeight;
	fSATWindow->GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	fSATWindow->AddDecorator(&minWidth, &maxWidth, &minHeight, &maxHeight);

	fMinWidthConstraint = linearSpec->AddConstraint(1.0, fRightBorder, -1.0,
		fLeftBorder, kGE, minWidth);
	fMinHeightConstraint = linearSpec->AddConstraint(1.0, fBottomBorder, -1.0,
		fTopBorder, kGE, minHeight);

	fMaxWidthConstraint = linearSpec->AddConstraint(1.0, fRightBorder, -1.0,
		fLeftBorder, kLE, maxWidth, kInequalityPenalty, kInequalityPenalty);
	fMaxHeightConstraint = linearSpec->AddConstraint(1.0, fBottomBorder, -1.0,
		fTopBorder, kLE, maxHeight, kInequalityPenalty, kInequalityPenalty);

	// Width and height have soft constraints
 	BRect frame = fSATWindow->CompleteWindowFrame();
	fWidthConstraint = linearSpec->AddConstraint(1.0, fRightBorder, -1.0,
		fLeftBorder, kEQ, frame.Width(), kExtentPenalty,
		kExtentPenalty);
	fHeightConstraint = linearSpec->AddConstraint(-1.0, fTopBorder, 1.0,
		fBottomBorder, kEQ, frame.Height(), kExtentPenalty,
		kExtentPenalty);

	if (!fMinWidthConstraint || !fMinHeightConstraint || !fWidthConstraint
		|| !fHeightConstraint || !fMaxWidthConstraint
		|| !fMaxHeightConstraint) {
		// clean up
		Uninit();
		return false;
	}

	return true;
}


void
GroupCookie::Uninit()
{
	fLeftBorder = NULL;
	fTopBorder = NULL;
	fRightBorder = NULL;
	fBottomBorder = NULL;

	delete fMinWidthConstraint;
	delete fMinHeightConstraint;
	delete fMaxWidthConstraint;
	delete fMaxHeightConstraint;
	delete fWidthConstraint;
	delete fHeightConstraint;
	fMinWidthConstraint = NULL;
	fMinHeightConstraint = NULL;
	fMaxWidthConstraint = NULL;
	fMaxHeightConstraint = NULL;
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

	// read initial limit values
	fWindow->GetSizeLimits(&fOriginalMinWidth, &fOriginalMaxWidth,
		&fOriginalMinHeight, &fOriginalMaxHeight);
	BRect frame = fWindow->Frame();
	fOriginalWidth = frame.Width();
	fOriginalHeight = frame.Height();

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

	return true;
}


bool
SATWindow::RemovedFromGroup(SATGroup* group, bool stayBelowMouse)
{
	STRACE_SAT("SATWindow::RemovedFromGroup group: %p window %s\n", group,
		fWindow->Title());

	_RestoreOriginalSize(stayBelowMouse);

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

	if (group->AddWindow(child, area, this) == false)
		return false;

	DoGroupLayout();

	if (fWindow->AddWindowToStack(child->GetWindow()) == false) {
		group->RemoveWindow(child);
		DoGroupLayout();
		return false;
	}

	return true;
}


void
SATWindow::RemovedFromArea(WindowArea* area)
{
	fWindow->DetachFromWindowStack(true);
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
SATWindow::DoGroupLayout()
{
	if (!PositionManagedBySAT())
		return;

	fGroupCookie->DoGroupLayout();
}


void
SATWindow::AdjustSizeLimits(BRect targetFrame)
{
	SATDecorator* decorator = GetDecorator();
	if (decorator == NULL)
		return;

	targetFrame.right -= 2 * (int32)decorator->BorderWidth();
	targetFrame.bottom -= 2 * (int32)decorator->BorderWidth()
		+ (int32)decorator->TabHeight() + 1;

	int32 minWidth, maxWidth;
	int32 minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);

	if (maxWidth < targetFrame.Width())
		maxWidth = targetFrame.IntegerWidth();
	if (maxHeight < targetFrame.Height())
		maxHeight = targetFrame.IntegerHeight();

	fWindow->SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);
}


void
SATWindow::GetSizeLimits(int32* minWidth, int32* maxWidth, int32* minHeight,
	int32* maxHeight) const
{
	*minWidth = fOriginalMinWidth;
	*minHeight = fOriginalMinHeight;
	*maxWidth = fOriginalMaxWidth;
	*maxHeight = fOriginalMaxHeight;

	SATDecorator* decorator = GetDecorator();
	if (decorator == NULL)
		return;

	int32 minDecorWidth = 1, maxDecorWidth = 1;
	int32 minDecorHeight = 1, maxDecorHeight = 1;
	decorator->GetSizeLimits(&minDecorWidth, &minDecorHeight,
		&maxDecorWidth, &maxDecorHeight);

	// if no size limit is set but the window is not resizeable choose the
	// current size as limit
	if (IsHResizeable() == false && fOriginalMinWidth <= minDecorWidth)
		*minWidth = (int32)fOriginalWidth;
	if (IsVResizeable() == false && fOriginalMinHeight <= minDecorHeight)
		*minHeight = (int32)fOriginalHeight;

	if (*minWidth > *maxWidth)
		*maxWidth = *minWidth;
	if (*minHeight > *maxHeight)
		*maxHeight = *minHeight;
}


void
SATWindow::AddDecorator(int32* minWidth, int32* maxWidth, int32* minHeight,
	int32* maxHeight)
{
	SATDecorator* decorator = GetDecorator();
	if (decorator == NULL)
		return;

	*minWidth += 2 * (int32)decorator->BorderWidth();
	*minHeight += 2 * (int32)decorator->BorderWidth()
		+ (int32)decorator->TabHeight() + 1;
	*maxWidth += 2 * (int32)decorator->BorderWidth();
	*maxHeight += 2 * (int32)decorator->BorderWidth()
		+ (int32)decorator->TabHeight() + 1;
}


void
SATWindow::AddDecorator(BRect& frame)
{
	SATDecorator* decorator = GetDecorator();
	if (!decorator)
		return;
	frame.left -= decorator->BorderWidth();
	frame.right += decorator->BorderWidth() + 1;
	frame.top -= decorator->BorderWidth() + decorator->TabHeight() + 1;
	frame.bottom += decorator->BorderWidth();
}


void
SATWindow::SetOriginalSizeLimits(int32 minWidth, int32 maxWidth,
	int32 minHeight, int32 maxHeight)
{
	fOriginalMinWidth = minWidth;
	fOriginalMaxWidth = maxWidth;
	fOriginalMinHeight = minHeight;
	fOriginalMaxHeight = maxHeight;

	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	AddDecorator(&minWidth, &maxWidth, &minHeight, &maxHeight);
	fGroupCookie->SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);
}


void
SATWindow::Resized()
{
	bool hResizeable = IsHResizeable();
	bool vResizeable = IsVResizeable();
	if (hResizeable == false && vResizeable == false)
		return;

	BRect frame = fWindow->Frame();
	if (hResizeable)
		fOriginalWidth = frame.Width();
	if (vResizeable)
		fOriginalHeight = frame.Height();

	fGroupCookie->UpdateSizeConstaints(CompleteWindowFrame());
}


bool
SATWindow::IsHResizeable() const
{
	if (fWindow->Look() == B_MODAL_WINDOW_LOOK
		|| fWindow->Look() == B_BORDERED_WINDOW_LOOK
		|| fWindow->Look() == B_NO_BORDER_WINDOW_LOOK
		|| (fWindow->Flags() & B_NOT_RESIZABLE) != 0
		|| (fWindow->Flags() & B_NOT_H_RESIZABLE) != 0)
		return false;
	return true;
}


bool
SATWindow::IsVResizeable() const
{
	if (fWindow->Look() == B_MODAL_WINDOW_LOOK
		|| fWindow->Look() == B_BORDERED_WINDOW_LOOK
		|| fWindow->Look() == B_NO_BORDER_WINDOW_LOOK
		|| (fWindow->Flags() & B_NOT_RESIZABLE) != 0
		|| (fWindow->Flags() & B_NOT_V_RESIZABLE) != 0)
		return false;
	return true;
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

	AddDecorator(frame);
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
	SATDecorator* decorator = GetDecorator();
	if (!decorator)
		return false;

	int32 tabIndex = fWindow->PositionInStack();
	BRegion dirty;
	uint8 highlight = active ?  SATDecorator::HIGHLIGHT_STACK_AND_TILE : 0;
	decorator->SetRegionHighlight(Decorator::REGION_TAB, highlight, &dirty,
		tabIndex);
	decorator->SetRegionHighlight(Decorator::REGION_CLOSE_BUTTON, highlight,
		&dirty, tabIndex);
	decorator->SetRegionHighlight(Decorator::REGION_ZOOM_BUTTON, highlight,
		&dirty, tabIndex);

	fWindow->TopLayerStackWindow()->ProcessDirtyRegion(dirty);
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
	if (IsHResizeable() == false)
		x = fOriginalWidth - frame.Width();
	if (IsVResizeable() == false)
		y = fOriginalHeight - frame.Height();
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
	BRect tabRect = decorator->TitleBarRect();
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
