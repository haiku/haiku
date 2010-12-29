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
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>
#include <fs_attr.h>
#include <IconEditorProtocol.h>
#include <Locale.h>
#include <Message.h>
#include <Mime.h>

#include "support_settings.h"

#include "AttributeSaver.h"
#include "AutoLocker.h"
#include "BitmapExporter.h"
#include "BitmapSetSaver.h"
#include "CommandStack.h"
#include "Defines.h"
#include "Document.h"
#include "FlatIconExporter.h"
#include "FlatIconFormat.h"
#include "FlatIconImporter.h"
#include "Icon.h"
#include "MainWindow.h"
#include "MessageExporter.h"
#include "MessageImporter.h"
#include "MessengerSaver.h"
#include "NativeSaver.h"
#include "PathContainer.h"
#include "RDefExporter.h"
#include "SavePanel.h"
#include "ShapeContainer.h"
#include "SimpleFileSaver.h"
#include "SourceExporter.h"
#include "SVGExporter.h"
#include "SVGImporter.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Icon-O-Matic-Main"


using std::nothrow;

static const char* kAppSig = "application/x-vnd.haiku-icon_o_matic";

// constructor
IconEditorApp::IconEditorApp()
	: BApplication(kAppSig),
	  fMainWindow(NULL),
	  fDocument(new Document("test")),

	  fOpenPanel(NULL),
	  fSavePanel(NULL),

	  fLastOpenPath(""),
	  fLastSavePath(""),
	  fLastExportPath(""),

	  fMessageAfterSave(NULL)
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

	delete fMessageAfterSave;
}

// #pragma mark -

// QuitRequested
bool
IconEditorApp::QuitRequested()
{
	if (!_CheckSaveIcon(CurrentMessage()))
		return false;

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
			BMessage openMessage(B_REFS_RECEIVED);
			fOpenPanel->SetMessage(&openMessage);
			fOpenPanel->Show();
			break;
		}
		case MSG_APPEND: {
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
				_PickUpActionBeforeSave();
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
						_PickUpActionBeforeSave();
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
	BMessage message(B_REFS_RECEIVED);
	fOpenPanel = new BFilePanel(B_OPEN_PANEL,
								&messenger,
								NULL,
								B_FILE_NODE,
								true,
								&message);

	message.what = MSG_SAVE_AS;
	fSavePanel = new SavePanel("save panel",
							   &messenger,
								NULL,
								B_FILE_NODE
									| B_DIRECTORY_NODE
									| B_SYMLINK_NODE,
								false,
								&message);

	// create main window
	BMessage settings('stns');
	_RestoreSettings(settings);

	fMainWindow = new MainWindow(this, fDocument, &settings);
	fMainWindow->Show();

	_InstallDocumentMimeType();
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

// _CheckSaveIcon
bool
IconEditorApp::_CheckSaveIcon(const BMessage* currentMessage)
{
	if (fDocument->IsEmpty() || fDocument->CommandStack()->IsSaved())
		return true;

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
			// cancel (save first) but pick what we were doing before
			PostMessage(MSG_SAVE);
			if (currentMessage) {
				delete fMessageAfterSave;
				fMessageAfterSave = new BMessage(*currentMessage);
			}
			return false;
	}
}

// _PickUpActionBeforeSave
void
IconEditorApp::_PickUpActionBeforeSave()
{
	if (fDocument->WriteLock()) {
		fDocument->CommandStack()->Save();
		fDocument->WriteUnlock();
	}

	if (!fMessageAfterSave)
		return;

	PostMessage(fMessageAfterSave);
	delete fMessageAfterSave;
	fMessageAfterSave = NULL;
}

// #pragma mark -

// _MakeIconEmpty
void
IconEditorApp::_MakeIconEmpty()
{
	if (!_CheckSaveIcon(CurrentMessage()))
		return;

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
	if (!_CheckSaveIcon(CurrentMessage()))
		return;

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
			B_TRANSLATE_WITH_CONTEXT("bad news", "Title of error alert"),
			helper.String(), 
			B_TRANSLATE_WITH_CONTEXT("Bummer", 
				"Cancel button - error alert"),	
			NULL, NULL);
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
					new NativeSaver(ref));
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

	if (mainWindowLocked) {
		fMainWindow->Unlock();
		// cause the mainwindow to adopt icon in
		// it's own thread
		fMainWindow->PostMessage(MSG_SET_ICON);
	}
}

