/*
 * Copyright 2006-2011, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "MainWindow.h"

#include <new>
#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <GridLayout.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <fs_attr.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <Screen.h>
#include <ScrollView.h>

#include "support_ui.h"

#include "AddPathsCommand.h"
#include "AddShapesCommand.h"
#include "AddStylesCommand.h"
#include "AttributeSaver.h"
#include "BitmapExporter.h"
#include "BitmapSetSaver.h"
#include "CanvasView.h"
#include "CommandStack.h"
#include "CompoundCommand.h"
#include "CurrentColor.h"
#include "Document.h"
#include "FlatIconExporter.h"
#include "FlatIconFormat.h"
#include "FlatIconImporter.h"
#include "IconObjectListView.h"
#include "IconEditorApp.h"
#include "IconView.h"
#include "MessageExporter.h"
#include "MessageImporter.h"
#include "MessengerSaver.h"
#include "NativeSaver.h"
#include "PathListView.h"
#include "RDefExporter.h"
#include "ScrollView.h"
#include "SimpleFileSaver.h"
#include "ShapeListView.h"
#include "SourceExporter.h"
#include "StyleListView.h"
#include "StyleView.h"
#include "SVGExporter.h"
#include "SVGImporter.h"
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


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-Main"


using std::nothrow;

enum {
	MSG_UNDO						= 'undo',
	MSG_REDO						= 'redo',
	MSG_UNDO_STACK_CHANGED			= 'usch',

	MSG_PATH_SELECTED				= 'vpsl',
	MSG_STYLE_SELECTED				= 'stsl',
	MSG_SHAPE_SELECTED				= 'spsl',

	MSG_SHAPE_RESET_TRANSFORMATION	= 'rtsh',
	MSG_STYLE_RESET_TRANSFORMATION	= 'rtst',

	MSG_MOUSE_FILTER_MODE			= 'mfmd',

	MSG_RENAME_OBJECT				= 'rnam',
};


MainWindow::MainWindow(BRect frame, IconEditorApp* app,
		const BMessage* settings)
	:
	BWindow(frame, B_TRANSLATE_SYSTEM_NAME("Icon-O-Matic"),
		B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fApp(app),
	fDocument(new Document(B_TRANSLATE("Untitled"))),
	fCurrentColor(new CurrentColor()),
	fIcon(NULL),
	fMessageAfterSave(NULL)
{
	_Init();

	RestoreSettings(settings);
}


MainWindow::~MainWindow()
{
	SetIcon(NULL);

	delete fState;

	// Make sure there are no listeners attached to the document anymore.
	while (BView* child = ChildAt(0L)) {
		child->RemoveSelf();
		delete child;
	}

	fDocument->CommandStack()->RemoveObserver(this);

	// NOTE: it is important that the GUI has been deleted
	// at this point, so that all the listener/observer
	// stuff is properly detached
	delete fDocument;

	delete fMessageAfterSave;
}


// #pragma mark -


void
MainWindow::MessageReceived(BMessage* message)
{
	bool discard = false;

	// Figure out if we need the write lock on the Document. For most
	// messages we do, but exporting takes place in another thread and
	// locking is taken care of there.
	bool requiresWriteLock = true;
	switch (message->what) {
		case MSG_SAVE:
		case MSG_EXPORT:
		case MSG_SAVE_AS:
		case MSG_EXPORT_AS:
			requiresWriteLock = false;
			break;
		default:
			break;
	}
	if (requiresWriteLock && !fDocument->WriteLock()) {
		BWindow::MessageReceived(message);
		return;
	}

	if (message->WasDropped()) {
		const rgb_color* color;
		int32 length;
		// create styles from dropped colors
		for (int32 i = 0; message->FindData("RGBColor", B_RGB_COLOR_TYPE, i, 
			(const void**)&color, &length) == B_OK; i++) {
			if (length != sizeof(rgb_color))
				continue;
			char name[30];
			sprintf(name, 
				B_TRANSLATE_CONTEXT("Color (#%02x%02x%02x)", 
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
			// If our icon is empty, open the file in this window,
			// otherwise forward to the application which will open
			// it in another window, unless we append.
			message->what = B_REFS_RECEIVED;
			if (fDocument->Icon()->Styles()->CountStyles() == 0
				&& fDocument->Icon()->Paths()->CountPaths() == 0
				&& fDocument->Icon()->Shapes()->CountShapes() == 0) {
				entry_ref ref;
				if (message->FindRef("refs", &ref) == B_OK)
					Open(ref);
				break;
			}
			if (modifiers() & B_SHIFT_KEY) {
				// We want the icon appended to this window.
				message->AddBool("append", true);
				message->AddPointer("window", this);
			}
			be_app->PostMessage(message);
			break;

		case B_PASTE:
		case B_MIME_DATA:
		{
			BMessage* clip = message;
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

			Icon* icon = new (std::nothrow) Icon(*fDocument->Icon());
			if (icon != NULL) {
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

		case MSG_OPEN:
			// If our icon is empty, we want the icon to open in this
			// window.
			if (fDocument->Icon()->Styles()->CountStyles() == 0
				&& fDocument->Icon()->Paths()->CountPaths() == 0
				&& fDocument->Icon()->Shapes()->CountShapes() == 0) {
				message->AddPointer("window", this);
			}
			be_app->PostMessage(message);
			break;

		case MSG_SAVE:
		case MSG_EXPORT:
		{
			DocumentSaver* saver;
			if (message->what == MSG_SAVE)
				saver = fDocument->NativeSaver();
			else
				saver = fDocument->ExportSaver();
			if (saver != NULL) {
				saver->Save(fDocument);
				_PickUpActionBeforeSave();
				break;
			} // else fall through
		}
		case MSG_SAVE_AS:
		case MSG_EXPORT_AS:
		{
			int32 exportMode;
			if (message->FindInt32("export mode", &exportMode) < B_OK)
				exportMode = EXPORT_MODE_MESSAGE;
			entry_ref ref;
			const char* name;
			if (message->FindRef("directory", &ref) == B_OK
				&& message->FindString("name", &name) == B_OK) {
				// this message comes from the file panel
				BDirectory dir(&ref);
				BEntry entry;
				if (dir.InitCheck() >= B_OK
					&& entry.SetTo(&dir, name, true) >= B_OK
					&& entry.GetRef(&ref) >= B_OK) {

					// create the document saver and remember it for later
					DocumentSaver* saver = _CreateSaver(ref, exportMode);
					if (saver != NULL) {
						if (fDocument->WriteLock()) {
							if (exportMode == EXPORT_MODE_MESSAGE)
								fDocument->SetNativeSaver(saver);
							else
								fDocument->SetExportSaver(saver);
							_UpdateWindowTitle();
							fDocument->WriteUnlock();
						}
						saver->Save(fDocument);
						_PickUpActionBeforeSave();
					}
				}
// TODO: ...
//				_SyncPanels(fSavePanel, fOpenPanel);
			} else {
				// configure the file panel
				uint32 requestRefWhat = MSG_SAVE_AS;
				bool isExportMode = message->what == MSG_EXPORT_AS
					|| message->what == MSG_EXPORT;
				if (isExportMode)
					requestRefWhat = MSG_EXPORT_AS;
				const char* saveText = _FileName(isExportMode);

				BMessage requestRef(requestRefWhat);
				if (saveText != NULL)
					requestRef.AddString("save text", saveText);
				requestRef.AddMessenger("target", BMessenger(this, this));
				be_app->PostMessage(&requestRef);
			}
			break;
		}

		case MSG_UNDO:
			fDocument->CommandStack()->Undo();
			break;
		case MSG_REDO:
			fDocument->CommandStack()->Redo();
			break;
		case MSG_UNDO_STACK_CHANGED:
		{
			// relable Undo item and update enabled status
			BString label(B_TRANSLATE("Undo"));
			fUndoMI->SetEnabled(fDocument->CommandStack()->GetUndoName(label));
			if (fUndoMI->IsEnabled())
				fUndoMI->SetLabel(label.String());
			else {
				fUndoMI->SetLabel(B_TRANSLATE_CONTEXT("<nothing to undo>",
					"Icon-O-Matic-Menu-Edit"));
			}
	
			// relable Redo item and update enabled status
			label.SetTo(B_TRANSLATE("Redo"));
			fRedoMI->SetEnabled(fDocument->CommandStack()->GetRedoName(label));
			if (fRedoMI->IsEnabled())
				fRedoMI->SetLabel(label.String());
			else {
				fRedoMI->SetLabel(B_TRANSLATE_CONTEXT("<nothing to redo>",
					"Icon-O-Matic-Menu-Edit"));
			}
			break;
		}

		case MSG_MOUSE_FILTER_MODE:
		{
			uint32 mode;
			if (message->FindInt32("mode", (int32*)&mode) == B_OK)
				fCanvasView->SetMouseFilterMode(mode);
			break;
		}

		case MSG_ADD_SHAPE: {
			AddStylesCommand* styleCommand = NULL;
			Style* style = NULL;
			if (message->HasBool("style")) {
				new_style(fCurrentColor->Color(),
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
						B_TRANSLATE_CONTEXT("Add shape with path & style",
							"Icon-O-Matic-Menu-Shape"),
						0);
				} else if (styleCommand) {
					Command** commands = new Command*[2];
					commands[0] = styleCommand;
					commands[1] = shapeCommand;
					command = new CompoundCommand(commands, 2,
						B_TRANSLATE_CONTEXT("Add shape with style",
							"Icon-O-Matic-Menu-Shape"), 
						0);
				} else {
					Command** commands = new Command*[2];
					commands[0] = pathCommand;
					commands[1] = shapeCommand;
					command = new CompoundCommand(commands, 2,
						B_TRANSLATE_CONTEXT("Add shape with path",
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
	if (gradient != NULL) {
		TransformGradientBox* transformBox
			= new (nothrow) TransformGradientBox(fCanvasView, gradient, NULL);
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

	if (requiresWriteLock)
		fDocument->WriteUnlock();
}


bool
MainWindow::QuitRequested()
{
	if (!_CheckSaveIcon(CurrentMessage()))
		return false;

	BMessage message(MSG_WINDOW_CLOSED);

	BMessage settings;
	StoreSettings(&settings);	
	message.AddMessage("settings", &settings);
	message.AddRect("window frame", Frame());

	be_app->PostMessage(&message);

	return true;
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
	if (!fDocument || !fDocument->ReadLock())
		return;

	if (object == fDocument->CommandStack())
		PostMessage(MSG_UNDO_STACK_CHANGED);

	fDocument->ReadUnlock();
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
MainWindow::Open(const entry_ref& ref, bool append)
{
	BFile file(&ref, B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

	Icon* icon;
	if (append)
		icon = new (nothrow) Icon(*fDocument->Icon());
	else
		icon = new (nothrow) Icon();

	if (icon == NULL) {
		// TODO: Report error to user.
		return;
	}

	enum {
		REF_NONE = 0,
		REF_MESSAGE,
		REF_FLAT,
		REF_FLAT_ATTR,
		REF_SVG
	};
	uint32 refMode = REF_NONE;

	// try different file types
	FlatIconImporter flatImporter;
	status_t ret = flatImporter.Import(icon, &file);
	if (ret >= B_OK) {
		refMode = REF_FLAT;
	} else {
		file.Seek(0, SEEK_SET);
		MessageImporter msgImporter;
		ret = msgImporter.Import(icon, &file);
		if (ret >= B_OK) {
			refMode = REF_MESSAGE;
		} else {
			file.Seek(0, SEEK_SET);
			SVGImporter svgImporter;
			ret = svgImporter.Import(icon, &ref);
			if (ret >= B_OK) {
				refMode = REF_SVG;
			} else {
				// fall back to flat icon format but use the icon attribute
				ret = B_OK;
				attr_info attrInfo;
				if (file.GetAttrInfo(kVectorAttrNodeName, &attrInfo) == B_OK) {
					if (attrInfo.type != B_VECTOR_ICON_TYPE)
						ret = B_ERROR;
					// If the attribute is there, we must succeed in reading
					// an icon! Otherwise we may overwrite an existing icon
					// when the user saves.
					uint8* buffer = NULL;
					if (ret == B_OK) {
						buffer = new(nothrow) uint8[attrInfo.size];
						if (buffer == NULL)
							ret = B_NO_MEMORY;
					}
					if (ret == B_OK) {
						ssize_t bytesRead = file.ReadAttr(kVectorAttrNodeName,
							B_VECTOR_ICON_TYPE, 0, buffer, attrInfo.size);
						if (bytesRead != (ssize_t)attrInfo.size) {
							ret = bytesRead < 0 ? (status_t)bytesRead
								: B_IO_ERROR;
						}
					}
					if (ret == B_OK) {
						ret = flatImporter.Import(icon, buffer, attrInfo.size);
						if (ret == B_OK)
							refMode = REF_FLAT_ATTR;
					}

					delete[] buffer;
				} else {
					// If there is no icon attribute, simply fall back
					// to creating an icon for this file. TODO: We may or may
					// not want to display an alert asking the user if that is
					// what he wants to do.
					refMode = REF_FLAT_ATTR;
				}
			}
		}
	}

	if (ret < B_OK) {
		// inform user of failure at this point
		BString helper(B_TRANSLATE("Opening the document failed!"));
		helper << "\n\n" << B_TRANSLATE("Error: ") << strerror(ret);
		BAlert* alert = new BAlert(
			B_TRANSLATE_CONTEXT("bad news", "Title of error alert"),
			helper.String(), 
			B_TRANSLATE_CONTEXT("Bummer", 
				"Cancel button - error alert"),	
			NULL, NULL);
		// launch alert asynchronously
		alert->Go(NULL);

		delete icon;
		return;
	}

	AutoWriteLocker locker(fDocument);

	// incorporate the loaded icon into the document
	// (either replace it or append to it)
	fDocument->MakeEmpty(!append);
		// if append, the document savers are preserved
	fDocument->SetIcon(icon);
	if (!append) {
		// document got replaced, but we have at
		// least one ref already
		switch (refMode) {
			case REF_MESSAGE:
				fDocument->SetNativeSaver(new NativeSaver(ref));
				break;
			case REF_FLAT:
				fDocument->SetExportSaver(
					new SimpleFileSaver(new FlatIconExporter(), ref));
				break;
			case REF_FLAT_ATTR:
				fDocument->SetNativeSaver(
					new AttributeSaver(ref, kVectorAttrNodeName));
				break;
			case REF_SVG:
				fDocument->SetExportSaver(
					new SimpleFileSaver(new SVGExporter(), ref));
				break;
		}
	}

	locker.Unlock();

	SetIcon(icon);

	_UpdateWindowTitle();
}


void
MainWindow::Open(const BMessenger& externalObserver, const uint8* data,
	size_t size)
{
	if (!_CheckSaveIcon(CurrentMessage()))
		return;

	if (!externalObserver.IsValid())
		return;

	Icon* icon = new (nothrow) Icon();
	if (!icon)
		return;

	if (data && size > 0) {
		// try to open the icon from the provided data
		FlatIconImporter flatImporter;
		status_t ret = flatImporter.Import(icon, const_cast<uint8*>(data),
			size);
			// NOTE: the const_cast is a bit ugly, but no harm is done
			// the reason is that the LittleEndianBuffer knows read and write
			// mode, in this case it is used read-only, and it does not assume
			// ownership of the buffer

		if (ret < B_OK) {
			// inform user of failure at this point
			BString helper(B_TRANSLATE("Opening the icon failed!"));
			helper << "\n\n" << B_TRANSLATE("Error: ") << strerror(ret);
			BAlert* alert = new BAlert(
				B_TRANSLATE_CONTEXT("bad news", "Title of error alert"),
				helper.String(), 
				B_TRANSLATE_CONTEXT("Bummer", 
					"Cancel button - error alert"),	
				NULL, NULL);
			// launch alert asynchronously
			alert->Go(NULL);

			delete icon;
			return;
		}
	}

	AutoWriteLocker locker(fDocument);

	SetIcon(NULL);

	// incorporate the loaded icon into the document
	// (either replace it or append to it)
	fDocument->MakeEmpty();
	fDocument->SetIcon(icon);

	fDocument->SetNativeSaver(new MessengerSaver(externalObserver));

	locker.Unlock();

	SetIcon(icon);
}


void
MainWindow::SetIcon(Icon* icon)
{
	if (fIcon == icon)
		return;

	Icon* oldIcon = fIcon;

	fIcon = icon;

	if (fIcon != NULL)
		fIcon->Acquire();
	else
		MakeEmpty();

	fCanvasView->SetIcon(fIcon);

	fPathListView->SetPathContainer(fIcon != NULL ? fIcon->Paths() : NULL);
	fPathListView->SetShapeContainer(fIcon != NULL ? fIcon->Shapes() : NULL);

	fStyleListView->SetStyleContainer(fIcon != NULL ? fIcon->Styles() : NULL);
	fStyleListView->SetShapeContainer(fIcon != NULL ? fIcon->Shapes() : NULL);

	fShapeListView->SetShapeContainer(fIcon != NULL ? fIcon->Shapes() : NULL);

	// icon previews
	fIconPreview16Folder->SetIcon(fIcon);
	fIconPreview16Menu->SetIcon(fIcon);
	fIconPreview32Folder->SetIcon(fIcon);
	fIconPreview32Desktop->SetIcon(fIcon);
//	fIconPreview48->SetIcon(fIcon);
	fIconPreview64->SetIcon(fIcon);

	// keep this last
	if (oldIcon != NULL)
		oldIcon->Release();
}


// #pragma mark -


void
MainWindow::StoreSettings(BMessage* archive)
{
	if (archive->ReplaceUInt32("mouse filter mode",
			fCanvasView->MouseFilterMode()) != B_OK) {
		archive->AddUInt32("mouse filter mode",
			fCanvasView->MouseFilterMode());
	}
}


void
MainWindow::RestoreSettings(const BMessage* archive)
{
	uint32 mouseFilterMode;
	if (archive->FindUInt32("mouse filter mode", &mouseFilterMode) == B_OK) {
		fCanvasView->SetMouseFilterMode(mouseFilterMode);
		fMouseFilterOffMI->SetMarked(mouseFilterMode == SNAPPING_OFF);
		fMouseFilter64MI->SetMarked(mouseFilterMode == SNAPPING_64);
		fMouseFilter32MI->SetMarked(mouseFilterMode == SNAPPING_32);
		fMouseFilter16MI->SetMarked(mouseFilterMode == SNAPPING_16);
	}
}


// #pragma mark -


void
MainWindow::_Init()
{
	// create the GUI
	_CreateGUI();

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
	fMouseFilter64MI->SetMarked(true);
//	fCanvasView->SetSelection(fDocument->Selection());

	fPathListView->SetMenu(fPathMenu);
	fPathListView->SetCommandStack(fDocument->CommandStack());
	fPathListView->SetSelection(fDocument->Selection());

	fStyleListView->SetMenu(fStyleMenu);
	fStyleListView->SetCommandStack(fDocument->CommandStack());
	fStyleListView->SetSelection(fDocument->Selection());
	fStyleListView->SetCurrentColor(fCurrentColor);

	fStyleView->SetCommandStack(fDocument->CommandStack());
	fStyleView->SetCurrentColor(fCurrentColor);

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

	fSwatchGroup->SetCurrentColor(fCurrentColor);

	SetIcon(fDocument->Icon());

	AddShortcut('Y', 0, new BMessage(MSG_UNDO));
	AddShortcut('Y', B_SHIFT_KEY, new BMessage(MSG_REDO));
	AddShortcut('E', 0, new BMessage(MSG_RENAME_OBJECT));
}


void
MainWindow::_CreateGUI()
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	BGridLayout* layout = new BGridLayout();
	layout->SetSpacing(0, 0);
	BView* rootView = new BView("root view", 0, layout);
	AddChild(rootView);
	rootView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BGroupView* leftTopView = new BGroupView(B_VERTICAL, 0);
	layout->AddView(leftTopView, 0, 0);

	// views along the left side
	leftTopView->AddChild(_CreateMenuBar());

	float splitWidth = 13 * be_plain_font->Size();
	BSize minSize = leftTopView->MinSize();
	splitWidth = std::max(splitWidth, minSize.width);
	leftTopView->SetExplicitMaxSize(BSize(splitWidth, B_SIZE_UNSET));
	leftTopView->SetExplicitMinSize(BSize(splitWidth, B_SIZE_UNSET));

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
	leftSideView->SetExplicitMaxSize(BSize(splitWidth, B_SIZE_UNSET));

	// path menu and list view
	BMenuBar* menuBar = new BMenuBar("path menu bar");
	menuBar->AddItem(fPathMenu);
	leftSideView->AddChild(menuBar);

	fPathListView = new PathListView(BRect(0, 0, splitWidth, 100),
		"path list view", new BMessage(MSG_PATH_SELECTED), this);

	BView* scrollView = new BScrollView("path list scroll view",
		fPathListView, B_FOLLOW_NONE, 0, false, true, B_NO_BORDER);
	leftSideView->AddChild(scrollView);

	// shape list view
	menuBar = new BMenuBar("shape menu bar");
	menuBar->AddItem(fShapeMenu);
	leftSideView->AddChild(menuBar);

	fShapeListView = new ShapeListView(BRect(0, 0, splitWidth, 100),
		"shape list view", new BMessage(MSG_SHAPE_SELECTED), this);
	scrollView = new BScrollView("shape list scroll view",
		fShapeListView, B_FOLLOW_NONE, 0, false, true, B_NO_BORDER);
	leftSideView->AddChild(scrollView);

	// transformer list view
	menuBar = new BMenuBar("transformer menu bar");
	menuBar->AddItem(fTransformerMenu);
	leftSideView->AddChild(menuBar);

	fTransformerListView = new TransformerListView(BRect(0, 0, splitWidth, 100),
		"transformer list view");
	scrollView = new BScrollView("transformer list scroll view",
		fTransformerListView, B_FOLLOW_NONE, 0, false, true, B_NO_BORDER);
	leftSideView->AddChild(scrollView);

	// property list view
	menuBar = new BMenuBar("property menu bar");
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
	menuBar = new BMenuBar("style menu bar");
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

	menuBar = new BMenuBar("swatches menu bar");
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
MainWindow::_CreateMenuBar()
{
	BMenuBar* menuBar = new BMenuBar("main menu");


	#undef B_TRANSLATION_CONTEXT
	#define B_TRANSLATION_CONTEXT "Icon-O-Matic-Menus"
	
	
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
	#undef B_TRANSLATION_CONTEXT
	#define B_TRANSLATION_CONTEXT "Icon-O-Matic-Menu-File"
	

	BMenuItem* item = new BMenuItem(B_TRANSLATE("New"),
		new BMessage(MSG_NEW), 'N');
	fileMenu->AddItem(item);
	item->SetTarget(be_app);
	item = new BMenuItem(B_TRANSLATE("Open"B_UTF8_ELLIPSIS),
		new BMessage(MSG_OPEN), 'O');
	fileMenu->AddItem(item);
	BMessage* appendMessage = new BMessage(MSG_APPEND);
	appendMessage->AddPointer("window", this);
	item = new BMenuItem(B_TRANSLATE("Append"B_UTF8_ELLIPSIS),
		appendMessage, 'O', B_SHIFT_KEY);
	fileMenu->AddItem(item);
	item->SetTarget(be_app);
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
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Close"),
		new BMessage(B_QUIT_REQUESTED), 'W'));
	item = new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q');
	fileMenu->AddItem(item);
	item->SetTarget(be_app);

	// Edit
	#undef B_TRANSLATION_CONTEXT
	#define B_TRANSLATION_CONTEXT "Icon-O-Matic-Menu-Edit"
	
	
	fUndoMI = new BMenuItem(B_TRANSLATE("<nothing to undo>"),
		new BMessage(MSG_UNDO), 'Z');
	fRedoMI = new BMenuItem(B_TRANSLATE("<nothing to redo>"),
		new BMessage(MSG_REDO), 'Z', B_SHIFT_KEY);

	fUndoMI->SetEnabled(false);
	fRedoMI->SetEnabled(false);

	editMenu->AddItem(fUndoMI);
	editMenu->AddItem(fRedoMI);


	// Settings
	#undef B_TRANSLATION_CONTEXT
	#define B_TRANSLATION_CONTEXT "Icon-O-Matic-Menu-Settings"
	
	
	BMenu* filterModeMenu = new BMenu(B_TRANSLATE("Snap to grid"));
	BMessage* message = new BMessage(MSG_MOUSE_FILTER_MODE);
	message->AddInt32("mode", SNAPPING_OFF);
	fMouseFilterOffMI = new BMenuItem(B_TRANSLATE("Off"), message, '4');
	filterModeMenu->AddItem(fMouseFilterOffMI);

	message = new BMessage(MSG_MOUSE_FILTER_MODE);
	message->AddInt32("mode", SNAPPING_64);
	fMouseFilter64MI = new BMenuItem("64 x 64", message, '3');
	filterModeMenu->AddItem(fMouseFilter64MI);

	message = new BMessage(MSG_MOUSE_FILTER_MODE);
	message->AddInt32("mode", SNAPPING_32);
	fMouseFilter32MI = new BMenuItem("32 x 32", message, '2');
	filterModeMenu->AddItem(fMouseFilter32MI);

	message = new BMessage(MSG_MOUSE_FILTER_MODE);
	message->AddInt32("mode", SNAPPING_16);
	fMouseFilter16MI = new BMenuItem("16 x 16", message, '1');
	filterModeMenu->AddItem(fMouseFilter16MI);

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


// #pragma mark -


bool
MainWindow::_CheckSaveIcon(const BMessage* currentMessage)
{
	if (fDocument->IsEmpty() || fDocument->CommandStack()->IsSaved())
		return true;

	// Make sure the user sees us.
	Activate();

	BAlert* alert = new BAlert("save", 
		B_TRANSLATE("Save changes to current icon?"), B_TRANSLATE("Discard"),
		 B_TRANSLATE("Cancel"), B_TRANSLATE("Save"));
	int32 choice = alert->Go();
	switch (choice) {
		case 0:
			// discard
			return true;
		case 1:
			// cancel
			return false;
		case 2:
		default:
			// cancel (save first) but pick up what we were doing before
			PostMessage(MSG_SAVE);
			if (currentMessage != NULL) {
				delete fMessageAfterSave;
				fMessageAfterSave = new BMessage(*currentMessage);
			}
			return false;
	}
}


void
MainWindow::_PickUpActionBeforeSave()
{
	if (fDocument->WriteLock()) {
		fDocument->CommandStack()->Save();
		fDocument->WriteUnlock();
	}

	if (fMessageAfterSave == NULL)
		return;

	PostMessage(fMessageAfterSave);
	delete fMessageAfterSave;
	fMessageAfterSave = NULL;
}


// #pragma mark -


void
MainWindow::_MakeIconEmpty()
{
	if (!_CheckSaveIcon(CurrentMessage()))
		return;

	AutoWriteLocker locker(fDocument);

	MakeEmpty();
	fDocument->MakeEmpty();

	locker.Unlock();
}


DocumentSaver*
MainWindow::_CreateSaver(const entry_ref& ref, uint32 exportMode)
{
	DocumentSaver* saver;

	switch (exportMode) {
		case EXPORT_MODE_FLAT_ICON:
			saver = new SimpleFileSaver(new FlatIconExporter(), ref);
			break;

		case EXPORT_MODE_ICON_ATTR:
		case EXPORT_MODE_ICON_MIME_ATTR: {
			const char* attrName
				= exportMode == EXPORT_MODE_ICON_ATTR ?
					kVectorAttrNodeName : kVectorAttrMimeName;
			saver = new AttributeSaver(ref, attrName);
			break;
		}

		case EXPORT_MODE_ICON_RDEF:
			saver = new SimpleFileSaver(new RDefExporter(), ref);
			break;
		case EXPORT_MODE_ICON_SOURCE:
			saver = new SimpleFileSaver(new SourceExporter(), ref);
			break;

		case EXPORT_MODE_BITMAP_16:
			saver = new SimpleFileSaver(new BitmapExporter(16), ref);
			break;
		case EXPORT_MODE_BITMAP_32:
			saver = new SimpleFileSaver(new BitmapExporter(32), ref);
			break;
		case EXPORT_MODE_BITMAP_64:
			saver = new SimpleFileSaver(new BitmapExporter(64), ref);
			break;

		case EXPORT_MODE_BITMAP_SET:
			saver = new BitmapSetSaver(ref);
			break;

		case EXPORT_MODE_SVG:
			saver = new SimpleFileSaver(new SVGExporter(), ref);
			break;

		case EXPORT_MODE_MESSAGE:
		default:
			saver = new NativeSaver(ref);
			break;
	}

	return saver;
}


const char*
MainWindow::_FileName(bool preferExporter) const
{
	FileSaver* saver1;
	FileSaver* saver2;
	if (preferExporter) {
		saver1 = dynamic_cast<FileSaver*>(fDocument->ExportSaver());
		saver2 = dynamic_cast<FileSaver*>(fDocument->NativeSaver());
	} else {
		saver1 = dynamic_cast<FileSaver*>(fDocument->NativeSaver());
		saver2 = dynamic_cast<FileSaver*>(fDocument->ExportSaver());
	}
	const char* fileName = NULL;
	if (saver1 != NULL)
		fileName = saver1->Ref()->name;
	if ((fileName == NULL || fileName[0] == '\0') && saver2 != NULL)
		fileName = saver2->Ref()->name;
	return fileName;
}


void
MainWindow::_UpdateWindowTitle()
{
	const char* fileName = _FileName(false);
	if (fileName != NULL)
		SetTitle(fileName);
	else
		SetTitle(B_TRANSLATE_SYSTEM_NAME("Icon-O-Matic"));
}

