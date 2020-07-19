/*
 * Copyright 2009, Adrien Destugues, pulkomandy@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FORMAT_SETTINGS_H
#define _FORMAT_SETTINGS_H


#include <Box.h>
#include <FormattingConventions.h>
#include <String.h>
#include <View.h>


class BCheckBox;
class BCountry;
class BMenuField;
class BMessage;
class BRadioButton;
class BStringView;
class BTextControl;


static const uint32 kClockFormatChange = 'cfmc';
static const uint32 kStringsLanguageChange = 'strc';
static const uint32 kMsgFilesystemTranslationChanged = 'fsys';


class FormatSettingsView : public BView {
public:
								FormatSettingsView();
								~FormatSettingsView();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

	virtual	void				Revert();
	virtual	void				Refresh(bool setInitial = false);
	virtual	bool				IsReversible() const;

private:
			void				_UpdateExamples();

private:
			BCheckBox*			fFilesystemTranslationCheckbox;
			BCheckBox*			fUseLanguageStringsCheckBox;

			BRadioButton*		f24HourRadioButton;
			BRadioButton*		f12HourRadioButton;

			BStringView*		fFullDateExampleView;
			BStringView*		fLongDateExampleView;
			BStringView*		fMediumDateExampleView;
			BStringView*		fShortDateExampleView;

			BStringView*		fFullTimeExampleView;
			BStringView*		fLongTimeExampleView;
			BStringView*		fMediumTimeExampleView;
			BStringView*		fShortTimeExampleView;

			BStringView*		fPositiveNumberExampleView;
			BStringView*		fNegativeNumberExampleView;
			BStringView*		fPositiveMonetaryExampleView;
			BStringView*		fNegativeMonetaryExampleView;

			bool				fLocaleIs24Hour;

			BFormattingConventions	fInitialConventions;
			bool	fInitialTranslateNames;

			BBox*				fDateBox;
			BBox*				fTimeBox;
			BBox*				fNumberBox;
			BBox*				fMonetaryBox;
};


#endif	// _FORMAT_SETTINGS_H
