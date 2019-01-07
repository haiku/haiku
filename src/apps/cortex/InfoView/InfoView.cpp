/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// InfoView.cpp

#include "InfoView.h"
#include "cortex_ui.h"

#include "array_delete.h"

// Interface Kit
#include <Bitmap.h>
#include <Region.h>
#include <ScrollBar.h>
#include <StringView.h>
#include <TextView.h>
#include <Window.h>
// Storage Kit
#include <Mime.h>
// Support Kit
#include <List.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_ALLOC(X) //PRINT (x)			// ctor/dtor
#define D_HOOK(X) //PRINT (x)			// BView impl.
#define D_ACCESS(X) //PRINT (x)			// Accessors
#define D_METHOD(x) //PRINT (x)

// -------------------------------------------------------- //
// *** internal class: _InfoTextField
//
// * PURPOSE:
//   store the label & text for each field, and provide methods
//   for linewrapping and drawing
//
// -------------------------------------------------------- //

class _InfoTextField
{

public:					// *** ctor/dtor

						_InfoTextField(
							BString label,
							BString text,
							InfoView *parent);

						~_InfoTextField();

public:					// *** operations

	void				drawField(
							BPoint position);

	void				updateLineWrapping(
							bool *wrappingChanged = 0,
							bool *heightChanged = 0);

public:					// *** accessors

	float				getHeight() const;

	float				getWidth() const;

	bool				isWrapped() const;

private:				// *** static internal methods

	static bool			canEndLine(
							const char c);

	static bool			mustEndLine(
							const char c);

private:				// *** data members

	BString				m_label;
	
	BString				m_text;

	BList			   *m_textLines;

	InfoView		   *m_parent;
};

// -------------------------------------------------------- //
// *** static member init
// -------------------------------------------------------- //

const BRect InfoView::M_DEFAULT_FRAME	= BRect(0.0, 0.0, 250.0, 100.0);
const float InfoView::M_H_MARGIN		= 5.0;
const float InfoView::M_V_MARGIN		= 5.0;

// -------------------------------------------------------- //
// *** ctor/dtor (public)
// -------------------------------------------------------- //

InfoView::InfoView(
	BString title,
	BString subTitle,
	BBitmap *icon)
	: BView(M_DEFAULT_FRAME, "InfoView", B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW | B_FRAME_EVENTS),
	  m_title(title),
	  m_subTitle(subTitle),
	  m_icon(0),
	  m_fields(0) {
	D_ALLOC(("InfoView::InfoView()\n"));

	if (icon) {
		m_icon = new BBitmap(icon);
	}
	m_fields = new BList();
	SetViewColor(B_TRANSPARENT_COLOR);
}

InfoView::~InfoView() {
	D_ALLOC(("InfoView::~InfoView()\n"));

	// delete all the fields
	if (m_fields) {
		while (m_fields->CountItems() > 0) {
			_InfoTextField *field = static_cast<_InfoTextField *>
									(m_fields->RemoveItem((int32)0));
			if (field) {
				delete field;
			}
		}
		delete m_fields;
		m_fields = 0;
	}

	// delete the icon bitmap
	if (m_icon) {
		delete m_icon;
		m_icon = 0;
	}
}

// -------------------------------------------------------- //
// *** BView implementation
// -------------------------------------------------------- //

void InfoView::AttachedToWindow() {
	D_HOOK(("InfoView::AttachedToWindow()\n"));

	// adjust the windows title
	BString title = m_title;
	title << " info";
	Window()->SetTitle(title.String());
	
	// calculate the area occupied by title, subtitle and icon
	font_height fh;
	be_bold_font->GetHeight(&fh);
	float titleHeight = fh.leading + fh.descent;
	titleHeight += M_V_MARGIN * 2.0 + B_LARGE_ICON / 2.0;
	be_plain_font->GetHeight(&fh);
	titleHeight += fh.leading + fh.ascent + fh.descent;
	BFont font(be_bold_font);
	float titleWidth = font.StringWidth(title.String());
	titleWidth += M_H_MARGIN + B_LARGE_ICON + B_LARGE_ICON / 2.0;

	float width, height;
	GetPreferredSize(&width, &height);
	Window()->ResizeTo(width + B_V_SCROLL_BAR_WIDTH, height);
	ResizeBy(- B_V_SCROLL_BAR_WIDTH, 0.0);

	// add scroll bar
	BRect scrollRect = Window()->Bounds();
	scrollRect.left = scrollRect.right - B_V_SCROLL_BAR_WIDTH + 1.0;
	scrollRect.top -= 1.0;
	scrollRect.right += 1.0;
	scrollRect.bottom -= B_H_SCROLL_BAR_HEIGHT - 1.0;
	Window()->AddChild(new BScrollBar(scrollRect, "ScrollBar", this,
									  0.0, 0.0, B_VERTICAL));
	ScrollBar(B_VERTICAL)->SetRange(0.0, 0.0);
	be_plain_font->GetHeight(&fh);
	float step = fh.ascent + fh.descent + fh.leading + M_V_MARGIN;
	ScrollBar(B_VERTICAL)->SetSteps(step, step * 5);
									
	// set window size limits
	float minWidth, maxWidth, minHeight, maxHeight;
	Window()->GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	Window()->SetSizeLimits(titleWidth + B_V_SCROLL_BAR_WIDTH, maxWidth,
							titleHeight + B_H_SCROLL_BAR_HEIGHT, maxHeight);

	// cache the bounds rect for proper redraw later on...
	m_oldFrame = Bounds();
}

