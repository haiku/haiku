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


// MediaWire.cpp

#include "MediaWire.h"
// InfoWindow
#include "InfoWindowManager.h"
// MediaRoutingView
#include "MediaJack.h"
#include "MediaRoutingDefs.h"
#include "MediaRoutingView.h"
// Support
#include "cortex_ui.h"
#include "MediaString.h"
// TipManager
#include "TipManager.h"

// Application Kit
#include <Application.h>
// Interface Kit
#include <MenuItem.h>
#include <PopUpMenu.h>
// Media Kit
#include <MediaDefs.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)
#define D_DRAW(x) //PRINT (x)
#define D_MOUSE(x) //PRINT (x)

// -------------------------------------------------------- //
// constants
// -------------------------------------------------------- //

const float MediaWire::M_WIRE_OFFSET		= 4.0;

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

MediaWire::MediaWire(
	Connection connection,
	MediaJack *outputJack,
	MediaJack *inputJack)
	: DiagramWire(outputJack, inputJack),
	  connection(connection)
{
	D_METHOD(("MediaWire::MediaWire()\n"));
}

MediaWire::MediaWire(
	MediaJack *jack,
	bool isStartPoint)
	: DiagramWire(jack, isStartPoint)
{
	D_METHOD(("MediaWire::MediaWire(temp)\n"));
}

MediaWire::~MediaWire()
{
	D_METHOD(("MediaWire::~MediaWire()\n"));
}

// -------------------------------------------------------- //
// *** derived from DiagramWire (public)
// -------------------------------------------------------- //

void MediaWire::attachedToDiagram()
{
	D_METHOD(("MediaWire::detachedFromDiagram()\n"));

	endPointMoved(startPoint());
	endPointMoved(endPoint());
}

void MediaWire::detachedFromDiagram()
{
	D_METHOD(("MediaWire::detachedFromDiagram()\n"));

	// make sure we're no longer displaying a tooltip
	TipManager *tips = TipManager::Instance();
	tips->hideTip(view()->ConvertToScreen(Frame()));
}

BRect MediaWire::Frame() const
{
	D_DRAW(("MediaWire::Frame()\n"));
	return m_frame;
}

float MediaWire::howCloseTo(
	BPoint point) const
{
	D_MOUSE(("MediaWire::howCloseTo()\n"));
	if (Frame().Contains(point))
	{
		BPoint sp = m_startPoint;
		BPoint ep = m_endPoint;
		BPoint so = m_startOffset;
		BPoint eo = m_endOffset;
		BRect wireFrame, startFrame, endFrame;
		wireFrame.left = so.x < eo.x ? so.x : eo.x;
		wireFrame.top = so.y < eo.y ? so.y : eo.y;
		wireFrame.right = so.x > eo.x ? so.x : eo.x;
		wireFrame.bottom = so.y > eo.y ? so.y : eo.y;
		startFrame.Set(sp.x, sp.y, so.x, so.y);
		endFrame.Set(ep.x, ep.y, eo.x, eo.y);
		wireFrame.InsetBy(-1.0, -1.0);
		startFrame.InsetBy(-1.0, -1.0);
		endFrame.InsetBy(-1.0, -1.0);
		if ((wireFrame.Width() <= 5.0) || (wireFrame.Height() <= 5.0) || startFrame.Contains(point) || endFrame.Contains(point))
		{
			return 1.0;
		}
		else
		{
			float length, result;
			length = sqrt(pow(eo.x - so.x, 2) + pow(eo.y - so.y, 2));
 			result = ((so.y - point.y) * (eo.x - so.x)) - ((so.x - point.x) * (eo.y - so.y));
			result = 3.0 - fabs(result / length);
			return result;
		}
	}
	return 0.0;
}

void MediaWire::drawWire()
{
	D_DRAW(("MediaWire::drawWire()\n"));

	rgb_color border = isSelected() ? M_BLUE_COLOR : M_DARK_GRAY_COLOR;
	rgb_color fill = isSelected() ? M_LIGHT_BLUE_COLOR : M_LIGHT_GRAY_COLOR;
	view()->SetPenSize(3.0);
	view()->BeginLineArray(3);
		view()->AddLine(m_startPoint, m_startOffset, border);
		view()->AddLine(m_startOffset, m_endOffset, border);
		view()->AddLine(m_endOffset, m_endPoint, border);
	view()->EndLineArray();
	view()->SetPenSize(1.0);
	view()->BeginLineArray(3);
		view()->AddLine(m_startPoint, m_startOffset, fill);
		view()->AddLine(m_startOffset, m_endOffset, fill);
		view()->AddLine(m_endOffset, m_endPoint, fill);
	view()->EndLineArray();
}

void MediaWire::MouseDown(
	BPoint point,
	uint32 buttons,
	uint32 clicks)
{
	D_MOUSE(("MediaWire::MouseDown()\n"));
	_inherited::MouseDown(point, buttons, clicks);

	switch (buttons)
	{
		case B_SECONDARY_MOUSE_BUTTON:
		{
			if (clicks == 1)
			{
				showContextMenu(point);
			}
		}
	}
}

