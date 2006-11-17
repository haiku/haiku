/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "IconEditorApp.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>
#include <IconEditorProtocol.h>

#include "support_settings.h"

#include "AttributeSaver.h"
#include "AutoLocker.h"
#include "BitmapExporter.h"
#include "BitmapSetSaver.h"
#include "CommandStack.h"
#include "Document.h"
#include "FlatIconExporter.h"
#include "FlatIconFormat.h"
#include "FlatIconImporter.h"
#include "Icon.h"
#include "MainWindow.h"
#include "MessageExporter.h"
#include "MessageImporter.h"
#include "MessengerSaver.h"
#include "PathContainer.h"
#include "RDefExporter.h"
#include "SavePanel.h"
#include "ShapeContainer.h"
#include "SimpleFileSaver.h"
#include "SVGExporter.h"
#include "SVGImporter.h"

using std::nothrow;

// constructor
IconEditorApp::IconEditorApp()
	: BApplication("application/x-vnd.haiku-icon_o_matic"),
	  fMainWindow(NULL),
	  fDocument(new Document("test")),

	  fOpenPanel(NULL),
	  fSavePanel(NULL),

	  fLastOpenPath(""),
	  fLastSavePath(""),
	  fLastExportPath("")
{
}

// destructor
IconEditorApp::~IconEditorApp()
{
	// NOTE: it is important that the GUI has been deleted
	// at this point, so that all the listener/observer
	// stuff is properly detached
	delete fDocument;

	delete fOpenPanel;
	delete fSavePanel;
}

// #pragma mark -

// QuitRequested
bool
IconEditorApp::QuitRequested()
{
	// TODO: ask main window if quitting is ok
	_StoreSettings();

	fMainWindow->Lock();
	fMainWindow->Quit();
	fMainWindow = NULL;

	return true;
}

// MessageReceived
void
IconEditorApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_NEW:
			_MakeIconEmpty();
			break;
		case MSG_OPEN: {
//			fOpenPanel->Refresh();
			BMessage openMessage(B_REFS_RECEIVED);
			fOpenPanel->SetMessage(&openMessage);
			fOpenPanel->Show();
			break;
		}
		case MSG_APPEND: {
//			fOpenPanel->Refresh();
			BMessage openMessage(B_REFS_RECEIVED);
			openMessage.AddBool("append", true);
			fOpenPanel->SetMessage(&openMessage);
			fOpenPanel->Show();
			break;
		}
		case MSG_SAVE:
		case MSG_EXPORT: {
			DocumentSaver* saver;
			if (message->what == MSG_SAVE)
				saver = fDocument->NativeSaver();
			else
				saver = fDocument->ExportSaver();
			if (saver) {
				saver->Save(fDocument);
				break;
			} // else fall through
		}
		case MSG_SAVE_AS:
		case MSG_EXPORT_AS: {
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
					if (saver) {
						if (exportMode == EXPORT_MODE_MESSAGE)
							fDocument->SetNativeSaver(saver);
						else
							fDocument->SetExportSaver(saver);
						saver->Save(fDocument);
					}
				}
				_SyncPanels(fSavePanel, fOpenPanel);
			} else {
				// configure the file panel
				const char* saveText = NULL;
				FileSaver* saver = dynamic_cast<FileSaver*>(
					fDocument->NativeSaver());

				bool exportMode = message->what == MSG_EXPORT_AS
									|| message->what == MSG_EXPORT;
				if (exportMode) {
					saver = dynamic_cast<FileSaver*>(
						fDocument->ExportSaver());
				}

				if (saver)
					saveText = saver->Ref()->name;

				fSavePanel->SetExportMode(exportMode);
//				fSavePanel->Refresh();
				if (saveText)
					fSavePanel->SetSaveText(saveText);
				fSavePanel->Show();
			}
			break;
		}
		case B_EDIT_ICON_DATA: {
			BMessenger messenger;
			if (message->FindMessenger("reply to", &messenger) < B_OK) {
				// required
				break;
			}
			const uint8* data;
			ssize_t size;
			if (message->FindData("icon data", B_VECTOR_ICON_TYPE,
				(const void**)&data, &size) < B_OK) {
				// optional (new icon will be created)
				data = NULL;
				size = 0;
			}
			_Open(messenger, data, size);
			break;
		}

		default:
			BApplication::MessageReceived(message);
			break;
	}
}

