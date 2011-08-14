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


// MediaJack.cpp
// c.lenz 10oct99

#include "MediaJack.h"
// MediaRoutingView
#include "MediaRoutingDefs.h"
#include "MediaRoutingView.h"
#include "MediaWire.h"
// InfoWindow
#include "InfoWindowManager.h"
// Support
#include "cortex_ui.h"
#include "MediaString.h"
// TipManager
#include "TipManager.h"

// Application Kit
#include <Application.h>
// Interface Kit
#include <Bitmap.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)
#define D_DRAW(x) //PRINT (x)
#define D_MOUSE(x) //PRINT (x)

// -------------------------------------------------------- //
// constants
// -------------------------------------------------------- //

float MediaJack::M_DEFAULT_WIDTH				= 5.0;
float MediaJack::M_DEFAULT_HEIGHT				= 10.0;
const float MediaJack::M_DEFAULT_GAP			= 5.0;
const int32 MediaJack::M_MAX_ABBR_LENGTH		= 3;

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

MediaJack::MediaJack(
	media_input input)
	: DiagramEndPoint(BRect(0.0, 0.0, M_DEFAULT_WIDTH, M_DEFAULT_HEIGHT)),
	  m_jackType(M_INPUT),
	  m_bitmap(0),
	  m_index(input.destination.id),
	  m_node(input.node),
	  m_source(input.source),
	  m_destination(input.destination),
	  m_format(input.format),
	  m_label(input.name),
	  m_abbreviation("")
{
	D_METHOD(("MediaJack::MediaJack()\n"));
	makeSelectable(false);
	if (m_label == "")
		m_label = "Input";
	_updateAbbreviation();
}

MediaJack::MediaJack(
	media_output output)
	: DiagramEndPoint(BRect(0.0, 0.0, M_DEFAULT_WIDTH, M_DEFAULT_HEIGHT)),
	  m_jackType(M_OUTPUT),
	  m_bitmap(0),
	  m_index(output.source.id),
	  m_node(output.node),
	  m_source(output.source),
	  m_destination(output.destination),
	  m_format(output.format),
	  m_label(output.name),
	  m_abbreviation("")
{
	D_METHOD(("MediaJack::MediaJack()\n"));
	makeSelectable(false);
	if (m_label == "")
		m_label = "Output";
	_updateAbbreviation();
}

MediaJack::~MediaJack()
{
	D_METHOD(("MediaJack::~MediaJack()\n"));
	delete m_bitmap;
}

// -------------------------------------------------------- //
// *** accessors
// -------------------------------------------------------- //

status_t MediaJack::getInput(
	media_input *input) const
{
	D_METHOD(("MediaJack::getInput()\n"));
	if (isInput())
	{
		input->node = m_node;
		input->source = m_source;
		input->destination = m_destination;
		input->format = m_format;
		strlcpy(input->name, m_label.String(), B_MEDIA_NAME_LENGTH);
		return B_OK;
	}
	return B_ERROR;
}

status_t MediaJack::getOutput(
	media_output *output) const
{
	D_METHOD(("MediaJack::getOutput()\n"));
	if (isOutput())
	{
		output->node = m_node;
		output->source = m_source;
		output->destination = m_destination;
		output->format = m_format;
		strlcpy(output->name, m_label.String(), B_MEDIA_NAME_LENGTH);
		return B_OK;
	}
	return B_ERROR;
}

// -------------------------------------------------------- //
// *** derived from DiagramEndPoint (public)
// -------------------------------------------------------- //

void MediaJack::attachedToDiagram()
{
	D_METHOD(("MediaJack::attachedToDiagram()\n"));
	_updateBitmap();
}

void MediaJack::detachedFromDiagram()
{
	D_METHOD(("MediaJack::detachedFromDiagram()\n"));

	// make sure we're no longer displaying a tooltip
	TipManager *tips = TipManager::Instance();
	tips->hideTip(view()->ConvertToScreen(Frame()));
}

void MediaJack::drawEndPoint()
{
	D_DRAW(("MediaJack::drawEndPoint()\n"));

	if (m_bitmap)
	{
		view()->DrawBitmap(m_bitmap, Frame().LeftTop());
	}
}

