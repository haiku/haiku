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


// MediaNodePanel.cpp
// c.lenz 10oct99

#include "MediaNodePanel.h"
// InfoWindow
#include "InfoWindowManager.h"
// MediaRoutingView
#include "MediaRoutingView.h"
#include "MediaWire.h"
#include "RouteAppNodeManager.h"
// NodeManager
#include "NodeRef.h"
#include "NodeGroup.h"
// ParameterWindow
#include "ParameterWindow.h" 
// Support
#include "cortex_ui.h"
#include "MediaIcon.h"
#include "MediaString.h"
// RouteApp
#include "RouteWindow.h"
// TipManager
#include "TipManager.h"

// App Kit
#include <Application.h>
#include <Roster.h>
// Interface Kit
#include <MenuItem.h>
#include <PopUpMenu.h>
// Media Kit
#include <MediaDefs.h>
#include <MediaRoster.h>

using namespace std;

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)
#define D_MESSAGE(x) //PRINT (x)
#define D_DRAW(x) //PRINT (x)

// -------------------------------------------------------- //
// constants
// -------------------------------------------------------- //

float	MediaNodePanel::M_DEFAULT_WIDTH		= 90.0;
float	MediaNodePanel::M_DEFAULT_HEIGHT	= 60.0;
float	MediaNodePanel::M_LABEL_H_MARGIN	= 3.0;
float	MediaNodePanel::M_LABEL_V_MARGIN	= 3.0;
float	MediaNodePanel::M_BODY_H_MARGIN		= 5.0;
float	MediaNodePanel::M_BODY_V_MARGIN		= 5.0;

// [e.moon 7dec99]
const BPoint MediaNodePanel::s_invalidPosition(-200.0, -200.0);

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

MediaNodePanel::MediaNodePanel(
	BPoint position,
	NodeRef *nodeRef)
	: DiagramBox(BRect(position, position + BPoint(M_DEFAULT_WIDTH, M_DEFAULT_HEIGHT))),
	  BHandler(nodeRef->name()),
	  ref(nodeRef),
	  m_bitmap(0),
	  m_icon(0),
	  m_alternatePosition(s_invalidPosition)
{
	D_METHOD(("MediaNodePanel::MediaNodePanel()\n"));
	ASSERT(ref);
}

MediaNodePanel::~MediaNodePanel()
{
	D_METHOD(("MediaNodePanel::~MediaNodePanel()\n"));
	if (m_icon)
	{
		delete m_icon;
	}
	if (m_bitmap)
	{
		delete m_bitmap;
	}
}

// -------------------------------------------------------- //
// *** derived from DiagramBox
// -------------------------------------------------------- //

void MediaNodePanel::attachedToDiagram()
{
	D_METHOD(("MediaNodePanel::attachedToDiagram()\n"));

	resizeTo(M_DEFAULT_WIDTH, M_DEFAULT_HEIGHT);
	_updateIcon(dynamic_cast<MediaRoutingView *>(view())->getLayout());
	_prepareLabel();
	populateInit();
	arrangeIOJacks();

	view()->Looper()->AddHandler(this);
}

void MediaNodePanel::detachedFromDiagram()
{
	D_METHOD(("MediaNodePanel::detachedFromDiagram()\n"));

	BRect labelRect = m_labelRect.OffsetByCopy(Frame().LeftTop());
	if (m_mouseOverLabel && m_labelTruncated)
	{
		TipManager *tips = TipManager::Instance();
		tips->hideTip(view()->ConvertToScreen(labelRect));
	}

	view()->Looper()->RemoveHandler(this);
}

void MediaNodePanel::DrawBox()
{
	D_DRAW(("MediaNodePanel::DrawBox()\n"));
	if (m_bitmap)
	{
		view()->DrawBitmap(m_bitmap, Frame().LeftTop());
	}
}

void MediaNodePanel::MouseDown(
	BPoint point,
	uint32 buttons,
	uint32 clicks)
{
	D_METHOD(("MediaNodePanel::MouseDown()\n"));

	_inherited::MouseDown(point, buttons, clicks);

	// +++ REALLY BAD WORKAROUND
	MediaJack *jack = dynamic_cast<MediaJack *>(_LastItemUnder());
	if (jack && jack->Frame().Contains(point))
	{
		return;
	}

	switch (buttons) {
		case B_PRIMARY_MOUSE_BUTTON:
		{
			if (clicks == 2) {
				if (ref->kind() & B_CONTROLLABLE) {
					BMessage message(MediaRoutingView::M_NODE_TWEAK_PARAMETERS);
					DiagramView* v = view();
					BMessenger(v).SendMessage(&message);
				}
			}
			break;
		}
		case B_SECONDARY_MOUSE_BUTTON:
		{
			if (clicks == 1) {
				showContextMenu(point);
			}
			break;
		}
	}
}

