/*
 * Copyright 2006-2010, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "MainWindow.h"

#include <new>
#include <stdio.h>

#include <Clipboard.h>
#include <GridLayout.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <Screen.h>
#include <ScrollView.h>
#include <Catalog.h>
#include <Locale.h>

#include "support_ui.h"

#include "AddPathsCommand.h"
#include "AddShapesCommand.h"
#include "AddStylesCommand.h"
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
#include "TransformerListView.h"
#include "TransformGradientBox.h"
#include "TransformShapesBox.h"
#include "Util.h"

// TODO: just for testing
#include "AffineTransformer.h"
#include "GradientTransformable.h"
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

#include "StyledTextImporter.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Icon-O-Matic-Main"


using std::nothrow;

enum {
	MSG_UNDO						= 'undo',
	MSG_REDO						= 'redo',

	MSG_PATH_SELECTED				= 'vpsl',
	MSG_STYLE_SELECTED				= 'stsl',
	MSG_SHAPE_SELECTED				= 'spsl',

	MSG_SHAPE_RESET_TRANSFORMATION	= 'rtsh',
	MSG_STYLE_RESET_TRANSFORMATION	= 'rtst',

	MSG_MOUSE_FILTER_MODE			= 'mfmd',

	MSG_RENAME_OBJECT				= 'rnam',
};


MainWindow::MainWindow(IconEditorApp* app, Document* document,
		const BMessage* settings)
	:
	BWindow(BRect(50, 50, 900, 750), "Icon-O-Matic",
		B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fApp(app),
	fDocument(document),
	fIcon(NULL)
{
	_Init();

	RestoreSettings(settings);
}


MainWindow::~MainWindow()
{
	delete fState;

	if (fIcon)
		fIcon->Release();

	fDocument->CommandStack()->RemoveObserver(this);
}


// #pragma mark -


void
MainWindow::MessageReceived(BMessage* message)
{
	bool discard = false;

	if (!fDocument || !fDocument->WriteLock()) {
		BWindow::MessageReceived(message);
		return;
	}

	if (message->WasDropped()) {
		const rgb_color *color;
		int32 len;
		int32 i;
		// create styles from dropped colors
		for (i = 0; message->FindData("RGBColor", B_RGB_COLOR_TYPE, i, 
			(const void **)&color, &len) == B_OK; i++) {
			if (len != sizeof(rgb_color))
				continue;
			char name[30];
			sprintf(name, 
				B_TRANSLATE_WITH_CONTEXT("Color (#%02x%02x%02x)", 
					"Style name after dropping a color"), 
				color->red, color->green, color->blue);
			Style* style = new (nothrow) Style(*color);
			style->SetName(name);
			Style* styles[1] = { style };
			AddStylesCommand* styleCommand = new (nothrow) AddStylesCommand(
				fDocument->Icon()->Styles(), styles, 1,
				fDocument->Icon()->Styles()->CountStyles());
			fDocument->CommandStack()->Perform(styleCommand);
			// don't handle anything else,
			// or we might paste the clipboard on B_PASTE
			discard = true;
		}
	}

	switch (message->what) {

		case B_REFS_RECEIVED:
		case B_SIMPLE_DATA:
			message->what = B_REFS_RECEIVED;
			if (modifiers() & B_SHIFT_KEY)
				message->AddBool("append", true);
			be_app->PostMessage(message);
			break;

		case B_PASTE:
		case B_MIME_DATA:
		{
			BMessage *clip = message;
			status_t err;

			if (discard)
				break;

			if (message->what == B_PASTE) {
				if (!be_clipboard->Lock())
					break;
				clip = be_clipboard->Data();
			}

			if (!clip || !clip->HasData("text/plain", B_MIME_TYPE)) {
				if (message->what == B_PASTE)
					be_clipboard->Unlock();
				break;
			}

			Icon* icon;
			icon = new (nothrow) Icon(*fDocument->Icon());
			if (icon) {
				StyledTextImporter importer;
				err = importer.Import(icon, clip);
				if (err >= B_OK) {
						AutoWriteLocker locker(fDocument);

						SetIcon(NULL);

						// incorporate the loaded icon into the document
						// (either replace it or append to it)
						fDocument->MakeEmpty(false);
							// if append, the document savers are preserved
						fDocument->SetIcon(icon);
						SetIcon(icon);
				}
			}

			if (message->what == B_PASTE)
				be_clipboard->Unlock();
			break;
		}

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
						B_TRANSLATE_WITH_CONTEXT("Add shape with path & style",
							"Icon-O-Matic-Menu-Shape"),
						0);
				} else if (styleCommand) {
					Command** commands = new Command*[2];
					commands[0] = styleCommand;
					commands[1] = shapeCommand;
					command = new CompoundCommand(commands, 2,
						B_TRANSLATE_WITH_CONTEXT("Add shape with style",
							"Icon-O-Matic-Menu-Shape"), 
						0);
				} else {
					Command** commands = new Command*[2];
					commands[0] = pathCommand;
					commands[1] = shapeCommand;
					command = new CompoundCommand(commands, 2,
						B_TRANSLATE_WITH_CONTEXT("Add shape with path",
							"Icon-O-Matic-Menu-Shape"), 
						0);
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

	fPathListView->SetCurrentShape(NULL);
	fStyleListView->SetCurrentShape(NULL);
	fTransformerListView->SetShape(NULL);
	
	fState->DeleteManipulators();
	if (fDocument->Icon()->Paths()->HasPath(path)) {
		PathManipulator* pathManipulator = new (nothrow) PathManipulator(path);
		fState->AddManipulator(pathManipulator);
	}
	break;
}
case MSG_STYLE_SELECTED:
case MSG_STYLE_TYPE_CHANGED: {
	Style* style;
	if (message->FindPointer("style", (void**)&style) < B_OK)
		style = NULL;
	if (!fDocument->Icon()->Styles()->HasStyle(style))
		style = NULL;

	fStyleView->SetStyle(style);
	fPathListView->SetCurrentShape(NULL);
	fStyleListView->SetCurrentShape(NULL);
	fTransformerListView->SetShape(NULL);

	fState->DeleteManipulators();
	Gradient* gradient = style ? style->Gradient() : NULL;

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
		TransformShapesBox* transformBox = new (nothrow) TransformShapesBox(
			fCanvasView,
			(const Shape**)selectedShapes.Items(),
			selectedShapes.CountItems());
		fState->AddManipulator(transformBox);
	}
	break;
}
		case MSG_RENAME_OBJECT:
			fPropertyListView->FocusNameProperty();
			break;

		default:
			BWindow::MessageReceived(message);
	}

	fDocument->WriteUnlock();
}


bool
MainWindow::QuitRequested()
{
	// forward this to app but return "false" in order
	// to have a single code path for quitting
	be_app->PostMessage(B_QUIT_REQUESTED);

	return false;
}


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


void
MainWindow::ObjectChanged(const Observable* object)
{
	if (!fDocument)
		return;

	if (!Lock())
		return;

	if (object == fDocument->CommandStack()) {
		// relable Undo item and update enabled status
		BString label(B_TRANSLATE("Undo"));
		fUndoMI->SetEnabled(fDocument->CommandStack()->GetUndoName(label));
		if (fUndoMI->IsEnabled())
			fUndoMI->SetLabel(label.String());
		else
			fUndoMI->SetLabel(B_TRANSLATE_WITH_CONTEXT("<nothing to undo>",
				"Icon-O-Matic-Menu-Edit"));

		// relable Redo item and update enabled status
		label.SetTo(B_TRANSLATE("Redo"));
		fRedoMI->SetEnabled(fDocument->CommandStack()->GetRedoName(label));
		if (fRedoMI->IsEnabled())
			fRedoMI->SetLabel(label.String());
		else
			fRedoMI->SetLabel(B_TRANSLATE_WITH_CONTEXT("<nothing to redo>",
				"Icon-O-Matic-Menu-Edit"));
	}

	Unlock();
}


// #pragma mark -


void
MainWindow::MakeEmpty()
{
	fPathListView->SetCurrentShape(NULL);
	fStyleListView->SetCurrentShape(NULL);
	fStyleView->SetStyle(NULL);

	fTransformerListView->SetShape(NULL);

	fState->DeleteManipulators();
}


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


void
MainWindow::StoreSettings(BMessage* archive)
{
	if (archive->ReplaceRect("main window frame", Frame()) != B_OK)
		archive->AddRect("main window frame", Frame());
	if (archive->ReplaceUInt32("mouse filter mode",
			fCanvasView->MouseFilterMode()) != B_OK) {
		archive->AddUInt32("mouse filter mode",
			fCanvasView->MouseFilterMode());
	}
}


void
MainWindow::RestoreSettings(const BMessage* archive)
{
	BRect frame;
	if (archive->FindRect("main window frame", &frame) == B_OK) {
		make_sure_frame_is_on_screen(frame, this);
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}
	uint32 mouseFilterMode;
	if (archive->FindUInt32("mouse filter mode", &mouseFilterMode) == B_OK)
		fCanvasView->SetMouseFilterMode(mouseFilterMode);
}


// #pragma mark -


void
MainWindow::_Init()
{
	// create the GUI
	_CreateGUI(Bounds());

	// fix up scrollbar layout in listviews
	_ImproveScrollBarLayout(fPathListView);
	_ImproveScrollBarLayout(fStyleListView);
	_ImproveScrollBarLayout(fShapeListView);
	_ImproveScrollBarLayout(fTransformerListView);

	// TODO: move this to CanvasView?
	fState = new MultipleManipulatorState(fCanvasView);
	fCanvasView->SetState(fState);

	fCanvasView->SetCatchAllEvents(true);
	fCanvasView->SetCommandStack(fDocument->CommandStack());
	fCanvasView->SetMouseFilterMode(SNAPPING_64);
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

	fTransformerListView->SetMenu(fTransformerMenu);
	fTransformerListView->SetCommandStack(fDocument->CommandStack());
	fTransformerListView->SetSelection(fDocument->Selection());

	fPropertyListView->SetCommandStack(fDocument->CommandStack());
	fPropertyListView->SetSelection(fDocument->Selection());
	fPropertyListView->SetMenu(fPropertyMenu);

	fDocument->CommandStack()->AddObserver(this);

	fSwatchGroup->SetCurrentColor(CurrentColor::Default());

	SetIcon(fDocument->Icon());

	AddShortcut('Y', 0, new BMessage(MSG_UNDO));
	AddShortcut('Y', B_SHIFT_KEY, new BMessage(MSG_REDO));
	AddShortcut('E', 0, new BMessage(MSG_RENAME_OBJECT));
}


void
MainWindow::_CreateGUI(BRect bounds)
{
	const float splitWidth = 13 * be_plain_font->Size();

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	BGridLayout* layout = new BGridLayout();
	layout->SetSpacing(0, 0);
	BView* rootView = new BView("root view", 0, layout);
	AddChild(rootView);
	rootView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BGroupView* leftTopView = new BGroupView(B_VERTICAL, 0);
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

	
	BGroupView* leftSideView = new BGroupView(B_VERTICAL, 0);
	layout->AddView(leftSideView, 0, 1);
	leftSideView->SetExplicitMinSize(BSize(splitWidth, B_SIZE_UNSET));

	// path menu and list view
	BMenuBar* menuBar = new BMenuBar(bounds, "path menu bar");
	menuBar->AddItem(fPathMenu);
	leftSideView->AddChild(menuBar);

	fPathListView = new PathListView(BRect(0, 0, splitWidth, 100),
		"path list view", new BMessage(MSG_PATH_SELECTED), this);

	BView* scrollView = new BScrollView("path list scroll view",
		fPathListView, B_FOLLOW_NONE, 0, false, true, B_NO_BORDER);
	leftSideView->AddChild(scrollView);

	// shape list view
	menuBar = new BMenuBar(bounds, "shape menu bar");
	menuBar->AddItem(fShapeMenu);
	leftSideView->AddChild(menuBar);

	fShapeListView = new ShapeListView(BRect(0, 0, splitWidth, 100),
		"shape list view", new BMessage(MSG_SHAPE_SELECTED), this);
	scrollView = new BScrollView("shape list scroll view",
		fShapeListView, B_FOLLOW_NONE, 0, false, true, B_NO_BORDER);
	leftSideView->AddChild(scrollView);

	// transformer list view
	menuBar = new BMenuBar(bounds, "transformer menu bar");
	menuBar->AddItem(fTransformerMenu);
	leftSideView->AddChild(menuBar);

	fTransformerListView = new TransformerListView(BRect(0, 0, splitWidth, 100),
		"transformer list view");
	scrollView = new BScrollView("transformer list scroll view",
		fTransformerListView, B_FOLLOW_NONE, 0, false, true, B_NO_BORDER);
	leftSideView->AddChild(scrollView);

	// property list view
	menuBar = new BMenuBar(bounds, "property menu bar");
	menuBar->AddItem(fPropertyMenu);
	leftSideView->AddChild(menuBar);

	fPropertyListView = new IconObjectListView();

	// scroll view around property list view
	ScrollView* propScrollView = new ScrollView(fPropertyListView,
		SCROLL_VERTICAL, BRect(0, 0, splitWidth, 100), "property scroll view",
		B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER,
		BORDER_RIGHT);
	leftSideView->AddChild(propScrollView);

	BGroupLayout* topSide = new BGroupLayout(B_HORIZONTAL);
	topSide->SetSpacing(0);
	BView* topSideView = new BView("top side view", 0, topSide);
	layout->AddView(topSideView, 1, 0);

	// canvas view
	BRect canvasBounds = BRect(0, 0, 200, 200);
	fCanvasView = new CanvasView(canvasBounds);

	// scroll view around canvas view
	canvasBounds.bottom += B_H_SCROLL_BAR_HEIGHT;
	canvasBounds.right += B_V_SCROLL_BAR_WIDTH;
	ScrollView* canvasScrollView = new ScrollView(fCanvasView, SCROLL_VERTICAL
			| SCROLL_HORIZONTAL | SCROLL_VISIBLE_RECT_IS_CHILD_BOUNDS,
		canvasBounds, "canvas scroll view", B_FOLLOW_NONE,
		B_WILL_DRAW | B_FRAME_EVENTS, B_NO_BORDER);
	layout->AddView(canvasScrollView, 1, 1);

	// views along the top

	BGroupLayout* styleGroup = new BGroupLayout(B_VERTICAL, 0);
	BView* styleGroupView = new BView("style group", 0, styleGroup);
	topSide->AddView(styleGroupView);

	// style list view
	menuBar = new BMenuBar(bounds, "style menu bar");
	menuBar->AddItem(fStyleMenu);
	styleGroup->AddView(menuBar);

	fStyleListView = new StyleListView(BRect(0, 0, splitWidth, 100),
		"style list view", new BMessage(MSG_STYLE_SELECTED), this);
	scrollView = new BScrollView("style list scroll view", fStyleListView,
		B_FOLLOW_NONE, 0, false, true, B_NO_BORDER);
	scrollView->SetExplicitMaxSize(BSize(splitWidth, B_SIZE_UNLIMITED));
	styleGroup->AddView(scrollView);

	// style view
	fStyleView = new StyleView(BRect(0, 0, 200, 100));
	topSide->AddView(fStyleView);

	// swatch group
	BGroupLayout* swatchGroup = new BGroupLayout(B_VERTICAL);
	swatchGroup->SetSpacing(0);
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
}

BMenuBar*
MainWindow::_CreateMenuBar(BRect frame)
{
	BMenuBar* menuBar = new BMenuBar(frame, "main menu");


	#undef B_TRANSLATE_CONTEXT
	#define B_TRANSLATE_CONTEXT "Icon-O-Matic-Menus"
	
	
	BMenu* fileMenu = new BMenu(B_TRANSLATE("File"));
	BMenu* editMenu = new BMenu(B_TRANSLATE("Edit"));
	BMenu* settingsMenu = new BMenu(B_TRANSLATE("Options"));
	fPathMenu = new BMenu(B_TRANSLATE("Path"));
	fStyleMenu = new BMenu(B_TRANSLATE("Style"));
	fShapeMenu = new BMenu(B_TRANSLATE("Shape"));
	fTransformerMenu = new BMenu(B_TRANSLATE("Transformer"));
	fPropertyMenu = new BMenu(B_TRANSLATE("Properties"));
	fSwatchMenu = new BMenu(B_TRANSLATE("Swatches"));

	menuBar->AddItem(fileMenu);
	menuBar->AddItem(editMenu);
	menuBar->AddItem(settingsMenu);


	// File
	#undef B_TRANSLATE_CONTEXT
	#define B_TRANSLATE_CONTEXT "Icon-O-Matic-Menu-File"
	
	
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("New"),
		new BMessage(MSG_NEW), 'N'));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Open"B_UTF8_ELLIPSIS),
		new BMessage(MSG_OPEN), 'O'));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Append"B_UTF8_ELLIPSIS),
		new BMessage(MSG_APPEND), 'O', B_SHIFT_KEY));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Save"),
		new BMessage(MSG_SAVE), 'S'));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Save as"B_UTF8_ELLIPSIS),
		new BMessage(MSG_SAVE_AS), 'S', B_SHIFT_KEY));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Export"),
		new BMessage(MSG_EXPORT), 'P'));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Export as"B_UTF8_ELLIPSIS),
		new BMessage(MSG_EXPORT_AS), 'P', B_SHIFT_KEY));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	fileMenu->SetTargetForItems(be_app);


	// Edit
	#undef B_TRANSLATE_CONTEXT
	#define B_TRANSLATE_CONTEXT "Icon-O-Matic-Menu-Edit"
	
	
	fUndoMI = new BMenuItem(B_TRANSLATE("<nothing to undo>"),
		new BMessage(MSG_UNDO), 'Z');
	fRedoMI = new BMenuItem(B_TRANSLATE("<nothing to redo>"),
		new BMessage(MSG_REDO), 'Z', B_SHIFT_KEY);

	fUndoMI->SetEnabled(false);
	fRedoMI->SetEnabled(false);

	editMenu->AddItem(fUndoMI);
	editMenu->AddItem(fRedoMI);


	// Settings
	#undef B_TRANSLATE_CONTEXT
	#define B_TRANSLATE_CONTEXT "Icon-O-Matic-Menu-Settings"
	
	
	BMenu* filterModeMenu = new BMenu(B_TRANSLATE("Snap to grid"));
	BMessage* message = new BMessage(MSG_MOUSE_FILTER_MODE);
	message->AddInt32("mode", SNAPPING_OFF);
	filterModeMenu->AddItem(new BMenuItem(B_TRANSLATE("Off"), message, '4'));

	message = new BMessage(MSG_MOUSE_FILTER_MODE);
	message->AddInt32("mode", SNAPPING_64);
	filterModeMenu->AddItem(new BMenuItem("64 x 64", message, '3'));

	message = new BMessage(MSG_MOUSE_FILTER_MODE);
	message->AddInt32("mode", SNAPPING_32);
	filterModeMenu->AddItem(new BMenuItem("32 x 32", message, '2'));

	message = new BMessage(MSG_MOUSE_FILTER_MODE);
	message->AddInt32("mode", SNAPPING_16);
	filterModeMenu->AddItem(new BMenuItem("16 x 16", message, '1'));

	filterModeMenu->ItemAt(1)->SetMarked(true);
	filterModeMenu->SetRadioMode(true);

	settingsMenu->AddItem(filterModeMenu);

	return menuBar;
}


void
MainWindow::_ImproveScrollBarLayout(BView* target)
{
	// NOTE: The BListViews for which this function is used
	// are directly below a BMenuBar. If the BScrollBar and
	// the BMenuBar share bottom/top border respectively, the
	// GUI looks a little more polished. This trick can be
	// removed if/when the BScrollViews are embedded in a
	// surounding border like in WonderBrush.

	if (BScrollBar* scrollBar = target->ScrollBar(B_VERTICAL)) {
		scrollBar->MoveBy(0, -1);
		scrollBar->ResizeBy(0, 1);
	}
}
