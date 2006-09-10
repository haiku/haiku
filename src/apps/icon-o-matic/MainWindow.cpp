/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "MainWindow.h"

#include <new>
#include <stdio.h>

#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <Screen.h>
#include <ScrollView.h>

#if __HAIKU__
# include <GridLayout.h>
# include <GroupLayout.h>
# include <GroupView.h>
#endif

#include "AddPathsCommand.h"
#include "AddShapesCommand.h"
#include "AddStylesCommand.h"
#include "AddTransformersCommand.h"
#include "CanvasView.h"
#include "CommandStack.h"
#include "CompoundCommand.h"
#include "CurrentColor.h"
#include "Document.h"
#include "Exporter.h"
#include "IconObjectListView.h"
#include "IconEditorApp.h"
#include "IconView.h"
#include "PathListView.h"
#include "ScrollView.h"
#include "ShapeListView.h"
#include "StyleListView.h"
#include "StyleView.h"
#include "SwatchGroup.h"
#include "TransformerFactory.h"
#include "TransformerListView.h"
#include "TransformGradientBox.h"
#include "TransformShapesBox.h"
#include "Util.h"

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
#include "StyleContainer.h"
#include "VectorPath.h"

using std::nothrow;

enum {
	MSG_UNDO						= 'undo',
	MSG_REDO						= 'redo',

	MSG_PATH_SELECTED				= 'vpsl',
	MSG_STYLE_SELECTED				= 'stsl',
	MSG_SHAPE_SELECTED				= 'spsl',

	MSG_SHAPE_RESET_TRANSFORMATION	= 'rtsh',
	MSG_STYLE_RESET_TRANSFORMATION	= 'rtst',

	MSG_ADD_TRANSFORMER				= 'adtr',

	MSG_MOUSE_FILTER_MODE			= 'mfmd',
};

// constructor
MainWindow::MainWindow(IconEditorApp* app, Document* document)
	: BWindow(BRect(50, 50, 900, 750), "Icon-O-Matic",
			  B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			  B_ASYNCHRONOUS_CONTROLS),
	  fApp(app),
	  fDocument(document),
	  fIcon(NULL)
{
	_Init();
}

// destructor
MainWindow::~MainWindow()
{
	delete fState;

	if (fIcon)
		fIcon->Release();

	fDocument->CommandStack()->RemoveObserver(this);
}

// #pragma mark -

