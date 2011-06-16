/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2009, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "SourceView.h"

#include <algorithm>
#include <new>

#include <ctype.h>
#include <stdio.h>

#include <Clipboard.h>
#include <LayoutUtils.h>
#include <Looper.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Polygon.h>
#include <Region.h>
#include <ScrollBar.h>
#include <ScrollView.h>

#include <AutoLocker.h>
#include <ObjectList.h>

#include "Breakpoint.h"
#include "DisassembledCode.h"
#include "Function.h"
#include "FileSourceCode.h"
#include "LocatableFile.h"
#include "MessageCodes.h"
#include "SourceLanguage.h"
#include "StackTrace.h"
#include "Statement.h"
#include "Team.h"
#include "Tracing.h"


static const int32 kLeftTextMargin = 3;
static const float kMinViewHeight = 80.0f;
static const int32 kSpacesPerTab = 4;
	// TODO: Should be settable!
static const bigtime_t kScrollTimer = 10000LL;


class SourceView::BaseView : public BView {
public:
								BaseView(const char* name,
									SourceView* sourceView, FontInfo* fontInfo);

	virtual	void				SetSourceCode(SourceCode* sourceCode);

	virtual	BSize				PreferredSize();

protected:
	inline	int32				LineCount() const;

	inline	float				TotalHeight() const;

			int32				LineAtOffset(float yOffset) const;
			void				GetLineRange(BRect rect, int32& minLine,
									int32& maxLine) const;
			BRect				LineRect(uint32 line) const;

protected:
			SourceView*			fSourceView;
			FontInfo*			fFontInfo;
			SourceCode*			fSourceCode;
};


class SourceView::MarkerManager {
public:
								MarkerManager(SourceView* sourceView,
									Team* team, Listener* listener);

			void				SetSourceCode(SourceCode* sourceCode);

			void				SetStackTrace(StackTrace* stackTrace);
			void				SetStackFrame(StackFrame* stackFrame);

			void				UserBreakpointChanged(
									UserBreakpoint* breakpoint);

			struct Marker;
			struct InstructionPointerMarker;
			struct BreakpointMarker;

			template<typename MarkerType> struct MarkerByLinePredicate;

			typedef BObjectList<Marker>	MarkerList;
			typedef BObjectList<BreakpointMarker> BreakpointMarkerList;

			void				GetMarkers(uint32 minLine, uint32 maxLine,
									MarkerList& markers);
			BreakpointMarker*	BreakpointMarkerAtLine(uint32 line);

private:
			void				_InvalidateIPMarkers();
			void				_InvalidateBreakpointMarkers();
			void				_UpdateIPMarkers();
			void				_UpdateBreakpointMarkers();

// TODO: "public" to workaround a GCC2 problem:
public:
	static	int					_CompareMarkers(const Marker* a,
									const Marker* b);
	static	int					_CompareBreakpointMarkers(
									const BreakpointMarker* a,
									const BreakpointMarker* b);

	template<typename MarkerType>
	static	int					_CompareLineMarkerTemplate(const uint32* line,
									const MarkerType* marker);
	static	int					_CompareLineMarker(const uint32* line,
									const Marker* marker);
	static	int					_CompareLineBreakpointMarker(
									const uint32* line,
									const BreakpointMarker* marker);

private:
			Team*				fTeam;
			Listener*			fListener;
			SourceCode*			fSourceCode;
			StackTrace*			fStackTrace;
			StackFrame*			fStackFrame;
			MarkerList			fIPMarkers;
			BreakpointMarkerList fBreakpointMarkers;
			bool				fIPMarkersValid;
			bool				fBreakpointMarkersValid;

};


class SourceView::MarkerView : public BaseView {
public:
								MarkerView(SourceView* sourceView, Team* team,
									Listener* listener, MarkerManager *manager,
									FontInfo* fontInfo);
								~MarkerView();

	virtual	void				SetSourceCode(SourceCode* sourceCode);

			void				SetStackTrace(StackTrace* stackTrace);
			void				SetStackFrame(StackFrame* stackFrame);

			void				UserBreakpointChanged(
									UserBreakpoint* breakpoint);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);


private:
			Team*				fTeam;
			Listener*			fListener;
			MarkerManager*		fMarkerManager;
			StackTrace*			fStackTrace;
			StackFrame*			fStackFrame;
			rgb_color			fBreakpointOptionMarker;
};


struct SourceView::MarkerManager::Marker {
								Marker(uint32 line);
	virtual						~Marker();

	inline	uint32				Line() const;

	virtual	void				Draw(BView* view, BRect rect) = 0;

private:
	uint32	fLine;
};


struct SourceView::MarkerManager::InstructionPointerMarker : Marker {
								InstructionPointerMarker(uint32 line,
									bool topIP, bool currentIP);

	virtual	void				Draw(BView* view, BRect rect);

private:
			void				_DrawArrow(BView* view, BPoint tip, BSize size,
									BSize base, const rgb_color& color,
									bool fill);

private:
			bool				fIsTopIP;
			bool				fIsCurrentIP;
};


struct SourceView::MarkerManager::BreakpointMarker : Marker {
								BreakpointMarker(uint32 line,
									target_addr_t address, bool enabled);

			target_addr_t		Address() const		{ return fAddress; }
			bool				IsEnabled() const	{ return fEnabled; }

	virtual	void				Draw(BView* view, BRect rect);

private:
			target_addr_t		fAddress;
			bool				fEnabled;
};


template<typename MarkerType>
struct SourceView::MarkerManager::MarkerByLinePredicate
	: UnaryPredicate<MarkerType> {
	MarkerByLinePredicate(uint32 line)
		:
		fLine(line)
	{
	}

	virtual int operator()(const MarkerType* marker) const
	{
		return -_CompareLineMarkerTemplate<MarkerType>(&fLine, marker);
	}

private:
	uint32	fLine;
};


class SourceView::TextView : public BaseView {
public:
								TextView(SourceView* sourceView,
									MarkerManager* manager,
									FontInfo* fontInfo);

	virtual	void				SetSourceCode(SourceCode* sourceCode);
			void				UserBreakpointChanged(
									UserBreakpoint* breakpoint);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

	virtual	void				Draw(BRect updateRect);

	virtual void				KeyDown(const char* bytes, int32 numBytes);
	virtual void				MakeFocus(bool isFocused);
	virtual void				MessageReceived(BMessage* message);
	virtual void				MouseDown(BPoint where);
	virtual void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual void				MouseUp(BPoint where);

private:
			struct SelectionPoint
			{
				SelectionPoint(int32 _line, int32 _offset)
				{
					line = _line;
					offset = _offset;
				}

				bool operator==(const SelectionPoint& other)
				{
					return line == other.line && offset == other.offset;
				}

				int32 line;
				int32 offset;
			};