void MediaNodePanel::MouseOver(
	BPoint point,
	uint32 transit)
{
	D_METHOD(("MediaNodePanel::MouseOver()\n"));
	_inherited::MouseOver(point, transit);

	switch (transit)
	{
		case B_ENTERED_VIEW:
		{
			break;
		}
		case B_INSIDE_VIEW:
		{
			BRect labelRect = m_labelRect.OffsetByCopy(Frame().LeftTop());
			if (labelRect.Contains(point))
			{
				if (!m_mouseOverLabel && m_labelTruncated)
				{
					TipManager *tips = TipManager::Instance();
					tips->showTip(m_fullLabel.String(), view()->ConvertToScreen(labelRect));
					m_mouseOverLabel = true;
				}
			}
			else
			{
				m_mouseOverLabel = false;
			}
			break;
		}
		case B_EXITED_VIEW:
		{
			m_mouseOverLabel = false;
			break;
		}
	}
}

void MediaNodePanel::MessageDropped(
	BPoint point,
	BMessage *message)
{
	D_METHOD(("MediaNodePanel::MessageDropped()\n"));

	// +++ REALLY BAD WORKAROUND
	MediaJack *jack = dynamic_cast<MediaJack *>(ItemUnder(point));
	if (jack)
	{
		jack->MessageDropped(point, message);
		return;
	}
	else
	{
		be_app->SetCursor(B_HAND_CURSOR);
	}
}

void MediaNodePanel::selected()
{
	D_METHOD(("MediaNodePanel::selected()\n"));
	_updateBitmap();
}

void MediaNodePanel::deselected()
{
	D_METHOD(("MediaNodePanel::deselected()\n"));
	_updateBitmap();
}

// ---------------------------------------------------------------- //
// *** updating
// ---------------------------------------------------------------- //

void MediaNodePanel::layoutChanged(
	int32 layout)
{
	D_METHOD(("MediaNodePanel::layoutChanged()\n"));

	BPoint p = Frame().LeftTop();
	if (m_alternatePosition == s_invalidPosition)
	{
		m_alternatePosition = dynamic_cast<MediaRoutingView *>
							  (view())->findFreePositionFor(this);
	}
	moveTo(m_alternatePosition);
	m_alternatePosition = p;

	resizeTo(M_DEFAULT_WIDTH, M_DEFAULT_HEIGHT);
	for (uint32 i = 0; i < CountItems(); i++)
	{
		MediaJack *jack = dynamic_cast<MediaJack *>(ItemAt(i));
		jack->layoutChanged(layout);
	}
	_updateIcon(layout);
	_prepareLabel();
	arrangeIOJacks();
	_updateBitmap();
}

void MediaNodePanel::populateInit()
{
	D_METHOD(("MediaNodePanel::populateInit()\n"));
	if (ref->kind() & B_BUFFER_CONSUMER)
	{
		vector<media_input> freeInputs;
		ref->getFreeInputs(freeInputs);
		for (uint32 i = 0; i < freeInputs.size(); i++)
		{
			AddItem(new MediaJack(freeInputs[i]));
		}
	}
	if (ref->kind() & B_BUFFER_PRODUCER)
	{
		vector<media_output> freeOutputs;
		ref->getFreeOutputs(freeOutputs);
		for (uint32 i = 0; i < freeOutputs.size(); i++)
		{
			AddItem(new MediaJack(freeOutputs[i]));
		}
	}
}

void MediaNodePanel::updateIOJacks()
{
	D_METHOD(("MediaNodePanel::updateIOJacks()\n"));

	// remove all free inputs/outputs, they may be outdated
	for (uint32 i = 0; i < CountItems(); i++)
	{
		MediaJack *jack = dynamic_cast<MediaJack *>(ItemAt(i));
		if (jack && !jack->isConnected())
		{
			RemoveItem(jack);
			delete jack;
			i--; // account for reindexing in the BList
		}
	}

	// add free inputs
	if (ref->kind() & B_BUFFER_CONSUMER)
	{
		vector<media_input> freeInputs;
		ref->getFreeInputs(freeInputs);
		for (uint32 i = 0; i < freeInputs.size(); i++)
		{
			AddItem(new MediaJack(freeInputs[i]));
		}
	}

	// add free outputs
	if (ref->kind() & B_BUFFER_PRODUCER)
	{
		vector<media_output> freeOutputs;
		ref->getFreeOutputs(freeOutputs);
		for (uint32 i = 0; i < freeOutputs.size(); i++)
		{
			AddItem(new MediaJack(freeOutputs[i]));
		}
	}

	// the supported media types might have changed -> this could
	// require changing the icon
	_updateIcon(dynamic_cast<MediaRoutingView *>(view())->getLayout());
}

