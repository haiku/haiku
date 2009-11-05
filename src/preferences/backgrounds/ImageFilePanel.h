/*
 * Copyright 2002-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jerome Duval (jerome.duval@free.fr)
 */
#ifndef IMAGE_FILE_PANEL_H
#define IMAGE_FILE_PANEL_H


#include <FilePanel.h>
#include <Node.h>


class BStringView;
class BView;


class CustomRefFilter : public BRefFilter {
public:
							CustomRefFilter(bool imageFiltering);
	virtual					~CustomRefFilter() {};

			bool			Filter(const entry_ref* ref, BNode* node,
								struct stat_beos* st, const char* filetype);

protected:
			bool			fImageFiltering;
								// true for images
								// false for directory
};


class ImageFilePanel : public BFilePanel {
public:
							ImageFilePanel(file_panel_mode mode = B_OPEN_PANEL,
								BMessenger* target = NULL,
								const entry_ref* startDirectory = NULL,
								uint32 nodeFlavors = 0,
								bool allowMultipleSelection = true,
								BMessage* message = NULL,
								BRefFilter* filter = NULL,
								bool modal = false,
								bool hideWhenDone = true);
							~ImageFilePanel();

	virtual	void			SelectionChanged();
			void			Show();

protected:
			BView*			fImageView;
			BStringView*	fResolutionView;
			BStringView*	fImageTypeView;
};

#endif	// IMAGE_FILE_PANEL_H