			enum TrackingState
			{
				kNotTracking = 0,
				kTracking = 1,
				kDragging = 2
			};

			float				_MaxLineWidth();
			void				_FormatLine(const char* line,
									BString& formattedLine);
	inline 	int32				_NextTabStop(int32 column) const;
			float				_FormattedPosition(int32 line,
									int32 offset) const;
			SelectionPoint		_SelectionPointAt(BPoint where) const;
			void				_GetSelectionRegion(BRegion& region) const;
			void				_GetSelectionText(BString& text) const;
			void				_CopySelectionToClipboard() const;
			void				_SelectWordAt(const SelectionPoint& point,
									bool extend = false);
			void				_SelectLineAt(const SelectionPoint& point,
									bool extend = false);
			void				_HandleAutoScroll();
			void				_ScrollHorizontal(int32 charCount);
			void				_ScrollByLines(int32 lineCount);
			void				_ScrollByPages(int32 pageCount);
			void				_ScrollToTop();
			void				_ScrollToBottom();

private:

			float				fMaxLineWidth;
			float				fCharacterWidth;
			SelectionPoint		fSelectionStart;
			SelectionPoint		fSelectionEnd;
			SelectionPoint		fSelectionBase;
			SelectionPoint		fLastClickPoint;
			bigtime_t			fLastClickTime;
			int16				fClickCount;
			rgb_color			fTextColor;
			bool				fSelectionMode;
			TrackingState		fTrackState;
			BMessageRunner*		fScrollRunner;
			MarkerManager*		fMarkerManager;
};


// #pragma mark - BaseView


SourceView::BaseView::BaseView(const char* name, SourceView* sourceView,
	FontInfo* fontInfo)
	:
	BView(name, B_WILL_DRAW | B_SUBPIXEL_PRECISE),
	fSourceView(sourceView),
	fFontInfo(fontInfo),
	fSourceCode(NULL)
{
}


void
SourceView::BaseView::SetSourceCode(SourceCode* sourceCode)
{
	fSourceCode = sourceCode;

	InvalidateLayout();
	Invalidate();
}


BSize
SourceView::BaseView::PreferredSize()
{
	return MinSize();
}


int32
SourceView::BaseView::LineCount() const
{
	return fSourceCode != NULL ? fSourceCode->CountLines() : 0;
}


float
SourceView::BaseView::TotalHeight() const
{
	float height = LineCount() * fFontInfo->lineHeight - 1;
	return std::max(height, kMinViewHeight);
}


int32
SourceView::BaseView::LineAtOffset(float yOffset) const
{
	int32 lineCount = LineCount();
	if (yOffset < 0 || lineCount == 0)
		return -1;

	int32 line = (int32)yOffset / (int32)fFontInfo->lineHeight;
	return line < lineCount ? line : -1;
}


void
SourceView::BaseView::GetLineRange(BRect rect, int32& minLine,
	int32& maxLine) const
{
	int32 lineHeight = (int32)fFontInfo->lineHeight;
	minLine = (int32)rect.top / lineHeight;
	maxLine = ((int32)ceilf(rect.bottom) + lineHeight - 1) / lineHeight;
	minLine = std::max(minLine, 0L);
	maxLine = std::min(maxLine, fSourceCode->CountLines() - 1);
}


BRect
SourceView::BaseView::LineRect(uint32 line) const
{
	float y = (float)line * fFontInfo->lineHeight;
	return BRect(0, y, Bounds().right, y + fFontInfo->lineHeight - 1);
}


// #pragma mark - MarkerView::Marker


SourceView::MarkerManager::Marker::Marker(uint32 line)
	:
	fLine(line)
{
}


SourceView::MarkerManager::Marker::~Marker()
{
}


uint32
SourceView::MarkerManager::Marker::Line() const
{
	return fLine;
}


// #pragma mark - MarkerManager::InstructionPointerMarker


SourceView::MarkerManager::InstructionPointerMarker::InstructionPointerMarker(
	uint32 line, bool topIP, bool currentIP)
	:
	Marker(line),
	fIsTopIP(topIP),
	fIsCurrentIP(currentIP)
{
}


void
SourceView::MarkerManager::InstructionPointerMarker::Draw(BView* view,
	BRect rect)
{
	// Get the arrow color -- for the top IP, if current, we use blue,
	// otherwise a gray.
	rgb_color color;
	if (fIsCurrentIP && fIsTopIP) {
		color.set_to(0, 0, 255, 255);
	} else {
		color = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
			B_DARKEN_3_TINT);
	}

	// Draw a filled array for the current IP, otherwise just an
	// outline.
	BPoint tip(rect.right - 3.5f, floorf((rect.top + rect.bottom) / 2));
	if (fIsCurrentIP) {
		_DrawArrow(view, tip, BSize(10, 10), BSize(5, 5), color, true);
	} else {
		_DrawArrow(view, tip + BPoint(-0.5f, 0), BSize(9, 8),
			BSize(5, 4), color, false);
	}
}


void
SourceView::MarkerManager::InstructionPointerMarker::_DrawArrow(BView* view,
	BPoint tip, BSize size, BSize base, const rgb_color& color, bool fill)
{
	view->SetHighColor(color);

	float baseTop = tip.y - base.height / 2;
	float baseBottom = tip.y + base.height / 2;
	float top = tip.y - size.height / 2;
	float bottom = tip.y + size.height / 2;
	float left = tip.x - size.width;
	float middle = left + base.width;

	BPoint points[7];
	points[0].Set(tip.x, tip.y);
	points[1].Set(middle, top);
	points[2].Set(middle, baseTop);
	points[3].Set(left, baseTop);
	points[4].Set(left, baseBottom);
	points[5].Set(middle, baseBottom);
	points[6].Set(middle, bottom);

	if (fill)
		view->FillPolygon(points, 7);
	else
		view->StrokePolygon(points, 7);
}


// #pragma mark - MarkerManager::BreakpointMarker


SourceView::MarkerManager::BreakpointMarker::BreakpointMarker(uint32 line,
	target_addr_t address, bool enabled)
	:
	Marker(line),
	fAddress(address),
	fEnabled(enabled)
{
}


void
SourceView::MarkerManager::BreakpointMarker::Draw(BView* view, BRect rect)
{
	float y = (rect.top + rect.bottom) / 2;
	view->SetHighColor((rgb_color){255, 0, 0, 255});
	if (fEnabled)
		view->FillEllipse(BPoint(rect.right - 8, y), 4, 4);
	else
		view->StrokeEllipse(BPoint(rect.right - 8, y), 3.5f, 3.5f);
}


// #pragma mark - MarkerManager


SourceView::MarkerManager::MarkerManager(SourceView* sourceView, Team* team,
	Listener* listener)
	:
	fTeam(team),
	fListener(listener),
	fStackTrace(NULL),
	fStackFrame(NULL),
	fIPMarkers(10, true),
	fBreakpointMarkers(20, true),
	fIPMarkersValid(false),
	fBreakpointMarkersValid(false)
{
}


