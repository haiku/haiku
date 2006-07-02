/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "MainWindow.h"

#include <stdio.h>

#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <ScrollView.h>

#include "Document.h"
#include "CanvasView.h"
#include "CommandStack.h"
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

enum {
	MSG_UNDO						= 'undo',
	MSG_REDO						= 'redo',

	MSG_NEW_PATH					= 'nwvp',
	MSG_PATH_SELECTED				= 'vpsl',
};

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

	fDocument->CommandStack()->RemoveObserver(this);
}

// #pragma mark -

// MessageReceived
void
MainWindow::MessageReceived(BMessage* message)
{
	if (!fDocument->WriteLock())
		return;

	switch (message->what) {

		case MSG_UNDO:
			fDocument->CommandStack()->Undo();
			break;
		case MSG_REDO:
			fDocument->CommandStack()->Redo();
			break;

// TODO: use an AddPathCommand and listen to
// selection in CanvasView to add a manipulator
case MSG_NEW_PATH: {
	VectorPath* path = new VectorPath();
	fDocument->Icon()->Paths()->AddPath(path);
	break;
}
case MSG_PATH_SELECTED: {
	VectorPath* path;
	if (message->FindPointer("path", (void**)&path) == B_OK) {
		PathManipulator* pathManipulator = new PathManipulator(path);
		fState->DeleteManipulators();
		fState->AddManipulator(pathManipulator);
	}
	break;
}

		default:
			BWindow::MessageReceived(message);
	}

	fDocument->WriteUnlock();
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
// ObjectChanged
void
MainWindow::ObjectChanged(const Observable* object)
{
	if (!fDocument)
		return;

	if (!Lock())
		return;

	if (object == fDocument->CommandStack()) {
		// relable Undo item and update enabled status
		BString label("Undo");
		fUndoMI->SetEnabled(fDocument->CommandStack()->GetUndoName(label));
		if (fUndoMI->IsEnabled())
			fUndoMI->SetLabel(label.String());
		else
			fUndoMI->SetLabel("<nothing to undo>");

		// relable Redo item and update enabled status
		label.SetTo("Redo");
		fRedoMI->SetEnabled(fDocument->CommandStack()->GetRedoName(label));
		if (fRedoMI->IsEnabled())
			fRedoMI->SetLabel(label.String());
		else
			fRedoMI->SetLabel("<nothing to redo>");
	}

	Unlock();
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
	fPathListView->SetSelection(fDocument->Selection());

	fShapeListView->SetShapeContainer(fDocument->Icon()->Shapes());
	fShapeListView->SetCommandStack(fDocument->CommandStack());
	fShapeListView->SetSelection(fDocument->Selection());

	fIconPreview16->SetIcon(fDocument->Icon());
	fIconPreview32->SetIcon(fDocument->Icon());
//	fIconPreview48->SetIcon(fDocument->Icon());
	fIconPreview64->SetIcon(fDocument->Icon());

	fDocument->CommandStack()->AddObserver(this);

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
	BRect menuFrame = bounds;
	menuFrame.bottom = menuFrame.top + 15;
	BMenuBar* menuBar = _CreateMenuBar(menuFrame);
	AddChild(menuBar);

	menuBar->ResizeToPreferred();
	bounds.top = menuBar->Frame().bottom + 1;

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

	fPathListView = new PathListView(bounds, "path list view",
									 new BMessage(MSG_PATH_SELECTED), this);

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

// _CreateMenuBar
BMenuBar*
MainWindow::_CreateMenuBar(BRect frame)
{
	BMenuBar* menuBar = new BMenuBar(frame, "main menu");

	BMenu* fileMenu = new BMenu("File");
	BMenu* editMenu = new BMenu("Edit");
	BMenu* pathMenu = new BMenu("Path");
	BMenu* styleMenu = new BMenu("Style");
	BMenu* shapeMenu = new BMenu("Shape");

	menuBar->AddItem(fileMenu);
	menuBar->AddItem(editMenu);
	menuBar->AddItem(pathMenu);
	menuBar->AddItem(styleMenu);
	menuBar->AddItem(shapeMenu);

	// Edit
	fUndoMI = new BMenuItem("<nothing to undo>",
							new BMessage(MSG_UNDO), 'Z');
	fRedoMI = new BMenuItem("<nothing to redo>",
							new BMessage(MSG_REDO), 'Z', B_SHIFT_KEY);

	fUndoMI->SetEnabled(false);
	fRedoMI->SetEnabled(false);

	editMenu->AddItem(fUndoMI);
	editMenu->AddItem(fRedoMI);

	// Path
	pathMenu->AddItem(new BMenuItem("New", new BMessage(MSG_NEW_PATH)));

	return menuBar;
}

