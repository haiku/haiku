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


class ImageFileNavigator {
public:
								ImageFileNavigator(const entry_ref& ref,
									const BMessenger& trackerMessenger);
	virtual						~ImageFileNavigator();

			void				SetTo(const entry_ref& ref, int32 page = 1,
									int32 pageCount = 1);
			const entry_ref&	CurrentRef() const { return fCurrentRef; }

			// The same image file may have multiple pages, TIFF images for
			// example. The page count is determined at image loading time.
			int32				CurrentPage();
			int32				PageCount();

			bool				FirstPage();
			bool				LastPage();
			bool				NextPage();
			bool				PreviousPage();
			bool				GoToPage(int32 page);

			bool				FirstFile();
			bool				NextFile();
			bool				PreviousFile();
			bool				HasNextFile();
			bool				HasPreviousFile();

			bool				GetNextFile(const entry_ref& ref,
									entry_ref& nextRef);
			bool				GetPreviousFile(const entry_ref& ref,
									entry_ref& previousRef);

			bool				MoveFileToTrash();

private:
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