void MediaNodePanel::arrangeIOJacks()
{
	D_METHOD(("MediaNodePanel::arrangeIOJacks()\n"));
	SortItems(DiagramItem::M_ENDPOINT, &compareTypeAndID);

	switch (dynamic_cast<MediaRoutingView *>(view())->getLayout())
	{
		case MediaRoutingView::M_ICON_VIEW:
		{
			BRegion updateRegion;
			float align = 1.0;
			view()->GetItemAlignment(0, &align);

			// adjust this panel's size
			int32 numInputs = 0, numOutputs = 0;
			for (uint32 i = 0; i < CountItems(); i++)
			{
				MediaJack *jack = dynamic_cast<MediaJack *>(ItemAt(i));
				if (jack)
				{
					if (jack->isInput())
					{
						numInputs++;
					}
					if (jack->isOutput())
					{
						numOutputs++;
					}
				}
			}
			float minHeight = MediaJack::M_DEFAULT_HEIGHT + MediaJack::M_DEFAULT_GAP;
			minHeight *= numInputs > numOutputs ? numInputs : numOutputs;
			minHeight += m_labelRect.Height();
			minHeight += 2 * MediaJack::M_DEFAULT_GAP;
			minHeight = ((int)minHeight / (int)align) * align + align;
			if ((Frame().Height() < minHeight)
			 || ((Frame().Height() > minHeight)
			 && (minHeight >= MediaNodePanel::M_DEFAULT_HEIGHT)))
			{
				updateRegion.Include(Frame());
				resizeTo(Frame().Width(), minHeight);
				updateRegion.Include(Frame());
				_prepareLabel();		
			}

			// adjust the placement of the jacks
			BRect r = m_bodyRect;
			r.bottom -= M_BODY_V_MARGIN;
			float inputOffset = 0.0, outputOffset = 0.0;
			float center = Frame().top + r.top + (r.Height() / 2.0);
			center += MediaJack::M_DEFAULT_GAP - (MediaJack::M_DEFAULT_HEIGHT / 2.0);
			center = ((int)center / (int)align) * align;
			if (numInputs)
			{
				if (numInputs % 2) // odd number of inputs
				{
					inputOffset = center - (numInputs / 2) * (MediaJack::M_DEFAULT_HEIGHT + MediaJack::M_DEFAULT_GAP);
				}
				else // even number of inputs
				{
					inputOffset = center - ((numInputs + 1) / 2) * (MediaJack::M_DEFAULT_HEIGHT + MediaJack::M_DEFAULT_GAP);
				}
			}
			if (numOutputs)
			{
				if (numOutputs % 2) // odd number of outputs
				{
					outputOffset = center - (numOutputs / 2) * (MediaJack::M_DEFAULT_HEIGHT + MediaJack::M_DEFAULT_GAP);
				}
				else // even number of outputs
				{
					outputOffset = center - ((numOutputs + 1) / 2) * (MediaJack::M_DEFAULT_HEIGHT + MediaJack::M_DEFAULT_GAP);
				}
			}
			for (uint32 i = 0; i < CountItems(); i++)
			{
				MediaJack *jack = dynamic_cast<MediaJack *>(ItemAt(i));
				if (jack)
				{
					if (jack->isInput())
					{
						jack->setPosition(inputOffset, Frame().left, Frame().right, &updateRegion);
						inputOffset += jack->Frame().Height() + MediaJack::M_DEFAULT_GAP;
					}
					if (jack->isOutput())
					{
						jack->setPosition(outputOffset, Frame().left, Frame().right, &updateRegion);
						outputOffset += jack->Frame().Height() + MediaJack::M_DEFAULT_GAP;
					}
				}
			}
			for (int32 i = 0; i < updateRegion.CountRects(); i++)
			{
				view()->Invalidate(updateRegion.RectAt(i));
			}
			break;
		}
		case MediaRoutingView::M_MINI_ICON_VIEW:
		{
			BRegion updateRegion;
			float align = 1.0;
			view()->GetItemAlignment(&align, 0);

			// adjust this panel's size
			int32 numInputs = 0, numOutputs = 0;
			for (uint32 i = 0; i < CountItems(); i++)
			{
				MediaJack *jack = dynamic_cast<MediaJack *>(ItemAt(i));
				if (jack)
				{
					if (jack->isInput())
					{
						numInputs++;
					}
					if (jack->isOutput())
					{
						numOutputs++;
					}
				}
			}
			float minWidth = MediaJack::M_DEFAULT_WIDTH + MediaJack::M_DEFAULT_GAP;
			minWidth *= numInputs > numOutputs ? numInputs : numOutputs;
			minWidth += m_bodyRect.Width();
			minWidth += 2 * MediaJack::M_DEFAULT_GAP;
			minWidth = ((int)minWidth / (int)align) * align + align;
			if ((Frame().Width() < minWidth)
			 || ((Frame().Width() > minWidth)
			 && (minWidth >= MediaNodePanel::M_DEFAULT_WIDTH)))
			{
				updateRegion.Include(Frame());
				resizeTo(minWidth, Frame().Height());
				updateRegion.Include(Frame());
				_prepareLabel();
			}
			// adjust the placement of the jacks
			float inputOffset = 0.0, outputOffset = 0.0;
			float center = Frame().left + m_labelRect.left + (m_labelRect.Width() / 2.0);
			center += MediaJack::M_DEFAULT_GAP - (MediaJack::M_DEFAULT_WIDTH / 2.0);
			center = ((int)center / (int)align) * align;
			if (numInputs)
			{
				if (numInputs % 2) // odd number of inputs
				{
					inputOffset = center - (numInputs / 2) * (MediaJack::M_DEFAULT_WIDTH + MediaJack::M_DEFAULT_GAP);
				}
				else // even number of inputs
				{
					inputOffset = center - ((numInputs + 1) / 2) * (MediaJack::M_DEFAULT_WIDTH + MediaJack::M_DEFAULT_GAP);
				}
			}
			if (numOutputs)
			{
				if (numOutputs % 2) // odd number of outputs
				{
					outputOffset = center - (numOutputs / 2) * (MediaJack::M_DEFAULT_WIDTH + MediaJack::M_DEFAULT_GAP);
				}
				else // even number of outputs
				{
					outputOffset = center - ((numOutputs + 1) / 2) * (MediaJack::M_DEFAULT_WIDTH + MediaJack::M_DEFAULT_GAP);
				}
			}
			for (uint32 i = 0; i < CountItems(); i++)
			{
				MediaJack *jack = dynamic_cast<MediaJack *>(ItemAt(i));
				if (jack)
				{
					if (jack->isInput())
					{
						jack->setPosition(inputOffset, Frame().top, Frame().bottom, &updateRegion);
						inputOffset += jack->Frame().Width() + MediaJack::M_DEFAULT_GAP;
					}
					if (jack->isOutput())
					{
						jack->setPosition(outputOffset, Frame().top, Frame().bottom, &updateRegion);
						outputOffset += jack->Frame().Width() + MediaJack::M_DEFAULT_GAP;
					}
				}
			}
			for (int32 i = 0; i < updateRegion.CountRects(); i++)
			{
				view()->Invalidate(updateRegion.RectAt(i));
			}
			break;
		}		
	}
	_updateBitmap();
}