// ReadyToRun
void
IconEditorApp::ReadyToRun()
{
	// create file panels
	BMessenger messenger(this, this);
	fOpenPanel = new BFilePanel(B_OPEN_PANEL,
								&messenger,
								NULL,
								B_FILE_NODE,
								true,
								new BMessage(B_REFS_RECEIVED));

	fSavePanel = new SavePanel("save panel",
							   &messenger,
								NULL,
								B_FILE_NODE
									| B_DIRECTORY_NODE
									| B_SYMLINK_NODE,
								false,
								new BMessage(MSG_SAVE_AS));

	// create main window
	fMainWindow = new MainWindow(this, fDocument);

	_RestoreSettings();

	fMainWindow->Show();
}

// RefsReceived
void
IconEditorApp::RefsReceived(BMessage* message)
{
	// TODO: multiple documents (iterate over refs)
	bool append;
	if (message->FindBool("append", &append) < B_OK)
		append = false;
	entry_ref ref;
	if (message->FindRef("refs", &ref) == B_OK)
		_Open(ref, append);

	if (fOpenPanel && fSavePanel)
		_SyncPanels(fOpenPanel, fSavePanel);
}

// ArgvReceived
void
IconEditorApp::ArgvReceived(int32 argc, char** argv)
{
	if (argc < 2)
		return;

	// TODO: multiple documents (iterate over argv)
	entry_ref ref;
	if (get_ref_for_path(argv[1], &ref) == B_OK)
		_Open(ref);
}

// #pragma mark -

void
IconEditorApp::_MakeIconEmpty()
{
	bool mainWindowLocked = fMainWindow && fMainWindow->Lock();

	AutoWriteLocker locker(fDocument);

	if (fMainWindow)
		fMainWindow->MakeEmpty();

	fDocument->MakeEmpty();

	locker.Unlock();

	if (mainWindowLocked)
		fMainWindow->Unlock();
}

// _Open
void
IconEditorApp::_Open(const entry_ref& ref, bool append)
{
	BFile file(&ref, B_READ_ONLY);
	if (file.InitCheck() < B_OK)
		return;

	Icon* icon;
	if (append)
		icon = new (nothrow) Icon(*fDocument->Icon());
	else
		icon = new (nothrow) Icon();

	if (!icon)
		return;

	enum {
		REF_NONE = 0,
		REF_MESSAGE,
		REF_FLAT,
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
			if (ret >= B_OK)
				refMode = REF_SVG;
		}
	}

	if (ret < B_OK) {
		// inform user of failure at this point
		BString helper("Opening the document failed!");
		helper << "\n\n" << "Error: " << strerror(ret);
		BAlert* alert = new BAlert("bad news", helper.String(),
								   "Bummer", NULL, NULL);
		// launch alert asynchronously
		alert->Go(NULL);

		delete icon;
		return;
	}

	// keep the mainwindow locked while switching icons
	bool mainWindowLocked = fMainWindow && fMainWindow->Lock();

	AutoWriteLocker locker(fDocument);

	if (mainWindowLocked)
		fMainWindow->SetIcon(NULL);

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
				fDocument->SetNativeSaver(
					new SimpleFileSaver(new MessageExporter(), ref));
				break;
			case REF_FLAT:
				fDocument->SetExportSaver(
					new SimpleFileSaver(new FlatIconExporter(), ref));
				break;
			case REF_SVG:
				fDocument->SetExportSaver(
					new SimpleFileSaver(new SVGExporter(), ref));
				break;
		}
	}

	locker.Unlock();

	if (mainWindowLocked) {
		fMainWindow->Unlock();
		// cause the mainwindow to adopt icon in
		// it's own thread
		fMainWindow->PostMessage(MSG_SET_ICON);
	}
}