void InfoView::Draw(
	BRect updateRect) {
	D_HOOK(("InfoView::Draw()\n"));

	// Draw side bar
	SetDrawingMode(B_OP_COPY);
	BRect r = Bounds();
	r.right = B_LARGE_ICON - 1.0;
	SetLowColor(M_LIGHT_BLUE_COLOR);
	FillRect(r, B_SOLID_LOW);
	SetHighColor(M_DARK_BLUE_COLOR);
	r.right += 1.0;
	StrokeLine(r.RightTop(), r.RightBottom(), B_SOLID_HIGH);

	// Draw background
	BRegion region;
	region.Include(updateRect);
	region.Exclude(r);
	SetLowColor(M_GRAY_COLOR);
	FillRegion(&region, B_SOLID_LOW);

	// Draw title
	SetDrawingMode(B_OP_OVER);
	font_height fh;
	be_bold_font->GetHeight(&fh);
	SetFont(be_bold_font);
	BPoint p(M_H_MARGIN + B_LARGE_ICON + B_LARGE_ICON / 2.0,
			 M_V_MARGIN * 2.0 + fh.ascent);
	SetHighColor(M_BLACK_COLOR);
	DrawString(m_title.String(), p);
	
	// Draw sub-title
	p.y += fh.descent;
	be_plain_font->GetHeight(&fh);
	SetFont(be_plain_font);
	p.y += fh.ascent + fh.leading;
	SetHighColor(M_DARK_GRAY_COLOR);
	DrawString(m_subTitle.String(), p);

	// Draw icon
	p.y = 2 * M_V_MARGIN;
	if (m_icon) {
		p.x = B_LARGE_ICON / 2.0;
		DrawBitmapAsync(m_icon, p);
	}

	// Draw fields
	be_plain_font->GetHeight(&fh);
	p.x = B_LARGE_ICON;
	p.y += B_LARGE_ICON + 2 * M_V_MARGIN + fh.ascent + 2 * fh.leading;
	for (int32 i = 0; i < m_fields->CountItems(); i++) {
		_InfoTextField *field = static_cast<_InfoTextField *>(m_fields->ItemAt(i));
		field->drawField(p);
		p.y += field->getHeight() + M_V_MARGIN;
	}
}

void InfoView::FrameResized(
	float width,
	float height) {
	D_HOOK(("InfoView::FrameResized()\n"));

	BRect newFrame = BRect(0.0, 0.0, width, height);

	// update the each lines' line-wrapping and redraw as necessary
	font_height fh;
	BPoint p;
	be_plain_font->GetHeight(&fh);
	p.x = B_LARGE_ICON;
	p.y += B_LARGE_ICON + M_V_MARGIN * 2.0 + fh.ascent + fh.leading * 2.0;
	bool heightChanged = false;
	for (int32 i = 0; i < m_fields->CountItems(); i++) {
		bool wrappingChanged = false;
		_InfoTextField *field = static_cast<_InfoTextField *>(m_fields->ItemAt(i));
		field->updateLineWrapping(&wrappingChanged,
								  heightChanged ? 0 : &heightChanged);
		float fieldHeight = field->getHeight() + M_V_MARGIN;
		if (heightChanged) {
			Invalidate(BRect(p.x, p.y, width, p.y + fieldHeight));
		}
		else if (wrappingChanged) {
			Invalidate(BRect(p.x + m_sideBarWidth, p.y, width, p.y + fieldHeight));
		}
		p.y += fieldHeight;
	}

	// clean up the rest of the view
	BRect updateRect;
	updateRect.left = B_LARGE_ICON;
	updateRect.top = p.y < (m_oldFrame.bottom - M_V_MARGIN - 15.0) ?
					 p.y - 15.0 : m_oldFrame.bottom - M_V_MARGIN - 15.0;
	updateRect.right = width - M_H_MARGIN;
	updateRect.bottom = m_oldFrame.bottom < newFrame.bottom ?
						newFrame.bottom : m_oldFrame.bottom;
	Invalidate(updateRect);

	if (p.y > height) {
		ScrollBar(B_VERTICAL)->SetRange(0.0, ceil(p.y - height));
	}
	else {
		ScrollBar(B_VERTICAL)->SetRange(0.0, 0.0);
	}

	// cache the new frame rect for the next time
	m_oldFrame = newFrame;
}