void MediaNodePanel::showContextMenu(
	BPoint point)
{
	D_METHOD(("MediaNodePanel::showContextMenu()\n"));

	BPopUpMenu *menu = new BPopUpMenu("MediaNodePanel PopUp", false, false, B_ITEMS_IN_COLUMN);
	menu->SetFont(be_plain_font);

	BMenuItem *item;
	BMessage *message;

	// add the "Tweak Parameters" item
	message = new BMessage(MediaRoutingView::M_NODE_TWEAK_PARAMETERS);
	menu->AddItem(item = new BMenuItem("Tweak parameters", message, 'P'));
	if (!(ref->kind() & B_CONTROLLABLE))
	{
		item->SetEnabled(false);
	}

	message = new BMessage(InfoWindowManager::M_INFO_WINDOW_REQUESTED);
	message->AddInt32("nodeID", ref->id());
	menu->AddItem(new BMenuItem("Get info", message, 'I'));
	menu->AddSeparatorItem();

	menu->AddItem(item = new BMenuItem("Release", new BMessage(MediaRoutingView::M_DELETE_SELECTION), 'T'));
	if (!ref->isInternal())
	{
		item->SetEnabled(false);
	}
	menu->AddSeparatorItem();

	// add the "Cycle" item
	message = new BMessage(MediaRoutingView::M_NODE_CHANGE_CYCLING);
	message->AddBool("cycle", !ref->isCycling());
	menu->AddItem(item = new BMenuItem("Cycle", message));
	item->SetMarked(ref->isCycling());
	if (ref->flags() & NodeRef::NO_SEEK)
	{
		item->SetEnabled(false);
	}

	// add the "Run Mode" sub menu
	BMenu *subMenu = new BMenu("Run mode");
	subMenu->SetFont(be_plain_font);
	for (uint32 runMode = 1; runMode <= BMediaNode::B_RECORDING; runMode++)
	{
		BString itemName = MediaString::getStringFor(static_cast<BMediaNode::run_mode>
													 (runMode));
		message = new BMessage(MediaRoutingView::M_NODE_CHANGE_RUN_MODE);
		message->AddInt32("run_mode", runMode);
		subMenu->AddItem(item = new BMenuItem(itemName.String(), message));
		if (ref->runMode() == runMode)
		{
			item->SetMarked(true);
		}
		else if ((ref->runMode() == 0)
			  && (ref->group()) && (ref->group()->runMode() == BMediaNode::run_mode(runMode)))
		{
			item->SetMarked(true);
		}
	}
	subMenu->AddSeparatorItem();
	message = new BMessage(MediaRoutingView::M_NODE_CHANGE_RUN_MODE);
	message->AddInt32("run_mode", 0);
	subMenu->AddItem(item = new BMenuItem("(same as group)", message));
	if (ref->group() == 0)
	{
		item->SetEnabled(false);
	}
	else if ((ref->runMode() < 1) && (ref->group()->runMode() > 0))
	{
		item->SetMarked(true);
	}
	menu->AddItem(subMenu);
	subMenu->SetTargetForItems(view());
	
	// [c.lenz 24dec99] hide rarely used commands in a 'Advanced' submenu
	subMenu = new BMenu("Advanced");
	subMenu->SetFont(be_plain_font);
	// [e.moon 5dec99] ad-hoc timesource support
	if(ref->kind() & B_TIME_SOURCE) {
		message = new BMessage(MediaRoutingView::M_NODE_START_TIME_SOURCE);
		message->AddInt32("nodeID", ref->id());
		subMenu->AddItem(new BMenuItem(
			"Start time source",
			message));
		message = new BMessage(MediaRoutingView::M_NODE_START_TIME_SOURCE);
		message->AddInt32("nodeID", ref->id());
		subMenu->AddItem(new BMenuItem(
			"Stop time source",
			message));
	}
	// [c.lenz 24dec99] support for BControllable::StartControlPanel()
	if(ref->kind() & B_CONTROLLABLE) {
		if (subMenu->CountItems() > 0)
			subMenu->AddSeparatorItem();
		message = new BMessage(MediaRoutingView::M_NODE_START_CONTROL_PANEL);
		subMenu->AddItem(new BMenuItem("Start Control Panel", message,
									   'P', B_COMMAND_KEY | B_SHIFT_KEY));
	}
	// [em 1feb00] group tweaks
	if(ref->group())
	{
		message = new BMessage(MediaRoutingView::M_GROUP_SET_LOCKED);
		message->AddInt32("groupID", ref->group()->id());
		bool isLocked = (ref->group()->groupFlags() & NodeGroup::GROUP_LOCKED);
		message->AddBool("locked", !isLocked);
		if (subMenu->CountItems() > 0)
			subMenu->AddSeparatorItem();
		subMenu->AddItem(
			new BMenuItem(
				isLocked ? "Unlock group" : "Lock group", message));
	}
	
	if (subMenu->CountItems() > 0)
	{
		menu->AddItem(subMenu);
		subMenu->SetTargetForItems(view());
	}
	
	menu->SetTargetForItems(view());
	view()->ConvertToScreen(&point);
	point -= BPoint(1.0, 1.0);
	menu->Go(point, true, true, true);
}

