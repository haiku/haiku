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


// RouteWindow.cpp
// e.moon 14may99

#include "RouteApp.h"
#include "RouteWindow.h"
#include "MediaRoutingView.h"
#include "StatusView.h"

#include "DormantNodeWindow.h"
#include "TransportWindow.h"

#include "RouteAppNodeManager.h"
#include "NodeGroup.h"
#include "TipManager.h"

#include <Alert.h>
#include <Autolock.h>
#include <Debug.h>
#include <Font.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <Roster.h>
#include <Screen.h>
#include <ScrollView.h>
#include <StringView.h>

#include <algorithm>

#include "debug_tools.h"
#define D_HOOK(x) //PRINT (x)
#define D_INTERNAL(x) //PRINT (x)

__USE_CORTEX_NAMESPACE


const char* const RouteWindow::s_windowName = "Cortex";

const BRect RouteWindow::s_initFrame(100,100,700,550);

const char* const g_aboutText =
	"Cortex/Route 2.1.2\n\n"
	"Copyright 1999-2000 Eric Moon\n"
	"All rights reserved.\n\n"
	"The Cortex Team:\n\n"
	"Christopher Lenz: UI\n"
	"Eric Moon: UI, back-end\n\n"
	"Thanks to:\nJohn Ashmun\nJon Watte\nDoug Wright\n<your name here>\n\n"
	"Certain icons used herein are the property of\n"
	"Be, Inc. and are used by permission.";


RouteWindow::~RouteWindow()
{
}


RouteWindow::RouteWindow(RouteAppNodeManager* manager)
	:
	BWindow(s_initFrame, s_windowName, B_DOCUMENT_WINDOW, 0),
	m_hScrollBar(0),
	m_vScrollBar(0),
	m_transportWindow(0),
	m_dormantNodeWindow(0),
	m_selectedGroupID(0),
	m_zoomed(false),
	m_zooming(false)
{
	BRect b = Bounds();

	// initialize the menu bar: add all menus that target this window
	BMenuBar* pMenuBar = new BMenuBar(b, "menuBar");
	BMenu* pFileMenu = new BMenu("File");
	BMenuItem* item = new BMenuItem("Open" B_UTF8_ELLIPSIS,
		new BMessage(RouteApp::M_SHOW_OPEN_PANEL), 'O');
	item->SetTarget(be_app);
	pFileMenu->AddItem(item);
	pFileMenu->AddItem(new BSeparatorItem());
	item = new BMenuItem("Save nodes" B_UTF8_ELLIPSIS,
		new BMessage(RouteApp::M_SHOW_SAVE_PANEL), 'S');
	item->SetTarget(be_app);
	pFileMenu->AddItem(item);
	pFileMenu->AddItem(new BSeparatorItem());
	pFileMenu->AddItem(new BMenuItem("About Cortex/Route" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	pFileMenu->AddItem(new BSeparatorItem());
	pFileMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED)));
	pMenuBar->AddItem(pFileMenu);
	AddChild(pMenuBar);

	// build the routing view
	BRect rvBounds = b;
	rvBounds.top = pMenuBar->Frame().bottom+1;
	rvBounds.right -= B_V_SCROLL_BAR_WIDTH;
	rvBounds.bottom -= B_H_SCROLL_BAR_HEIGHT;
	m_routingView = new MediaRoutingView(manager, rvBounds, "routingView");

	BRect hsBounds = rvBounds;
	hsBounds.left = rvBounds.left + 199;
	hsBounds.top = hsBounds.bottom + 1;
	hsBounds.right++;
	hsBounds.bottom = b.bottom + 1;

	m_hScrollBar = new BScrollBar(hsBounds, "hScrollBar", m_routingView,
		0, 0, B_HORIZONTAL);
	AddChild(m_hScrollBar);

	BRect vsBounds = rvBounds;
	vsBounds.left = vsBounds.right + 1;
	vsBounds.top--;
	vsBounds.right = b.right + 1;
	vsBounds.bottom++;

	m_vScrollBar = new BScrollBar(vsBounds, "vScrollBar", m_routingView,
		0, 0, B_VERTICAL);
	AddChild(m_vScrollBar);

	BRect svBounds = rvBounds;
	svBounds.left -= 1;
	svBounds.right = hsBounds.left - 1;
	svBounds.top = svBounds.bottom + 1;
	svBounds.bottom = b.bottom + 1;

	m_statusView = new StatusView(svBounds, manager, m_hScrollBar);
	AddChild(m_statusView);

	AddChild(m_routingView);

	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	minWidth = m_statusView->Frame().Width() + 6 * B_V_SCROLL_BAR_WIDTH;
	minHeight = 6 * B_H_SCROLL_BAR_HEIGHT;
	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	// construct the Window menu
	BMenu* windowMenu = new BMenu("Window");
	m_transportWindowItem = new BMenuItem(
		"Show transport",
		new BMessage(M_TOGGLE_TRANSPORT_WINDOW));
	windowMenu->AddItem(m_transportWindowItem);

	m_dormantNodeWindowItem = new BMenuItem(
		"Show add-ons",
		new BMessage(M_TOGGLE_DORMANT_NODE_WINDOW));
	windowMenu->AddItem(m_dormantNodeWindowItem);

	windowMenu->AddItem(new BSeparatorItem());

	m_pullPalettesItem = new BMenuItem(
		"Pull palettes",
		new BMessage(M_TOGGLE_PULLING_PALETTES));
	windowMenu->AddItem(m_pullPalettesItem);

	pMenuBar->AddItem(windowMenu);

	// create the dormant-nodes palette
	_toggleDormantNodeWindow();

	// display group inspector
	_toggleTransportWindow();
}