void
InfoView::GetPreferredSize(
	float *width,
	float *height) {
	D_HOOK(("InfoView::GetPreferredSize()\n"));

	*width = 0;
	*height = 0;

	// calculate the height needed to display everything, avoiding line wrapping
	font_height fh;
	be_plain_font->GetHeight(&fh);
	for (int32 i = 0; i < m_fields->CountItems(); i++) {
		_InfoTextField *field = static_cast<_InfoTextField *>(m_fields->ItemAt(i));
		*height += fh.ascent + fh.descent + fh.leading + M_V_MARGIN;
		float tfw = field->getWidth();
		if (tfw > *width) {
			*width = tfw;
		}
	}

	*width += B_LARGE_ICON + 2 * M_H_MARGIN;
	*height += B_LARGE_ICON + 2 * M_V_MARGIN + fh.ascent + 2 * fh.leading;
	*height += B_H_SCROLL_BAR_HEIGHT;
}

// -------------------------------------------------------- //
// *** operations (protected)
// -------------------------------------------------------- //

void InfoView::addField(
	BString label,
	BString text) {
	D_METHOD(("InfoView::addField()\n"));

	m_fields->AddItem(new _InfoTextField(label, text, this));
}

// -------------------------------------------------------- //
// *** internal class: _InfoTextField
//
// *** ctor/dtor
// -------------------------------------------------------- //

_InfoTextField::_InfoTextField(
	BString label,
	BString text,
	InfoView *parent)
	: m_label(label),
	  m_text(text),
	  m_textLines(0),
	  m_parent(parent) {
	D_ALLOC(("_InfoTextField::_InfoTextField()\n"));

	if (m_label != "") {
		m_label << ":  ";
	}
	m_textLines = new BList();
}

_InfoTextField::~_InfoTextField() {
	D_ALLOC(("_InfoTextField::~_InfoTextField()\n"));

	// delete every line
	if (m_textLines) {
		while (m_textLines->CountItems() > 0) {
			BString *line = static_cast<BString *>(m_textLines->RemoveItem((int32)0));
			if (line) {
				delete line;
			}
		}
		delete m_textLines;
		m_textLines = 0;
	}
}

// -------------------------------------------------------- //
// *** internal class: _InfoTextField
//
// *** operations (public)
// -------------------------------------------------------- //

void _InfoTextField::drawField(
	BPoint position) {
	D_METHOD(("_InfoTextField::drawField()\n"));

	float sideBarWidth = m_parent->getSideBarWidth();

	// Draw label
	BPoint p = position;
	p.x += sideBarWidth - be_plain_font->StringWidth(m_label.String());
	m_parent->SetHighColor(M_DARK_GRAY_COLOR);
	m_parent->SetDrawingMode(B_OP_OVER);
	m_parent->SetFont(be_plain_font);
	m_parent->DrawString(m_label.String(), p);
	
	// Draw text
	font_height fh;
	be_plain_font->GetHeight(&fh);
	p.x = position.x + sideBarWidth;// + InfoView::M_H_MARGIN;
	m_parent->SetHighColor(M_BLACK_COLOR);
	for (int32 i = 0; i < m_textLines->CountItems(); i++) {
		BString *line = static_cast<BString *>(m_textLines->ItemAt(i));
		m_parent->DrawString(line->String(), p);
		p.y += fh.ascent + fh.descent + fh.leading;
	}
}

