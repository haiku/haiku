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
#include "CurrentColor.h"
#include "IconObjectListView.h"
#include "IconEditorApp.h"
#include "IconView.h"
#include "PathListView.h"
#include "ScrollView.h"
#include "ShapeListView.h"
#include "StyleListView.h"
#include "SwatchGroup.h"
#include "TransformerFactory.h"
#include "TransformerListView.h"

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
	MSG_NEW_STYLE					= 'nwst',
	MSG_STYLE_SELECTED				= 'stsl',
	MSG_NEW_SHAPE					= 'nwsp',
	MSG_SHAPE_SELECTED				= 'spsl',

	MSG_ADD_TRANSFORMER				= 'adtr',
};

// constructor
MainWindow::MainWindow(IconEditorApp* app, Document* document)
	: BWindow(BRect(50, 50, 900, 750), "Icon-O-Matic",
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
// TODO: use an AddStyleCommand
case MSG_NEW_STYLE: {
	Style* style = new Style();
	style->SetColor((rgb_color){ rand() % 255,
								 rand() % 255,
								 rand() % 255,
								 255 });
	StyleManager::Default()->AddStyle(style);
	break;
}
case MSG_STYLE_SELECTED: {
	Style* style;
	if (message->FindPointer("style", (void**)&style) < B_OK)
		style = NULL;
	fSwatchGroup->SetCurrentStyle(style);
	break;
}
// TODO: use an AddShapeCommand
case MSG_NEW_SHAPE: {
	Shape* shape = new Shape(StyleManager::Default()->StyleAt(0));
	fDocument->Icon()->Shapes()->AddShape(shape);
	break;
}
case MSG_SHAPE_SELECTED: {
	Shape* shape;
	if (message->FindPointer("shape", (void**)&shape) < B_OK)
		shape = NULL;
	fPathListView->SetCurrentShape(shape);
	fStyleListView->SetCurrentShape(shape);
	fTransformerListView->SetShape(shape);
	break;
}

		case MSG_ADD_TRANSFORMER: {
			Shape* shape = fPathListView->CurrentShape();
			if (!shape)
				break;

			uint32 type;
			if (message->FindInt32("type", (int32*)&type) < B_OK)
				break;

			Transformer* transformer
				= TransformerFactory::TransformerFor(type,
													 shape->VertexSource());
			// TODO: command
			if (transformer)
				shape->AddTransformer(transformer);
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
	_CreateGUI(Bounds());

	fCanvasView->SetCatchAllEvents(true);
	fCanvasView->SetCommandStack(fDocument->CommandStack());
//	fCanvasView->SetSelection(fDocument->Selection());
	fCanvasView->SetIcon(fDocument->Icon());

	fPathListView->SetPathContainer(fDocument->Icon()->Paths());
	fPathListView->SetShapeContainer(fDocument->Icon()->Shapes());
//	fPathListView->SetCommandStack(fDocument->CommandStack());
	fPathListView->SetSelection(fDocument->Selection());

	fStyleListView->SetStyleManager(StyleManager::Default());
	fStyleListView->SetShapeContainer(fDocument->Icon()->Shapes());
//	fStyleListView->SetCommandStack(fDocument->CommandStack());
	fStyleListView->SetSelection(fDocument->Selection());

	fShapeListView->SetShapeContainer(fDocument->Icon()->Shapes());
	fShapeListView->SetCommandStack(fDocument->CommandStack());
	fShapeListView->SetSelection(fDocument->Selection());

	fTransformerListView->SetCommandStack(fDocument->CommandStack());
	fTransformerListView->SetSelection(fDocument->Selection());

	fPropertyListView->SetCommandStack(fDocument->CommandStack());
	fPropertyListView->SetSelection(fDocument->Selection());
	fPropertyListView->SetMenu(fPropertyMenu);

	fIconPreview16->SetIcon(fDocument->Icon());
	fIconPreview32->SetIcon(fDocument->Icon());
//	fIconPreview48->SetIcon(fDocument->Icon());
	fIconPreview64->SetIcon(fDocument->Icon());

	fDocument->CommandStack()->AddObserver(this);

	fSwatchGroup->SetCurrentColor(CurrentColor::Default());

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
	shape->AddTransformer(transformer);

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
	shape->AddTransformer(transformer2);

	shape->SetName("Transformed");
	fDocument->Icon()->Shapes()->AddShape(shape);

	PathManipulator* pathManipulator = new PathManipulator(path);
	state->AddManipulator(pathManipulator);
	fState = state;
// ----
}

// _CreateGUI
void
MainWindow::_CreateGUI(BRect bounds)
{
	const float splitWidth = 150;

	BView* bg = new BView(bounds, "bg", B_FOLLOW_ALL, 0);
	bg->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(bg);

	BRect menuFrame = bounds;
	menuFrame.bottom = menuFrame.top + 15;
	BMenuBar* menuBar = _CreateMenuBar(menuFrame);
	bg->AddChild(menuBar);
	float menuWidth;
	float menuHeight;
	menuBar->GetPreferredSize(&menuWidth, &menuHeight);
	menuBar->ResizeTo(splitWidth - 1, menuHeight);
	menuBar->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);
	
	bounds.top = menuBar->Frame().bottom + 1;

	// swatch group
	fSwatchGroup = new SwatchGroup(bounds);
		// SwatchGroup auto resizes itself
	fSwatchGroup->MoveTo(bounds.right - fSwatchGroup->Bounds().Width(),
						 bounds.top);
	fSwatchGroup->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_TOP);

	bounds.left = fSwatchGroup->Frame().left;
	bounds.right = bg->Bounds().right;
	bounds.top = bg->Bounds().top;
	bounds.bottom = bounds.top + menuHeight;
	menuBar = new BMenuBar(bounds, "swatches menu bar");
	menuBar->AddItem(fSwatchMenu);
	bg->AddChild(menuBar);
	menuBar->ResizeTo(bounds.Width(), menuHeight);
		// menu bars resize themselves to window width
	menuBar->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_TOP);

	// canvas view
	bounds.left = splitWidth;
	bounds.top = fSwatchGroup->Frame().bottom + 1;
	bounds.right = bg->Bounds().right;
	bounds.bottom = bg->Bounds().bottom;
	fCanvasView = new CanvasView(bounds);

	// icon previews
	bounds.left = 5;
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

	// style list view
	bounds.left = fCanvasView->Frame().left;
	bounds.right = bounds.left + splitWidth;
	bounds.top = bg->Bounds().top;
	bounds.bottom = bounds.top + menuHeight;
	menuBar = new BMenuBar(bounds, "style menu bar");
	menuBar->AddItem(fStyleMenu);
	bg->AddChild(menuBar);
	menuBar->ResizeTo(bounds.Width(), menuHeight);
		// menu bars resize themselves to window width
	menuBar->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);

	bounds.right -= B_V_SCROLL_BAR_WIDTH + 1;
	bounds.top = menuBar->Frame().bottom + 1;
	bounds.bottom = fCanvasView->Frame().top - 1;

	fStyleListView = new StyleListView(bounds, "style list view",
									   new BMessage(MSG_STYLE_SELECTED), this);


	// path list view
	bounds.left = 0;
	bounds.right = fCanvasView->Frame().left - 1;
	bounds.top = fCanvasView->Frame().top;
	bounds.bottom = bounds.top + menuHeight;
	menuBar = new BMenuBar(bounds, "path menu bar");
	menuBar->AddItem(fPathMenu);
	bg->AddChild(menuBar);
	menuBar->ResizeTo(bounds.Width(), menuHeight);
		// menu bars resize themselves to window width
	menuBar->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);

	bounds.right -= B_V_SCROLL_BAR_WIDTH + 1;
	bounds.top = menuBar->Frame().bottom + 1;
	bounds.bottom = bounds.top + 130;

	fPathListView = new PathListView(bounds, "path list view",
									 new BMessage(MSG_PATH_SELECTED), this);


	// shape list view
	bounds.right += B_V_SCROLL_BAR_WIDTH + 1;
	bounds.top = fPathListView->Frame().bottom + 1;
	bounds.bottom = bounds.top + menuHeight;
	menuBar = new BMenuBar(bounds, "shape menu bar");
	menuBar->AddItem(fShapeMenu);
	bg->AddChild(menuBar);
	menuBar->ResizeTo(bounds.Width(), menuHeight);
		// menu bars resize themselves to window width
	menuBar->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);

	bounds.right -= B_V_SCROLL_BAR_WIDTH + 1;
	bounds.top = menuBar->Frame().bottom + 1;
	bounds.bottom = bounds.top + 130;

	fShapeListView = new ShapeListView(bounds, "shape list view",
									   new BMessage(MSG_SHAPE_SELECTED), this);

	// transformer list view
	bounds.right += B_V_SCROLL_BAR_WIDTH + 1;
	bounds.top = fShapeListView->Frame().bottom + 1;
	bounds.bottom = bounds.top + menuHeight;
	menuBar = new BMenuBar(bounds, "transformer menu bar");
	menuBar->AddItem(fTransformerMenu);
	bg->AddChild(menuBar);
	menuBar->ResizeTo(bounds.Width(), bounds.Height());
		// menu bars resize themselves to window width
	menuBar->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);

	bounds.right -= B_V_SCROLL_BAR_WIDTH + 1;
	bounds.top = menuBar->Frame().bottom + 1;
	bounds.bottom = bounds.top + 80;
	fTransformerListView = new TransformerListView(bounds,
												   "transformer list view");

	// property list view
	bounds.right += B_V_SCROLL_BAR_WIDTH + 1;
	bounds.top = fTransformerListView->Frame().bottom + 1;
	bounds.bottom = bounds.top + menuHeight;
	menuBar = new BMenuBar(bounds, "property menu bar");
	menuBar->AddItem(fPropertyMenu);
	bg->AddChild(menuBar);
	menuBar->ResizeTo(bounds.Width(), bounds.Height());
		// menu bars resize themselves to window width
	menuBar->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);

	fPropertyListView = new IconObjectListView();

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
	bg->AddChild(new BScrollView("style list scroll view",
								 fStyleListView,
								 B_FOLLOW_LEFT | B_FOLLOW_TOP,
								 0, false, true,
								 B_NO_BORDER));
	bg->AddChild(new BScrollView("shape list scroll view",
								 fShapeListView,
								 B_FOLLOW_LEFT | B_FOLLOW_TOP,
								 0, false, true,
								 B_NO_BORDER));
	bg->AddChild(new BScrollView("transformer list scroll view",
								 fTransformerListView,
								 B_FOLLOW_LEFT | B_FOLLOW_TOP,
								 0, false, true,
								 B_NO_BORDER));

	// scroll view around property list view
	bounds.top = menuBar->Frame().bottom + 1;
	bounds.bottom = bg->Bounds().bottom;
	bg->AddChild(new ScrollView(fPropertyListView,
								SCROLL_VERTICAL | SCROLL_NO_FRAME,
								bounds, "property scroll view",
								B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM,
								B_WILL_DRAW | B_FRAME_EVENTS));


	bg->AddChild(fCanvasView);
}