void
SourceView::MarkerManager::SetSourceCode(SourceCode* sourceCode)
{
	fSourceCode = sourceCode;
	_InvalidateIPMarkers();
	_InvalidateBreakpointMarkers();
}


void
SourceView::MarkerManager::SetStackTrace(StackTrace* stackTrace)
{
	fStackTrace = stackTrace;
	_InvalidateIPMarkers();
}


void
SourceView::MarkerManager::SetStackFrame(StackFrame* stackFrame)
{
	fStackFrame = stackFrame;
	_InvalidateIPMarkers();
}


void
SourceView::MarkerManager::UserBreakpointChanged(UserBreakpoint* breakpoint)
{
	_InvalidateBreakpointMarkers();
}


void
SourceView::MarkerManager::_InvalidateIPMarkers()
{
	fIPMarkersValid = false;
	fIPMarkers.MakeEmpty();
}


void
SourceView::MarkerManager::_InvalidateBreakpointMarkers()
{
	fBreakpointMarkersValid = false;
	fBreakpointMarkers.MakeEmpty();
}


void
SourceView::MarkerManager::_UpdateIPMarkers()
{
	if (fIPMarkersValid)
		return;

	fIPMarkers.MakeEmpty();

	if (fSourceCode != NULL && fStackTrace != NULL) {
		LocatableFile* sourceFile = fSourceCode->GetSourceFile();

		AutoLocker<Team> locker(fTeam);

		for (int32 i = 0; StackFrame* frame = fStackTrace->FrameAt(i);
				i++) {
			target_addr_t ip = frame->InstructionPointer();
			FunctionInstance* functionInstance;
			Statement* statement;
			if (fTeam->GetStatementAtAddress(ip,
					functionInstance, statement) != B_OK) {
				continue;
			}
			BReference<Statement> statementReference(statement, true);

			int32 line = statement->StartSourceLocation().Line();
			if (line < 0 || line >= fSourceCode->CountLines())
				continue;

			if (sourceFile != NULL) {
				if (functionInstance->GetFunction()->SourceFile() != sourceFile)
					continue;
			} else {
				if (functionInstance->GetSourceCode() != fSourceCode)
					continue;
			}

			bool isTopFrame = i == 0
				&& frame->Type() != STACK_FRAME_TYPE_SYSCALL;

			Marker* marker = new(std::nothrow) InstructionPointerMarker(
				line, isTopFrame, frame == fStackFrame);
			if (marker == NULL || !fIPMarkers.AddItem(marker)) {
				delete marker;
				break;
			}
		}

		// sort by line
		fIPMarkers.SortItems(&_CompareMarkers);

		// TODO: Filter duplicate IP markers (recursive functions)!
	}

	fIPMarkersValid = true;
}


void
SourceView::MarkerManager::_UpdateBreakpointMarkers()
{
	if (fBreakpointMarkersValid)
		return;

	fBreakpointMarkers.MakeEmpty();

	if (fSourceCode != NULL) {
		LocatableFile* sourceFile = fSourceCode->GetSourceFile();

		AutoLocker<Team> locker(fTeam);

		// get the breakpoints in our source code range
		BObjectList<UserBreakpoint> breakpoints;
		fTeam->GetBreakpointsForSourceCode(fSourceCode, breakpoints);

		for (int32 i = 0; UserBreakpoint* breakpoint = breakpoints.ItemAt(i);
				i++) {
			UserBreakpointInstance* breakpointInstance
				= breakpoint->InstanceAt(0);
			FunctionInstance* functionInstance;
			Statement* statement;
			if (fTeam->GetStatementAtAddress(
					breakpointInstance->Address(), functionInstance,
					statement) != B_OK) {
				continue;
			}
			BReference<Statement> statementReference(statement, true);

			int32 line = statement->StartSourceLocation().Line();
			if (line < 0 || line >= fSourceCode->CountLines())
				continue;

			if (sourceFile != NULL) {
				if (functionInstance->GetFunction()->SourceFile() != sourceFile)
					continue;
			} else {
				if (functionInstance->GetSourceCode() != fSourceCode)
					continue;
			}

			BreakpointMarker* marker = new(std::nothrow) BreakpointMarker(
				line, breakpointInstance->Address(), breakpoint->IsEnabled());
			if (marker == NULL || !fBreakpointMarkers.AddItem(marker)) {
				delete marker;
				break;
			}
		}

		// sort by line
		fBreakpointMarkers.SortItems(&_CompareBreakpointMarkers);
	}

	fBreakpointMarkersValid = true;
}


void
SourceView::MarkerManager::GetMarkers(uint32 minLine, uint32 maxLine,
	MarkerList& markers)
{
	_UpdateIPMarkers();
	_UpdateBreakpointMarkers();

	int32 ipIndex = fIPMarkers.FindBinaryInsertionIndex(
		MarkerByLinePredicate<Marker>(minLine));
	int32 breakpointIndex = fBreakpointMarkers.FindBinaryInsertionIndex(
		MarkerByLinePredicate<BreakpointMarker>(minLine));

	Marker* ipMarker = fIPMarkers.ItemAt(ipIndex);
	Marker* breakpointMarker = fBreakpointMarkers.ItemAt(breakpointIndex);

	while (ipMarker != NULL && breakpointMarker != NULL
		&& ipMarker->Line() <= maxLine && breakpointMarker->Line() <= maxLine) {
		if (breakpointMarker->Line() <= ipMarker->Line()) {
			markers.AddItem(breakpointMarker);
			breakpointMarker = fBreakpointMarkers.ItemAt(++breakpointIndex);
		} else {
			markers.AddItem(ipMarker);
			ipMarker = fIPMarkers.ItemAt(++ipIndex);
		}
	}

	while (breakpointMarker != NULL && breakpointMarker->Line() <= maxLine) {
		markers.AddItem(breakpointMarker);
		breakpointMarker = fBreakpointMarkers.ItemAt(++breakpointIndex);
	}

	while (ipMarker != NULL && ipMarker->Line() <= maxLine) {
		markers.AddItem(ipMarker);
		ipMarker = fIPMarkers.ItemAt(++ipIndex);
	}
}


SourceView::MarkerManager::BreakpointMarker*
SourceView::MarkerManager::BreakpointMarkerAtLine(uint32 line)
{
	return fBreakpointMarkers.BinarySearchByKey(line,
		&_CompareLineBreakpointMarker);
}


/*static*/ int
SourceView::MarkerManager::_CompareMarkers(const Marker* a,
	const Marker* b)
{
	if (a->Line() < b->Line())
		return -1;
	return a->Line() == b->Line() ? 0 : 1;
}


