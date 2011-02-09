/* ProtocolConfigView - the standard config view for all protocols
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/
#ifndef ZOIDBERG_PROTOCOL_CONFIG_VIEW_H
#define ZOIDBERG_PROTOCOL_CONFIG_VIEW_H


#include <CheckBox.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>

#include "MailSettings.h"


class BodyDownloadConfig : public BView {
public:
								BodyDownloadConfig();

			void				SetTo(MailAddonSettings& settings);

			void				MessageReceived(BMessage *msg);
			void				AttachedToWindow();
			void				GetPreferredSize(float *width, float *height);
			status_t			Archive(BMessage *into, bool) const;
private:
			BTextControl*		fSizeBox;
			BCheckBox*			fPartialBox;
			BStringView*		fBytesLabel;
};


typedef enum {
	B_MAIL_PROTOCOL_HAS_AUTH_METHODS 		= 1,
	B_MAIL_PROTOCOL_HAS_FLAVORS 				= 2,
	B_MAIL_PROTOCOL_HAS_USERNAME 			= 4,
	B_MAIL_PROTOCOL_HAS_PASSWORD 			= 8,
	B_MAIL_PROTOCOL_HAS_HOSTNAME 			= 16,
	B_MAIL_PROTOCOL_CAN_LEAVE_MAIL_ON_SERVER = 32,
	B_MAIL_PROTOCOL_PARTIAL_DOWNLOAD = 64
} b_mail_protocol_config_options;


class BMailProtocolConfigView : public BView {
public:
								BMailProtocolConfigView(uint32 options_mask
									= B_MAIL_PROTOCOL_HAS_FLAVORS
									| B_MAIL_PROTOCOL_HAS_USERNAME
									| B_MAIL_PROTOCOL_HAS_PASSWORD
									| B_MAIL_PROTOCOL_HAS_HOSTNAME);
	virtual						~BMailProtocolConfigView();
		
			void				SetTo(MailAddonSettings& archive);
		
			void				AddFlavor(const char *label);
			void				AddAuthMethod(const char *label,
									bool needUserPassword = true);

	virtual	status_t			Archive(BMessage *into, bool deep = true) const;
	virtual	void				GetPreferredSize(float *width, float *height);
	virtual	void				AttachedToWindow();
	virtual void				MessageReceived(BMessage *msg);
	
private:
			BodyDownloadConfig*	fBodyDownloadConfig;
};

#endif	/* ZOIDBERG_PROTOCOL_CONFIG_VIEW_H */
