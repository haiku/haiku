/*
 * Copyright 2004-2012, Haiku Inc. All rights reserved.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _PROTOCOL_CONFIG_VIEW_H
#define _PROTOCOL_CONFIG_VIEW_H


#include <CheckBox.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>

#include <MailSettings.h>
#include <MailSettingsView.h>


class BCheckBox;
class BGridLayout;
class BMenuField;
class BTextControl;


namespace BPrivate {


class BodyDownloadConfigView : public BView {
public:
								BodyDownloadConfigView();

			void				SetTo(const BMailProtocolSettings& settings);

			status_t			SaveInto(BMailAddOnSettings& settings) const;

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

private:
			BTextControl*		fSizeControl;
			BCheckBox*			fPartialBox;
};


enum mail_protocol_config_options {
	B_MAIL_PROTOCOL_HAS_AUTH_METHODS 			= 1,
	B_MAIL_PROTOCOL_HAS_FLAVORS 				= 2,
	B_MAIL_PROTOCOL_HAS_USERNAME 				= 4,
	B_MAIL_PROTOCOL_HAS_PASSWORD 				= 8,
	B_MAIL_PROTOCOL_HAS_HOSTNAME 				= 16,
	B_MAIL_PROTOCOL_CAN_LEAVE_MAIL_ON_SERVER	= 32,
	B_MAIL_PROTOCOL_PARTIAL_DOWNLOAD			= 64
};


class MailProtocolConfigView : public BMailSettingsView {
public:
								MailProtocolConfigView(uint32 optionsMask
										= B_MAIL_PROTOCOL_HAS_FLAVORS
											| B_MAIL_PROTOCOL_HAS_USERNAME
											| B_MAIL_PROTOCOL_HAS_PASSWORD
											| B_MAIL_PROTOCOL_HAS_HOSTNAME);
	virtual						~MailProtocolConfigView();

			void				SetTo(const BMailProtocolSettings& settings);

			void				AddFlavor(const char* label);
			void				AddAuthMethod(const char* label,
									bool needUserPassword = true);

			BGridLayout*		Layout() const;

	virtual status_t			SaveInto(BMailAddOnSettings& settings) const;

	virtual	void				AttachedToWindow();
	virtual void				MessageReceived(BMessage* message);

private:
			BTextControl*		_AddTextControl(BGridLayout* layout,
									const char* name, const char* label);
			BMenuField*			_AddMenuField(BGridLayout* layout,
									const char* name, const char* label);
			void				_StoreIndexOfMarked(BMessage& message,
									const char* name, BMenuField* field) const;
			void				_StoreCheckBox(BMessage& message,
									const char* name,
									BCheckBox* checkBox) const;
			void				_SetCredentialsEnabled(bool enabled);

private:
			BTextControl*		fHostControl;
			BTextControl*		fUserControl;
			BTextControl*		fPasswordControl;
			BMenuField*			fFlavorField;
			BMenuField*			fAuthenticationField;
			BCheckBox*			fLeaveOnServerCheckBox;
			BCheckBox*			fRemoveFromServerCheckBox;
			BodyDownloadConfigView* fBodyDownloadConfig;
};


}	// namespace BPrivate


#endif	/* _PROTOCOL_CONFIG_VIEW_H */
