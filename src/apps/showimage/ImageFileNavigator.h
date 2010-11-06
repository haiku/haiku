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


class Navigator;
class ProgressWindow;

enum {
	kMsgImageLoaded = 'ifnL'
};


class ImageFileNavigator {
public:
								ImageFileNavigator(const BMessenger& target,
									const entry_ref& ref,
									const BMessenger& trackerMessenger);
	virtual						~ImageFileNavigator();

			void				SetProgressWindow(
									ProgressWindow* progressWindow);

			status_t			LoadImage(const entry_ref& ref, int32 page = 1);
			const entry_ref&	ImageRef() const { return fCurrentRef; }

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

			void				FirstFile();
			void				NextFile();
			void				PreviousFile();
			bool				HasNextFile();
			bool				HasPreviousFile();

			bool				MoveFileToTrash();

private:
			status_t			_LoadNextImage(bool next, bool rewind);

private:
			BMessenger			fTarget;
			ProgressWindow*		fProgressWindow;
			Navigator*			fNavigator;

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
