/*
 * Copyright 2013, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */ 

#ifndef CONFIG_VIEW_H
#define CONFIG_VIEW_H

#include "TranslatorSettings.h"

#include <View.h>
#include <TextView.h>
#include <String.h>
#include <GroupView.h>
#include <CheckBox.h>
#include <MenuField.h>
	
#define MSG_COMPRESSION_CHANGED 'cchg'
#define MSG_VERSION_CHANGED 	'vchg'
	
class ConfigView : public BGroupView {
	public:
		ConfigView(TranslatorSettings *settings);
		virtual ~ConfigView();

		virtual void AllAttached();
		virtual void MessageReceived(BMessage* message);

	private:
		void _AddItemToMenu(BMenu* menu, const char* label,
			uint32 mess, uint32 value, uint32 current_value);

		BTextView*			fCopyrightView;
		BMenuField*			fCompressionField;
		BMenuField*			fVersionField;

		TranslatorSettings *fSettings;
};

#endif	// CONFIG_VIEW_H