/*static*/ int
SourceView::MarkerManager::_CompareBreakpointMarkers(const BreakpointMarker* a,
	const BreakpointMarker* b)
{
	if (a->Line() < b->Line())
		return -1;
	return a->Line() == b->Line() ? 0 : 1;
}


template<typename MarkerType>
/*static*/ int
SourceView::MarkerManager::_CompareLineMarkerTemplate(const uint32* line,
	const MarkerType* marker)
{
	if (*line < marker->Line())
		return -1;
	return *line == marker->Line() ? 0 : 1;
}


/*static*/ int
SourceView::MarkerManager::_CompareLineMarker(const uint32* line,
	const Marker* marker)
{
	return _CompareLineMarkerTemplate<Marker>(line, marker);
}


/*static*/ int
SourceView::MarkerManager::_CompareLineBreakpointMarker(const uint32* line,
	const BreakpointMarker* marker)
{
	return _CompareLineMarkerTemplate<BreakpointMarker>(line, marker);
}


// #pragma mark - MarkerView


SourceView::MarkerView::MarkerView(SourceView* sourceView, Team* team,
	Listener* listener, MarkerManager* manager, FontInfo* fontInfo)
	:
	BaseView("source marker view", sourceView, fontInfo),
	fTeam(team),
	fListener(listener),
	fMarkerManager(manager),
	fStackTrace(NULL),
	fStackFrame(NULL)
{
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	fBreakpointOptionMarker = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_1_TINT);
	SetViewColor(tint_color(background, B_LIGHTEN_2_TINT));
}


SourceView::MarkerView::~MarkerView()
{
}


void
SourceView::MarkerView::SetSourceCode(SourceCode* sourceCode)
{
	BaseView::SetSourceCode(sourceCode);
}


void
SourceView::MarkerView::SetStackTrace(StackTrace* stackTrace)
{
	Invalidate();
}


void
SourceView::MarkerView::SetStackFrame(StackFrame* stackFrame)
{
	Invalidate();
}


void
SourceView::MarkerView::UserBreakpointChanged(UserBreakpoint* breakpoint)
{
	Invalidate();
}


BSize
SourceView::MarkerView::MinSize()
{
	return BSize(40, TotalHeight());
}


BSize
SourceView::MarkerView::MaxSize()
{
	return BSize(MinSize().width, B_SIZE_UNLIMITED);
}

void
SourceView::MarkerView::Draw(BRect updateRect)
{
	if (fSourceCode == NULL)
		return;

	// get the lines intersecting with the update rect
	int32 minLine, maxLine;
	GetLineRange(updateRect, minLine, maxLine);
	if (minLine > maxLine)
		return;

	// get the markers in that range
	SourceView::MarkerManager::MarkerList markers;
	fMarkerManager->GetMarkers(minLine, maxLine, markers);

	float width = Bounds().Width();

	AutoLocker<SourceCode> sourceLocker(fSourceCode);

	int32 markerIndex = 0;
	for (int32 line = minLine; line <= maxLine; line++) {
		bool drawBreakpointOptionMarker = true;

		SourceView::MarkerManager::Marker* marker;
		while ((marker = markers.ItemAt(markerIndex)) != NULL
				&& marker->Line() == (uint32)line) {
			marker->Draw(this, LineRect(line));
			drawBreakpointOptionMarker = false;
			markerIndex++;
		}

		if (!drawBreakpointOptionMarker)
			continue;

		SourceLocation statementStart, statementEnd;
		if (!fSourceCode->GetStatementLocationRange(SourceLocation(line),
				statementStart, statementEnd)
			|| statementStart.Line() != line) {
			continue;
		}

		float y = ((float)line + 0.5f) * fFontInfo->lineHeight;
		SetHighColor(fBreakpointOptionMarker);
		FillEllipse(BPoint(width - 8, y), 2, 2);
	}
}


void
SourceView::MarkerView::MouseDown(BPoint where)
{
	if (fSourceCode == NULL)
		return;

	int32 line = LineAtOffset(where.y);
	if (line < 0)
		return;

	AutoLocker<Team> locker(fTeam);
	Statement* statement;
	if (fTeam->GetStatementAtSourceLocation(fSourceCode,
			SourceLocation(line), statement) != B_OK) {
		return;
	}
	BReference<Statement> statementReference(statement, true);
	if (statement->StartSourceLocation().Line() != line)
		return;

	int32 modifiers;
	if (Looper()->CurrentMessage()->FindInt32("modifiers", &modifiers) != B_OK)
		modifiers = 0;

	SourceView::MarkerManager::BreakpointMarker* marker =
		fMarkerManager->BreakpointMarkerAtLine(line);
	target_addr_t address = marker != NULL
		? marker->Address() : statement->CoveringAddressRange().Start();

	if ((modifiers & B_SHIFT_KEY) != 0) {
		if (marker != NULL && !marker->IsEnabled())
			fListener->ClearBreakpointRequested(address);
		else
			fListener->SetBreakpointRequested(address, false);
	} else {
		if (marker != NULL && marker->IsEnabled())
			fListener->ClearBreakpointRequested(address);
		else
			fListener->SetBreakpointRequested(address, true);
	}
}


// #pragma mark - TextView


SourceView::TextView::TextView(SourceView* sourceView, MarkerManager* manager,
	FontInfo* fontInfo)
	:
	BaseView("source text view", sourceView, fontInfo),
	fMaxLineWidth(-1),
	fCharacterWidth(fontInfo->font.StringWidth("Q")),
	fSelectionStart(-1, -1),
	fSelectionEnd(-1, -1),
	fSelectionBase(-1, -1),
	fLastClickPoint(-1, -1),
	fLastClickTime(0),
	fClickCount(0),
	fSelectionMode(false),
	fTrackState(kNotTracking),
	fScrollRunner(NULL),
	fMarkerManager(manager)
{
	SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
	fTextColor = ui_color(B_DOCUMENT_TEXT_COLOR);
	SetFlags(Flags() | B_NAVIGABLE);
}


void
SourceView::TextView::SetSourceCode(SourceCode* sourceCode)
{
	fMaxLineWidth = -1;
	fSelectionStart = fSelectionBase = fSelectionEnd = SelectionPoint(-1, -1);
	fClickCount = 0;
	BaseView::SetSourceCode(sourceCode);
}


void
SourceView::TextView::UserBreakpointChanged(UserBreakpoint* breakpoint)
{
	Invalidate();
}


BSize
SourceView::TextView::MinSize()
{
	return BSize(kLeftTextMargin + _MaxLineWidth() - 1, TotalHeight());
}


BSize
SourceView::TextView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


