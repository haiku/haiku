/*
 *  Copyright 2010-2011 Haiku, Inc. All rights reserved.
 *  Distributed under the terms of the MIT license.
 *
 *  Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef DECOR_SETTINGS_VIEW_H
#define DECOR_SETTINGS_VIEW_H


#include <Button.h>
#include <GroupView.h>
#include <InterfaceDefs.h>
#include <View.h>

#include <DecorInfo.h>


class BBox;
class BMenuField;
class BPopUpMenu;


class DecorSettingsView : public BView {
public:
							DecorSettingsView(const char* name);
	virtual					~DecorSettingsView();

	virtual	void			AttachedToWindow();
	virtual	void			MessageReceived(BMessage* message);

			void			SetDefaults();
			void			Revert();
			bool			IsDefaultable();
			bool			IsRevertable();

private:
			void			_BuildDecorMenu();
			void			_SetCurrentDecor();

			BButton*		fDecorInfoButton;	

protected:
			float			fDivider;

			BMenuField*		fDecorMenuField;
			BPopUpMenu*		fDecorMenu;

			char*			fSavedDecor;
			char*			fCurrentDecor;
};

#endif // DECOR_SETTINGS_VIEW_H
