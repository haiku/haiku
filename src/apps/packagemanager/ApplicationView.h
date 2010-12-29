/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 * Copyright (C) 2010 Adrien Destugues <pulkomandy@pulkomandy.ath.cx>
 *
 * Distributed under the terms of the MIT licence.
 */
#ifndef DOWNLOAD_PROGRESS_VIEW_H
#define DOWNLOAD_PROGRESS_VIEW_H


#include <GroupView.h>
#include <Path.h>
#include <String.h>

class BEntry;
class BStatusBar;
class BStringView;
class BWebDownload;
class IconView;
class SmallButton;


enum {
	SAVE_SETTINGS = 'svst'
};


class ApplicationView : public BGroupView {
public:
								ApplicationView(const char* name,
									const char* icon,
									const char* description);
								ApplicationView(const BMessage* archive);

			bool				Init(const BMessage* archive = NULL);

	virtual	void				AttachedToWindow();
	virtual	void				AllAttached();

	virtual	void				Draw(BRect updateRect);

	virtual	void				MessageReceived(BMessage* message);

			void				ShowContextMenu(BPoint screenWhere);

private:
			IconView*			fIconView;
			BStringView*		fNameView;
			BStringView*		fInfoView;
			SmallButton*		fTopButton;
			SmallButton*		fBottomButton;
};

#endif // DOWNLOAD_PROGRESS_VIEW_H