void
SourceView::TextView::Draw(BRect updateRect)
{
	if (fSourceCode == NULL)
		return;

	// get the lines intersecting with the update rect
	int32 minLine, maxLine;
	GetLineRange(updateRect, minLine, maxLine);
	SourceView::MarkerManager::MarkerList markers;
	fMarkerManager->GetMarkers(minLine, maxLine, markers);

	// draw the affected lines
	SetHighColor(fTextColor);
	SetFont(&fFontInfo->font);
	SourceView::MarkerManager::Marker* marker;
	int32 markerIndex = 0;
	for (int32 i = minLine; i <= maxLine; i++) {
		SetLowColor(ViewColor());
		float y = i * fFontInfo->lineHeight;
		BString lineString;
		_FormatLine(fSourceCode->LineAt(i), lineString);

		for (int32 j = markerIndex; j < markers.CountItems(); j++) {
			marker = markers.ItemAt(j);
			 if (marker->Line() < (uint32)i) {
				++markerIndex;
			 	continue;
			 } else if (marker->Line() == (uint32)i) {
			 	++markerIndex;
			 	if (dynamic_cast<SourceView::MarkerManager
			 		::InstructionPointerMarker*>(marker) != NULL)
			 		SetLowColor(96, 216, 216, 255);
			 	else
					SetLowColor(255, 255, 0, 255);
				FillRect(BRect(kLeftTextMargin, y, Bounds().right,
					y + fFontInfo->lineHeight), B_SOLID_LOW);
				break;
			 } else
			 	break;
		}

		DrawString(lineString,
			BPoint(kLeftTextMargin, y + fFontInfo->fontHeight.ascent));
	}

	if (fSelectionStart.line != -1 && fSelectionEnd.line != -1) {
		PushState();
		BRegion selectionRegion;
		_GetSelectionRegion(selectionRegion);
		SetDrawingMode(B_OP_INVERT);
		FillRegion(&selectionRegion, B_SOLID_HIGH);
		PopState();
	}
}


void
SourceView::TextView::KeyDown(const char* bytes, int32 numBytes)
{
	switch(bytes[0]) {
		case B_UP_ARROW:
			_ScrollByLines(-1);
			break;

		case B_DOWN_ARROW:
			_ScrollByLines(1);
			break;

		case B_PAGE_UP:
			_ScrollByPages(-1);
			break;

		case B_PAGE_DOWN:
			_ScrollByPages(1);
			break;

		case B_HOME:
			_ScrollToTop();
			break;

		case B_END:
			_ScrollToBottom();
			break;
	}

	SourceView::BaseView::KeyDown(bytes, numBytes);
}


void
SourceView::TextView::MakeFocus(bool isFocused)
{
	fSourceView->HighlightBorder(isFocused);

	SourceView::BaseView::MakeFocus(isFocused);
}


void
SourceView::TextView::MessageReceived(BMessage* message)
{
	switch (message->what)
	{
		case B_COPY:
			_CopySelectionToClipboard();
			break;

		case B_SELECT_ALL:
			fSelectionStart.line = 0;
			fSelectionStart.offset = 0;
			fSelectionEnd.line = fSourceCode->CountLines() - 1;
			fSelectionEnd.offset = fSourceCode->LineLengthAt(
				fSelectionEnd.line);
			Invalidate();
			break;

		case MSG_TEXTVIEW_AUTOSCROLL:
			_HandleAutoScroll();
			break;

		default:
			SourceView::BaseView::MessageReceived(message);
			break;
	}
}


void
SourceView::TextView::MouseDown(BPoint where)
{
	if (fSourceCode != NULL) {
		if (!IsFocus())
			MakeFocus(true);
		fTrackState = kTracking;

		// don't reset the selection if the user clicks within the
		// current selection range
		BRegion region;
		_GetSelectionRegion(region);
		bigtime_t clickTime = system_time();
		SelectionPoint point = _SelectionPointAt(where);
		fLastClickPoint = point;
		bigtime_t clickSpeed = 0;
		get_click_speed(&clickSpeed);
		if (clickTime - fLastClickTime < clickSpeed
				&& fSelectionBase == point) {
			if (fClickCount > 3) {
				fClickCount = 0;
				fLastClickTime = 0;
			} else {
				fClickCount++;
				fLastClickTime = clickTime;
			}
		} else {
			fClickCount = 1;
			fLastClickTime = clickTime;
		}

		if (fClickCount == 2) {
			_SelectWordAt(point);
			fSelectionMode = true;
		} else if (fClickCount == 3) {
			_SelectLineAt(point);
			fSelectionMode = true;
		} else if (!region.Contains(where)) {
			fSelectionBase = fSelectionStart = fSelectionEnd = point;
			fSelectionMode = true;
			Invalidate();
			SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
		}
	}
}


void
SourceView::TextView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	BRegion region;
	if (fSelectionMode) {
		BRegion oldRegion;
		_GetSelectionRegion(oldRegion);
		SelectionPoint point = _SelectionPointAt(where);
		if (point.line < 0)
			return;

		switch (transit) {
			case B_INSIDE_VIEW:
			case B_OUTSIDE_VIEW:
				if (fClickCount == 2)
					_SelectWordAt(point, true);
				else if (fClickCount == 3)
					_SelectLineAt(point, true);
				else {
					if (point.line > fSelectionBase.line) {
						fSelectionStart = fSelectionBase;
						fSelectionEnd = point;
					} else if (point.line < fSelectionBase.line) {
						fSelectionEnd = fSelectionBase;
						fSelectionStart = point;
					} else if (point.offset > fSelectionBase.offset) {
						fSelectionStart = fSelectionBase;
						fSelectionEnd = point;
					} else {
						fSelectionEnd = fSelectionBase;
						fSelectionStart = point;
					}
				}
				break;

			case B_EXITED_VIEW:
				fScrollRunner = new BMessageRunner(BMessenger(this),
					new BMessage(MSG_TEXTVIEW_AUTOSCROLL), kScrollTimer);
				break;

			case B_ENTERED_VIEW:
				delete fScrollRunner;
				fScrollRunner = NULL;
				break;
		}
		_GetSelectionRegion(region);
		region.Include(&oldRegion);
		Invalidate(&region);
	} else if (fTrackState == kTracking) {
		_GetSelectionRegion(region);
		if (region.CountRects() > 0) {
			BString text;
			_GetSelectionText(text);
			BMessage message;
			message.AddData ("text/plain", B_MIME_TYPE, text.String(),
				text.Length());
			BString clipName;
			if (fSourceCode->GetSourceFile() != NULL)
				clipName = fSourceCode->GetSourceFile()->Name();
			else if (fSourceCode->GetSourceLanguage() != NULL)
				clipName = fSourceCode->GetSourceLanguage()->Name();
			else
				clipName = "Text";
			clipName << " clipping";
			message.AddString ("be:clip_name", clipName.String());
			message.AddInt32 ("be:actions", B_COPY_TARGET);
			BRect dragRect = region.Frame();
			BRect visibleRect = fSourceView->Bounds();
			if (dragRect.Height() > visibleRect.Height()) {
				dragRect.top = 0;
				dragRect.bottom = visibleRect.Height();
			}
			if (dragRect.Width() > visibleRect.Width()) {
				dragRect.left = 0;
				dragRect.right = visibleRect.Width();
			}
			DragMessage(&message, dragRect);
			fTrackState = kDragging;
		}
	}
}


