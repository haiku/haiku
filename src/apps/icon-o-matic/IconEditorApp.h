/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef ICON_EDITOR_APP_H
#define ICON_EDITOR_APP_H

#include <Application.h>
#include <String.h>

class BFilePanel;
class Document;
class DocumentSaver;
class MainWindow;
class SavePanel;

enum {
	MSG_NEW							= 'newi',

	MSG_OPEN						= 'open',
	MSG_APPEND						= 'apnd',

	MSG_SAVE						= 'save',
	MSG_SAVE_AS						= 'svas',

	MSG_EXPORT						= 'xprt',
	MSG_EXPORT_AS					= 'xpas',
};

enum {
	EXPORT_MODE_MESSAGE = 0,
	EXPORT_MODE_FLAT_ICON,
	EXPORT_MODE_SVG,
	EXPORT_MODE_BITMAP,
	EXPORT_MODE_BITMAP_SET,
	EXPORT_MODE_ICON_ATTR,
	EXPORT_MODE_ICON_MIME_ATTR,
	EXPORT_MODE_ICON_RDEF,
};

typedef enum {
	LAST_PATH_OPEN = 0,
	LAST_PATH_SAVE,
	LAST_PATH_EXPORT,
} path_kind;

class IconEditorApp : public BApplication {
 public:
								IconEditorApp();
	virtual						~IconEditorApp();

	// BApplication interface
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				ReadyToRun();
	virtual	void				RefsReceived(BMessage* message);
	virtual	void				ArgvReceived(int32 argc, char** argv);

	// IconEditorApp

 private:
			void				_MakeIconEmpty();
			void				_Open(const entry_ref& ref,
									  bool append = false);
			void				_Open(const BMessenger& externalObserver,
									  const uint8* data, size_t size);
			DocumentSaver*		_CreateSaver(const entry_ref& ref,
											 uint32 exportMode);

			void				_SyncPanels(BFilePanel* from,
											BFilePanel* to);

			const char*			_LastFilePath(path_kind which);

			void				_StoreSettings();
			void				_RestoreSettings();

			MainWindow*			fMainWindow;
			Document*			fDocument;

			BFilePanel*			fOpenPanel;
			SavePanel*			fSavePanel;

			BString				fLastOpenPath;
			BString				fLastSavePath;
			BString				fLastExportPath;
};

#endif // ICON_EDITOR_APP_H
