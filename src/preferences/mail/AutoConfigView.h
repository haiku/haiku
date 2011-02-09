/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef AUTO_CONFIG_VIEW_H
#define AUTO_CONFIG_VIEW_H

#include "AutoConfig.h"
#include "ConfigViews.h"

#include <Box.h>
#include <Entry.h>
#include <MenuField.h>
#include <String.h>
#include <TextControl.h>


const int32	kNameChangedMsg			=	'?nch';
const int32	kEMailChangedMsg		=	'?ech';
const int32 kProtokollChangedMsg	=	'?pch';
const int32 kServerChangedMsg		=	'?sch';


enum protocol_type
{
	POP,
	IMAP,
	SMTP
};


struct account_info
{
	protocol_type	inboundType;
	entry_ref		inboundProtocol;
	entry_ref		outboundProtocol;
	BString			name;
	BString			accountName;
	BString			email;
	BString			loginName;
	BString			password;
	provider_info	providerInfo;
};


class AutoConfigView : public BBox
{
	public:
						AutoConfigView(BRect rect, AutoConfig &config);
						
		virtual void	AttachedToWindow();
		virtual void	MessageReceived(BMessage *msg);

		bool			GetBasicAccountInfo(account_info &info);
		bool			IsValidMailAddress(BString email);

	private:
		BMenuField*		SetupProtocolView(BRect rect);
		status_t		GetSMTPAddonRef(entry_ref *ref);

		BString			ExtractLocalPart(const char* email);
		void			ProposeUsername();

		entry_ref		fSMTPAddonRef;
		BMenuField		*fInProtocolsField;
		BTextControl	*fNameView;
		BTextControl	*fAccountNameView;
		BTextControl	*fEmailView;
		BTextControl	*fLoginNameView;
		BTextControl	*fPasswordView;

		// ref to the parent autoconfig so you only ones read the database
		AutoConfig		&fAutoConfig;
};


class ServerSettingsView : public BView
{
public:
								ServerSettingsView(BRect rect,
									const account_info &info);
								~ServerSettingsView();
			void				GetServerInfo(account_info &info);

private:
			void				DetectMenuChanges();
			void				GetAuthEncrMenu(entry_ref protocol,
									BMenuField **authField,
									BMenuField **sslField);
			bool				fInboundAccount;
			bool				fOutboundAccount;
			BTextControl*		fInboundNameView;
			BMenuField*			fInboundAuthMenu;
			BMenuField*			fInboundEncryptionMenu;
			BTextControl*		fOutboundNameView;
			BMenuField*			fOutboundAuthMenu;
			BMenuField*			fOutboundEncryptionMenu;

			BMenuItem*			fInboundAuthItemStart;
			BMenuItem*			fInboundEncrItemStart;
			BMenuItem*			fOutboundAuthItemStart;
			BMenuItem*			fOutboundEncrItemStart;

			image_id			fImageId;
};


#endif
