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


// ParameterWindow.cpp

#include "ParameterWindow.h"
// ParameterWindow
#include "ParameterContainerView.h"

// Application Kit
#include <Message.h>
#include <Messenger.h>
// Interface Kit
#include <Alert.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Screen.h>
#include <ScrollBar.h>
// Media Kit
#include <MediaRoster.h>
#include <MediaTheme.h>
#include <ParameterWeb.h>
// Storage Kit
#include <FilePanel.h>
#include <Path.h>
// Support Kit
#include <String.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_ALLOC(x) //PRINT (x)
#define D_HOOK(x) //PRINT (x)
#define D_INTERNAL(x) //PRINT (x)
#define D_MESSAGE(x) //PRINT (x)

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

ParameterWindow::ParameterWindow(
	BPoint position,
	live_node_info &nodeInfo,
	BMessenger *notifyTarget)
	: BWindow(BRect(position, position + BPoint(50.0, 50.0)),
			  "parameters", B_DOCUMENT_WINDOW,
			  B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS),
	  m_node(nodeInfo.node),
	  m_parameters(0),
	  m_notifyTarget(0),
	  m_zoomed(false),
	  m_zooming(false) {
	D_ALLOC(("ParameterWindow::ParameterWindow()\n"));

	// add the nodes name to the title
	{
		char* title = new char[strlen(nodeInfo.name) + strlen(" parameters") + 1];
		sprintf(title, "%s parameters", nodeInfo.name);
		SetTitle(title);
		delete [] title;
	}
	// add the menu bar
	BMenuBar *menuBar = new BMenuBar(Bounds(), "ParameterWindow MenuBar");

	BMenu *menu = new BMenu("Window");
	menu->AddItem(new BMenuItem("Start control panel",
								new BMessage(M_START_CONTROL_PANEL),
								'P', B_COMMAND_KEY | B_SHIFT_KEY));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Close",
								new BMessage(B_QUIT_REQUESTED),
								'W', B_COMMAND_KEY));
	menuBar->AddItem(menu);

	// future Media Theme selection capabilities go here
	menu = new BMenu("Themes");
	BMessage *message = new BMessage(M_THEME_SELECTED);
	BMediaTheme *theme = BMediaTheme::PreferredTheme();
	message->AddInt32("themeID", theme->ID());
	BMenuItem *item = new BMenuItem(theme->Name(), message);
	item->SetMarked(true);
	menu->AddItem(item);
	menuBar->AddItem(menu);
	AddChild(menuBar);

	_updateParameterView();
	_init();

	// start watching for parameter web changes
	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	roster->StartWatching(this, nodeInfo.node, B_MEDIA_WILDCARD);

	if (notifyTarget) {
		m_notifyTarget = new BMessenger(*notifyTarget);
	}
}

ParameterWindow::~ParameterWindow() {
	D_ALLOC(("ParameterWindow::~ParameterWindow()\n"));

	if (m_notifyTarget) {
		delete m_notifyTarget;
	}
}

// -------------------------------------------------------- //
// BWindow impl
// -------------------------------------------------------- //

void ParameterWindow::FrameResized(
	float width,
	float height) {
	D_HOOK(("ParameterWindow::FrameResized()\n"));

	if (!m_zooming) {
		m_zoomed = false;
	}
	else {
		m_zooming = false;
	}
}

