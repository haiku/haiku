/*
 * Copyright 2007-2015, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef AUTO_CONFIG_VIEW_H
#define AUTO_CONFIG_VIEW_H


#include "AutoConfig.h"
#include "ConfigViews.h"

#include <Box.h>
#include <Entry.h>
#include <GroupView.h>
#include <MenuField.h>
#include <String.h>
#include <TextControl.h>

class BPopUpMenu;


const int32	kNameChangedMsg			=	'?nch';
const int32	kEMailChangedMsg		=	'?ech';
const int32 kProtokollChangedMsg	=	'?pch';
const int32 kServerChangedMsg		=	'?sch';


enum protocol_type {
	POP,
	IMAP,
	SMTP
};


struct account_info {
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


class AutoConfigView : public BBox {
public:
								AutoConfigView(AutoConfig& config);

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* msg);

			bool				GetBasicAccountInfo(account_info& info);
			bool				IsValidMailAddress(BString email);

private:
			BPopUpMenu*			_SetupProtocolMenu();
			status_t			_GetSMTPAddOnRef(entry_ref* ref);

			BString				_ExtractLocalPart(const char* email);
			void				_ProposeUsername();

private:
			entry_ref			fSMTPAddOnRef;
			BMenuField*			fInProtocolsField;
			BTextControl*		fNameView;
			BTextControl*		fAccountNameView;
			BTextControl*		fEmailView;
			BTextControl*		fLoginNameView;
			BTextControl*		fPasswordView;

			// ref to the parent autoconfig so you only ones read the database
			AutoConfig&			fAutoConfig;
};


class ServerSettingsView : public BGroupView {
public:
								ServerSettingsView(const account_info& info);
								~ServerSettingsView();
			void				GetServerInfo(account_info& info);

private:
			void				_DetectMenuChanges();
			bool				_HasMarkedChanged(BMenuField* field,
									BMenuItem* originalItem);
			void				_GetAuthEncrMenu(entry_ref protocol,
									BMenuField*& authField,
									BMenuField*& sslField);

private:
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

			image_id			fImageID;
};


#endif	// AUTO_CONFIG_VIEW_H
