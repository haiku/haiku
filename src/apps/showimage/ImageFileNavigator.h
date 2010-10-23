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


#include <Bitmap.h>
#include <Entry.h>
#include <NodeInfo.h>
#include <String.h>
#include <TranslatorRoster.h>


class ProgressWindow;

class ImageFileNavigator {
public:
								ImageFileNavigator(
									ProgressWindow* progressWindow);
	virtual						~ImageFileNavigator();

			void				SetTrackerMessenger(
									const BMessenger& trackerMessenger);

			status_t			LoadImage(const entry_ref* ref, BBitmap** bitmap);
			const entry_ref*	ImageRef() const { return &fCurrentRef; }

			void				GetName(BString* name);
			void				GetPath(BString* name);

			// The same image file may have multiple pages, TIFF images for
			// example. The page count is determined at image loading time.
			int32				CurrentPage();
			int32				PageCount();

			status_t			FirstPage(BBitmap** bitmap);
			status_t			LastPage(BBitmap** bitmap);
			status_t			NextPage(BBitmap** bitmap);
			status_t			PrevPage(BBitmap** bitmap);
			status_t			GoToPage(int32 page, BBitmap** bitmap);

			// Navigation to the next/previous image file is based on
			// communication with Tracker, the folder containing the current
			// image needs to be open for this to work. The routine first tries
			// to find the next candidate file, then tries to load it as image.
			// As long as loading fails, the operation is repeated for the next
			// candidate file.
			status_t			FirstFile(BBitmap** bitmap);
			status_t			NextFile(BBitmap** bitmap);
			status_t			PrevFile(BBitmap** bitmap);
			bool				HasNextFile();
			bool				HasPrevFile();

private:
			enum image_orientation {
				k0,    // 0
				k90,   // 1
				k180,  // 2
				k270,  // 3
				k0V,   // 4
				k90V,  // 5
				k0H,   // 6
				k270V, // 7
				kNumberOfOrientations,
			};

			bool				_IsImage(const entry_ref* pref);
			bool				_FindNextImage(entry_ref* inCurrent,
									entry_ref* outImage, bool next,
									bool rewind);
			status_t			_LoadNextImage(bool next, bool rewind);
			void				_SetTrackerSelectionToCurrent();

private:
			BMessenger			fTrackerMessenger;
				// of the window that this was launched from
			entry_ref			fCurrentRef;

			int32				fDocumentIndex;
				// of the image in the file
			int32				fDocumentCount;
				// number of images in the file

			BString				fImageType;
				// Type of image, for use in status bar and caption
			BString				fImageMime;

			ProgressWindow*		fProgressWindow;

			image_orientation	fImageOrientation;
	static	image_orientation	fTransformation[
									ImageProcessor
										::kNumberOfAffineTransformations]
									[kNumberOfOrientations];
};

#endif	// IMAGE_FILE_NAVIGATOR_H
