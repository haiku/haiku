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

#include <MailSettingsView.h>
#include <MailSettings.h>

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


class ProtocolSettingsView : public BBox {
public:
								ProtocolSettingsView(const entry_ref& ref,
									const BMailAccountSettings& accountSettings,
									BMailProtocolSettings& settings);

			void 				DetachedFromWindow();

private:
			status_t			_CreateSettingsView(const entry_ref& ref,
									const BMailAccountSettings& accountSettings,
									BMailProtocolSettings& settings);

private:
			BMailProtocolSettings& fSettings;
			BMailSettingsView*	fSettingsView;
			image_id			fImage;
};


#endif	/* CONFIG_VIEWS_H */