// ---------------------------------------------------------------- //
// BHandler impl
// ---------------------------------------------------------------- //

void MediaNodePanel::MessageReceived(
	BMessage *message)
{
	D_METHOD(("MediaNodePanel::MessageReceived()\n"));
	switch (message->what)
	{
		case NodeRef::M_INPUTS_CHANGED:
		{
			D_MESSAGE(("MediaNodePanel::MessageReceived(NodeRef::M_INPUTS_CHANGED)\n"));
			_updateIcon(dynamic_cast<MediaRoutingView *>(view())->getLayout());
			break;
		}	
		case NodeRef::M_OUTPUTS_CHANGED:
		{
			D_MESSAGE(("MediaNodePanel::MessageReceived(NodeRef::M_OUTPUTS_CHANGED)\n"));
			_updateIcon(dynamic_cast<MediaRoutingView *>(view())->getLayout());
			break;
		}	
		default:
		{
			BHandler::MessageReceived(message);
			break;
		}
	}	
}

// -------------------------------------------------------- //
// *** IStateArchivable
// -------------------------------------------------------- //

status_t MediaNodePanel::importState(
	const BMessage*						archive) {

	BPoint iconPos(s_invalidPosition);
	BPoint miniIconPos(s_invalidPosition);

	MediaRoutingView* v = dynamic_cast<MediaRoutingView*>(view());
	ASSERT(v);
	MediaRoutingView::layout_t layoutMode = v->getLayout();
	archive->FindPoint("iconPos", &iconPos);
	archive->FindPoint("miniIconPos", &miniIconPos);
	
	switch(layoutMode) {
		case MediaRoutingView::M_ICON_VIEW:
			if(iconPos != s_invalidPosition)
				moveTo(iconPos);
			m_alternatePosition = miniIconPos;
			break;

		case MediaRoutingView::M_MINI_ICON_VIEW:
			if(miniIconPos != s_invalidPosition)
				moveTo(miniIconPos);
			m_alternatePosition = iconPos;
			break;
	}
	
	return B_OK;
}

