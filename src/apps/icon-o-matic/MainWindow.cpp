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
#include "AffineTransformer.h"
#include "Gradient.h"
#include "Icon.h"
#include "MultipleManipulatorState.h"
#include "PathManipulator.h"
#include "Shape.h"
#include "ShapeContainer.h"
#include "StrokeTransformer.h"
#include "Style.h"
#include "StyleManager.h"
#include "VectorPath.h"
														
// constructor
MainWindow::MainWindow(IconEditorApp* app, Document* document)
	: BWindow(BRect(50, 50, 661, 661), "Icon-O-Matic",
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
	fCanvasView->SetIcon(fDocument->Icon());

// TODO: for testing only:
	MultipleManipulatorState* state = new MultipleManipulatorState(fCanvasView);
	fCanvasView->SetState(state);

	VectorPath* path = new VectorPath();

	fDocument->Icon()->Paths()->AddPath(path);

	Style* style1 = new Style();
	style1->SetColor((rgb_color){ 255, 255, 255, 255 });

	StyleManager::Default()->AddStyle(style1);

	Style* style2 = new Style();
	Gradient* gradient = new Gradient(true);
	gradient->AddColor((rgb_color){ 255, 211, 6, 255 }, 0.0);
	gradient->AddColor((rgb_color){ 255, 238, 160, 255 }, 0.5);
	gradient->AddColor((rgb_color){ 208, 43, 92, 255 }, 1.0);
	style2->SetGradient(gradient);

	StyleManager::Default()->AddStyle(style2);

	Shape* shape = new Shape(style2);
	shape->Paths()->AddPath(path);

	fDocument->Icon()->Shapes()->AddShape(shape);

	shape = new Shape(style1);
	shape->Paths()->AddPath(path);
	StrokeTransformer* transformer
		= new StrokeTransformer(shape->VertexSource());
	transformer->width(5.0);
	shape->AppendTransformer(transformer);

	fDocument->Icon()->Shapes()->AddShape(shape);

	Style* style3 = new Style();
	style3->SetColor((rgb_color){ 255, 0, 169,200 });

	StyleManager::Default()->AddStyle(style3);

	shape = new Shape(style3);
	shape->Paths()->AddPath(path);
	AffineTransformer* transformer2
		= new AffineTransformer(shape->VertexSource());
	*transformer2 *= agg::trans_affine_translation(10.0, 6.0);
	*transformer2 *= agg::trans_affine_rotation(0.2);
	*transformer2 *= agg::trans_affine_scaling(0.8, 0.6);
	shape->AppendTransformer(transformer2);

	fDocument->Icon()->Shapes()->AddShape(shape);

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

