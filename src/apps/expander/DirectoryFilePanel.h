/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */
#ifndef _DIRECTORY_FILE_PANEL_H
#define _DIRECTORY_FILE_PANEL_H


#include <FilePanel.h>
#include <Button.h>


const uint32 MSG_DIRECTORY = 'mDIR';


class DirectoryRefFilter : public BRefFilter {
	public:
		DirectoryRefFilter();
		bool Filter(const entry_ref *ref, BNode* node, struct stat_beos *st,
			const char *filetype);
};

class DirectoryFilePanel : public BFilePanel {
	public:
		DirectoryFilePanel(file_panel_mode mode = B_OPEN_PANEL,
			BMessenger *target = NULL, const entry_ref *startDirectory = NULL,
			uint32 nodeFlavors = 0, bool allowMultipleSelection = true,
			BMessage *message = NULL, BRefFilter *filter = NULL,
			bool modal = false, bool hideWhenDone = true);
		virtual ~DirectoryFilePanel() {};

		virtual void SelectionChanged();
		virtual void Show();
			// overrides non-virtual BFilePanel::Show()

	protected:
		BButton *fCurrentButton;
};

#endif	// _DIRECTORY_FILE_PANEL_H