status_t MediaNodePanel::exportState(
	BMessage*									archive) const {

	BPoint iconPos, miniIconPos;
	
	MediaRoutingView* v = dynamic_cast<MediaRoutingView*>(view());
	ASSERT(v);
	MediaRoutingView::layout_t layoutMode = v->getLayout();
	switch(layoutMode) {
		case MediaRoutingView::M_ICON_VIEW:
			iconPos = Frame().LeftTop();
			miniIconPos = m_alternatePosition;
			break;

		case MediaRoutingView::M_MINI_ICON_VIEW:
			miniIconPos = Frame().LeftTop();
			iconPos = m_alternatePosition;
			break;
	}

	if(iconPos != s_invalidPosition)
		archive->AddPoint("iconPos", iconPos);
	if(miniIconPos != s_invalidPosition)
		archive->AddPoint("miniIconPos", miniIconPos);
		
	// determine if I'm a 'system' node
	port_info portInfo;
	app_info appInfo;

	if ((get_port_info(ref->node().port, &portInfo) == B_OK)
		&& (be_roster->GetRunningAppInfo(portInfo.team, &appInfo) == B_OK)) {
		BEntry appEntry(&appInfo.ref);
		char appName[B_FILE_NAME_LENGTH];
		if(
			appEntry.InitCheck() == B_OK &&
			appEntry.GetName(appName) == B_OK &&
			(!strcmp(appName, "media_addon_server") ||
			 !strcmp(appName, "audio_server"))) {
		
			archive->AddBool("sysOwned", true);	 
		}
	}
	
	return B_OK;	
}

// ---------------------------------------------------------------- //
// *** internal operations
// ---------------------------------------------------------------- //

void MediaNodePanel::_prepareLabel()
{
	// find out if its a file node first
	if (ref->kind() & B_FILE_INTERFACE)
	{
		entry_ref nodeFile;
		status_t error = BMediaRoster::Roster()->GetRefFor(ref->node(),	&nodeFile);
		if (error)
		{
			m_fullLabel = ref->name();
			m_fullLabel += " (no file)";
		}
		else
		{
			BEntry entry(&nodeFile);
			char fileName[B_FILE_NAME_LENGTH];
			entry.GetName(fileName);
			m_fullLabel = fileName;
		}
	} 
	else
	{
		m_fullLabel = ref->name();
	}

	int32 layout = dynamic_cast<MediaRoutingView *>(view())->getLayout();

	// Construct labelRect
	font_height fh;
	be_plain_font->GetHeight(&fh);
	switch (layout)
	{
		case MediaRoutingView::M_ICON_VIEW:
		{
			m_labelRect = Frame();
			m_labelRect.OffsetTo(0.0, 0.0);
			m_labelRect.bottom = 2 * M_LABEL_V_MARGIN + (fh.ascent + fh.descent + fh.leading) + 1.0;
			break;
		}
		case MediaRoutingView::M_MINI_ICON_VIEW:
		{
			m_labelRect = Frame();
			m_labelRect.OffsetTo(0.0, 0.0);
			m_labelRect.left = M_BODY_H_MARGIN + B_MINI_ICON;
			m_labelRect.top += MediaJack::M_DEFAULT_HEIGHT;
			m_labelRect.bottom -= MediaJack::M_DEFAULT_HEIGHT;
			break;
		}
	}

	// truncate the label to fit in the panel
	m_label = m_fullLabel;
	float maxWidth = m_labelRect.Width() - (2.0 * M_LABEL_H_MARGIN) - 2.0;
	if (be_plain_font->StringWidth(m_fullLabel.String()) > maxWidth)
	{
		be_plain_font->TruncateString(&m_label, B_TRUNCATE_END, maxWidth);
		m_labelTruncated = true;
	}

	// Construct labelOffset
	float fw = be_plain_font->StringWidth(m_label.String());
	m_labelOffset.x = m_labelRect.left + m_labelRect.Width() / 2.0 - fw / 2.0;
	m_labelOffset.y = m_labelRect.bottom - M_LABEL_V_MARGIN - fh.descent - (fh.leading / 2.0) - 1.0;

	// Construct bodyRect
	switch (layout)
	{
		case MediaRoutingView::M_ICON_VIEW:
		{
			m_bodyRect = Frame();
			m_bodyRect.OffsetTo(0.0, 0.0);
			m_bodyRect.top = m_labelRect.bottom;
			break;
		}
		case MediaRoutingView::M_MINI_ICON_VIEW:
		{
			m_bodyRect = Frame();
			m_bodyRect.OffsetTo(0.0, 0.0);
			m_bodyRect.right = m_labelRect.left;
			break;
		}
	}
}