void MediaWire::MouseOver(
	BPoint point,
	uint32 transit)
{
	D_MOUSE(("MediaWire::MouseOver()\n"));

	if (isDragging())
	{
		return;
	}
	switch (transit)
	{
		case B_ENTERED_VIEW:
		{
			TipManager *tips = TipManager::Instance();
			BString tipText = MediaString::getStringFor(connection.format(), false);
			tips->showTip(tipText.String(), view()->ConvertToScreen(Frame()), 
						  TipManager::LEFT_OFFSET_FROM_POINTER, BPoint(12.0, 8.0));
			be_app->SetCursor(M_CABLE_CURSOR);
			break;
		}
		case B_EXITED_VIEW:
		{
			be_app->SetCursor(B_HAND_CURSOR);
			TipManager *tips = TipManager::Instance();
			tips->hideTip(view()->ConvertToScreen(Frame()));
			break;
		}
	}
}

void MediaWire::selected()
{
	D_METHOD(("MediaWire::selected()\n"));
	if (startPoint())
	{
		MediaJack *outputJack = static_cast<MediaJack *>(startPoint());
		outputJack->select();
	}
	if (endPoint())
	{
		MediaJack *inputJack = static_cast<MediaJack *>(endPoint());
		inputJack->select();
	}
}

void MediaWire::deselected()
{
	D_METHOD(("MediaWire::deselected()\n"));
	if (startPoint())
	{
		MediaJack *outputJack = static_cast<MediaJack *>(startPoint());
		outputJack->deselect();
	}
	if (endPoint())
	{
		MediaJack *inputJack = static_cast<MediaJack *>(endPoint());
		inputJack->deselect();
	}
}

void MediaWire::endPointMoved(
	DiagramEndPoint *which)
{
	if (which == startPoint())
	{
		m_startPoint = startConnectionPoint();
		switch (dynamic_cast<MediaRoutingView *>(view())->getLayout())
		{
			case MediaRoutingView::M_ICON_VIEW:
			{
				m_startOffset = m_startPoint + BPoint(M_WIRE_OFFSET, 0.0);
				break;
			}
			case MediaRoutingView::M_MINI_ICON_VIEW:
			{
				m_startOffset = m_startPoint + BPoint(0.0, M_WIRE_OFFSET);
				break;
			}
		}
		m_frame.left = m_startPoint.x < m_endOffset.x ? m_startPoint.x - 2.0: m_endOffset.x - 2.0;
		m_frame.top = m_startPoint.y < m_endOffset.y ? m_startPoint.y - 2.0 : m_endOffset.y - 2.0;
		m_frame.right = m_startOffset.x > m_endPoint.x ? m_startOffset.x + 2.0 : m_endPoint.x + 2.0;
		m_frame.bottom = m_startOffset.y > m_endPoint.y ? m_startOffset.y + 2.0 : m_endPoint.y + 2.0;
	}
	else if (which == endPoint())
	{
		m_endPoint = endConnectionPoint();
		switch (dynamic_cast<MediaRoutingView *>(view())->getLayout())
		{
			case MediaRoutingView::M_ICON_VIEW:
			{
				m_endOffset = m_endPoint - BPoint(M_WIRE_OFFSET, 0.0);
				break;
			}
			case MediaRoutingView::M_MINI_ICON_VIEW:
			{
				m_endOffset = m_endPoint - BPoint(0.0, M_WIRE_OFFSET);
				break;
			}
		}
		m_frame.left = m_startPoint.x < m_endOffset.x ? m_startPoint.x - 2.0: m_endOffset.x - 2.0;
		m_frame.top = m_startPoint.y < m_endOffset.y ? m_startPoint.y - 2.0 : m_endOffset.y - 2.0;
		m_frame.right = m_startOffset.x > m_endPoint.x ? m_startOffset.x + 2.0 : m_endPoint.x + 2.0;
		m_frame.bottom = m_startOffset.y > m_endPoint.y ? m_startOffset.y + 2.0 : m_endPoint.y + 2.0;
	}
}

// -------------------------------------------------------- //
// *** internal operations (protected)
// -------------------------------------------------------- //

void MediaWire::showContextMenu(
	BPoint point)
{
	D_METHOD(("MediaWire::showContextMenu()\n"));

	BPopUpMenu *menu = new BPopUpMenu("MediaWire PopUp", false, false, B_ITEMS_IN_COLUMN);
	menu->SetFont(be_plain_font);
	BMenuItem *item;

	// add the "Get Info" item
	media_output output;
	connection.getOutput(&output);
	BMessage *message = new BMessage(InfoWindowManager::M_INFO_WINDOW_REQUESTED);
	message->AddData("connection", B_RAW_TYPE, 
					 reinterpret_cast<const void *>(&output), sizeof(output));
	menu->AddItem(item = new BMenuItem("Get info", message, 'I'));

	// add the "Disconnect" item
	menu->AddItem(item = new BMenuItem("Disconnect", new BMessage(MediaRoutingView::M_DELETE_SELECTION), 'T'));
	if (connection.flags() & Connection::LOCKED)
	{
		item->SetEnabled(false);
	}

	menu->SetTargetForItems(view());
	view()->ConvertToScreen(&point);
	point -= BPoint(1.0, 1.0);
	menu->Go(point, true, true, true);
}

// END -- MediaWire.cpp --
