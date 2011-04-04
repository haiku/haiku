/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef EXPORT_SAVE_PANEL_H
#define EXPORT_SAVE_PANEL_H

#include <FilePanel.h>
#include <MenuItem.h>
#include <TranslationDefs.h>
#include <String.h>

class BButton;
class BMenuField;
class BPopUpMenu;
class BWindow;

class SaveItem : public BMenuItem {
 public:
								SaveItem(const char* name,
										 BMessage* message,
										 uint32 exportMode);
		
		uint32					ExportMode() const
									{ return fExportMode; }

 private:
		uint32					fExportMode;
};

class SavePanel : public BFilePanel,
				  public BHandler {
 public:
								SavePanel(const char* name,
										  BMessenger* target = NULL,
										  entry_ref* startDirectory = NULL,
										  uint32 nodeFlavors = 0,
										  bool allowMultipleSelection = true,
										  BMessage* message = NULL,
										  BRefFilter *filter = NULL,
										  bool modal = false,
										  bool hideWhenDone = true);
		virtual					~SavePanel();

		// BFilePanel
		virtual	void			SendMessage(const BMessenger* messenger,
											BMessage* message);
		// BHandler
		virtual	void			MessageReceived(BMessage* message);

		// SavePanel
								// setting and retrieving settings
				void			SetExportMode(bool exportMode);
				void			SetExportMode(int32 mode);
				int32			ExportMode() const;

				void			AdjustExtension();

 private:
				SaveItem*		_GetCurrentMenuItem() const;
				void			_BuildMenu();
				void			_ExportSettings();
				void			_EnableSettings() const;

				BWindow*		fConfigWindow;
				BPopUpMenu*		fFormatM;
				BMenuField*		fFormatMF;
			
				BButton*		fSettingsB;
			
				SaveItem*		fNativeMI;
				SaveItem*		fHVIFMI;
				SaveItem*		fRDefMI;
				SaveItem*		fSourceMI;
				SaveItem*		fSVGMI;
				SaveItem*		fBitmap16MI;
				SaveItem*		fBitmap32MI;
				SaveItem*		fBitmap64MI;
				SaveItem*		fBitmapSetMI;
				SaveItem*		fIconAttrMI;
				SaveItem*		fIconMimeAttrMI;

				int32			fExportMode;
};

#endif // EXPORT_SAVE_PANEL_H