void MediaNodePanel::_updateBitmap()
{
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
	
		int32 layout = dynamic_cast<MediaRoutingView *>(view())->getLayout();	
		_drawInto(tempView, tempView->Bounds(), layout);

		tempView->Sync();
		tempBitmap->RemoveChild(tempView);
		delete tempView;
	}
	tempBitmap->Unlock();
	m_bitmap = new BBitmap(tempBitmap);
	delete tempBitmap;
}

void MediaNodePanel::_drawInto(
	BView *target,
	BRect targetRect,
	int32 layout)
{
	switch (layout)
	{
		case MediaRoutingView::M_ICON_VIEW:
		{
			BRect r;
			BPoint p;

			// Draw borders
			r = targetRect;
			target->BeginLineArray(16);
				target->AddLine(r.LeftTop(), r.RightTop(), M_DARK_GRAY_COLOR);
				target->AddLine(r.RightTop(), r.RightBottom(), M_DARK_GRAY_COLOR);
				target->AddLine(r.RightBottom(), r.LeftBottom(), M_DARK_GRAY_COLOR);
				target->AddLine(r.LeftBottom(), r.LeftTop(), M_DARK_GRAY_COLOR);
				r.InsetBy(1.0, 1.0);
				target->AddLine(r.LeftTop(), r.RightTop(), M_LIGHT_GRAY_COLOR);
				target->AddLine(r.RightTop(), r.RightBottom(), M_MED_GRAY_COLOR);
				target->AddLine(r.RightBottom(), r.LeftBottom(), M_MED_GRAY_COLOR);
				target->AddLine(r.LeftBottom(), r.LeftTop(), M_LIGHT_GRAY_COLOR);
			target->EndLineArray();
		
			// Fill background
			r.InsetBy(1.0, 1.0);
			target->SetLowColor(M_GRAY_COLOR);
			target->FillRect(r, B_SOLID_LOW);
		
			// Draw icon
			if (m_icon)
			{
				p.x = m_bodyRect.left + m_bodyRect.Width() / 2.0 - B_LARGE_ICON / 2.0;
				p.y = m_labelRect.bottom + m_bodyRect.Height() / 2.0 - B_LARGE_ICON / 2.0;
				if (isSelected())
				{
					target->SetDrawingMode(B_OP_INVERT);
					target->DrawBitmapAsync(m_icon, p);
					target->SetDrawingMode(B_OP_ALPHA); 
					target->SetHighColor(0, 0, 0, 180);       
					target->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
					target->DrawBitmapAsync(m_icon, p);
					target->SetDrawingMode(B_OP_OVER);
				}
				else
				{
					target->SetDrawingMode(B_OP_OVER);
					target->DrawBitmapAsync(m_icon, p);
				}
			}
		
			// Draw label
			if (isSelected())
			{
				r = m_labelRect;
				r.InsetBy(M_LABEL_H_MARGIN, M_LABEL_V_MARGIN);
				target->BeginLineArray(4);
					target->AddLine(r.LeftTop(), r.RightTop(), M_LIGHT_BLUE_COLOR);
					target->AddLine(r.RightTop(), r.RightBottom(), M_LIGHT_BLUE_COLOR);
					target->AddLine(r.RightBottom(), r.LeftBottom(), M_LIGHT_BLUE_COLOR);
					target->AddLine(r.LeftBottom(), r.LeftTop(), M_LIGHT_BLUE_COLOR);
				target->EndLineArray();
				r.InsetBy(1.0, 1.0);
				target->SetHighColor(M_DARK_BLUE_COLOR);
				target->FillRect(r, B_SOLID_HIGH);
			}
			target->SetDrawingMode(B_OP_OVER);
			target->SetHighColor(isSelected() ? M_WHITE_COLOR : M_BLACK_COLOR);
			target->DrawString(m_label.String(), m_labelOffset);
			break;
		}
		case MediaRoutingView::M_MINI_ICON_VIEW:
		{
			BRect r;
			BPoint p;

			// Draw borders
			r = targetRect;
			target->BeginLineArray(16);
				target->AddLine(r.LeftTop(), r.RightTop(), M_DARK_GRAY_COLOR);
				target->AddLine(r.RightTop(), r.RightBottom(), M_DARK_GRAY_COLOR);
				target->AddLine(r.RightBottom(), r.LeftBottom(), M_DARK_GRAY_COLOR);
				target->AddLine(r.LeftBottom(), r.LeftTop(), M_DARK_GRAY_COLOR);
				r.InsetBy(1.0, 1.0);
				target->AddLine(r.LeftTop(), r.RightTop(), M_LIGHT_GRAY_COLOR);
				target->AddLine(r.RightTop(), r.RightBottom(), M_MED_GRAY_COLOR);
				target->AddLine(r.RightBottom(), r.LeftBottom(), M_MED_GRAY_COLOR);
				target->AddLine(r.LeftBottom(), r.LeftTop(), M_LIGHT_GRAY_COLOR);
			target->EndLineArray();
		
			// Fill background
			r.InsetBy(1.0, 1.0);
			target->SetLowColor(M_GRAY_COLOR);
			target->FillRect(r, B_SOLID_LOW);
		
			// Draw icon
			if (m_icon)
			{
				p.x = m_bodyRect.left + M_BODY_H_MARGIN;
				p.y = m_bodyRect.top + (m_bodyRect.Height() / 2.0) - (B_MINI_ICON / 2.0);
				if (isSelected())
				{
					target->SetDrawingMode(B_OP_INVERT);
					target->DrawBitmapAsync(m_icon, p);
					target->SetDrawingMode(B_OP_ALPHA); 
					target->SetHighColor(0, 0, 0, 180);       
					target->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
					target->DrawBitmapAsync(m_icon, p);
					target->SetDrawingMode(B_OP_OVER);
				}
				else
				{
					target->SetDrawingMode(B_OP_OVER);
					target->DrawBitmapAsync(m_icon, p);
				}
			}
		
			// Draw label
			if (isSelected())
			{
				r = m_labelRect;
				r.InsetBy(M_LABEL_H_MARGIN, M_LABEL_V_MARGIN);
				target->BeginLineArray(4);
					target->AddLine(r.LeftTop(), r.RightTop(), M_LIGHT_BLUE_COLOR);
					target->AddLine(r.RightTop(), r.RightBottom(), M_LIGHT_BLUE_COLOR);
					target->AddLine(r.RightBottom(), r.LeftBottom(), M_LIGHT_BLUE_COLOR);
					target->AddLine(r.LeftBottom(), r.LeftTop(), M_LIGHT_BLUE_COLOR);
				target->EndLineArray();
				r.InsetBy(1.0, 1.0);
				target->SetHighColor(M_DARK_BLUE_COLOR);
				target->FillRect(r, B_SOLID_HIGH);
			}
			target->SetDrawingMode(B_OP_OVER);
			target->SetHighColor(isSelected() ? M_WHITE_COLOR : M_BLACK_COLOR);
			target->DrawString(m_label.String(), m_labelOffset);
			break;
		}
	}
}

