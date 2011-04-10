/*
 * Copyright 2006, 2011, Stephan Aßmus <superstippi@gmx.de>.
 * Distributed under the terms of the MIT License.
 */
#ifndef ICON_EDITOR_APP_H
#define ICON_EDITOR_APP_H


#include <Application.h>
#include <String.h>


class BFilePanel;
class MainWindow;
class SavePanel;


enum {
	MSG_NEW							= 'newi',
	MSG_SAVE_AS						= 'svas',
	MSG_EXPORT_AS					= 'xpas',
	MSG_WINDOW_CLOSED				= 'wndc',
};


enum {
	EXPORT_MODE_MESSAGE = 0,
	EXPORT_MODE_FLAT_ICON,
	EXPORT_MODE_SVG,
	EXPORT_MODE_BITMAP_16,
	EXPORT_MODE_BITMAP_32,
	EXPORT_MODE_BITMAP_64,
	EXPORT_MODE_BITMAP_SET,
	EXPORT_MODE_ICON_ATTR,
	EXPORT_MODE_ICON_MIME_ATTR,
	EXPORT_MODE_ICON_RDEF,
	EXPORT_MODE_ICON_SOURCE,
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
			MainWindow*			_NewWindow();

			void				_SyncPanels(BFilePanel* from,
											BFilePanel* to);

			const char*			_LastFilePath(path_kind which);

			void				_StoreSettings();
			void				_RestoreSettings();
			void				_InstallDocumentMimeType();

private:
			int32				fWindowCount;
			BMessage			fLastWindowSettings;
			BRect				fLastWindowFrame;

			BFilePanel*			fOpenPanel;
			SavePanel*			fSavePanel;

			BString				fLastOpenPath;
			BString				fLastSavePath;
			BString				fLastExportPath;
};


#endif // ICON_EDITOR_APP_H