BPoint MediaJack::connectionPoint() const
{
	D_METHOD(("MediaJack::connectionPoint()\n"));

	switch (dynamic_cast<MediaRoutingView *>(view())->getLayout())
	{
		case MediaRoutingView::M_ICON_VIEW:
		{
			if (isInput())
			{
				return BPoint(Frame().left - 1.0, Frame().top + Frame().Height() / 2.0);
			}
			else if (isOutput())
			{
				return BPoint(Frame().right + 1.0, Frame().top + Frame().Height() / 2.0);
			}
			break;
		}
		case MediaRoutingView::M_MINI_ICON_VIEW:
		{
			if (isInput())
			{
				return BPoint(Frame().left + Frame().Width() / 2.0, Frame().top - 1.0);
			}
			else if (isOutput())
			{
				return BPoint(Frame().left + Frame().Width() / 2.0, Frame().bottom + 1.0);
			}
			break;
		}
	}
	return BPoint(-1.0, -1.0);
}

bool MediaJack::connectionRequested(
	DiagramEndPoint *which)
{
	D_METHOD(("MediaJack::connectionRequested()\n"));

	MediaJack *otherJack = dynamic_cast<MediaJack *>(which);
	if (otherJack && (otherJack->m_jackType != m_jackType) && !isConnected())
	{
		return true;
	}
	return false;
}

void MediaJack::MouseDown(
	BPoint point,
	uint32 buttons,
	uint32 clicks)
{
	D_MOUSE(("MediaJack::MouseOver()\n"));

	// if connected, redirect to the wire
	if (isConnected())
	{
		dynamic_cast<MediaWire *>(wire())->MouseDown(point, buttons, clicks);
		return;
	}

	// else we handle the mouse event ourselves
	switch (buttons)
	{
		case B_SECONDARY_MOUSE_BUTTON:
		{
			showContextMenu(point);
			break;
		}
		default:
		{
			DiagramEndPoint::MouseDown(point, buttons, clicks);
		}
	}
}

void MediaJack::MouseOver(
	BPoint point,
	uint32 transit)
{
	D_MOUSE(("MediaJack::MouseOver()\n"));
	switch (transit)
	{
		case B_ENTERED_VIEW:
		{
			be_app->SetCursor(M_CABLE_CURSOR);
			TipManager *tips = TipManager::Instance();
			BString tipText = m_label.String();
			tipText << " (" << MediaString::getStringFor(m_format.type) << ")";
			tips->showTip(tipText.String(),view()->ConvertToScreen(Frame()),
						  TipManager::LEFT_OFFSET_FROM_POINTER, BPoint(12.0, 8.0));
			break;
		}
		case B_EXITED_VIEW:
		{
			if (!view()->isWireTracking())
			{
				be_app->SetCursor(B_HAND_CURSOR);
			}
			break;
		}
	}
}

void MediaJack::MessageDragged(
	BPoint point,
	uint32 transit,
	const BMessage *message)
{
	D_MOUSE(("MediaJack::MessageDragged()\n"));
	switch (transit)
	{
		case B_ENTERED_VIEW:
		{
			be_app->SetCursor(M_CABLE_CURSOR);
			break;
		}
		case B_EXITED_VIEW:
		{
			if (!view()->isWireTracking())
			{
				be_app->SetCursor(B_HAND_CURSOR);
			}
			break;
		}
	}
	DiagramEndPoint::MessageDragged(point, transit, message);
}

void MediaJack::selected()
{
	D_METHOD(("MediaJack::selected()\n"));
	_updateBitmap();
	view()->Invalidate(Frame());
}

void MediaJack::deselected()
{
	D_METHOD(("MediaJack::deselected()\n"));
	_updateBitmap();
	view()->Invalidate(Frame());
}

void MediaJack::connected()
{
	D_METHOD(("MediaJack::connected()\n"));
	_updateBitmap();
	view()->Invalidate(Frame());
}