//	#pragma mark - operations


/*!	Enable/disable palette position-locking (when the main
	window is moved, all palettes follow).
*/
bool
RouteWindow::isPullPalettes() const
{
	return m_pullPalettesItem->IsMarked();
}


void
RouteWindow::setPullPalettes(bool enabled)
{
	m_pullPalettesItem->SetMarked(enabled);
}


void
RouteWindow::constrainToScreen()
{
	BScreen screen(this);

	const BRect sr = screen.Frame();

	// [c.lenz 1mar2000] this should be handled by every window
	// itself. will probably change soon ;-)
	_constrainToScreen();
/*	// main window
	BRect r = Frame();
	BPoint offset(0.0, 0.0);
	if(r.left < 0.0)
		offset.x = -r.left;
	if(r.top < 0.0)
		offset.y = -r.top;
	if(r.left >= (sr.right - 20.0))
		offset.x -= (r.left - (sr.Width()/2));
	if(r.top >= (sr.bottom - 20.0))
		offset.y -= (r.top - (sr.Height()/2));
	if(offset.x != 0.0 || offset.y != 0.0) {
		setPullPalettes(false);
		MoveBy(offset.x, offset.y);
	}*/

	// transport window
	BPoint offset = BPoint(0.0, 0.0);
	BRect r = (m_transportWindow) ?
			   m_transportWindow->Frame() :
			   m_transportWindowFrame;
	if(r.left < 0.0)
		offset.x = (sr.Width()*.75) - r.left;
	if(r.top < 0.0)
		offset.y = (sr.Height()*.25) - r.top;
	if(r.left >= (sr.right - 20.0))
		offset.x -= (r.left - (sr.Width()/2));
	if(r.top >= (sr.bottom - 20.0))
		offset.y -= (r.top - (sr.Height()/2));

	if(offset.x != 0.0 || offset.y != 0.0) {
		if(m_transportWindow)
			m_transportWindow->MoveBy(offset.x, offset.y);
		else
			m_transportWindowFrame.OffsetBy(offset.x, offset.y);
	}

	// addon palette
	offset = BPoint(0.0, 0.0);
	r = (m_dormantNodeWindow) ?
		m_dormantNodeWindow->Frame() :
		m_dormantNodeWindowFrame;
	if(r.left < 0.0)
		offset.x = (sr.Width()*.25) - r.left;
	if(r.top < 0.0)
		offset.y = (sr.Height()*.125) - r.top;
	if(r.left >= (sr.right - 20.0))
		offset.x -= (r.left - (sr.Width()/2));
	if(r.top >= (sr.bottom - 20.0))
		offset.y -= (r.top - (sr.Height()/2));

	if(offset.x != 0.0 || offset.y != 0.0) {
		if(m_dormantNodeWindow)
			m_dormantNodeWindow->MoveBy(offset.x, offset.y);
		else
			m_dormantNodeWindowFrame.OffsetBy(offset.x, offset.y);
	}

}