// MessageReceived
void
MainWindow::MessageReceived(BMessage* message)
{
	if (!fDocument || !fDocument->WriteLock()) {
		BWindow::MessageReceived(message);
		return;
	}

	switch (message->what) {

		case B_REFS_RECEIVED:
		case B_SIMPLE_DATA:
			message->what = B_REFS_RECEIVED;
			if (modifiers() & B_SHIFT_KEY)
				message->AddBool("append", true);
			be_app->PostMessage(message);
			break;

		case MSG_UNDO:
			fDocument->CommandStack()->Undo();
			break;
		case MSG_REDO:
			fDocument->CommandStack()->Redo();
			break;

		case MSG_MOUSE_FILTER_MODE: {
			uint32 mode;
			if (message->FindInt32("mode", (int32*)&mode) == B_OK)
				fCanvasView->SetMouseFilterMode(mode);
			break;
		}

		case MSG_SET_ICON:
			SetIcon(fDocument->Icon());
			break;

		case MSG_ADD_SHAPE: {
			AddStylesCommand* styleCommand = NULL;
			Style* style = NULL;
			if (message->HasBool("style")) {
				new_style(CurrentColor::Default()->Color(),
						  fDocument->Icon()->Styles(), &style, &styleCommand);
			}
		
			AddPathsCommand* pathCommand = NULL;
			VectorPath* path = NULL;
			if (message->HasBool("path")) {
				new_path(fDocument->Icon()->Paths(), &path, &pathCommand);
			}
		
			if (!style) {
				// use current or first style
				int32 currentStyle = fStyleListView->CurrentSelection(0);
				style = fDocument->Icon()->Styles()->StyleAt(currentStyle);
				if (!style)
					style = fDocument->Icon()->Styles()->StyleAt(0);
			}
		
			Shape* shape = new (nothrow) Shape(style);
			Shape* shapes[1];
			shapes[0] = shape;
			AddShapesCommand* shapeCommand = new (nothrow) AddShapesCommand(
				fDocument->Icon()->Shapes(), shapes, 1,
				fDocument->Icon()->Shapes()->CountShapes(),
				fDocument->Selection());
		
			if (path && shape)
				shape->Paths()->AddPath(path);
		
			::Command* command = NULL;
			if (styleCommand || pathCommand) {
				if (styleCommand && pathCommand) {
					Command** commands = new Command*[3];
					commands[0] = styleCommand;
					commands[1] = pathCommand;
					commands[2] = shapeCommand;
					command = new CompoundCommand(commands, 3,
										"Add Shape With Path & Style", 0);
				} else if (styleCommand) {
					Command** commands = new Command*[2];
					commands[0] = styleCommand;
					commands[1] = shapeCommand;
					command = new CompoundCommand(commands, 2,
										"Add Shape With Style", 0);
				} else {
					Command** commands = new Command*[2];
					commands[0] = pathCommand;
					commands[1] = shapeCommand;
					command = new CompoundCommand(commands, 2,
										"Add Shape With Path", 0);
				}
			} else {
				command = shapeCommand;
			}
			fDocument->CommandStack()->Perform(command);
			break;
		}

// TODO: listen to selection in CanvasView to add a manipulator
case MSG_PATH_SELECTED: {
	VectorPath* path;
	if (message->FindPointer("path", (void**)&path) < B_OK)
		path = NULL;
	
	fState->DeleteManipulators();
	if (fDocument->Icon()->Paths()->HasPath(path)) {
		PathManipulator* pathManipulator = new (nothrow) PathManipulator(path);
		fState->AddManipulator(pathManipulator);
	}
	break;
}
case MSG_STYLE_SELECTED: {
	Style* style;
	if (message->FindPointer("style", (void**)&style) < B_OK)
		style = NULL;
	if (!fDocument->Icon()->Styles()->HasStyle(style))
		style = NULL;

	fStyleView->SetStyle(style);
	break;
}
case MSG_GRADIENT_SELECTED: {
	// if there is a gradient, add a transform box around it
	Gradient* gradient;
	if (message->FindPointer("gradient", (void**)&gradient) < B_OK)
		gradient = NULL;
	fState->DeleteManipulators();
	if (gradient) {
		TransformGradientBox* transformBox
			= new (nothrow) TransformGradientBox(fCanvasView,
												 gradient,
												 NULL);
		fState->AddManipulator(transformBox);
	}
	break;
}
case MSG_SHAPE_SELECTED: {
	Shape* shape;
	if (message->FindPointer("shape", (void**)&shape) < B_OK)
		shape = NULL;
	if (!fIcon || !fIcon->Shapes()->HasShape(shape))
		shape = NULL;

	fPathListView->SetCurrentShape(shape);
	fStyleListView->SetCurrentShape(shape);
	fTransformerListView->SetShape(shape);

	BList selectedShapes;
	ShapeContainer* shapes = fDocument->Icon()->Shapes();
	int32 count = shapes->CountShapes();
	for (int32 i = 0; i < count; i++) {
		shape = shapes->ShapeAtFast(i);
		if (shape->IsSelected()) {
			selectedShapes.AddItem((void*)shape);
		}
	}

	fState->DeleteManipulators();
	if (selectedShapes.CountItems() > 0) {
printf("selected shapes: %ld\n", selectedShapes.CountItems());
		TransformShapesBox* transformBox = new (nothrow) TransformShapesBox(
			fCanvasView,
			(const Shape**)selectedShapes.Items(),
			selectedShapes.CountItems());
		fState->AddManipulator(transformBox);
	}
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
			if (!transformer)
				break;

			Transformer* transformers[1];
			transformers[0] = transformer;
			::Command* command = new (nothrow) AddTransformersCommand(
				shape, transformers, 1, shape->CountTransformers());

			if (!command)
				delete transformer;

			fDocument->CommandStack()->Perform(command);
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

// WorkspaceActivated
void
MainWindow::WorkspaceActivated(int32 workspace, bool active)
{
	BWindow::WorkspaceActivated(workspace, active);

	// NOTE: hack to workaround buggy BScreen::DesktopColor() on R5

	uint32 workspaces = Workspaces();
	if (!active || ((1 << workspace) & workspaces) == 0)
		return;

	WorkspacesChanged(workspaces, workspaces);
}

// WorkspacesChanged
void
MainWindow::WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces)
{
	if (oldWorkspaces != newWorkspaces)
		BWindow::WorkspacesChanged(oldWorkspaces, newWorkspaces);

	BScreen screen(this);

	// Unfortunately, this is buggy on R5: screen.DesktopColor()
	// as well as ui_color(B_DESKTOP_COLOR) return the color
	// of the *active* screen, not the one on which this window
	// is. So this trick only works when you drag this window
	// from another workspace onto the current workspace, not
	// when you drag the window from the current workspace onto
	// another workspace and then switch to the other workspace.

	fIconPreview32Desktop->SetIconBGColor(screen.DesktopColor());
	fIconPreview64->SetIconBGColor(screen.DesktopColor());
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

// MakeEmpty
void
MainWindow::MakeEmpty()
{
	fPathListView->SetCurrentShape(NULL);
	fStyleListView->SetCurrentShape(NULL);
	fStyleView->SetStyle(NULL);

	fTransformerListView->SetShape(NULL);

	fState->DeleteManipulators();
}

// SetIcon
void
MainWindow::SetIcon(Icon* icon)
{
	if (fIcon == icon)
		return;

	Icon* oldIcon = fIcon;

	fIcon = icon;

	if (fIcon)
		fIcon->Acquire();
	else
		MakeEmpty();

	fCanvasView->SetIcon(fIcon);

	fPathListView->SetPathContainer(fIcon ? fIcon->Paths() : NULL);
	fPathListView->SetShapeContainer(fIcon ? fIcon->Shapes() : NULL);

	fStyleListView->SetStyleContainer(fIcon ? fIcon->Styles() : NULL);
	fStyleListView->SetShapeContainer(fIcon ? fIcon->Shapes() : NULL);

	fShapeListView->SetShapeContainer(fIcon ? fIcon->Shapes() : NULL);

	// icon previews
	fIconPreview16Folder->SetIcon(fIcon);
	fIconPreview16Menu->SetIcon(fIcon);
	fIconPreview32Folder->SetIcon(fIcon);
	fIconPreview32Desktop->SetIcon(fIcon);
//	fIconPreview48->SetIcon(fIcon);
	fIconPreview64->SetIcon(fIcon);

	// keep this last
	if (oldIcon)
		oldIcon->Release();
}

// #pragma mark -

// _Init
void
MainWindow::_Init()
{
	// create the GUI
	_CreateGUI(Bounds());

	// TODO: move this to CanvasView?
	fState = new MultipleManipulatorState(fCanvasView);
	fCanvasView->SetState(fState);

	fCanvasView->SetCatchAllEvents(true);
	fCanvasView->SetCommandStack(fDocument->CommandStack());
//	fCanvasView->SetSelection(fDocument->Selection());

	fPathListView->SetMenu(fPathMenu);
	fPathListView->SetCommandStack(fDocument->CommandStack());
	fPathListView->SetSelection(fDocument->Selection());

	fStyleListView->SetMenu(fStyleMenu);
	fStyleListView->SetCommandStack(fDocument->CommandStack());
	fStyleListView->SetSelection(fDocument->Selection());

	fStyleView->SetCommandStack(fDocument->CommandStack());
	fStyleView->SetCurrentColor(CurrentColor::Default());

	fShapeListView->SetMenu(fShapeMenu);
	fShapeListView->SetCommandStack(fDocument->CommandStack());
	fShapeListView->SetSelection(fDocument->Selection());

	fTransformerListView->SetCommandStack(fDocument->CommandStack());
	fTransformerListView->SetSelection(fDocument->Selection());

	fPropertyListView->SetCommandStack(fDocument->CommandStack());
	fPropertyListView->SetSelection(fDocument->Selection());
	fPropertyListView->SetMenu(fPropertyMenu);

	fDocument->CommandStack()->AddObserver(this);

	fSwatchGroup->SetCurrentColor(CurrentColor::Default());

	SetIcon(fDocument->Icon());
}

// _CreateGUI
void
MainWindow::_CreateGUI(BRect bounds)
{
	const float splitWidth = 160;

#ifdef __HAIKU__

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	BGridLayout* layout = new BGridLayout();
	BView* rootView = new BView("root view", 0, layout);
	AddChild(rootView);
	rootView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BGroupView* leftTopView = new BGroupView(B_VERTICAL);
	layout->AddView(leftTopView, 0, 0);
	leftTopView->SetExplicitMinSize(BSize(splitWidth, B_SIZE_UNSET));

	// views along the left side
	leftTopView->AddChild(_CreateMenuBar(bounds));

	BGroupView* iconPreviews = new BGroupView(B_HORIZONTAL);
	iconPreviews->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	iconPreviews->GroupLayout()->SetSpacing(5);

	// icon previews
	fIconPreview16Folder = new IconView(BRect(0, 0, 15, 15),
										"icon preview 16 folder");
	fIconPreview16Menu = new IconView(BRect(0, 0, 15, 15),
									  "icon preview 16 menu");
	fIconPreview16Menu->SetLowColor(ui_color(B_MENU_BACKGROUND_COLOR));

	fIconPreview32Folder = new IconView(BRect(0, 0, 31, 31),
										"icon preview 32 folder");
	fIconPreview32Desktop = new IconView(BRect(0, 0, 31, 31),
										 "icon preview 32 desktop");
	fIconPreview32Desktop->SetLowColor(ui_color(B_DESKTOP_COLOR));

//	fIconPreview48 = new IconView(bounds, "icon preview 48");
	fIconPreview64 = new IconView(BRect(0, 0, 63, 63), "icon preview 64");
	fIconPreview64->SetLowColor(ui_color(B_DESKTOP_COLOR));


	BGroupView* smallPreviews = new BGroupView(B_VERTICAL);
	smallPreviews->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	smallPreviews->GroupLayout()->SetSpacing(5);

	smallPreviews->AddChild(fIconPreview16Folder);
	smallPreviews->AddChild(fIconPreview16Menu);

	BGroupView* mediumPreviews = new BGroupView(B_VERTICAL);
	mediumPreviews->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	mediumPreviews->GroupLayout()->SetSpacing(5);

	mediumPreviews->AddChild(fIconPreview32Folder);
	mediumPreviews->AddChild(fIconPreview32Desktop);

//	iconPreviews->AddChild(fIconPreview48);

	iconPreviews->AddChild(smallPreviews);
	iconPreviews->AddChild(mediumPreviews);
	iconPreviews->AddChild(fIconPreview64);
	iconPreviews->SetExplicitMaxSize(BSize(B_SIZE_UNSET, B_SIZE_UNLIMITED));

	leftTopView->AddChild(iconPreviews);

	
	BGroupView* leftSideView = new BGroupView(B_VERTICAL);
	layout->AddView(leftSideView, 0, 1);
	leftSideView->SetExplicitMinSize(BSize(splitWidth, B_SIZE_UNSET));

	// path menu and list view
	BMenuBar* menuBar = new BMenuBar(bounds, "path menu bar");
	menuBar->AddItem(fPathMenu);
	leftSideView->AddChild(menuBar);

	fPathListView = new PathListView(BRect(0, 0, splitWidth, 100),
									 "path list view",
									 new BMessage(MSG_PATH_SELECTED), this);

	BView* scrollView = new BScrollView("path list scroll view",
										fPathListView,
										B_FOLLOW_NONE, 0, false, true,
										B_NO_BORDER);
	leftSideView->AddChild(scrollView);

	// shape list view
	menuBar = new BMenuBar(bounds, "shape menu bar");
	menuBar->AddItem(fShapeMenu);
	leftSideView->AddChild(menuBar);

	fShapeListView = new ShapeListView(BRect(0, 0, splitWidth, 100),
									   "shape list view",
									   new BMessage(MSG_SHAPE_SELECTED), this);
	scrollView = new BScrollView("shape list scroll view",
								 fShapeListView,
								 B_FOLLOW_NONE, 0, false, true,
								 B_NO_BORDER);
	leftSideView->AddChild(scrollView);

	// transformer list view
	menuBar = new BMenuBar(bounds, "transformer menu bar");
	menuBar->AddItem(fTransformerMenu);
	leftSideView->AddChild(menuBar);

	fTransformerListView = new TransformerListView(BRect(0, 0, splitWidth, 100),
												   "transformer list view");
	scrollView = new BScrollView("transformer list scroll view",
								 fTransformerListView,
								 B_FOLLOW_NONE, 0, false, true,
								 B_NO_BORDER);
	leftSideView->AddChild(scrollView);

	// property list view
	menuBar = new BMenuBar(bounds, "property menu bar");
	menuBar->AddItem(fPropertyMenu);
	leftSideView->AddChild(menuBar);

	fPropertyListView = new IconObjectListView();

	// scroll view around property list view
	ScrollView* propScrollView = new ScrollView(fPropertyListView,
										SCROLL_VERTICAL | SCROLL_NO_FRAME,
										BRect(0, 0, splitWidth, 100),
										"property scroll view",
										B_FOLLOW_NONE,
										B_WILL_DRAW | B_FRAME_EVENTS);
	leftSideView->AddChild(propScrollView);

	BGroupLayout* topSide = new BGroupLayout(B_HORIZONTAL);
	BView* topSideView = new BView("top side view", 0, topSide);
	layout->AddView(topSideView, 1, 0);

	// canvas view
	fCanvasView = new CanvasView(BRect(0, 0, 200, 200));
	layout->AddView(fCanvasView, 1, 1);

	// views along the top

	BGroupLayout* styleGroup = new BGroupLayout(B_VERTICAL);
	BView* styleGroupView = new BView("style group", 0, styleGroup);
	topSide->AddView(styleGroupView);

	// style list view
	menuBar = new BMenuBar(bounds, "style menu bar");
	menuBar->AddItem(fStyleMenu);
	styleGroup->AddView(menuBar);

	fStyleListView = new StyleListView(BRect(0, 0, splitWidth, 100),
									   "style list view",
									   new BMessage(MSG_STYLE_SELECTED), this);
	scrollView = new BScrollView("style list scroll view",
								 fStyleListView,
								 B_FOLLOW_NONE, 0, false, true,
								 B_NO_BORDER);
	scrollView->SetExplicitMaxSize(BSize(splitWidth, B_SIZE_UNLIMITED));
	styleGroup->AddView(scrollView);

	// style view
	fStyleView = new StyleView(BRect(0, 0, 200, 100));
	topSide->AddView(fStyleView);

	// swatch group
	BGroupLayout* swatchGroup = new BGroupLayout(B_VERTICAL);
	BView* swatchGroupView = new BView("swatch group", 0, swatchGroup);
	topSide->AddView(swatchGroupView);

	menuBar = new BMenuBar(bounds, "swatches menu bar");
	menuBar->AddItem(fSwatchMenu);
	swatchGroup->AddView(menuBar);

	fSwatchGroup = new SwatchGroup(BRect(0, 0, 100, 100));
	swatchGroup->AddView(fSwatchGroup);

	swatchGroupView->SetExplicitMaxSize(swatchGroupView->MinSize());

	// make sure the top side has fixed height
	topSideView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED,
		swatchGroupView->MinSize().height));

#else // __HAIKU__

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
	bounds.right = bg->Bounds().right - B_V_SCROLL_BAR_WIDTH;
	bounds.bottom = bg->Bounds().bottom - B_H_SCROLL_BAR_HEIGHT;
	fCanvasView = new CanvasView(bounds);

	// scroll view around canvas view
	bounds.bottom += B_H_SCROLL_BAR_HEIGHT;
	bounds.right += B_V_SCROLL_BAR_WIDTH;
	ScrollView* canvasScrollView
		= new ScrollView(fCanvasView,
						 SCROLL_HORIZONTAL | SCROLL_VERTICAL
						 | SCROLL_VISIBLE_RECT_IS_CHILD_BOUNDS
						 | SCROLL_NO_FRAME,
						 bounds, "canvas scroll view",
						 B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);

	// icon previews
	bounds.left = 5;
	bounds.top = fSwatchGroup->Frame().top + 5;
	bounds.right = bounds.left + 15;
	bounds.bottom = bounds.top + 15;
	fIconPreview16Folder = new IconView(bounds, "icon preview 16 folder");

	bounds.top = fIconPreview16Folder->Frame().bottom + 5;
	bounds.bottom = bounds.top + 15;
	fIconPreview16Menu = new IconView(bounds, "icon preview 16 menu");
	fIconPreview16Menu->SetLowColor(ui_color(B_MENU_BACKGROUND_COLOR));

	bounds.left = fIconPreview16Folder->Frame().right + 5;
	bounds.top = fSwatchGroup->Frame().top + 5;
	bounds.right = bounds.left + 31;
	bounds.bottom = bounds.top + 31;
	fIconPreview32Folder = new IconView(bounds, "icon preview 32 folder");

	bounds.top = fIconPreview32Folder->Frame().bottom + 5;
	bounds.bottom = bounds.top + 31;
	fIconPreview32Desktop = new IconView(bounds, "icon preview 32 desktop");
	fIconPreview32Desktop->SetLowColor(ui_color(B_DESKTOP_COLOR));

//	bounds.OffsetBy(bounds.Width() + 6, 0);
//	bounds.right = bounds.left + 47;
//	bounds.bottom = bounds.top + 47;
//	fIconPreview48 = new IconView(bounds, "icon preview 48");

	bounds.left = fIconPreview32Folder->Frame().right + 5;
	bounds.top = fSwatchGroup->Frame().top + 5;
	bounds.right = bounds.left + 63;
	bounds.bottom = bounds.top + 63;
	fIconPreview64 = new IconView(bounds, "icon preview 64");
	fIconPreview64->SetLowColor(ui_color(B_DESKTOP_COLOR));

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


	// style view
	bounds.left = menuBar->Frame().right + 1;
	bounds.top = bg->Bounds().top;
	bounds.right = fSwatchGroup->Frame().left - 1;
	bounds.bottom = fCanvasView->Frame().top - 1;
	fStyleView = new StyleView(bounds);
	bg->AddChild(fStyleView);

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

	bg->AddChild(fIconPreview16Folder);
	bg->AddChild(fIconPreview16Menu);
	bg->AddChild(fIconPreview32Folder);
	bg->AddChild(fIconPreview32Desktop);
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


	bg->AddChild(canvasScrollView);
#endif // __HAIKU__
}

// _CreateMenuBar
BMenuBar*
MainWindow::_CreateMenuBar(BRect frame)
{
	BMenuBar* menuBar = new BMenuBar(frame, "main menu");

	BMenu* fileMenu = new BMenu("File");
	BMenu* editMenu = new BMenu("Edit");
	BMenu* settingsMenu = new BMenu("Options");
	fPathMenu = new BMenu("Path");
	fStyleMenu = new BMenu("Style");
	fShapeMenu = new BMenu("Shape");
	fTransformerMenu = new BMenu("Transformer");
	fPropertyMenu = new BMenu("Property");
	fSwatchMenu = new BMenu("Swatches");

	menuBar->AddItem(fileMenu);
	menuBar->AddItem(editMenu);
	menuBar->AddItem(settingsMenu);

	// File
	fileMenu->AddItem(new BMenuItem("New",
									new BMessage(MSG_NEW),
									'N'));
	fileMenu->AddItem(new BMenuItem("Open",
									new BMessage(MSG_OPEN),
									'O'));
	fileMenu->AddItem(new BMenuItem("Append",
									new BMessage(MSG_APPEND),
									'O', B_SHIFT_KEY));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Save",
									new BMessage(MSG_SAVE),
									'S'));
	fileMenu->AddItem(new BMenuItem("Save As",
									new BMessage(MSG_SAVE_AS),
									'S', B_SHIFT_KEY));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Export Flat",
									new BMessage(MSG_EXPORT),
									'E'));
	fileMenu->AddItem(new BMenuItem("Export Flat As",
									new BMessage(MSG_EXPORT_AS),
									'E', B_SHIFT_KEY));
	fileMenu->AddItem(new BMenuItem("Export RDef As",
									new BMessage(MSG_EXPORT_ICON_RDEF)));
	fileMenu->AddItem(new BMenuItem("Export SVG As",
									new BMessage(MSG_EXPORT_SVG)));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Export Bitmap As",
									new BMessage(MSG_EXPORT_BITMAP)));
	fileMenu->AddItem(new BMenuItem("Export Bitmap Set As",
									new BMessage(MSG_EXPORT_BITMAP_SET)));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Export Icon Attribute",
									new BMessage(MSG_EXPORT_ICON_ATTRIBUTE)));
	fileMenu->AddItem(new BMenuItem("Export Icon Mime Attribute",
									new BMessage(MSG_EXPORT_ICON_MIME_ATTRIBUTE)));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Quit",
									new BMessage(B_QUIT_REQUESTED),
									'Q'));
	fileMenu->SetTargetForItems(be_app);

	// Edit
	fUndoMI = new BMenuItem("<nothing to undo>",
							new BMessage(MSG_UNDO), 'Z');
	fRedoMI = new BMenuItem("<nothing to redo>",
							new BMessage(MSG_REDO), 'Z', B_SHIFT_KEY);

	fUndoMI->SetEnabled(false);
	fRedoMI->SetEnabled(false);

	editMenu->AddItem(fUndoMI);
	editMenu->AddItem(fRedoMI);

	// Settings
	BMenu* filterModeMenu = new BMenu("Snap to Grid");
	BMessage* message = new BMessage(MSG_MOUSE_FILTER_MODE);
	message->AddInt32("mode", SNAPPING_OFF);
	filterModeMenu->AddItem(new BMenuItem("Off", message));

	message = new BMessage(MSG_MOUSE_FILTER_MODE);
	message->AddInt32("mode", SNAPPING_64);
	filterModeMenu->AddItem(new BMenuItem("64 x 64", message));

	message = new BMessage(MSG_MOUSE_FILTER_MODE);
	message->AddInt32("mode", SNAPPING_32);
	filterModeMenu->AddItem(new BMenuItem("32 x 32", message));

	message = new BMessage(MSG_MOUSE_FILTER_MODE);
	message->AddInt32("mode", SNAPPING_16);
	filterModeMenu->AddItem(new BMenuItem("16 x 16", message));

	filterModeMenu->ItemAt(0)->SetMarked(true);
	filterModeMenu->SetRadioMode(true);

	settingsMenu->AddItem(filterModeMenu);

	// Transformer
	BMenu* addMenu = new BMenu("Add");
	int32 cookie = 0;
	uint32 type;
	BString name;
	while (TransformerFactory::NextType(&cookie, &type, &name)) {
		message = new BMessage(MSG_ADD_TRANSFORMER);
		message->AddInt32("type", type);
		addMenu->AddItem(new BMenuItem(name.String(), message));
	}
	addMenu->SetTargetForItems(this);
	fTransformerMenu->AddItem(addMenu);

	return menuBar;
}

