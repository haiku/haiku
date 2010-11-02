/*
 * Copyright 2003-2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2004-2005 yellowTAB GmbH. All Rights Reserverd.
 * Copyright 2006 Bernd Korz. All Rights Reserved
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Michael Pfeiffer
 *		yellowTAB GmbH
 *		Bernd Korz
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef IMAGE_FILE_NAVIGATOR_H
#define IMAGE_FILE_NAVIGATOR_H


#include <Entry.h>
#include <Messenger.h>
#include <String.h>


class ProgressWindow;

enum {
	kMsgImageLoaded = 'ifnL'
};


class ImageFileNavigator {
public:
								ImageFileNavigator(const BMessenger& target);
	virtual						~ImageFileNavigator();

			void				SetTrackerMessenger(
									const BMessenger& trackerMessenger);
			void				SetProgressWindow(
									ProgressWindow* progressWindow);

			status_t			LoadImage(const entry_ref& ref, int32 page = 1);
			const entry_ref&	ImageRef() const { return fCurrentRef; }

			void				GetName(BString* name);
			void				GetPath(BString* name);

			// The same image file may have multiple pages, TIFF images for
			// example. The page count is determined at image loading time.
			int32				CurrentPage();
			int32				PageCount();

			bool				FirstPage();
			bool				LastPage();
			bool				NextPage();
			bool				PreviousPage();
			bool				GoToPage(int32 page);

			// Navigation to the next/previous image file is based on
			// communication with Tracker, the folder containing the current
			// image needs to be open for this to work. The routine first tries
			// to find the next candidate file, then tries to load it as image.
			// As long as loading fails, the operation is repeated for the next
			// candidate file.
			void				FirstFile();
			void				NextFile();
			void				PreviousFile();
			bool				HasNextFile();
			bool				HasPreviousFile();

			void				DeleteFile();

private:
			bool				_IsImage(const entry_ref& ref);
			bool				_FindNextImage(const entry_ref& current,
									entry_ref& ref, bool next,
									bool rewind);
			status_t			_LoadNextImage(bool next, bool rewind);
			void				_SetTrackerSelectionToCurrent();

private:
			BMessenger			fTarget;
			BMessenger			fTrackerMessenger;
				// of the window that this was launched from
			ProgressWindow*		fProgressWindow;

			entry_ref			fCurrentRef;
			int32				fDocumentIndex;
				// of the image in the file
			int32				fDocumentCount;
				// number of images in the file

			BString				fImageType;
				// Type of image, for use in status bar and caption
			BString				fImageMime;
};


#endif	// IMAGE_FILE_NAVIGATOR_H
