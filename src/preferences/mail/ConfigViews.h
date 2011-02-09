/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef CONFIG_VIEWS_H
#define CONFIG_VIEWS_H


#include <Box.h>
#include <image.h>

#include "MailSettings.h"
#include <ProtocolConfigView.h>
#include "FilterConfigView.h"


class BTextControl;
class BListView;
class BMenuField;
class BButton;
struct entry_ref;


class ProtocolsConfigView;


BView* CreateConfigView(entry_ref addon, MailAddonSettings& settings,
	BMailAccountSettings& accountSettings, image_id* image);


class AccountConfigView : public BBox {
public:
								AccountConfigView(BRect rect,
									BMailAccountSettings *account);

	virtual void				DetachedFromWindow();
	virtual void				AttachedToWindow();
	virtual void				MessageReceived(BMessage *msg);

			void				UpdateViews();

private:
			BTextControl*		fNameControl;
			BTextControl*		fRealNameControl;
			BTextControl*		fReturnAddressControl;
			BMailAccountSettings	*fAccount;
};


class InProtocolsConfigView : public BBox {
public:
								InProtocolsConfigView(
									BMailAccountSettings* account);

			void 				DetachedFromWindow();
private:
			BMailAccountSettings*	fAccount;
			BView*				fConfigView;
			image_id			fImageID;
};


class OutProtocolsConfigView : public BBox {
public:
								OutProtocolsConfigView(
									BMailAccountSettings* account);
			void 				DetachedFromWindow();
private:
			BMailAccountSettings*	fAccount;
			BView*				fConfigView;
			image_id			fImageID;
};


#endif	/* CONFIG_VIEWS_H */
