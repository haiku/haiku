/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "SourceView.h"

#include <algorithm>
#include <new>

#include <stdio.h>

#include <LayoutUtils.h>
#include <Polygon.h>
#include <ScrollBar.h>

#include <AutoLocker.h>
#include <ObjectList.h>

#include "SourceCode.h"
#include "StackTrace.h"
#include "Statement.h"
#include "TeamDebugModel.h"


static const int32 kLeftTextMargin = 3;
static const float kMinViewHeight = 80.0f;


class SourceView::BaseView : public BView {
public:
	BaseView(const char* name, SourceView* sourceView, FontInfo* fontInfo)
		:
		BView(name, B_WILL_DRAW | B_SUBPIXEL_PRECISE),
		fSourceView(sourceView),
		fFontInfo(fontInfo),
		fSourceCode(NULL)
	{
	}

	virtual void SetSourceCode(SourceCode* sourceCode)
	{
		fSourceCode = sourceCode;

		InvalidateLayout();
		Invalidate();
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

protected:
	int32 LineCount() const
	{
		return fSourceCode != NULL ? fSourceCode->CountLines() : 0;
	}

	float TotalHeight() const
	{
		float height = LineCount() * fFontInfo->lineHeight - 1;
		return std::max(height, kMinViewHeight);
	}

	void GetLineRange(BRect rect, int32& minLine, int32& maxLine)
	{
		int32 lineHeight = (int32)fFontInfo->lineHeight;
		minLine = (int32)rect.top / lineHeight;
		maxLine = ((int32)ceilf(rect.bottom) + lineHeight - 1) / lineHeight;
		minLine = std::max(minLine, 0L);
		maxLine = std::min(maxLine, fSourceCode->CountLines() - 1);
	}


protected:
	SourceView*	fSourceView;
	FontInfo*	fFontInfo;
	SourceCode*	fSourceCode;
};


class SourceView::MarkerView : public BaseView {
public:
	MarkerView(SourceView* sourceView, FontInfo* fontInfo)
		:
		BaseView("source marker view", sourceView, fontInfo),
		fStackTrace(NULL),
		fStackFrame(NULL),
		fMarkers(20, true),
		fMarkersValid(false)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}

	~MarkerView()
	{
	}

	virtual void SetSourceCode(SourceCode* sourceCode)
	{
		fMarkers.MakeEmpty();
		fMarkersValid = false;
		BaseView::SetSourceCode(sourceCode);
	}

	void SetStackTrace(StackTrace* stackTrace)
	{
		fStackTrace = stackTrace;
		fMarkers.MakeEmpty();
		fMarkersValid = false;
		Invalidate();
	}

	void SetStackFrame(StackFrame* stackFrame)
	{
		fStackFrame = stackFrame;
		fMarkers.MakeEmpty();
		fMarkersValid = false;
		Invalidate();
	}

	virtual BSize MinSize()
	{
		return BSize(40, TotalHeight());
	}

	virtual BSize MaxSize()
	{
		return BSize(MinSize().width, B_SIZE_UNLIMITED);
	}

	virtual void Draw(BRect updateRect)
	{
		_UpdateMarkers();

		if (fSourceCode == NULL || fMarkers.IsEmpty())
			return;

		// get the lines intersecting with the update rect
		int32 minLine, maxLine;
		GetLineRange(updateRect, minLine, maxLine);

		// draw the markers
		float width = Bounds().Width();
		// TODO: The markers should be sorted, so we don't need to iterate over
		// all of them.
		for (int32 i = 0; Marker* marker = fMarkers.ItemAt(i); i++) {
			int32 line = marker->Line();
			if (line < minLine || line > maxLine)
				continue;

			float y = (float)line * fFontInfo->lineHeight;
			BRect rect(0, y, width, y + fFontInfo->lineHeight - 1);
			marker->Draw(this, rect);
		}

		// TODO: Draw possible breakpoint marks!
	}

private:
	struct Marker {
		Marker(uint32 line)
			:
			fLine(line)
		{
		}

		virtual ~Marker()
		{
		}

		uint32 Line() const
		{
			return fLine;
		}

		virtual void Draw(MarkerView* view, BRect rect) = 0;

	private:
		uint32	fLine;
	};

	struct InstructionPointerMarker : Marker {
		InstructionPointerMarker(uint32 line, bool topIP, bool currentIP)
			:
			Marker(line),
			fIsTopIP(topIP),
			fIsCurrentIP(currentIP)
		{
		}

		virtual void Draw(MarkerView* view, BRect rect)
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

	private:
		void _DrawArrow(BView* view, BPoint tip, BSize size, BSize base,
			const rgb_color& color, bool fill)
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

	private:
		bool	fIsTopIP;
		bool	fIsCurrentIP;
	};

	typedef BObjectList<Marker>	MarkerList;

private:
	void _UpdateMarkers()
	{
		if (fMarkersValid)
			return;

		fMarkers.MakeEmpty();

		if (fSourceCode != NULL && fStackTrace != NULL) {
			for (int32 i = 0; StackFrame* frame = fStackTrace->FrameAt(i);
					i++) {
				target_addr_t ip = frame->InstructionPointer();
				Statement* statement = fSourceCode->StatementAtAddress(ip);
				if (statement == NULL)
					continue;
				uint32 line = statement->StartSourceLocation().Line();
				if (line >= (uint32)LineCount())
					continue;

				Marker* marker = new(std::nothrow) InstructionPointerMarker(
					line, i == 0, frame == fStackFrame);
				if (marker == NULL || !fMarkers.AddItem(marker)) {
					delete marker;
					break;
				}
			}
		}
		// TODO: Filter duplicate IP markers (recursive functions)!

		fMarkersValid = true;
	}

private:
	StackTrace*	fStackTrace;
	StackFrame*	fStackFrame;
	MarkerList	fMarkers;
	bool		fMarkersValid;
};