void _InfoTextField::updateLineWrapping(
	bool *wrappingChanged,
	bool *heightChanged)
{
	D_METHOD(("_InfoTextField::updateLineWrapping()\n"));
	
	// clear the current list of lines but remember their number and
	// the number of characters per line (to know if something changed)
	int32 numLines = m_textLines->CountItems();
	int32* numChars = new int32[numLines];
	array_delete<int32> _d(numChars);
	
	for (int32 i = 0; i < numLines; i++)
	{
		BString *line = static_cast<BString *>(m_textLines->ItemAt(i));
		numChars[i] = line->CountChars();
		delete line;
	}
	m_textLines->MakeEmpty();

	// calculate the maximum width for a line
	float maxWidth = m_parent->Bounds().Width();
	maxWidth -= m_parent->getSideBarWidth() + 3 * InfoView::M_H_MARGIN + B_LARGE_ICON;
	if (maxWidth <= be_plain_font->StringWidth("M"))
	{
		return;
	}

	// iterate through the text and split into new lines as
	// necessary
	BString *currentLine = new BString(m_text);
	while (currentLine && (currentLine->CountChars() > 0))
	{
		int32 lastBreak = 0;
		for (int32 i = 0; i < currentLine->CountChars(); i++)
		{
			if (canEndLine(currentLine->ByteAt(i)))
			{
				lastBreak = i + 1;
				if (mustEndLine(currentLine->ByteAt(i)))
				{
					BString *newLine = new BString();
					currentLine->Remove(i, 1);
					currentLine->MoveInto(*newLine, i,
										  currentLine->CountChars() - i);
					m_textLines->AddItem(currentLine);
					currentLine = newLine;
					break;
				}
			}
			else
			{
				if (i == currentLine->CountChars() - 1) // the last char in the text
				{
					m_textLines->AddItem(currentLine);
					currentLine = 0;
					break;
				}
				else
				{
					BString buffer;
					currentLine->CopyInto(buffer, 0, i);
					if (be_plain_font->StringWidth(buffer.String()) > maxWidth)
					{
						if (lastBreak < 1)
						{
							lastBreak = i - 1;
						}
						BString *newLine = new BString();
						currentLine->MoveInto(*newLine, lastBreak,
											  currentLine->CountChars() - lastBreak);
						m_textLines->AddItem(currentLine);
						currentLine = newLine;
						break;
					}
				}
			}
		}
	}

	// report changes in the fields total height (i.e. if the number
	// of lines changed)
	if (heightChanged && (numLines != m_textLines->CountItems()))
	{
		*heightChanged = true;
	}

	// report changes in the wrapping (e.g. if a word slipped into the
	// next line)
	else if (wrappingChanged)
	{
		for (int32 i = 0; i < m_textLines->CountItems(); i++)
		{
			BString *line = static_cast<BString *>(m_textLines->ItemAt(i));
			if (line->CountChars() != numChars[i])
			{
				*wrappingChanged = true;
				break;
			}
		}
	}
}

// -------------------------------------------------------- //
// *** internal class: _InfoTextField
//
// *** accessors (public)
// -------------------------------------------------------- //

float
_InfoTextField::getHeight() const {
	D_ACCESS(("_InfoTextField::getHeight()\n"));

	font_height fh;
	be_plain_font->GetHeight(&fh);

	// calculate the width for an empty line (separator)
	if (m_textLines->CountItems() == 0)
	{
		return fh.ascent + fh.descent + fh.leading;
	}

	// calculate the total height of the field by counting the
	else
	{
		float height = fh.ascent + fh.descent + fh.leading;
		height *= m_textLines->CountItems();
		height += fh.leading;
		return height;
	}
}

float
_InfoTextField::getWidth() const {
	D_ACCESS(("_InfoTextField::getWidth()\n"));

	float width = be_plain_font->StringWidth(m_text.String());
	width += m_parent->getSideBarWidth();

	return width;
}

bool
_InfoTextField::isWrapped() const {
	D_ACCESS(("_InfoTextField::isWrapped()\n"));

	return (m_textLines->CountItems() > 1);
}

// -------------------------------------------------------- //
// *** internal class: _InfoTextField
//
// *** static internal methods (private)
// -------------------------------------------------------- //

bool _InfoTextField::canEndLine(
	const char c)
{
	D_METHOD(("_InfoTextField::canEndLine()\n"));

	if ((c == B_SPACE) || (c == B_TAB) || (c == B_ENTER)
	 || ( c == '-'))
	{
		return true;
	}
	return false;
}

bool _InfoTextField::mustEndLine(
	const char c)
{
	D_METHOD(("_InfoTextField::mustEndLine()\n"));

	if (c == B_ENTER)
	{
		return true;
	}
	return false;
}

// END -- InfoView.cpp --
