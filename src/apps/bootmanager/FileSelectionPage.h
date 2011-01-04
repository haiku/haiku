/*
 * Copyright 2008-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef FILE_SELECTION_PAGE_H
#define FILE_SELECTION_PAGE_H


#include "WizardPageView.h"

#include <FilePanel.h>


class BButton;
class BTextControl;
class BTextView;


class FileSelectionPage : public WizardPageView {
public:
								FileSelectionPage(BMessage* settings,
									BRect frame, const char* name,
									const char* description,
									file_panel_mode mode);
	virtual						~FileSelectionPage();

	virtual	void				FrameResized(float width, float height);
	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

	virtual	void				PageCompleted();

private:
			void				_BuildUI(const char* description);
			void				_Layout();
			void				_OpenFilePanel();
			void				_SetFileFromFilePanelMessage(BMessage* message);
			void				_FilePanelCanceled();

private:
			file_panel_mode		fMode;
			BFilePanel*			fFilePanel;

			BTextView*			fDescription;
			BTextControl*		fFile;
			BButton*			fSelect;
};


#endif	// FILE_SELECTION_PAGE_H