//	#pragma mark - BWindow implementation


void
RouteWindow::FrameMoved(BPoint point)
{
	// ignore notification if the window isn't yet visible
	if(IsHidden())
		return;

	BPoint delta = point - m_lastFramePosition;
	m_lastFramePosition = point;


	if (m_pullPalettesItem->IsMarked())
		_movePalettesBy(delta.x, delta.y);
}


void
RouteWindow::FrameResized(float width, float height)
{
	D_HOOK(("RouteWindow::FrameResized()\n"));

	if (!m_zooming) {
		m_zoomed = false;
	}
	else {
		m_zooming = false;
	}
}


bool
RouteWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return false; // [e.moon 20oct99] app now quits window
}


void
RouteWindow::Zoom(BPoint origin, float width, float height)
{
	D_HOOK(("RouteWindow::Zoom()\n"));

	m_zooming = true;

	BScreen screen(this);
	if (!screen.Frame().Contains(Frame())) {
		m_zoomed = false;
	}

	if (!m_zoomed) {
		// resize to the ideal size
		m_manualSize = Bounds();
		float width, height;
		m_routingView->GetPreferredSize(&width, &height);
		width += B_V_SCROLL_BAR_WIDTH;
		height += B_H_SCROLL_BAR_HEIGHT;
		if (KeyMenuBar()) {
			height += KeyMenuBar()->Frame().Height();
		}
		ResizeTo(width, height);
		_constrainToScreen();
		m_zoomed = true;
	}
	else {
		// resize to the most recent manual size
		ResizeTo(m_manualSize.Width(), m_manualSize.Height());
		m_zoomed = false;
	}
}


//	#pragma mark - BHandler implemenation


void
RouteWindow::MessageReceived(BMessage* pMsg)
{
//	PRINT((
//		"RouteWindow::MessageReceived()\n"));
//	pMsg->PrintToStream();
//
	switch (pMsg->what) {
		case B_ABOUT_REQUESTED:
			(new BAlert("About", g_aboutText, "OK"))->Go();
			break;

		case MediaRoutingView::M_GROUP_SELECTED:
			_handleGroupSelected(pMsg);
			break;

		case MediaRoutingView::M_SHOW_ERROR_MESSAGE:
			_handleShowErrorMessage(pMsg);
			break;

		case M_TOGGLE_TRANSPORT_WINDOW:
			_toggleTransportWindow();
			break;

		case M_REFRESH_TRANSPORT_SETTINGS:
			_refreshTransportSettings(pMsg);
			break;

		case M_TOGGLE_PULLING_PALETTES:
			_togglePullPalettes();
			break;

		case M_TOGGLE_DORMANT_NODE_WINDOW:
			_toggleDormantNodeWindow();
			break;

		case M_TOGGLE_GROUP_ROLLING:
			_toggleGroupRolling();
			break;

		default:
			_inherited::MessageReceived(pMsg);
			break;
	}
}


//	#pragma mark - IStateArchivable


status_t
RouteWindow::importState(const BMessage* archive)
{
	status_t err;

	// frame rect
	BRect r;
	err = archive->FindRect("frame", &r);
	if(err == B_OK) {
		MoveTo(r.LeftTop());
		ResizeTo(r.Width(), r.Height());
		m_lastFramePosition = r.LeftTop();
	}

	// status view width
	int32 i;
	err = archive->FindInt32("statusViewWidth", &i);
	if (err == B_OK) {
		float diff = i - m_statusView->Bounds().IntegerWidth();
		m_statusView->ResizeBy(diff, 0.0);
		m_hScrollBar->ResizeBy(-diff, 0.0);
		m_hScrollBar->MoveBy(diff, 0.0);
	}

	// settings
	bool b;
	err = archive->FindBool("pullPalettes", &b);
	if(err == B_OK)
		m_pullPalettesItem->SetMarked(b);

//	const char* p;
//	err = archive->FindString("saveDir", &p);
//	if(err == B_OK) {
//		m_openPanel.SetPanelDirectory(p);
//		m_savePanel.SetPanelDirectory(p);
//	}
//
	// dormant-node window
	err = archive->FindRect("addonPaletteFrame", &r);
	if (err == B_OK)
		m_dormantNodeWindowFrame = r;
	err = archive->FindBool("addonPaletteVisible", &b);
	if (err == B_OK && (b != (m_dormantNodeWindow != 0))) {
		_toggleDormantNodeWindow();
		if(!m_dormantNodeWindow)
			m_dormantNodeWindowFrame = r;
	}

	if (m_dormantNodeWindow) {
		m_dormantNodeWindow->MoveTo(m_dormantNodeWindowFrame.LeftTop());
		m_dormantNodeWindow->ResizeTo(
			m_dormantNodeWindowFrame.Width(),
			m_dormantNodeWindowFrame.Height());
	}

	// transport window
	err = archive->FindRect("transportFrame", &r);
	if (err == B_OK)
		m_transportWindowFrame = r;
	err = archive->FindBool("transportVisible", &b);
	if (err == B_OK && (b != (m_transportWindow != 0))) {
		_toggleTransportWindow();
		if (!m_transportWindow)
			m_transportWindowFrame = r;
	}

	if (m_transportWindow) {
		m_transportWindow->MoveTo(m_transportWindowFrame.LeftTop());
		m_transportWindow->ResizeTo(
			m_transportWindowFrame.Width(),
			m_transportWindowFrame.Height());
	}

	return B_OK;
}