void MediaNodePanel::_updateIcon(
	int32 layout)
{
	D_METHOD(("MediaNodePanel::_updateIcon()\n"));

	if (m_icon)
	{
		delete m_icon;
		m_icon = 0;
	}
	RouteAppNodeManager *manager;
	manager = dynamic_cast<MediaRoutingView *>(view())->manager;
	switch (layout)
	{
		case MediaRoutingView::M_ICON_VIEW:
		{
			const MediaIcon *icon = manager->mediaIconFor(ref->id(), B_LARGE_ICON);
			m_icon = new BBitmap(dynamic_cast<const BBitmap *>(icon));
			break;
		}
		case MediaRoutingView::M_MINI_ICON_VIEW:
		{
			const MediaIcon *icon = manager->mediaIconFor(ref->id(), B_MINI_ICON);
			m_icon = new BBitmap(dynamic_cast<const BBitmap *>(icon));
			break;
		}
	}
}

// -------------------------------------------------------- //
// *** sorting methods (friend)
// -------------------------------------------------------- //

int __CORTEX_NAMESPACE__ compareID(
	const void *lValue,
	const void *rValue)
{
	int retValue = 0;
	const MediaNodePanel *lPanel = *(reinterpret_cast<MediaNodePanel * const*>(reinterpret_cast<void * const*>(lValue)));
	const MediaNodePanel *rPanel = *(reinterpret_cast<MediaNodePanel * const*>(reinterpret_cast<void * const*>(rValue)));
	if (lPanel && rPanel)
	{
		if (lPanel->ref->id() < rPanel->ref->id())
		{
			retValue = -1;
		}
		else if (lPanel->ref->id() > rPanel->ref->id())
		{
			retValue = 1;
		}
	}
	return retValue;
}

// END -- MediaNodePanel.cpp --