void MediaJack::disconnected()
{
	D_METHOD(("MediaJack::disconnected()\n"));
	_updateBitmap();
	view()->Invalidate(Frame());
}

// -------------------------------------------------------- //
// *** operations (public)
// -------------------------------------------------------- //

void MediaJack::layoutChanged(
	int32 layout)
{
	D_METHOD(("MediaJack::layoutChanged\n"));
	resizeTo(M_DEFAULT_WIDTH, M_DEFAULT_HEIGHT);
	_updateBitmap();
}

void MediaJack::setPosition(
	float offset,
	float leftTopBoundary,
	float rightBottomBoundary,
	BRegion *updateRegion)
{
	D_METHOD(("MediaJack::setPosition\n"));
	switch (dynamic_cast<MediaRoutingView *>(view())->getLayout())
	{
		case MediaRoutingView::M_ICON_VIEW:
		{
			if (isInput())
			{
				moveTo(BPoint(leftTopBoundary, offset), updateRegion);
			}
			else if (isOutput())
			{
				moveTo(BPoint(rightBottomBoundary - Frame().Width(), offset), updateRegion);
			}
			break;
		}
		case MediaRoutingView::M_MINI_ICON_VIEW:
		{
			if (isInput())
			{
				moveTo(BPoint(offset, leftTopBoundary), updateRegion);
			}
			else if (isOutput())
			{
				moveTo(BPoint(offset, rightBottomBoundary - Frame().Height()), updateRegion);
			}
			break;
		}
	}
}

// -------------------------------------------------------- //
// *** internal methods (private)
// -------------------------------------------------------- //

void MediaJack::_updateBitmap()
{
	D_METHOD(("MediaJack::_updateBitmap()\n"));

	if (m_bitmap)
	{
		delete m_bitmap;
	}
	BBitmap *tempBitmap = new BBitmap(Frame().OffsetToCopy(0.0, 0.0), B_CMAP8, true);
	tempBitmap->Lock();
	{
		BView *tempView = new BView(tempBitmap->Bounds(), "", B_FOLLOW_NONE, 0);
		tempBitmap->AddChild(tempView);
		tempView->SetOrigin(0.0, 0.0);

		MediaRoutingView* mediaView = dynamic_cast<MediaRoutingView*>(view());
		int32 layout = mediaView ? mediaView->getLayout() : MediaRoutingView::M_ICON_VIEW;

		_drawInto(tempView, tempView->Bounds(), layout);

		tempView->Sync();
		tempBitmap->RemoveChild(tempView);
		delete tempView;
	}
	tempBitmap->Unlock();
	m_bitmap = new BBitmap(tempBitmap);
	delete tempBitmap;
}