// _CreateMenuBar
BMenuBar*
MainWindow::_CreateMenuBar(BRect frame)
{
	BMenuBar* menuBar = new BMenuBar(frame, "main menu");

	BMenu* fileMenu = new BMenu("File");
	BMenu* editMenu = new BMenu("Edit");
	fPathMenu = new BMenu("Path");
	fStyleMenu = new BMenu("Style");
	fShapeMenu = new BMenu("Shape");
	fTransformerMenu = new BMenu("Transformer");
	fPropertyMenu = new BMenu("Property");
	fSwatchMenu = new BMenu("Swatches");

	menuBar->AddItem(fileMenu);
	menuBar->AddItem(editMenu);

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
	fPathMenu->AddItem(new BMenuItem("New", new BMessage(MSG_NEW_PATH)));

	// Style
	fStyleMenu->AddItem(new BMenuItem("New", new BMessage(MSG_NEW_STYLE)));

	// Shape
	fShapeMenu->AddItem(new BMenuItem("New", new BMessage(MSG_NEW_SHAPE)));


	// Transformer
	BMenu* addMenu = new BMenu("Add");
	int32 cookie = 0;
	uint32 type;
	BString name;
	while (TransformerFactory::NextType(&cookie, &type, &name)) {
		BMessage* message = new BMessage(MSG_ADD_TRANSFORMER);
		message->AddInt32("type", type);
		addMenu->AddItem(new BMenuItem(name.String(), message));
	}
	addMenu->SetTargetForItems(this);
	fTransformerMenu->AddItem(addMenu);

	return menuBar;
}