// _Open
void
IconEditorApp::_Open(const BMessenger& externalObserver, const uint8* data,
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
		status_t ret = flatImporter.Import(icon, const_cast<uint8*>(data), size);
			// NOTE: the const_cast is a bit ugly, but no harm is done
			// the reason is that the LittleEndianBuffer knows read and write
			// mode, in this case it is used read-only, and it does not assume
			// ownership of the buffer

		if (ret < B_OK) {
			// inform user of failure at this point
			BString helper(B_TRANSLATE("Opening the icon failed!"));
			helper << "\n\n" << B_TRANSLATE("Error: ") << strerror(ret);
			BAlert* alert = new BAlert(
				B_TRANSLATE_WITH_CONTEXT("bad news", "Title of error alert"),
				helper.String(), 
				B_TRANSLATE_WITH_CONTEXT("Bummer", 
					"Cancel button - error alert"),	
				NULL, NULL);
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
		case EXPORT_MODE_ICON_SOURCE:
			saver = new SimpleFileSaver(new SourceExporter(), ref);
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
			saver = new NativeSaver(ref);
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
IconEditorApp::_RestoreSettings(BMessage& settings)
{
	load_settings(&settings, "Icon-O-Matic");

	int32 mode;
	if (settings.FindInt32("export mode", &mode) >= B_OK)
		fSavePanel->SetExportMode(mode);
}

// _InstallDocumentMimeType
void
IconEditorApp::_InstallDocumentMimeType()
{
	// install mime type of documents
	BMimeType mime(kNativeIconMimeType);
	status_t ret = mime.InitCheck();
	if (ret < B_OK) {
		fprintf(stderr, "Could not init native document mime type (%s): %s.\n",
			kNativeIconMimeType, strerror(ret));
		return;
	}

	if (mime.IsInstalled() && !(modifiers() & B_SHIFT_KEY)) {
		// mime is already installed, and the user is not
		// pressing the shift key to force a re-install
		return;
	}

	ret = mime.Install();
	if (ret < B_OK) {
		fprintf(stderr, "Could not install native document mime type (%s): %s.\n",
			kNativeIconMimeType, strerror(ret));
		return;
	}
	// set preferred app
	ret = mime.SetPreferredApp(kAppSig);
	if (ret < B_OK)
		fprintf(stderr, "Could not set native document preferred app: %s\n",
			strerror(ret));

	// set descriptions
	ret = mime.SetShortDescription("Haiku Icon");
	if (ret < B_OK)
		fprintf(stderr, "Could not set short description of mime type: %s\n",
			strerror(ret));
	ret = mime.SetLongDescription("Native Haiku vector icon");
	if (ret < B_OK)
		fprintf(stderr, "Could not set long description of mime type: %s\n",
			strerror(ret));

	// set extensions
	BMessage message('extn');
	message.AddString("extensions", "icon");
	ret = mime.SetFileExtensions(&message);
	if (ret < B_OK)
		fprintf(stderr, "Could not set extensions of mime type: %s\n",
			strerror(ret));

	// set sniffer rule
	const char* snifferRule = "0.9 ('IMSG')";
	ret = mime.SetSnifferRule(snifferRule);
	if (ret < B_OK) {
		BString parseError;
		BMimeType::CheckSnifferRule(snifferRule, &parseError);
		fprintf(stderr, "Could not set sniffer rule of mime type: %s\n",
			parseError.String());
	}

// NOTE: Icon-O-Matic writes the icon being saved as icon of the file anyways
// therefor, the following code is not needed, it is also not tested and I am
// spotting an error with SetIcon()
//	// set document icon
//	BResources* resources = AppResources();
//		// does not need to be freed (belongs to BApplication base)
//	if (resources) {
//		size_t size;
//		const void* iconData = resources->LoadResource('VICN', "IOM:DOC_ICON", &size);
//		if (iconData && size > 0) {
//			memcpy(largeIcon.Bits(), iconData, size);
//			if (mime.SetIcon(&largeIcon, B_LARGE_ICON) < B_OK)
//				fprintf(stderr, "Could not set large icon of mime type.\n");
//		} else
//			fprintf(stderr, "Could not find icon in app resources (data: %p, size: %ld).\n", iconData, size);
//	} else
//		fprintf(stderr, "Could not find app resources.\n");
}
