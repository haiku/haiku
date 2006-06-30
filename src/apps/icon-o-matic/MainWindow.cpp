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
#include <ScrollView.h>

#include "Document.h"
#include "CanvasView.h"
#include "IconEditorApp.h"
#include "IconView.h"
#include "PathListView.h"
#include "ShapeListView.h"
#include "SwatchGroup.h"

// TODO: just for testing
#include "AffineTransformer.h"
#include "Gradient.h"
#include "Icon.h"
#include "MultipleManipulatorState.h"
#include "PathManipulator.h"
#include "Shape.h"
#include "ShapeContainer.h"
#include "ShapeListView.h"
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
	  fDocument(document)
{
	_Init();
}

// destructor
MainWindow::~MainWindow()
{
	delete fState;
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

	fPathListView->SetPathContainer(fDocument->Icon()->Paths());
//	fPathListView->SetCommandStack(fDocument->CommandStack());
//	fPathListView->SetSelection(fDocument->Selection());

	fShapeListView->SetShapeContainer(fDocument->Icon()->Shapes());
//	fShapeListView->SetCommandStack(fDocument->CommandStack());
//	fShapeListView->SetSelection(fDocument->Selection());

	fIconPreview16->SetIcon(fDocument->Icon());
	fIconPreview32->SetIcon(fDocument->Icon());
//	fIconPreview48->SetIcon(fDocument->Icon());
	fIconPreview64->SetIcon(fDocument->Icon());

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

	shape->SetName("Gradient");
	fDocument->Icon()->Shapes()->AddShape(shape);

	shape = new Shape(style1);
	shape->Paths()->AddPath(path);
	StrokeTransformer* transformer
		= new StrokeTransformer(shape->VertexSource());
	transformer->width(5.0);
	shape->AppendTransformer(transformer);

	shape->SetName("Outline");
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

	shape->SetName("Transformed");
	fDocument->Icon()->Shapes()->AddShape(shape);

	PathManipulator* pathManipulator = new PathManipulator(path);
	state->AddManipulator(pathManipulator);
	fState = state;
// ----
}

// _CreateGUI
BView*
MainWindow::_CreateGUI(BRect bounds)
{
	BView* bg = new BView(bounds, "bg", B_FOLLOW_ALL, 0);
	bg->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fSwatchGroup = new SwatchGroup(BRect(0, 0, 100, 100));
	bounds.top = fSwatchGroup->Frame().bottom + 1;
	
	fCanvasView = new CanvasView(bounds);

	bounds.left = fSwatchGroup->Frame().right + 5;
	bounds.top = fSwatchGroup->Frame().top + 5;
	bounds.right = bounds.left + 15;
	bounds.bottom = bounds.top + 15;
	fIconPreview16 = new IconView(bounds, "icon preview 16");

	bounds.OffsetBy(bounds.Width() + 6, 0);
	bounds.right = bounds.left + 31;
	bounds.bottom = bounds.top + 31;
	fIconPreview32 = new IconView(bounds, "icon preview 32");

//	bounds.OffsetBy(bounds.Width() + 6, 0);
//	bounds.right = bounds.left + 47;
//	bounds.bottom = bounds.top + 47;
//	fIconPreview48 = new IconView(bounds, "icon preview 48");

	bounds.OffsetBy(bounds.Width() + 6, 0);
	bounds.right = bounds.left + 63;
	bounds.bottom = bounds.top + 63;
	fIconPreview64 = new IconView(bounds, "icon preview 64");

	bounds.OffsetBy(bounds.Width() + 6, 0);
	bounds.right = bounds.left + 100;
	bounds.bottom = fCanvasView->Frame().top - 5.0;

	fPathListView = new PathListView(bounds, "path list view");

	bounds.OffsetBy(bounds.Width() + 6 + B_V_SCROLL_BAR_WIDTH, 0);
	fShapeListView = new ShapeListView(bounds, "shape list view");

	bg->AddChild(fSwatchGroup);

	bg->AddChild(fIconPreview16);
	bg->AddChild(fIconPreview32);
//	bg->AddChild(fIconPreview48);
	bg->AddChild(fIconPreview64);

	bg->AddChild(new BScrollView("path list scroll view",
								 fPathListView,
								 B_FOLLOW_LEFT | B_FOLLOW_TOP,
								 0, false, true,
								 B_NO_BORDER));
	bg->AddChild(new BScrollView("shape list scroll view",
								 fShapeListView,
								 B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
								 0, false, true,
								 B_NO_BORDER));

	bg->AddChild(fCanvasView);

	return bg;
}

