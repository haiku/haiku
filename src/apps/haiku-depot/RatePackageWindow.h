/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef RATE_PACKAGE_WINDOW_H
#define RATE_PACKAGE_WINDOW_H

#include <Window.h>

#include "PackageInfo.h"
#include "TextDocument.h"


class BButton;
class TextDocumentView;


class RatePackageWindow : public BWindow {
public:
								RatePackageWindow(BWindow* parent, BRect frame);
	virtual						~RatePackageWindow();

	virtual	void				MessageReceived(BMessage* message);

			void				SetPackage(const PackageInfoRef& package);

private:
			void				_SendRating();

private:
			TextDocumentRef		fRatingText;
			BButton*			fSendButton;
			PackageInfoRef		fPackage;
};


#endif // RATE_PACKAGE_WINDOW_H
