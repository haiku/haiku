/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 */
#ifndef SERVICE_VIEW_H
#define SERVICE_VIEW_H


#include <NetworkSettings.h>
#include <View.h>


using namespace BNetworkKit;

class BButton;


class ServiceView : public BView {
public:
								ServiceView(const char* name,
									const char* executable, const char* title,
									const char* description,
									BNetworkSettings& settings);
	virtual						~ServiceView();

			void				SettingsUpdated(uint32 which);

	virtual	void				AttachedToWindow();
	virtual void				MessageReceived(BMessage* message);

protected:
	virtual	bool				IsEnabled() const;
	virtual	void				Enable();
	virtual	void				Disable();

private:
			void				_UpdateEnableButton();

protected:
			const char*			fName;
			const char*			fExecutable;
			BNetworkSettings&	fSettings;
			BButton*			fEnableButton;
};


#endif // SERVICE_VIEW_H
