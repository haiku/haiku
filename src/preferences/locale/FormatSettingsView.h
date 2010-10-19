/*
 * Copyright 2009, Adrien Destugues, pulkomandy@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FORMAT_SETTINGS_H
#define _FORMAT_SETTINGS_H


#include <Box.h>
#include <Locale.h>
#include <String.h>
#include <View.h>


class BCountry;
class BMenuField;
class BMessage;
class BRadioButton;
class BStringView;
class BTextControl;


enum FormatSeparator {
		kNoSeparator,
		kSpaceSeparator,
		kMinusSeparator,
		kSlashSeparator,
		kBackslashSeparator,
		kDotSeparator,
		kSeparatorsEnd
};

const uint32 kSettingsContentsModified = 'Scmo';
const uint32 kClockFormatChange = 'cfmc';
const uint32 kMenuMessage = 'FRMT';


class FormatView : public BView {
public:
								FormatView(const BLocale& locale);
								~FormatView();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

	virtual	void				Revert();
	virtual	void				SetLocale(const BLocale& locale);
	virtual	void				RecordRevertSettings();
	virtual	bool				IsRevertable() const;

private:
			void				_UpdateExamples();
			void				_SendNotices();
			void				_ParseDateFormat();
			void				_ParseCurrencyFormat();
			void				_UpdateLongDateFormatString();

private:
			BRadioButton*		f24HrRadioButton;
			BRadioButton*		f12HrRadioButton;

			BMenuField*			fLongDateMenu[4];
			BString				fLongDateString[4];
			BTextControl*		fLongDateSeparator[4];
			BMenuField*			fDateMenu[3];
			BString				fDateString[3];

			BMenuField*			fSeparatorMenuField;

			BStringView*		fLongDateExampleView;
			BStringView*		fShortDateExampleView;
			BStringView*		fLongTimeExampleView;
			BStringView*		fShortTimeExampleView;
			BStringView*		fNumberFormatExampleView;
			BStringView*		fMonetaryView;

			BTextControl*		fCurrencySymbolView;
			BRadioButton*		fCurrencySymbolBefore;
			BRadioButton*		fCurrencySymbolAfter;

			bool				f24HrClock;

			FormatSeparator		fSeparator;
			BString				fDateFormat;

			BString				fOriginalTimeFormat;
			BString				fOriginalLongTimeFormat;
			bool				fLocaleIs24Hr;

			BLocale				fLocale;

			BBox*				fDateBox;
			BBox*				fTimeBox;
			BBox*				fNumbersBox;
			BBox*				fCurrencyBox;
};


#endif	// _FORMAT_SETTINGS_H