status_t
RouteWindow::exportState(BMessage* archive) const
{
	BRect r = Frame();
	archive->AddRect("frame", r);
	archive->AddBool("pullPalettes", m_pullPalettesItem->IsMarked());

	bool b = (m_dormantNodeWindow != 0);
	r = b ? m_dormantNodeWindow->Frame() : m_dormantNodeWindowFrame;
	archive->AddRect("addonPaletteFrame", r);
	archive->AddBool("addonPaletteVisible", b);

	b = (m_transportWindow != 0);
	r = b ? m_transportWindow->Frame() : m_transportWindowFrame;

	archive->AddRect("transportFrame", r);
	archive->AddBool("transportVisible", b);

	// [c.lenz 23may00] remember status view width
	int i = m_statusView->Bounds().IntegerWidth();
	archive->AddInt32("statusViewWidth", i);

//	entry_ref saveRef;
//	m_savePanel.GetPanelDirectory(&saveRef);
//	BEntry saveEntry(&saveRef);
//	if(saveEntry.InitCheck() == B_OK) {
//		BPath p;
//		saveEntry.GetPath(&p);
//		archive->AddString("saveDir", p.Path());
//	}

	return B_OK;
}


//	#pragma mark - implementation


void
RouteWindow::_constrainToScreen()
{
	D_INTERNAL(("RouteWindow::_constrainToScreen()\n"));

	BScreen screen(this);
	BRect screenRect = screen.Frame();
	BRect windowRect = Frame();

	// if the window is outside the screen rect
	// move it to the default position
	if (!screenRect.Intersects(windowRect)) {
		windowRect.OffsetTo(screenRect.LeftTop());
		MoveTo(windowRect.LeftTop());
		windowRect = Frame();
	}

	// if the window is larger than the screen rect
	// resize it to fit at each side
	if (!screenRect.Contains(windowRect)) {
		if (windowRect.left < screenRect.left) {
			windowRect.left = screenRect.left + 5.0;
			MoveTo(windowRect.LeftTop());
			windowRect = Frame();
		}
		if (windowRect.top < screenRect.top) {
			windowRect.top = screenRect.top + 5.0;
			MoveTo(windowRect.LeftTop());
			windowRect = Frame();
		}
		if (windowRect.right > screenRect.right) {
			windowRect.right = screenRect.right - 5.0;
		}
		if (windowRect.bottom > screenRect.bottom) {
			windowRect.bottom = screenRect.bottom - 5.0;
		}
		ResizeTo(windowRect.Width(), windowRect.Height());
	}
}


void
RouteWindow::_toggleTransportWindow()
{
	if (m_transportWindow) {
		m_transportWindowFrame = m_transportWindow->Frame();
		m_transportWindow->Lock();
		m_transportWindow->Quit();
		m_transportWindow = 0;
		m_transportWindowItem->SetMarked(false);
	} else {
		m_transportWindow = new TransportWindow(m_routingView->manager,
			this, "Transport");

		// ask for a selection update
		BMessenger(m_routingView).SendMessage(
			MediaRoutingView::M_BROADCAST_SELECTION);

		// place & display the window
		if (m_transportWindowFrame.IsValid()) {
			m_transportWindow->MoveTo(m_transportWindowFrame.LeftTop());
			m_transportWindow->ResizeTo(m_transportWindowFrame.Width(),
				m_transportWindowFrame.Height());
		}

		m_transportWindow->Show();
		m_transportWindowItem->SetMarked(true);
	}
}