void ParameterWindow::MessageReceived(
	BMessage *message) {
	D_MESSAGE(("ParameterWindow::MessageReceived()\n"));

	switch (message->what) {
		case M_START_CONTROL_PANEL: {
			D_MESSAGE((" -> M_START_CONTROL_PANEL\n"));
			status_t error = _startControlPanel();
			if (error) {
				BString s = "Could not start control panel";
				s << " (" << strerror(error) << ")";
				BAlert *alert = new BAlert("", s.String(), "OK", 0, 0,
										   B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go(0);
			}
			bool replace = false;
			if ((message->FindBool("replace", &replace) == B_OK)
			 && (replace == true)) {
				PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		case M_THEME_SELECTED: {
			D_MESSAGE((" -> M_THEME_SELECTED\n"));
			int32 themeID;
			if (message->FindInt32("themeID", &themeID) != B_OK) {
				return;
			}
			// not yet implemented
			break;
		}
		case B_MEDIA_WEB_CHANGED: {
			D_MESSAGE((" -> B_MEDIA_WEB_CHANGED\n"));
			_updateParameterView();
			_constrainToScreen();
			break;
		}
		default: {
			BWindow::MessageReceived(message);
		}
	}
}

bool ParameterWindow::QuitRequested() {
	D_HOOK(("ParameterWindow::QuitRequested()\n"));

	// stop watching the MediaRoster
	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	if (roster)	{
		roster->StopWatching(this, m_node, B_MEDIA_WILDCARD);
	}

	// tell the notification target to forget about us
	if (m_notifyTarget && m_notifyTarget->IsValid()) {
		BMessage message(M_CLOSED);
		message.AddInt32("nodeID", m_node.node);
		status_t error = m_notifyTarget->SendMessage(&message);
		if (error) {
			D_HOOK((" -> error sending message (%s) !\n", strerror(error)));
		}
	}

	return true;
}

void ParameterWindow::Zoom(
	BPoint origin,
	float width,
	float height) {
	D_HOOK(("ParameterWindow::Zoom()\n"));

	m_zooming = true;

	BScreen screen(this);
	if (!screen.Frame().Contains(Frame())) {
		m_zoomed = false;
	}

	if (!m_zoomed) {
		// resize to the ideal size
		m_manualSize = Bounds();
		ResizeTo(m_idealSize.Width(), m_idealSize.Height());
		_constrainToScreen();
		m_zoomed = true;
	}
	else {
		// resize to the most recent manual size
		ResizeTo(m_manualSize.Width(), m_manualSize.Height());
		m_zoomed = false;
	}
}

// -------------------------------------------------------- //
// internal operations
// -------------------------------------------------------- //

void ParameterWindow::_init() {
	D_INTERNAL(("ParameterWindow::_init()\n"));

	// offset to a new position
	_constrainToScreen();
	m_manualSize = Bounds().OffsetToCopy(0.0, 0.0);

	// add the hidden option to close this window when the
	// control panel has been started successfully
	BMessage *message = new BMessage(M_START_CONTROL_PANEL);
	message->AddBool("replace", true);
	AddShortcut('P', B_COMMAND_KEY | B_SHIFT_KEY | B_OPTION_KEY,
				message);
}

void ParameterWindow::_updateParameterView(
	BMediaTheme *theme) {
	D_INTERNAL(("ParameterWindow::_updateParameterView()\n"));

	// clear the old version
	if (m_parameters) {
		ParameterContainerView *view = dynamic_cast<ParameterContainerView *>(FindView("ParameterContainerView"));
		RemoveChild(view);
		delete m_parameters;
		m_parameters = 0;
		delete view;
	}

	// fetch ParameterWeb from the MediaRoster
	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	if (roster) {
		BParameterWeb *web;
		status_t error = roster->GetParameterWebFor(m_node, &web);
		if (!error && (web->CountParameters() || web->CountGroups())) {
			// if no theme was specified, use the preferred theme
			if (!theme) {
				theme = BMediaTheme::PreferredTheme();
			}
			// acquire the view
			m_parameters = BMediaTheme::ViewFor(web, 0, theme);
			if (m_parameters) {
				BMenuBar *menuBar = KeyMenuBar();
				m_idealSize = m_parameters->Bounds();
				m_idealSize.right += B_V_SCROLL_BAR_WIDTH;
				m_idealSize.bottom += B_H_SCROLL_BAR_HEIGHT;
				if (menuBar) {
					m_parameters->MoveTo(0.0, menuBar->Bounds().bottom + 1.0);
					m_idealSize.bottom += menuBar->Bounds().bottom + 1.0;
				}
			}
		}
	}

	// limit min size to avoid funny-looking scrollbars
	float hMin = B_V_SCROLL_BAR_WIDTH*6, vMin = B_H_SCROLL_BAR_HEIGHT*3;
	// limit max size to full extents of the parameter view
	float hMax = m_idealSize.Width(), vMax = m_idealSize.Height();
	SetSizeLimits(hMin, hMax, vMin, vMax);

	// adapt the window to the new dimensions
	ResizeTo(m_idealSize.Width(), m_idealSize.Height());
	m_zoomed = true;

	if (m_parameters) {
		BRect paramRect = m_parameters->Bounds();
		AddChild(new ParameterContainerView(paramRect, m_parameters));
	}
}

void ParameterWindow::_constrainToScreen() {
	D_INTERNAL(("ParameterWindow::_constrainToScreen()\n"));

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

status_t ParameterWindow::_startControlPanel() {
	D_INTERNAL(("ParameterWindow::_startControlPanel()\n"));

	// get roster instance
	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	if (!roster) {
		D_INTERNAL((" -> MediaRoster not available\n"));
		return B_ERROR;
	}

	// try to StartControlPanel()
	BMessenger messenger;
	status_t error = roster->StartControlPanel(m_node, &messenger);
	if (error) {
		D_INTERNAL((" -> StartControlPanel() failed (%s)\n", strerror(error)));
		return error;
	}

	// notify the notification target, if any
	if (m_notifyTarget) {
		BMessage message(M_CONTROL_PANEL_STARTED);
		message.AddInt32("nodeID", m_node.node);
		message.AddMessenger("messenger", messenger);
		error = m_notifyTarget->SendMessage(&message);
		if (error) {
			D_INTERNAL((" -> failed to notify target\n"));
		}
	}

	return B_OK;
}

// END -- ParameterWindow.cpp --
