/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "MainWindow.h"

#include <stdio.h>

#include <Message.h>

#include "Document.h"
#include "CanvasView.h"
#include "IconEditorApp.h"

// TODO: just for testing
#include "MultipleManipulatorState.h"
#include "PathManipulator.h"
#include "VectorPath.h"
														
// constructor
MainWindow::MainWindow(IconEditorApp* app, Document* document)
	: BWindow(BRect(50.0, 50.0, 689, 529), "Icon-O-Matic",
			  B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			  B_ASYNCHRONOUS_CONTROLS),
	  fApp(app),
	  fDocument(document),
	  fCanvasView(NULL)
{
	_Init();
}

// destructor
MainWindow::~MainWindow()
{
}

// #pragma mark -

// MessageReceived
void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {

		default:
			BWindow::MessageReceived(message);
	}
}

// QuitRequested
bool
MainWindow::QuitRequested()
{
	// forward this to app but return "false" in order
	// to have a single code path for quitting
	be_app->PostMessage(B_QUIT_REQUESTED);

	return false;
}

// #pragma mark -

// _Init
void
MainWindow::_Init()
{
	// create the GUI
	BView* topView = _CreateGUI(Bounds());
	AddChild(topView);

	fCanvasView->SetCatchAllEvents(true);
	fCanvasView->SetCommandStack(fDocument->CommandStack());
//	fCanvasView->SetSelection(fDocument->Selection());

// TODO: for testing only:
	MultipleManipulatorState* state = new MultipleManipulatorState(fCanvasView);
	fCanvasView->SetState(state);

	VectorPath* path = new VectorPath();
	PathManipulator* pathManipulator = new PathManipulator(path);
	state->AddManipulator(pathManipulator);
// ----
}

// _CreateGUI
BView*
MainWindow::_CreateGUI(BRect bounds)
{
	fCanvasView = new CanvasView(bounds);

	return fCanvasView;
}