//// _CreateDefaultIcon
//void
//MainWindow::_CreateDefaultIcon()
//{
//	// add some stuff to an empty document (NOTE: for testing only)
//	VectorPath* path = new VectorPath();
//
//	fDocument->Icon()->Paths()->AddPath(path);
//
//	Style* style1 = new Style();
//	style1->SetName("Style White");
//	style1->SetColor((rgb_color){ 255, 255, 255, 255 });
//
//	fDocument->Icon()->Styles()->AddStyle(style1);
//
//	Style* style2 = new Style();
//	style2->SetName("Style Gradient");
//	Gradient gradient(true);
//	gradient.AddColor((rgb_color){ 255, 211, 6, 255 }, 0.0);
//	gradient.AddColor((rgb_color){ 255, 238, 160, 255 }, 0.5);
//	gradient.AddColor((rgb_color){ 208, 43, 92, 255 }, 1.0);
//	style2->SetGradient(&gradient);
//
//	fDocument->Icon()->Styles()->AddStyle(style2);
//
//	Shape* shape = new Shape(style2);
//	shape->Paths()->AddPath(path);
//
//	shape->SetName("Gradient");
//	fDocument->Icon()->Shapes()->AddShape(shape);
//
//	shape = new Shape(style1);
//	shape->Paths()->AddPath(path);
//	StrokeTransformer* transformer
//		= new StrokeTransformer(shape->VertexSource());
//	transformer->width(5.0);
//	shape->AddTransformer(transformer);
//
//	shape->SetName("Outline");
//	fDocument->Icon()->Shapes()->AddShape(shape);
//
//	Style* style3 = new Style();
//	style3->SetName("Style Red");
//	style3->SetColor((rgb_color){ 255, 0, 169,200 });
//
//	fDocument->Icon()->Styles()->AddStyle(style3);
//
//	shape = new Shape(style3);
//	shape->Paths()->AddPath(path);
//	AffineTransformer* transformer2
//		= new AffineTransformer(shape->VertexSource());
//	*transformer2 *= agg::trans_affine_translation(10.0, 6.0);
//	*transformer2 *= agg::trans_affine_rotation(0.2);
//	*transformer2 *= agg::trans_affine_scaling(0.8, 0.6);
//	shape->AddTransformer(transformer2);
//
//	shape->SetName("Transformed");
//	fDocument->Icon()->Shapes()->AddShape(shape);
//
//	PathManipulator* pathManipulator = new PathManipulator(path);
//	fState->AddManipulator(pathManipulator);
//}
