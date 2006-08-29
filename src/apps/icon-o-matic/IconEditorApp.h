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

class BFilePanel;
class Document;
class MainWindow;

enum {
	MSG_NEW							= 'newi',

	MSG_OPEN						= 'open',
	MSG_APPEND						= 'apnd',

	MSG_SAVE						= 'save',
	MSG_SAVE_AS						= 'svas',

	MSG_EXPORT						= 'xprt',
	MSG_EXPORT_AS					= 'xpas',

	// TODO: remove once export format can be chosen
	MSG_EXPORT_BITMAP				= 'xpbm',
	MSG_EXPORT_BITMAP_SET			= 'xpbs',

	MSG_EXPORT_SVG					= 'xpsv',

	MSG_EXPORT_ICON_ATTRIBUTE		= 'xpia',
	MSG_EXPORT_ICON_MIME_ATTRIBUTE	= 'xpma',
	MSG_EXPORT_ICON_RDEF			= 'xprd',
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
			void				_Save(const entry_ref& ref,
									  uint32 exportMode);

			void				_SyncPanels(BFilePanel* from,
											BFilePanel* to);

			MainWindow*			fMainWindow;
			Document*			fDocument;

			BFilePanel*			fOpenPanel;
			BFilePanel*			fSavePanel;
};

#endif // ICON_EDITOR_APP_H
