/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "IconEditorApp.h"

#include <Alert.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>

#include <new>
#include <stdio.h>
#include <string.h>

#include "AutoLocker.h"
#include "BitmapExporter.h"
#include "CommandStack.h"
#include "Document.h"
#include "FlatIconExporter.h"
#include "FlatIconFormat.h"
#include "FlatIconImporter.h"
#include "Icon.h"
#include "MainWindow.h"
#include "MessageExporter.h"
#include "MessageImporter.h"
#include "PathContainer.h"
#include "RDefExporter.h"
#include "ShapeContainer.h"
#include "SVGExporter.h"
#include "SVGImporter.h"

using std::nothrow;

// constructor
IconEditorApp::IconEditorApp()
	: BApplication("application/x-vnd.haiku-icon_o_matic"),
	  fMainWindow(NULL),
	  fDocument(new Document("test")),

	  fOpenPanel(NULL),
	  fSavePanel(NULL)
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
			const entry_ref* ref;
			if (message->what == MSG_SAVE)
				ref = fDocument->Ref();
			else
				ref = fDocument->ExportRef();
			if (ref) {
				_Save(*ref, message->what == MSG_EXPORT ?
								EXPORT_MODE_FLAT_ICON :
								EXPORT_MODE_MESSAGE);
				break;
			} // else fall through
		}
		case MSG_SAVE_AS:
		case MSG_EXPORT_AS:
		case MSG_EXPORT_BITMAP:
		case MSG_EXPORT_BITMAP_SET:
		case MSG_EXPORT_SVG:
		case MSG_EXPORT_ICON_ATTRIBUTE:
		case MSG_EXPORT_ICON_MIME_ATTRIBUTE:
		case MSG_EXPORT_ICON_RDEF: {
			uint32 exportMode;
			if (message->FindInt32("mode", (int32*)&exportMode) < B_OK)
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

					_Save(ref, exportMode);
				}
				_SyncPanels(fSavePanel, fOpenPanel);
			} else {
				switch (message->what) {
					case MSG_EXPORT_AS:
					case MSG_EXPORT:
						exportMode = EXPORT_MODE_FLAT_ICON;
						break;
					case MSG_EXPORT_BITMAP:
						exportMode = EXPORT_MODE_BITMAP;
						break;
					case MSG_EXPORT_BITMAP_SET:
						exportMode = EXPORT_MODE_BITMAP_SET;
						break;
					case MSG_EXPORT_SVG:
						exportMode = EXPORT_MODE_SVG;
						break;
					case MSG_EXPORT_ICON_ATTRIBUTE:
						exportMode = EXPORT_MODE_ICON_ATTR;
						break;
					case MSG_EXPORT_ICON_MIME_ATTRIBUTE:
						exportMode = EXPORT_MODE_ICON_MIME_ATTR;
						break;
					case MSG_EXPORT_ICON_RDEF:
						exportMode = EXPORT_MODE_ICON_RDEF;
						break;
					case MSG_SAVE_AS:
					case MSG_SAVE:
					default:
						exportMode = EXPORT_MODE_MESSAGE;
						break;
				}
				BMessage fpMessage(MSG_SAVE_AS);
				fpMessage.AddInt32("mode", exportMode);

				fSavePanel->SetMessage(&fpMessage);
//				fSavePanel->Refresh();
				fSavePanel->Show();
			}
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

	fSavePanel = new BFilePanel(B_SAVE_PANEL,
								&messenger,
								NULL,
								B_FILE_NODE
									| B_DIRECTORY_NODE
									| B_SYMLINK_NODE,
								false,
								new BMessage(MSG_SAVE));

	// create main window
	fMainWindow = new MainWindow(this, fDocument);
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
		REF_FLAT
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

	fDocument->MakeEmpty();

	fDocument->SetIcon(icon);

	switch (refMode) {
		case REF_MESSAGE:
			fDocument->SetRef(ref);
			break;
		case REF_FLAT:
			fDocument->SetExportRef(ref);
			break;
	}

	locker.Unlock();

	if (mainWindowLocked) {
		fMainWindow->Unlock();
		// cause the mainwindow to adopt icon in
		// it's own thread
		fMainWindow->PostMessage(MSG_SET_ICON);
	}
}

// _Save
void
IconEditorApp::_Save(const entry_ref& ref, uint32 exportMode)
{
	Exporter* exporter;
	switch (exportMode) {
		case EXPORT_MODE_FLAT_ICON:
			exporter = new FlatIconExporter();
			fDocument->SetExportRef(ref);
			break;

		case EXPORT_MODE_ICON_ATTR:
		case EXPORT_MODE_ICON_MIME_ATTR: {
			BNode node(&ref);
			FlatIconExporter iconExporter;
			const char* attrName
				= exportMode == EXPORT_MODE_ICON_ATTR ?
					kVectorAttrNodeName : kVectorAttrMimeName;
			iconExporter.Export(fDocument->Icon(), &node, attrName);
			return;
		}

		case EXPORT_MODE_ICON_RDEF:
			exporter = new RDefExporter();
			break;

		case EXPORT_MODE_BITMAP:
			exporter = new BitmapExporter(64);
			break;

		case EXPORT_MODE_BITMAP_SET: {
			entry_ref smallRef(ref);
			// 64x64
			char name[B_OS_NAME_LENGTH];
			sprintf(name, "%s_64.png", ref.name);
			smallRef.set_name(name);
			exporter = new BitmapExporter(64);
			exporter->SetSelfDestroy(true);
			exporter->Export(fDocument, smallRef);
			// 16x16
			sprintf(name, "%s_16.png", ref.name);
			smallRef.set_name(name);
			Exporter* smallExporter = new BitmapExporter(16);
			smallExporter->SetSelfDestroy(true);
			smallExporter->Export(fDocument, smallRef);
			// 32x32
			sprintf(name, "%s_32.png", ref.name);
			smallRef.set_name(name);
			smallExporter = new BitmapExporter(32);
			smallExporter->SetSelfDestroy(true);
			smallExporter->Export(fDocument, smallRef);
			return;
		}

		case EXPORT_MODE_SVG:
			exporter = new SVGExporter();
			break;

		case EXPORT_MODE_MESSAGE:
		default:
			exporter = new MessageExporter();
			fDocument->SetRef(ref);
			break;
	}

	exporter->SetSelfDestroy(true);
		// we don't wait for the export thread to finish here
	exporter->Export(fDocument, ref);
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