void MediaJack::_drawInto(
	BView *target,
	BRect targetRect,
	int32 layout)
{
	D_METHOD(("MediaJack::_drawInto()\n"));

	bool selected = isConnecting() || isSelected();
	switch (layout)
	{
		case MediaRoutingView::M_ICON_VIEW:
		{
			if (isInput())
			{
				BRect r;
				BPoint p;

				// fill rect
				r = targetRect;
				target->SetLowColor(M_GRAY_COLOR);
				r.left += 2.0;
				target->FillRect(r, B_SOLID_LOW);

				// draw connection point
				r = targetRect;
				p.Set(0.0, Frame().Height() / 2.0 - 2.0);
				target->BeginLineArray(4);
				{
					target->AddLine(r.LeftTop(),
									p,
									M_DARK_GRAY_COLOR);
					target->AddLine(r.LeftTop() + BPoint(1.0, 0.0),
									p + BPoint(1.0, 0.0),
									M_LIGHT_GRAY_COLOR);
					target->AddLine(p + BPoint(0.0, 5.0),
									r.LeftBottom(),
									M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 5.0),
									r.LeftBottom() + BPoint(1.0, 0.0),
									M_LIGHT_GRAY_COLOR);
				}
				target->EndLineArray();

				if (isConnected() || isConnecting())
				{
					target->BeginLineArray(11);
					{
						target->AddLine(p, p, M_DARK_GRAY_COLOR);
						target->AddLine(p + BPoint(0.0, 4.0), p + BPoint(0.0, 4.0), M_DARK_GRAY_COLOR);
						target->AddLine(p + BPoint(1.0, 0.0), p + BPoint(4.0, 0.0), M_DARK_GRAY_COLOR);
						target->AddLine(p + BPoint(1.0, 4.0), p + BPoint(4.0, 4.0), M_LIGHT_GRAY_COLOR);
						target->AddLine(p + BPoint(4.0, 1.0), p + BPoint(4.0, 3.0), M_LIGHT_GRAY_COLOR);
						target->AddLine(p + BPoint(0.0, 1.0), p + BPoint(2.0, 1.0), selected ? M_BLUE_COLOR : M_DARK_GRAY_COLOR);
						target->AddLine(p + BPoint(3.0, 1.0), p + BPoint(3.0, 1.0), M_MED_GRAY_COLOR);
						target->AddLine(p + BPoint(0.0, 2.0), p + BPoint(2.0, 2.0), selected ? M_LIGHT_BLUE_COLOR : M_LIGHT_GRAY_COLOR);
						target->AddLine(p + BPoint(3.0, 2.0), p + BPoint(3.0, 2.0), selected ? M_BLUE_COLOR : M_DARK_GRAY_COLOR);
						target->AddLine(p + BPoint(0.0, 3.0), p + BPoint(2.0, 3.0), selected ? M_BLUE_COLOR : M_DARK_GRAY_COLOR);
						target->AddLine(p + BPoint(3.0, 3.0), p + BPoint(3.0, 3.0), M_MED_GRAY_COLOR);
					}
					target->EndLineArray();
				}
				else
				{
					target->BeginLineArray(7);
					{
						target->AddLine(p, p + BPoint(0.0, 4.0), M_DARK_GRAY_COLOR);
						target->AddLine(p + BPoint(1.0, 0.0), p + BPoint(4.0, 0.0), M_DARK_GRAY_COLOR);
						target->AddLine(p + BPoint(1.0, 4.0), p + BPoint(4.0, 4.0), M_LIGHT_GRAY_COLOR);
						target->AddLine(p + BPoint(4.0, 1.0), p + BPoint(4.0, 3.0), M_LIGHT_GRAY_COLOR);
						target->AddLine(p + BPoint(1.0, 1.0), p + BPoint(3.0, 1.0), M_MED_GRAY_COLOR);
						target->AddLine(p + BPoint(1.0, 2.0), p + BPoint(3.0, 2.0), M_MED_GRAY_COLOR);
						target->AddLine(p + BPoint(1.0, 3.0), p + BPoint(3.0, 3.0), M_MED_GRAY_COLOR);
					}
					target->EndLineArray();
				}

				// draw abbreviation string
				BFont font(be_plain_font);
				font_height fh;
				font.SetSize(font.Size() - 2.0);
				font.GetHeight(&fh);
				p.x += 7.0;
				p.y = (Frame().Height() / 2.0) + (fh.ascent / 2.0);
				target->SetFont(&font);
				target->SetDrawingMode(B_OP_OVER);
				target->SetHighColor((isConnected() || isConnecting()) ?
									  M_MED_GRAY_COLOR :
									  M_DARK_GRAY_COLOR);
				target->DrawString(m_abbreviation.String(), p);
			}
			else if (isOutput())
			{
				BRect r;
				BPoint p;

				// fill rect
				r = targetRect;
				target->SetLowColor(M_GRAY_COLOR);
				r.right -= 2.0;
				target->FillRect(r, B_SOLID_LOW);

				// draw connection point
				r = targetRect;
				p.Set(targetRect.right - 4.0, Frame().Height() / 2.0 - 2.0);
				target->BeginLineArray(4);
				{
					target->AddLine(r.RightTop(),
									p + BPoint(4.0, 0.0),
									M_DARK_GRAY_COLOR);
					target->AddLine(r.RightTop() + BPoint(-1.0, 0.0),
									p + BPoint(3.0, 0.0),
									M_MED_GRAY_COLOR);
					target->AddLine(p + BPoint(4.0, 5.0),
									r.RightBottom(),
									M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(3.0, 5.0),
									r.RightBottom() + BPoint(-1.0, 0.0),
									M_MED_GRAY_COLOR);
				}
				target->EndLineArray();

				if (isConnected() || isConnecting())
				{
					target->BeginLineArray(11);
					target->AddLine(p + BPoint(4.0, 0.0), p + BPoint(4.0, 0.0), M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(4.0, 4.0), p + BPoint(4.0, 4.0), M_DARK_GRAY_COLOR);
					target->AddLine(p, p + BPoint(3.0, 0.0), M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(0.0, 1.0), p + BPoint(0.0, 3.0), M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(0.0, 4.0), p + BPoint(3.0, 4.0), M_LIGHT_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 1.0), p + BPoint(1.0, 1.0), M_MED_GRAY_COLOR);
					target->AddLine(p + BPoint(2.0, 1.0), p + BPoint(4.0, 1.0), selected ? M_BLUE_COLOR : M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 2.0), p + BPoint(1.0, 2.0), selected ? M_BLUE_COLOR : M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(2.0, 2.0), p + BPoint(4.0, 2.0), selected ? M_LIGHT_BLUE_COLOR : M_LIGHT_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 3.0), p + BPoint(1.0, 3.0), M_MED_GRAY_COLOR);
					target->AddLine(p + BPoint(2.0, 3.0), p + BPoint(4.0, 3.0), selected ? M_BLUE_COLOR : M_DARK_GRAY_COLOR);
					target->EndLineArray();
				}
				else
				{
					target->BeginLineArray(7);
					target->AddLine(p + BPoint(4.0, 0.0), p + BPoint(4.0, 4.0), M_DARK_GRAY_COLOR);
					target->AddLine(p, p + BPoint(3.0, 0.0), M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(0.0, 1.0), p + BPoint(0.0, 3.0), M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(0.0, 4.0), p + BPoint(3.0, 4.0), M_LIGHT_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 1.0), p + BPoint(3.0, 1.0), M_MED_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 2.0), p + BPoint(3.0, 2.0), M_MED_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 3.0), p + BPoint(3.0, 3.0), M_MED_GRAY_COLOR);
					target->EndLineArray();
				}

				// draw abbreviation string
				BFont font(be_plain_font);
				font_height fh;
				font.SetSize(font.Size() - 2.0);
				font.GetHeight(&fh);
				p.x -= font.StringWidth(m_abbreviation.String()) + 2.0;
				p.y = (Frame().Height() / 2.0) + (fh.ascent / 2.0);
				target->SetFont(&font);
				target->SetDrawingMode(B_OP_OVER);
				target->SetHighColor((isConnected() || isConnecting()) ?
									  M_MED_GRAY_COLOR :
									  M_DARK_GRAY_COLOR);
				target->DrawString(m_abbreviation.String(), p);
			}
			break;
		}
		case MediaRoutingView::M_MINI_ICON_VIEW:
		{
			if (isInput())
			{
				BRect r;
				BPoint p;

				// fill rect
				r = targetRect;
				target->SetLowColor(M_GRAY_COLOR);
				r.top += 2.0;
				target->FillRect(r, B_SOLID_LOW);

				// draw connection point
				r = targetRect;
				p.Set(Frame().Width() / 2.0 - 2.0, 0.0);
				target->BeginLineArray(4);
				{
					target->AddLine(r.LeftTop(),
									p,
									M_DARK_GRAY_COLOR);
					target->AddLine(r.LeftTop() + BPoint(0.0, 1.0),
									p + BPoint(0.0, 1.0),
									M_LIGHT_GRAY_COLOR);
					target->AddLine(p + BPoint(5.0, 0.0),
									r.RightTop(),
									M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(5.0, 1.0),
									r.RightTop() + BPoint(0.0, 1.0),
									M_LIGHT_GRAY_COLOR);
				}
				target->EndLineArray();
				if (isConnected() || isConnecting())
				{
					target->BeginLineArray(11);
					target->AddLine(p, p, M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(4.0, 0.0), p + BPoint(4.0, 0.0), M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(0.0, 1.0), p + BPoint(0.0, 4.0), M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(4.0, 1.0), p + BPoint(4.0, 4.0), M_LIGHT_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 4.0), p + BPoint(3.0, 4.0), M_LIGHT_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 0.0), p + BPoint(1.0, 2.0), selected ? M_BLUE_COLOR : M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 3.0), p + BPoint(1.0, 3.0), M_MED_GRAY_COLOR);
					target->AddLine(p + BPoint(2.0, 0.0), p + BPoint(2.0, 2.0), selected ? M_LIGHT_BLUE_COLOR : M_LIGHT_GRAY_COLOR);
					target->AddLine(p + BPoint(2.0, 3.0), p + BPoint(2.0, 3.0), selected ? M_BLUE_COLOR : M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(3.0, 0.0), p + BPoint(3.0, 2.0), selected ? M_BLUE_COLOR : M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(3.0, 3.0), p + BPoint(3.0, 3.0), M_MED_GRAY_COLOR);
					target->EndLineArray();
				}
				else
				{
					target->BeginLineArray(7);
					target->AddLine(p, p + BPoint(4.0, 0.0), M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(0.0, 1.0), p + BPoint(0.0, 4.0), M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(4.0, 1.0), p + BPoint(4.0, 4.0), M_LIGHT_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 4.0), p + BPoint(3.0, 4.0), M_LIGHT_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 1.0), p + BPoint(1.0, 3.0), M_MED_GRAY_COLOR);
					target->AddLine(p + BPoint(2.0, 1.0), p + BPoint(2.0, 3.0), M_MED_GRAY_COLOR);
					target->AddLine(p + BPoint(3.0, 1.0), p + BPoint(3.0, 3.0), M_MED_GRAY_COLOR);
					target->EndLineArray();
				}
			}
			else if (isOutput())
			{
				BRect r = targetRect;
				BPoint p;

				// fill rect
				r = targetRect;
				target->SetLowColor(M_GRAY_COLOR);
				r.bottom -= 2.0;
				target->FillRect(r, B_SOLID_LOW);

				// draw connection point
				r = targetRect;
				p.Set(Frame().Width() / 2.0 - 2.0, targetRect.bottom - 4.0);
				target->BeginLineArray(4);
				{
					target->AddLine(r.LeftBottom(),
									p + BPoint(0.0, 4.0),
									M_DARK_GRAY_COLOR);
					target->AddLine(r.LeftBottom() + BPoint(0.0, -1.0),
									p + BPoint(0.0, 3.0),
									M_MED_GRAY_COLOR);
					target->AddLine(p + BPoint(5.0, 4.0),
									r.RightBottom(),
									M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(5.0, 3.0),
									r.RightBottom() + BPoint(0.0, -1.0),
									M_MED_GRAY_COLOR);
				}
				target->EndLineArray();
				if (isConnected() || isConnecting())
				{
					target->BeginLineArray(11);
					target->AddLine(p + BPoint(0.0, 4.0), p + BPoint(0.0, 4.0), M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(4.0, 4.0), p + BPoint(4.0, 4.0), M_DARK_GRAY_COLOR);
					target->AddLine(p, p + BPoint(0.0, 3.0), M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 0.0), p + BPoint(3.0, 0.0), M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(4.0, 0.0), p + BPoint(4.0, 3.0), M_LIGHT_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 1.0), p + BPoint(1.0, 1.0), M_MED_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 2.0), p + BPoint(1.0, 4.0), selected ? M_BLUE_COLOR : M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(2.0, 1.0), p + BPoint(2.0, 1.0), selected ? M_BLUE_COLOR : M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(2.0, 2.0), p + BPoint(2.0, 4.0), selected ? M_LIGHT_BLUE_COLOR : M_LIGHT_GRAY_COLOR);
					target->AddLine(p + BPoint(3.0, 1.0), p + BPoint(3.0, 1.0), M_MED_GRAY_COLOR);
					target->AddLine(p + BPoint(3.0, 2.0), p + BPoint(3.0, 4.0), selected ? M_BLUE_COLOR : M_DARK_GRAY_COLOR);
					target->EndLineArray();
				}
				else
				{
					target->BeginLineArray(7);
					target->AddLine(p + BPoint(0.0, 4.0), p + BPoint(4.0, 4.0), M_DARK_GRAY_COLOR);
					target->AddLine(p, p + BPoint(0.0, 3.0), M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 0.0), p + BPoint(3.0, 0.0), M_DARK_GRAY_COLOR);
					target->AddLine(p + BPoint(4.0, 0.0), p + BPoint(4.0, 3.0), M_LIGHT_GRAY_COLOR);
					target->AddLine(p + BPoint(1.0, 1.0), p + BPoint(1.0, 3.0), M_MED_GRAY_COLOR);
					target->AddLine(p + BPoint(2.0, 1.0), p + BPoint(2.0, 3.0), M_MED_GRAY_COLOR);
					target->AddLine(p + BPoint(3.0, 1.0), p + BPoint(3.0, 3.0), M_MED_GRAY_COLOR);
					target->EndLineArray();
				}
			}
			break;
		}
	}
}