void
SourceView::TextView::MouseUp(BPoint where)
{
	fSelectionMode = false;
	if (fTrackState == kTracking && fClickCount < 2) {

		// if we clicked without dragging or double/triple clicking,
		// clear the current selection (if any)
		SelectionPoint point = _SelectionPointAt(where);
		if (fLastClickPoint == point) {
			fSelectionBase = fSelectionStart = fSelectionEnd;
			Invalidate();
		}
	}
	delete fScrollRunner;
	fScrollRunner = NULL;
	fTrackState = kNotTracking;
}


float
SourceView::TextView::_MaxLineWidth()
{
	if (fMaxLineWidth >= 0)
		return fMaxLineWidth;

	fMaxLineWidth = 0;
	if (fSourceCode != NULL) {
		for (int32 i = 0; const char* line = fSourceCode->LineAt(i); i++) {
			fMaxLineWidth = std::max(fMaxLineWidth,
				fFontInfo->font.StringWidth(line));
		}
	}

	return fMaxLineWidth;
}


void
SourceView::TextView::_FormatLine(const char* line, BString& formattedLine)
{
	int32 column = 0;
	for (; *line != '\0'; line++) {
		// TODO: That's probably not very efficient!
		if (*line == '\t') {
			int32 nextTabStop = _NextTabStop(column);
			for (; column < nextTabStop; column++)
				formattedLine << ' ';
		} else {
			formattedLine << *line;
			column++;
		}
	}
}


int32
SourceView::TextView::_NextTabStop(int32 column) const
{
	return (column / kSpacesPerTab + 1) * kSpacesPerTab;
}


float
SourceView::TextView::_FormattedPosition(int32 line, int32 offset) const
{
	int32 column = 0;
	for (int32 i = 0; i < offset; i++) {
		if (fSourceCode->LineAt(line)[i] == '\t')
			column = _NextTabStop(column);
		else
			++column;
	}

	return column * fCharacterWidth;
}


SourceView::TextView::SelectionPoint
SourceView::TextView::_SelectionPointAt(BPoint where) const
{
	int32 line = LineAtOffset(where.y);
	int32 offset = -1;
	if (line >= 0) {
		int32 column = 0;
		int32 lineLength = fSourceCode->LineLengthAt(line);
		const char* sourceLine = fSourceCode->LineAt(line);

		for (int32 i = 0; i < lineLength; i++) {
			if (sourceLine[i] == '\t')
				column = _NextTabStop(column);
			else
				++column;

			if (column * fCharacterWidth > where.x) {
				offset = i;
				break;
			}
		}

		if (offset < 0)
			offset = lineLength;
	}

	return SelectionPoint(line, offset);
}


void
SourceView::TextView::_GetSelectionRegion(BRegion &region) const
{
	if (fSelectionStart.line == -1 && fSelectionEnd.line == -1)
		return;

	BRect selectionRect;

	if (fSelectionStart.line == fSelectionEnd.line) {
		if (fSelectionStart.offset != fSelectionEnd.offset) {
			selectionRect.left = _FormattedPosition(fSelectionStart.line,
				fSelectionStart.offset);
			selectionRect.top = fSelectionStart.line * fFontInfo->lineHeight;
			selectionRect.right = _FormattedPosition(fSelectionEnd.line,
				fSelectionEnd.offset);
			selectionRect.bottom = selectionRect.top + fFontInfo->lineHeight;
			region.Include(selectionRect);
		}
	} else {
		// add rect for starting line
		selectionRect.left = _FormattedPosition(fSelectionStart.line,
			fSelectionStart.offset);
		selectionRect.top = fSelectionStart.line * fFontInfo->lineHeight;
		selectionRect.right = Bounds().right;
		selectionRect.bottom = selectionRect.top + fFontInfo->lineHeight;
		region.Include(selectionRect);

		// compute rect for all lines in middle of selection
		if (fSelectionEnd.line - fSelectionStart.line > 1) {
			selectionRect.left = 0.0;
			selectionRect.top = (fSelectionStart.line + 1)
				* fFontInfo->lineHeight;
			selectionRect.right = Bounds().right;
			selectionRect.bottom = fSelectionEnd.line * fFontInfo->lineHeight;
			region.Include(selectionRect);
		}

		// add rect for last line (if needed)
		if (fSelectionEnd.offset > 0) {
			selectionRect.left = 0.0;
			selectionRect.top = fSelectionEnd.line * fFontInfo->lineHeight;
			selectionRect.right = _FormattedPosition(fSelectionEnd.line,
				fSelectionEnd.offset);
			selectionRect.bottom = selectionRect.top + fFontInfo->lineHeight;
			region.Include(selectionRect);
		}
	}
	region.OffsetBy(kLeftTextMargin, 0.0);
}


void
SourceView::TextView::_GetSelectionText(BString& text) const
{
	if (fSelectionStart.line == -1 || fSelectionEnd.line == -1)
		return;

	if (fSelectionStart.line == fSelectionEnd.line) {
		text.SetTo(fSourceCode->LineAt(fSelectionStart.line)
			+ fSelectionStart.offset, fSelectionEnd.offset
			- fSelectionStart.offset);
	} else {
		text.SetTo(fSourceCode->LineAt(fSelectionStart.line)
			+ fSelectionStart.offset);
		text << "\n";
		for (int32 i = fSelectionStart.line + 1; i < fSelectionEnd.line; i++)
			text << fSourceCode->LineAt(i) << "\n";
		text.Append(fSourceCode->LineAt(fSelectionEnd.line),
			fSelectionEnd.offset);
	}
}


void
SourceView::TextView::_CopySelectionToClipboard(void) const
{
	BString text;
	_GetSelectionText(text);

	if (text.Length() > 0) {
		be_clipboard->Lock();
		be_clipboard->Data()->RemoveData("text/plain");
		be_clipboard->Data()->AddData ("text/plain",
			B_MIME_TYPE, text.String(), text.Length());
		be_clipboard->Commit();
		be_clipboard->Unlock();
	}
}