void
RouteWindow::_togglePullPalettes()
{
	m_pullPalettesItem->SetMarked(!m_pullPalettesItem->IsMarked());
}


void
RouteWindow::_toggleDormantNodeWindow()
{
	if (m_dormantNodeWindow) {
		m_dormantNodeWindowFrame = m_dormantNodeWindow->Frame();
		m_dormantNodeWindow->Lock();
		m_dormantNodeWindow->Quit();
		m_dormantNodeWindow = 0;
		m_dormantNodeWindowItem->SetMarked(false);
	} else {
		m_dormantNodeWindow = new DormantNodeWindow(this);
		if (m_dormantNodeWindowFrame.IsValid()) {
			m_dormantNodeWindow->MoveTo(m_dormantNodeWindowFrame.LeftTop());
			m_dormantNodeWindow->ResizeTo(m_dormantNodeWindowFrame.Width(),
				m_dormantNodeWindowFrame.Height());
		}
		m_dormantNodeWindow->Show();
		m_dormantNodeWindowItem->SetMarked(true);
	}
}


void
RouteWindow::_handleGroupSelected(BMessage* message)
{
	status_t err;
	uint32 groupID;

	err = message->FindInt32("groupID", (int32*)&groupID);
	if (err < B_OK) {
		PRINT((
			"! RouteWindow::_handleGroupSelected(): no groupID in message!\n"));
		return;
	}

	if (!m_transportWindow)
		return;

	BMessage m(TransportWindow::M_SELECT_GROUP);
	m.AddInt32("groupID", groupID);
	BMessenger(m_transportWindow).SendMessage(&m);

	m_selectedGroupID = groupID;
}


void
RouteWindow::_handleShowErrorMessage(BMessage* message)
{
	status_t err;
	BString text;

	err = message->FindString("text", &text);
	if (err < B_OK) {
		PRINT((
			"! RouteWindow::_handleShowErrorMessage(): no text in message!\n"));
		return;
	}

	m_statusView->setErrorMessage(text.String(), message->HasBool("error"));
}


//! Refresh the transport window for the given group, if any
void
RouteWindow::_refreshTransportSettings(BMessage* message)
{
	status_t err;
	uint32 groupID;

	err = message->FindInt32("groupID", (int32*)&groupID);
	if (err < B_OK) {
		PRINT((
			"! RouteWindow::_refreshTransportSettings(): no groupID in message!\n"));
		return;
	}

	if(m_transportWindow) {
		// relay the message
		BMessenger(m_transportWindow).SendMessage(message);
	}
}


void
RouteWindow::_closePalettes()
{
	BAutolock _l(this);

	if (m_transportWindow) {
		m_transportWindow->Lock();
		m_transportWindow->Quit();
		m_transportWindow = 0;
	}
}


//!	Move all palette windows by the specified amounts
void RouteWindow::_movePalettesBy(float xDelta, float yDelta)
{
	if (m_transportWindow)
		m_transportWindow->MoveBy(xDelta, yDelta);

	if (m_dormantNodeWindow)
		m_dormantNodeWindow->MoveBy(xDelta, yDelta);
}


//!	Toggle group playback
void
RouteWindow::_toggleGroupRolling()
{
	if (!m_selectedGroupID)
		return;

	NodeGroup* g;
	status_t err = m_routingView->manager->findGroup(m_selectedGroupID, &g);
	if (err < B_OK)
		return;

	Autolock _l(g);
	uint32 startAction = (g->runMode() == BMediaNode::B_OFFLINE)
		? NodeGroup::M_ROLL : NodeGroup::M_START;

	BMessenger m(g);
	switch (g->transportState()) {
		case NodeGroup::TRANSPORT_STOPPED:
			m.SendMessage(startAction);
			break;

		case NodeGroup::TRANSPORT_RUNNING:
		case NodeGroup::TRANSPORT_ROLLING:
			m.SendMessage(NodeGroup::M_STOP);
			break;

		default:
			break;
	}
}