void MediaJack::_updateAbbreviation()
{
	D_METHOD(("MediaJack::_updateAbbreviation()\n"));

	int32 offset;
	m_abbreviation = "";
	m_abbreviation += m_label[0];
	offset = m_label.FindFirst(" ") + 1;
	if ((offset > 1) && (offset < m_label.CountChars() - 1))
		m_abbreviation += m_label[offset];
	else
		m_abbreviation += m_label[1];
	offset = m_label.CountChars() - 1;
	m_abbreviation += m_label[offset];
}

// -------------------------------------------------------- //
// *** internal operations (protected)
// -------------------------------------------------------- //

void MediaJack::showContextMenu(
	BPoint point)
{
	D_METHOD(("MediaJack::showContextMenu()\n"));

	BPopUpMenu *menu = new BPopUpMenu("MediaJack PopUp", false, false, B_ITEMS_IN_COLUMN);
	menu->SetFont(be_plain_font);
	BMenuItem *item;

	// add the "Get Info" item
	if (isInput())
	{
		media_input input;
		getInput(&input);
		BMessage *message = new BMessage(InfoWindowManager::M_INFO_WINDOW_REQUESTED);
		message->AddData("input", B_RAW_TYPE,
						 reinterpret_cast<const void *>(&input), sizeof(input));
		menu->AddItem(item = new BMenuItem("Get info", message));
	}
	else if (isOutput())
	{
		media_output output;
		getOutput(&output);
		BMessage *message = new BMessage(InfoWindowManager::M_INFO_WINDOW_REQUESTED);
		message->AddData("output", B_RAW_TYPE,
						 reinterpret_cast<const void *>(&output), sizeof(output));
		menu->AddItem(item = new BMenuItem("Get info", message));
	}

	menu->SetTargetForItems(view());
	view()->ConvertToScreen(&point);
	point -= BPoint(1.0, 1.0);
	menu->Go(point, true, true, true);
}

// -------------------------------------------------------- //
// *** sorting methods (friend)
// -------------------------------------------------------- //

int __CORTEX_NAMESPACE__ compareTypeAndID(
	const void *lValue,
	const void *rValue)
{
	int retValue = 0;
	const MediaJack *lJack = *(reinterpret_cast<MediaJack * const*>(reinterpret_cast<void * const*>(lValue)));
	const MediaJack *rJack = *(reinterpret_cast<MediaJack * const*>(reinterpret_cast<void * const*>(rValue)));
	if (lJack && rJack)
	{
		if (lJack->m_jackType < lJack->m_jackType)
		{
			return -1;
		}
		if (lJack->m_jackType == lJack->m_jackType)
		{
			if (lJack->m_index < rJack->m_index)
			{
				return -1;
			}
			else
			{
				return 1;
			}
		}
		else if (lJack->m_jackType > rJack->m_jackType)
		{
			retValue = 1;
		}
	}
	return retValue;
}

// END -- LiveNodeView.cpp --
