/*
 * Copyright 2007-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
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


class AccountConfigView : public BBox {
public:
								AccountConfigView(
									BMailAccountSettings* account);

	virtual void				DetachedFromWindow();
	virtual void				AttachedToWindow();
	virtual void				MessageReceived(BMessage* message);

			void				UpdateViews();

private:
			BTextControl*		fNameControl;
			BTextControl*		fRealNameControl;
			BTextControl*		fReturnAddressControl;
			BMailAccountSettings* fAccount;
};


class ProtocolConfigView : public BBox {
public:
								ProtocolConfigView(
									BMailAccountSettings& accountSettings,
									const entry_ref& ref,
									BMailProtocolSettings& settings);

			void 				DetachedFromWindow();

private:
			status_t			_CreateConfigView(const entry_ref& ref,
									BMailProtocolSettings& settings,
									BMailAccountSettings& accountSettings);

private:
			BMailProtocolSettings& fSettings;
			BView*				fConfigView;
			image_id			fImage;
};


#endif	/* CONFIG_VIEWS_H */