class SourceView::TextView : public BaseView {
public:
	TextView(SourceView* sourceView, FontInfo* fontInfo)
		:
		BaseView("source text view", sourceView, fontInfo),
		fMaxLineWidth(-1)
	{
		SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
		fTextColor = ui_color(B_DOCUMENT_TEXT_COLOR);
	}

	virtual void SetSourceCode(SourceCode* sourceCode)
	{
		fMaxLineWidth = -1;
		BaseView::SetSourceCode(sourceCode);
	}

	virtual BSize MinSize()
	{
		return BSize(kLeftTextMargin + _MaxLineWidth() - 1, TotalHeight());
	}

	virtual BSize MaxSize()
	{
		return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
	}

	virtual void Draw(BRect updateRect)
	{
		if (fSourceCode == NULL)
			return;

		// get the lines intersecting with the update rect
		int32 minLine, maxLine;
		GetLineRange(updateRect, minLine, maxLine);

		// draw the affected lines
		SetHighColor(fTextColor);
		SetFont(&fFontInfo->font);
		for (int32 i = minLine; i <= maxLine; i++) {
			float y = (float)(i + 1) * fFontInfo->lineHeight
				- fFontInfo->fontHeight.descent;
			DrawString(fSourceCode->LineAt(i), BPoint(kLeftTextMargin, y));
		}
	}

private:
	float _MaxLineWidth()
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

private:
	float		fMaxLineWidth;
	rgb_color	fTextColor;
};


// #pragma mark - SourceView


SourceView::SourceView(TeamDebugModel* debugModel, Listener* listener)
	:
	BView("source view", 0),
	fDebugModel(debugModel),
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
SourceView::Create(TeamDebugModel* debugModel, Listener* listener)
{
	SourceView* self = new SourceView(debugModel, listener);

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
printf("SourceView::SetStackTrace(%p)\n", stackTrace);
	if (stackTrace == fStackTrace)
		return;

	if (fStackTrace != NULL) {
		fMarkerView->SetStackTrace(NULL);
		fStackTrace->RemoveReference();
	}

	fStackTrace = stackTrace;

	if (fStackTrace != NULL)
		fStackTrace->AddReference();

	fMarkerView->SetStackTrace(fStackTrace);
}


void
SourceView::SetStackFrame(StackFrame* stackFrame)
{
	if (stackFrame == fStackFrame)
		return;

	if (fStackFrame != NULL) {
		fMarkerView->SetStackFrame(NULL);
		fStackFrame->RemoveReference();
	}

	fStackFrame = stackFrame;

	if (fStackFrame != NULL)
		fStackFrame->AddReference();

	fMarkerView->SetStackFrame(fStackFrame);

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
		fTextView->SetSourceCode(NULL);
		fMarkerView->SetSourceCode(NULL);
		fSourceCode->RemoveReference();
	}

	fSourceCode = sourceCode;

	if (fSourceCode != NULL)
		fSourceCode->AddReference();

	fTextView->SetSourceCode(fSourceCode);
	fMarkerView->SetSourceCode(fSourceCode);
	_UpdateScrollBars();

	if (fStackFrame != NULL)
		ScrollToAddress(fStackFrame->InstructionPointer());
}


bool
SourceView::ScrollToAddress(target_addr_t address)
{
	if (fSourceCode == NULL)
		return false;

	Statement* statement = fSourceCode->StatementAtAddress(address);
	if (statement == NULL)
		return false;

	return ScrollToLine(statement->StartSourceLocation().Line());
}


bool
SourceView::ScrollToLine(uint32 line)
{
printf("SourceView::ScrollToLine(%lu)\n", line);
	if (fSourceCode == NULL || line >= (uint32)fSourceCode->CountLines())
		return false;

	float top = (float)line * fFontInfo.lineHeight;
	float bottom = top + fFontInfo.lineHeight - 1;

	BRect visible = _VisibleRect();

	// If not visible at all, scroll to the center, otherwise scroll so that at
	// least one more line is visible.
	if (top >= visible.bottom || bottom <= visible.top)
		ScrollTo(visible.left, top - (visible.Height() + 1) / 2);
	else if (top - fFontInfo.lineHeight < visible.top)
		ScrollBy(0, top - fFontInfo.lineHeight - visible.top);
	else if (bottom + fFontInfo.lineHeight > visible.bottom)
		ScrollBy(0, bottom + fFontInfo.lineHeight - visible.bottom);

	return true;
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
	AddChild(fMarkerView = new MarkerView(this, &fFontInfo));
	AddChild(fTextView = new TextView(this, &fFontInfo));
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


BRect
SourceView::_VisibleRect() const
{
	return BRect(Bounds().LeftTop(), Frame().Size());
}


// #pragma mark - Listener


SourceView::Listener::~Listener()
{
}