void
SourceView::TextView::_SelectWordAt(const SelectionPoint& point, bool extend)
{
	const char* line = fSourceCode->LineAt(point.line);
	int32 length = fSourceCode->LineLengthAt(point.line);
	int32 start = point.offset - 1;
	int32 end = point.offset + 1;
	while ((end) < length) {
		if (!isalpha(line[end]) && !isdigit(line[end]))
			break;
		++end;
	}
	while ((start - 1) >= 0) {
		if (!isalpha(line[start - 1]) && !isdigit(line[start - 1]))
			break;
		--start;
	}

	if (extend) {
		if (point.line >= fSelectionBase.line
			|| (point.line == fSelectionBase.line
				&& point.offset > fSelectionBase.offset)) {
			fSelectionStart.line = fSelectionBase.line;
			fSelectionStart.offset = fSelectionBase.offset;
			fSelectionEnd.line = point.line;
			fSelectionEnd.offset = end;
		} else if (point.line < fSelectionBase.line) {
			fSelectionStart.line = point.line;
			fSelectionStart.offset = start;
			fSelectionEnd.line = fSelectionBase.line;
			fSelectionEnd.offset = fSelectionBase.offset;
		} else if (point.line == fSelectionBase.line) {
			// if we hit here, our offset is before the actual start.
			fSelectionStart.line = point.line;
			fSelectionStart.offset = start;
			fSelectionEnd.line = point.line;
			fSelectionEnd.offset = fSelectionBase.offset;
		}
	} else {
		fSelectionBase.line = fSelectionStart.line = point.line;
		fSelectionBase.offset = fSelectionStart.offset = start;
		fSelectionEnd.line = point.line;
		fSelectionEnd.offset = end;
	}
	BRegion region;
	_GetSelectionRegion(region);
	Invalidate(&region);
}


void
SourceView::TextView::_SelectLineAt(const SelectionPoint& point, bool extend)
{
	if (extend) {
		if (point.line >= fSelectionBase.line) {
			fSelectionStart.line = fSelectionBase.line;
			fSelectionStart.offset = 0;
			fSelectionEnd.line = point.line;
			fSelectionEnd.offset = fSourceCode->LineLengthAt(point.line);
		} else {
			fSelectionStart.line = point.line;
			fSelectionStart.offset = 0;
			fSelectionEnd.line = fSelectionBase.line;
			fSelectionEnd.offset = fSourceCode->LineLengthAt(
				fSelectionBase.line);
		}
	} else {
		fSelectionStart.line = fSelectionEnd.line = point.line;
		fSelectionStart.offset = 0;
		fSelectionEnd.offset = fSourceCode->LineLengthAt(point.line);
	}
	BRegion region;
	_GetSelectionRegion(region);
	Invalidate(&region);
}


void
SourceView::TextView::_HandleAutoScroll(void)
{
	BPoint point;
	uint32 buttons;
	GetMouse(&point, &buttons);
	float difference = 0.0;
	int factor = 0;
	BRect visibleRect = Frame() & fSourceView->Bounds();
	if (point.y < visibleRect.top)
		difference = point.y - visibleRect.top;
	else if (point.y > visibleRect.bottom)
		difference = point.y - visibleRect.bottom;
	if (difference != 0.0) {
		factor = (int)(ceilf(difference / fFontInfo->lineHeight));
		_ScrollByLines(factor);
	}
	difference = 0.0;
	if (point.x < visibleRect.left)
		difference = point.x - visibleRect.left;
	else if (point.x > visibleRect.right)
		difference = point.x - visibleRect.right;
	if (difference != 0.0) {
		factor = (int)(ceilf(difference / fCharacterWidth));
		_ScrollHorizontal(factor);
	}

	MouseMoved(point, B_OUTSIDE_VIEW, NULL);
}


void
SourceView::TextView::_ScrollHorizontal(int32 charCount)
{
	BScrollBar* horizontal = fSourceView->ScrollBar(B_HORIZONTAL);
	if (horizontal == NULL)
		return;

	float value = horizontal->Value();
	horizontal->SetValue(value + fCharacterWidth * charCount);
}


void
SourceView::TextView::_ScrollByLines(int32 lineCount)
{
	BScrollBar* vertical = fSourceView->ScrollBar(B_VERTICAL);
	if (vertical == NULL)
		return;

	float value = vertical->Value();
	vertical->SetValue(value + fFontInfo->lineHeight * lineCount);
}


void
SourceView::TextView::_ScrollByPages(int32 pageCount)
{
	BScrollBar* vertical = fSourceView->ScrollBar(B_VERTICAL);
	if (vertical == NULL)
		return;

	float value = vertical->Value();
	vertical->SetValue(value
		+ fSourceView->Frame().Size().height * pageCount);
}


void
SourceView::TextView::_ScrollToTop(void)
{
	BScrollBar* vertical = fSourceView->ScrollBar(B_VERTICAL);
	if (vertical == NULL)
		return;

	vertical->SetValue(0.0);
}


void
SourceView::TextView::_ScrollToBottom(void)
{
	BScrollBar* vertical = fSourceView->ScrollBar(B_VERTICAL);
	if (vertical == NULL)
		return;

	float min, max;
	vertical->GetRange(&min, &max);
	vertical->SetValue(max);
}


// #pragma mark - SourceView


SourceView::SourceView(Team* team, Listener* listener)
	:
	BView("source view", 0),
	fTeam(team),
	fStackTrace(NULL),
	fStackFrame(NULL),
	fSourceCode(NULL),
	fMarkerView(NULL),
	fTextView(NULL),
	fListener(listener)
{
	// init font info
	fFontInfo.font = *be_fixed_font;
	fFontInfo.font.GetHeight(&fFontInfo.fontHeight);
	fFontInfo.lineHeight = ceilf(fFontInfo.fontHeight.ascent)
		+ ceilf(fFontInfo.fontHeight.descent);
}


SourceView::~SourceView()
{
	SetStackFrame(NULL);
	SetStackTrace(NULL);
	SetSourceCode(NULL);
}


