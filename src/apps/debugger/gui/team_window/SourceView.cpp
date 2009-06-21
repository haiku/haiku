/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "SourceView.h"

#include <algorithm>
#include <new>

#include <stdio.h>

#include <LayoutUtils.h>
#include <ScrollBar.h>

#include <AutoLocker.h>

#include "SourceCode.h"
#include "StackFrame.h"
#include "TeamDebugModel.h"


static const int32 kLeftTextMargin = 3;


class SourceView::MarkerView : public BView {
public:
	MarkerView(SourceView* sourceView, FontInfo* fontInfo)
		:
		BView("source marker view", B_WILL_DRAW),
		fSourceView(sourceView),
		fFontInfo(fontInfo)
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}

	virtual BSize MinSize()
	{
		return BSize(40, 10);
	}

	virtual BSize MaxSize()
	{
		return BSize(MinSize().width, B_SIZE_UNLIMITED);
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

	virtual void Draw(BRect updateRect)
	{
	}

private:
	SourceView*	fSourceView;
	FontInfo*	fFontInfo;
};


class SourceView::TextView : public BView {
public:
	TextView(SourceView* sourceView, FontInfo* fontInfo)
		:
		BView("source text view", B_WILL_DRAW),
		fSourceView(sourceView),
		fFontInfo(fontInfo),
		fSourceCode(NULL),
		fMaxLineWidth(-1)
	{
		SetViewColor(ui_color(B_DOCUMENT_BACKGROUND_COLOR));
		fTextColor = ui_color(B_DOCUMENT_TEXT_COLOR);
	}

	void SetSourceCode(SourceCode* sourceCode)
	{
		fSourceCode = sourceCode;
		fMaxLineWidth = -1;

		InvalidateLayout();
		Invalidate();
	}

	virtual BSize MinSize()
	{
		float height = _LineCount() * fFontInfo->lineHeight;
		return BSize(kLeftTextMargin + _MaxLineWidth() - 1,
			std::max(height - 1, 100.0f));
	}

	virtual BSize MaxSize()
	{
		return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

	virtual void Draw(BRect updateRect)
	{
		if (fSourceCode == NULL)
			return;

		// get the lines intersecting with the update rect
		int32 lineHeight = (int32)fFontInfo->lineHeight;
		int32 minLine = (int32)updateRect.top / lineHeight;
		int32 maxLine = ((int32)ceilf(updateRect.bottom) + lineHeight - 1)
			/ lineHeight;
		minLine = std::max(minLine, 0L);
		maxLine = std::min(maxLine, fSourceCode->CountLines() - 1);

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
	int32 _LineCount() const
	{
		return fSourceCode != NULL ? fSourceCode->CountLines() : 0;
	}

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
	SourceView*	fSourceView;
	FontInfo*	fFontInfo;
	SourceCode*	fSourceCode;
	float		fMaxLineWidth;
	rgb_color	fTextColor;
};


// #pragma mark - SourceView


SourceView::SourceView(TeamDebugModel* debugModel, Listener* listener)
	:
	BView("source view", 0),
	fDebugModel(debugModel),
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
SourceView::SetStackFrame(StackFrame* frame)
{
printf("SourceView::SetStackFrame(%p)\n", frame);
	if (frame == fStackFrame)
		return;

	if (fStackFrame != NULL)
		fStackFrame->RemoveReference();

	fStackFrame = frame;

	if (fStackFrame != NULL)
		fStackFrame->AddReference();

	UpdateSourceCode();
}


void
SourceView::UpdateSourceCode()
{
	// get a reference to the source code
	AutoLocker<TeamDebugModel> locker(fDebugModel);

	SourceCode* sourceCode = fStackFrame != NULL
		? fStackFrame->GetSourceCode() : NULL;
	Reference<SourceCode> sourceCodeReference(sourceCode);

	locker.Unlock();

	// set the source code, if it changed
	if (sourceCode == fSourceCode)
		return;

	if (fSourceCode != NULL) {
		fTextView->SetSourceCode(NULL);
		fSourceCode->RemoveReference();
	}

	fSourceCode = sourceCodeReference.Detach();

	fTextView->SetSourceCode(fSourceCode);
	_UpdateScrollBars();
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


// #pragma mark - Listener


SourceView::Listener::~Listener()
{
}