// _Open
void
IconEditorApp::_Open(const BMessenger& externalObserver,
					 const uint8* data, size_t size)
{
	if (!externalObserver.IsValid())
		return;

	Icon* icon = new (nothrow) Icon();
	if (!icon)
		return;

	if (data && size > 0) {
		// try to open the icon from the provided data
		FlatIconImporter flatImporter;
		status_t ret = flatImporter.Import(icon, const_cast<uint8*>(data), size);
			// NOTE: the const_cast is a bit ugly, but no harm is done
			// the reason is that the LittleEndianBuffer knows read and write
			// mode, in this case it is used read-only, and it does not assume
			// ownership of the buffer
	
		if (ret < B_OK) {
			// inform user of failure at this point
			BString helper("Opening the icon failed!");
			helper << "\n\n" << "Error: " << strerror(ret);
			BAlert* alert = new BAlert("bad news", helper.String(),
									   "Bummer", NULL, NULL);
			// launch alert asynchronously
			alert->Go(NULL);
	
			delete icon;
			return;
		}
	}

	// keep the mainwindow locked while switching icons
	bool mainWindowLocked = fMainWindow && fMainWindow->Lock();

	AutoWriteLocker locker(fDocument);

	if (mainWindowLocked)
		fMainWindow->SetIcon(NULL);

	// incorporate the loaded icon into the document
	// (either replace it or append to it)
	fDocument->MakeEmpty();
	fDocument->SetIcon(icon);

	fDocument->SetNativeSaver(new MessengerSaver(externalObserver));

	locker.Unlock();

	if (mainWindowLocked) {
		fMainWindow->Unlock();
		// cause the mainwindow to adopt icon in
		// it's own thread
		fMainWindow->PostMessage(MSG_SET_ICON);
	}
}

// _CreateSaver
DocumentSaver*
IconEditorApp::_CreateSaver(const entry_ref& ref, uint32 exportMode)
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

		case EXPORT_MODE_BITMAP:
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
			saver = new SimpleFileSaver(new MessageExporter(), ref);
			break;
	}

	return saver;
}

// _SyncPanels
void
IconEditorApp::_SyncPanels(BFilePanel* from, BFilePanel* to)
{
	if (from->Window()->Lock()) {
		// location
		if (to->Window()->Lock()) {
			BRect frame = from->Window()->Frame();
			to->Window()->MoveTo(frame.left, frame.top);
			to->Window()->ResizeTo(frame.Width(), frame.Height());
			to->Window()->Unlock();
		}
		// current folder
		entry_ref panelDir;
		from->GetPanelDirectory(&panelDir);
		to->SetPanelDirectory(&panelDir);
		from->Window()->Unlock();
	}
}

// _LastFilePath
const char*
IconEditorApp::_LastFilePath(path_kind which)
{
	const char* path = NULL;

	switch (which) {
		case LAST_PATH_OPEN:
			if (fLastOpenPath.Length() > 0)
				path = fLastOpenPath.String();
			else if (fLastSavePath.Length() > 0)
				path = fLastSavePath.String();
			else if (fLastExportPath.Length() > 0)
				path = fLastExportPath.String();
			break;
		case LAST_PATH_SAVE:
			if (fLastSavePath.Length() > 0)
				path = fLastSavePath.String();
			else if (fLastExportPath.Length() > 0)
				path = fLastExportPath.String();
			else if (fLastOpenPath.Length() > 0)
				path = fLastOpenPath.String();
			break;
		case LAST_PATH_EXPORT:
			if (fLastExportPath.Length() > 0)
				path = fLastExportPath.String();
			else if (fLastSavePath.Length() > 0)
				path = fLastSavePath.String();
			else if (fLastOpenPath.Length() > 0)
				path = fLastOpenPath.String();
			break;
	}
	if (!path)
		path = "/boot/home";

	return path;
}

// #pragma mark -

// _StoreSettings
void
IconEditorApp::_StoreSettings()
{
	BMessage settings('stns');

	fMainWindow->StoreSettings(&settings);

	if (settings.ReplaceInt32("export mode", fSavePanel->ExportMode()) < B_OK)
		settings.AddInt32("export mode", fSavePanel->ExportMode());

	save_settings(&settings, "Icon-O-Matic");
}

// _RestoreSettings
void
IconEditorApp::_RestoreSettings()
{
	BMessage settings('stns');
	load_settings(&settings, "Icon-O-Matic");

	int32 mode;
	if (settings.FindInt32("export mode", &mode) >= B_OK)
		fSavePanel->SetExportMode(mode);

	fMainWindow->RestoreSettings(&settings);
}