/*static*/ SourceView*
SourceView::Create(Team* team, Listener* listener)
{
	SourceView* self = new SourceView(team, listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
SourceView::UnsetListener()
{
	fListener = NULL;
}


void
SourceView::SetStackTrace(StackTrace* stackTrace)
{
	TRACE_GUI("SourceView::SetStackTrace(%p)\n", stackTrace);

	if (stackTrace == fStackTrace)
		return;

	if (fStackTrace != NULL) {
		fMarkerManager->SetStackTrace(NULL);
		fMarkerView->SetStackTrace(NULL);
		fStackTrace->ReleaseReference();
	}

	fStackTrace = stackTrace;

	if (fStackTrace != NULL)
		fStackTrace->AcquireReference();

	fMarkerManager->SetStackTrace(fStackTrace);
	fMarkerView->SetStackTrace(fStackTrace);
	fTextView->Invalidate();
}


void
SourceView::SetStackFrame(StackFrame* stackFrame)
{
	if (stackFrame == fStackFrame)
		return;

	if (fStackFrame != NULL) {
		fMarkerManager->SetStackFrame(NULL);
		fMarkerView->SetStackFrame(NULL);
		fStackFrame->ReleaseReference();
	}

	fStackFrame = stackFrame;

	if (fStackFrame != NULL)
		fStackFrame->AcquireReference();

	fMarkerManager->SetStackFrame(fStackFrame);
	fMarkerView->SetStackFrame(fStackFrame);
	fTextView->Invalidate();

	if (fStackFrame != NULL)
		ScrollToAddress(fStackFrame->InstructionPointer());
}


void
SourceView::SetSourceCode(SourceCode* sourceCode)
{
	// set the source code, if it changed
	if (sourceCode == fSourceCode)
		return;

	if (fSourceCode != NULL) {
		fMarkerManager->SetSourceCode(NULL);
		fTextView->SetSourceCode(NULL);
		fMarkerView->SetSourceCode(NULL);
		fSourceCode->ReleaseReference();
	}

	fSourceCode = sourceCode;

	if (fSourceCode != NULL)
		fSourceCode->AcquireReference();

	fMarkerManager->SetSourceCode(fSourceCode);
	fTextView->SetSourceCode(fSourceCode);
	fMarkerView->SetSourceCode(fSourceCode);
	_UpdateScrollBars();

	if (fStackFrame != NULL)
		ScrollToAddress(fStackFrame->InstructionPointer());
}


void
SourceView::UserBreakpointChanged(UserBreakpoint* breakpoint)
{
	fMarkerManager->UserBreakpointChanged(breakpoint);
	fMarkerView->UserBreakpointChanged(breakpoint);
	fTextView->UserBreakpointChanged(breakpoint);
}


bool
SourceView::ScrollToAddress(target_addr_t address)
{
	TRACE_GUI("SourceView::ScrollToAddress(%#llx)\n", address);

	if (fSourceCode == NULL)
		return false;

	AutoLocker<Team> locker(fTeam);

	FunctionInstance* functionInstance;
	Statement* statement;
	if (fTeam->GetStatementAtAddress(address, functionInstance,
			statement) != B_OK) {
		return false;
	}
	BReference<Statement> statementReference(statement, true);

	return ScrollToLine(statement->StartSourceLocation().Line());
}


bool
SourceView::ScrollToLine(uint32 line)
{
	TRACE_GUI("SourceView::ScrollToLine(%lu)\n", line);

	if (fSourceCode == NULL || line >= (uint32)fSourceCode->CountLines())
		return false;

	float top = (float)line * fFontInfo.lineHeight;
	float bottom = top + fFontInfo.lineHeight - 1;

	BRect visible = Bounds();

	TRACE_GUI("SourceView::ScrollToLine(%ld)\n", line);
	TRACE_GUI("  visible: (%f, %f) - (%f, %f), line: %f - %f\n", visible.left,
		visible.top, visible.right, visible.bottom, top, bottom);

	// If not visible at all, scroll to the center, otherwise scroll so that at
	// least one more line is visible.
	if (top >= visible.bottom || bottom <= visible.top) {
		TRACE_GUI("  -> scrolling to (%f, %f)\n", visible.left,
			top - (visible.Height() + 1) / 2);
		ScrollTo(visible.left, top - (visible.Height() + 1) / 2);
	} else if (top - fFontInfo.lineHeight < visible.top)
		ScrollBy(0, top - fFontInfo.lineHeight - visible.top);
	else if (bottom + fFontInfo.lineHeight > visible.bottom)
		ScrollBy(0, bottom + fFontInfo.lineHeight - visible.bottom);

	return true;
}


void
SourceView::HighlightBorder(bool state)
{
	BScrollView* parent = dynamic_cast<BScrollView*>(Parent());
	if (parent != NULL)
		parent->SetBorderHighlighted(state);
}


void
SourceView::TargetedByScrollView(BScrollView* scrollView)
{
	_UpdateScrollBars();
}


BSize
SourceView::MinSize()
{
//	BSize markerSize(fMarkerView->MinSize());
//	BSize textSize(fTextView->MinSize());
//	return BSize(BLayoutUtils::AddDistances(markerSize.width, textSize.width),
//		std::max(markerSize.height, textSize.height));
	return BSize(10, 10);
}


BSize
SourceView::MaxSize()
{
//	BSize markerSize(fMarkerView->MaxSize());
//	BSize textSize(fTextView->MaxSize());
//	return BSize(BLayoutUtils::AddDistances(markerSize.width, textSize.width),
//		std::min(markerSize.height, textSize.height));
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


BSize
SourceView::PreferredSize()
{
	BSize markerSize(fMarkerView->PreferredSize());
	BSize textSize(fTextView->PreferredSize());
	return BSize(BLayoutUtils::AddDistances(markerSize.width, textSize.width),
		std::max(markerSize.height, textSize.height));
//	return MinSize();
}


void
SourceView::DoLayout()
{
	BSize size = _DataRectSize();
	float markerWidth = fMarkerView->MinSize().width;

	fMarkerView->MoveTo(0, 0);
	fMarkerView->ResizeTo(markerWidth, size.height);

	fTextView->MoveTo(markerWidth + 1, 0);
	fTextView->ResizeTo(size.width - markerWidth - 1, size.height);

	_UpdateScrollBars();
}


void
SourceView::_Init()
{
	fMarkerManager = new MarkerManager(this, fTeam, fListener);
	AddChild(fMarkerView = new MarkerView(this, fTeam, fListener,
		fMarkerManager, &fFontInfo));
	AddChild(fTextView = new TextView(this, fMarkerManager, &fFontInfo));
}


void
SourceView::_UpdateScrollBars()
{
	BSize dataRectSize = _DataRectSize();
	BSize size = Frame().Size();

	if (BScrollBar* scrollBar = ScrollBar(B_HORIZONTAL)) {
		float range = dataRectSize.width - size.width;
		if (range > 0) {
			scrollBar->SetRange(0, range);
			scrollBar->SetProportion(
				(size.width + 1) / (dataRectSize.width + 1));
			scrollBar->SetSteps(fFontInfo.lineHeight, size.width + 1);
		} else {
			scrollBar->SetRange(0, 0);
			scrollBar->SetProportion(1);
		}
	}

	if (BScrollBar* scrollBar = ScrollBar(B_VERTICAL)) {
		float range = dataRectSize.height - size.height;
		if (range > 0) {
			scrollBar->SetRange(0, range);
			scrollBar->SetProportion(
				(size.height + 1) / (dataRectSize.height + 1));
			scrollBar->SetSteps(fFontInfo.lineHeight, size.height + 1);
		} else {
			scrollBar->SetRange(0, 0);
			scrollBar->SetProportion(1);
		}
	}
}


BSize
SourceView::_DataRectSize() const
{
	float width = fMarkerView->MinSize().width + fTextView->MinSize().width + 1;
	float height = std::max(fMarkerView->MinSize().height,
		fTextView->MinSize().height);

	BSize size = Frame().Size();
	return BSize(std::max(size.width, width), std::max(size.height, height));
}


// #pragma mark - Listener


SourceView::Listener::~Listener()
{
}
