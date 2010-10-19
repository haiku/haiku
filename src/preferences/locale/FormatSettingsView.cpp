/*
 * Copyright 2009, Adrien Destugues, pulkomandy@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FormatSettingsView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <Country.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MutableLocaleRoster.h>
#include <Message.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <ScrollView.h>
#include <ScrollBar.h>
#include <SeparatorView.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

#include "LocalePreflet.h"


using BPrivate::gMutableLocaleRoster;


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "TimeFormatSettings"


class DateMenuItem: public BMenuItem {
public:
	DateMenuItem(const char* label, const char* code, BMenuField* field)
		:
		BMenuItem(label, _MenuMessage(code, field))
	{
		fICUCode = code;
	}

	const BString& ICUCode() const
	{
		return fICUCode;
	}

private:
	static BMessage* _MenuMessage(const char* format, BMenuField* field)
	{
		BMessage* msg = new BMessage(kMenuMessage);
		msg->AddPointer("dest", field);
		msg->AddString("format", format);

		return msg;
	}

private:
	BString			fICUCode;
};


void
CreateDateMenu(BMenuField** field, bool longFormat = true)
{
	BMenu* menu = new BMenu("");
	*field = new BMenuField("", menu);

	BPopUpMenu* dayMenu = new BPopUpMenu(B_TRANSLATE("Day"));
	// Not all available ICU settings are listed here. It's possible to add some
	// other things if you ever need.
	menu->AddItem(dayMenu);
		dayMenu->AddItem(new DateMenuItem(
			B_TRANSLATE("Day in month"), "d", *field));
		dayMenu->AddItem(new DateMenuItem(
			B_TRANSLATE("Day in month (2 digits)"), "dd", *field));
		/*
		dayMenu->AddItem(new DateMenuItem(B_TRANSLATE("Day in year"),
			"D", *field));
		dayMenu->AddItem(new DateMenuItem(B_TRANSLATE("Day in year (2 digits)"),
			"DD", *field));
		dayMenu->AddItem(new DateMenuItem(B_TRANSLATE("Day in year (3 digits)"),
			"DDD", *field));
		*/
		dayMenu->AddItem(new DateMenuItem(
			B_TRANSLATE("Day of week"), "e", *field));
		// dayMenu->AddItem(new DateMenuItem("Day of week (short text)", "eee",
		//	*field));
		// dayMenu->AddItem(new DateMenuItem("Day of week (full text)", "eeee",
		//	*field));
		dayMenu->AddItem(new DateMenuItem(
			B_TRANSLATE("Day of week (short name)"), "E", *field));
		dayMenu->AddItem(new DateMenuItem(
			B_TRANSLATE("Day of week (name)"), "EEEE", *field));
		dayMenu->AddItem(new DateMenuItem(
			B_TRANSLATE("Day of week in month"), "F", *field));
		// dayMenu->AddItem(new DateMenuItem(
		//	B_TRANSLATE("julian day"), "g", *field));
		// dayMenu->AddItem(new BMenuItem("c", msg));
	BPopUpMenu* monthMenu = new BPopUpMenu(B_TRANSLATE("Month"));
	menu->AddItem(monthMenu);
		monthMenu->AddItem(new DateMenuItem(
			B_TRANSLATE("Month number"), "M", *field));
		monthMenu->AddItem(new DateMenuItem(
			B_TRANSLATE("Month number (2 digits)"), "MM", *field));
		monthMenu->AddItem(new DateMenuItem(
			B_TRANSLATE("Month name"), "MMMM", *field));
		// monthMenu->AddItem(new DateMenuItem("L", "L", *field));
	BPopUpMenu* yearMenu = new BPopUpMenu(B_TRANSLATE("Year"));
	menu->AddItem(yearMenu);
		// And here is some ICU kludge... sorry about that.
		if (longFormat)
			yearMenu->AddItem(new DateMenuItem(
				B_TRANSLATE("Year"), "y", *field));
		else {
			yearMenu->AddItem(new DateMenuItem(
				B_TRANSLATE("Year (4 digits)"), "yyyy", *field));
		}
		yearMenu->AddItem(new DateMenuItem(
			B_TRANSLATE("Year (2 digits)"), "yy", *field));
		// yearMenu->AddItem(new DateMenuItem("Y", "Y", *field));
		// yearMenu->AddItem(new DateMenuItem("u", "u", *field));
}


bool
IsSpecialDateChar(char charToTest)
{
	static const char* specials = "dDeEFgMLyYu";
	for (int i = 0; i < 11; i++)
		if (charToTest == specials[i])
			return true;
	return false;
}

// #pragma mark -


FormatView::FormatView(const BLocale& locale)
	:
	BView("WindowsSettingsView", B_FRAME_EVENTS),
	fLocale(locale)
{
	fLongDateExampleView = new BStringView("", "");

	for (int i = 0; i < 4; i++) {
		CreateDateMenu(&fLongDateMenu[i]);
		fLongDateSeparator[i] = new BTextControl("", "", "",
			new BMessage(kSettingsContentsModified));
		fLongDateSeparator[i]->SetModificationMessage(
			new BMessage(kSettingsContentsModified));
	}

	fShortDateExampleView = new BStringView("", "");

	for (int i = 0; i < 3; i++) {
		CreateDateMenu(&fDateMenu[i], false);
	}

	BPopUpMenu* menu = new BPopUpMenu(B_TRANSLATE("Separator"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("None"),
		new BMessage(kSettingsContentsModified)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Space"),
		new BMessage(kSettingsContentsModified)));
	menu->AddItem(new BMenuItem("-", new BMessage(kSettingsContentsModified)));
	menu->AddItem(new BMenuItem("/", new BMessage(kSettingsContentsModified)));
	menu->AddItem(new BMenuItem("\\", new BMessage(kSettingsContentsModified)));
	menu->AddItem(new BMenuItem(".", new BMessage(kSettingsContentsModified)));

	fSeparatorMenuField = new BMenuField(B_TRANSLATE("Separator:"), menu);

	f24HrRadioButton = new BRadioButton("", B_TRANSLATE("24 hour"),
		new BMessage(kClockFormatChange));

	f12HrRadioButton = new BRadioButton("", B_TRANSLATE("12 hour"),
		new BMessage(kClockFormatChange));

	fLocale.GetTimeFormat(fOriginalTimeFormat, false);
	fLocale.GetTimeFormat(fOriginalLongTimeFormat, true);
	if (fOriginalTimeFormat.FindFirst("a") != B_ERROR) {
		f12HrRadioButton->SetValue(B_CONTROL_ON);
		fLocaleIs24Hr = false;
	} else {
		f24HrRadioButton->SetValue(B_CONTROL_ON);
		fLocaleIs24Hr = true;
	}

	float spacing = be_control_look->DefaultItemSpacing();

	fLongTimeExampleView = new BStringView("", "");
	fShortTimeExampleView = new BStringView("", "");

	fNumberFormatExampleView = new BStringView("", "");

	BTextControl* numberThousand = new BTextControl("",
		B_TRANSLATE("Thousand separator: "), "",
		new BMessage(kSettingsContentsModified));
	BTextControl* numberDecimal = new BTextControl("",
		B_TRANSLATE("Decimal separator: "),	"",
		new BMessage(kSettingsContentsModified));
	// TODO number of decimal digits (spinbox ?)
	BCheckBox* numberLeadingZero = new BCheckBox("", B_TRANSLATE("Leading 0"),
		new BMessage(kSettingsContentsModified));
	BTextControl* numberList = new BTextControl("",
		B_TRANSLATE("List separator: "), "",
		new BMessage(kSettingsContentsModified));
	// Unit system (US/Metric) (radio)

	fCurrencySymbolView = new BTextControl("",
		B_TRANSLATE("Currency symbol:"), "",
		new BMessage(kSettingsContentsModified));
	menu = new BPopUpMenu(B_TRANSLATE("Negative marker"));
	menu->AddItem(new BMenuItem("-", new BMessage(kSettingsContentsModified)));
	menu->AddItem(new BMenuItem("()", new BMessage(kSettingsContentsModified)));

	BMenuField* currencyNegative = new BMenuField(
		B_TRANSLATE("Negative marker:"), menu);

	BTextControl* currencyDecimal = new BTextControl("",
		B_TRANSLATE("Decimal separator: "), "",
		new BMessage(kSettingsContentsModified));
	BCheckBox* currencyLeadingZero = new BCheckBox("",
		B_TRANSLATE("Leading 0"), new BMessage(kSettingsContentsModified));

	fCurrencySymbolBefore = new BRadioButton("CurrencySymbolPosition",
		B_TRANSLATE("Before"), new BMessage(kSettingsContentsModified));

	fCurrencySymbolAfter = new BRadioButton("CurrencySymbolPosition",
		B_TRANSLATE("After"), new BMessage(kSettingsContentsModified));

	fMonetaryView = new BStringView("", "");

	_UpdateExamples();
	_ParseDateFormat();
	_ParseCurrencyFormat();

	fDateBox = new BBox(B_TRANSLATE("Date"));
	fTimeBox = new BBox(B_TRANSLATE("Time"));
	fNumbersBox = new BBox(B_TRANSLATE("Numbers"));
	fCurrencyBox = new BBox(B_TRANSLATE("Currency"));

	fDateBox->SetLabel(B_TRANSLATE("Date"));
	fTimeBox->SetLabel(B_TRANSLATE("Time"));
	fNumbersBox->SetLabel(B_TRANSLATE("Numbers"));
	fCurrencyBox->SetLabel(B_TRANSLATE("Currency"));

	fDateBox->AddChild(BLayoutBuilder::Group<>(B_VERTICAL, spacing / 2)
		.SetInsets(spacing, spacing, spacing, spacing)
		.AddGroup(B_HORIZONTAL, spacing)
			.Add(new BStringView("", B_TRANSLATE("Long format:")))
			.Add(fLongDateExampleView)
			.AddGlue()
			.End()
		.AddGroup(B_HORIZONTAL, spacing)
			.Add(fLongDateMenu[0])
			.Add(fLongDateSeparator[0])
			.End()
		.AddGroup(B_HORIZONTAL, spacing)
			.Add(fLongDateMenu[1])
			.Add(fLongDateSeparator[1])
			.End()
		.AddGroup(B_HORIZONTAL, spacing)
			.Add(fLongDateMenu[2])
			.Add(fLongDateSeparator[2])
			.End()
		.AddGroup(B_HORIZONTAL, spacing)
			.Add(fLongDateMenu[3])
			.Add(fLongDateSeparator[3])
			.End()
		.AddGroup(B_HORIZONTAL, spacing)
			.Add(new BStringView("", B_TRANSLATE("Short format:")))
			.Add(fShortDateExampleView)
			.AddGlue()
			.End()
		.Add(fDateMenu[0])
		.Add(fDateMenu[1])
		.Add(fDateMenu[2])
		.View());

	fTimeBox->AddChild(BLayoutBuilder::Group<>(B_VERTICAL, spacing / 2)
		.SetInsets(spacing, spacing, spacing, spacing)
		.AddGroup(B_HORIZONTAL, spacing)
			.Add(new BStringView("", B_TRANSLATE("Long format:")))
			.Add(fLongTimeExampleView)
			.AddGlue()
			.End()
		.AddGroup(B_HORIZONTAL, spacing)
			.Add(new BStringView("", B_TRANSLATE("Short format:")))
			.Add(fShortTimeExampleView)
			.AddGlue()
			.End()
		.AddGroup(B_HORIZONTAL, spacing)
			.Add(f24HrRadioButton)
			.Add(f12HrRadioButton)
			.AddGlue()
			.End()
		.View());

	fNumbersBox->AddChild(BLayoutBuilder::Group<>(B_VERTICAL, spacing / 2)
		.SetInsets(spacing, spacing, spacing, spacing)
			.AddGroup(B_HORIZONTAL, spacing)
				.Add(new BStringView("", B_TRANSLATE("Example:")))
				.Add(fNumberFormatExampleView)
				.AddGlue()
				.End()
			.Add(numberThousand)
			.Add(numberDecimal)
			.Add(numberLeadingZero)
			.Add(numberList)
			.View());

	fCurrencyBox->AddChild(BLayoutBuilder::Group<>(B_VERTICAL, spacing / 2)
		.SetInsets(spacing, spacing, spacing, spacing)
		.Add(fMonetaryView)
		.Add(fCurrencySymbolView)
		.AddGroup(B_HORIZONTAL, spacing)
			.AddGlue()
			.Add(fCurrencySymbolBefore)
			.Add(fCurrencySymbolAfter)
			.End()
		.Add(currencyNegative)
		.Add(currencyDecimal)
		.Add(currencyLeadingZero)
		.View());


	BGroupLayout* rootLayout = new BGroupLayout(B_HORIZONTAL, spacing);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLayout(rootLayout);
	BLayoutBuilder::Group<>(rootLayout)
		.AddGroup(B_VERTICAL, spacing)
			.Add(fDateBox)
			.Add(fTimeBox)
			.AddGlue()
			.End()
		.AddGroup(B_VERTICAL, spacing)
			.Add(fNumbersBox)
			.Add(fCurrencyBox)
			.AddGlue();
}


FormatView::~FormatView()
{
	gMutableLocaleRoster->SetDefaultLocale(fLocale);
}


void
FormatView::AttachedToWindow()
{
	f24HrRadioButton->SetTarget(this);
	f12HrRadioButton->SetTarget(this);

	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < fLongDateMenu[j]->Menu()->CountItems(); i++)
			fLongDateMenu[j]->Menu()->SubmenuAt(i)->SetTargetForItems(this);
		fLongDateSeparator[j]->SetTarget(this);
	}

	for (int j = 0; j < 3; j++) {
		for (int i = 0; i < fDateMenu[j]->Menu()->CountItems(); i++)
			fDateMenu[j]->Menu()->SubmenuAt(i)->SetTargetForItems(this);
	}

	fSeparatorMenuField->Menu()->SetTargetForItems(this);
}


void
FormatView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMenuMessage:
		{
			// Update one of the dropdown menus
			void* pointerFromMessage;
			message->FindPointer("dest", &pointerFromMessage);
			BMenuField* menuField
			= static_cast<BMenuField*>(pointerFromMessage);
			BString format;
			message->FindString("format", &format);

			for (int i = 0; i < 4; i++) {
				if (fLongDateMenu[i]==menuField) {
					fLongDateString[i] = format;
					break;
				}
			}

			for (int i = 0; i < 3; i++) {
				if (fDateMenu[i]==menuField) {
					fDateString[i] = format;
					break;
				}
			}

			message->FindPointer("source", &pointerFromMessage);
			BMenuItem* menuItem = static_cast<BMenuItem*>(pointerFromMessage);

			menuField->MenuItem()->SetLabel(menuItem->Label());

			_UpdateLongDateFormatString();
			// fall through
		}

		case kSettingsContentsModified:
		{
			int32 separator = 0;
			BMenuItem* item = fSeparatorMenuField->Menu()->FindMarked();
			if (item) {
				separator = fSeparatorMenuField->Menu()->IndexOf(item);
				if (separator >= 0)
					// settings.SetTimeFormatSeparator(
					//	(FormatSeparator)separator);
					;
			}

			// Make the notification message and send it to the tracker:
			BMessage notificationMessage;
			notificationMessage.AddInt32("TimeFormatSeparator", separator);
			notificationMessage.AddBool("24HrClock",
				f24HrRadioButton->Value() == 1);

			_UpdateExamples();

			Window()->PostMessage(kSettingsContentsModified);
			break;
		}

		case kClockFormatChange:
		{
			BMessage newMessage(kMsgSettingsChanged);

			BString timeFormat;
			timeFormat = fOriginalTimeFormat;
			if (f24HrRadioButton->Value() == 1) {
				if (!fLocaleIs24Hr) {
					timeFormat.ReplaceAll("h", "H");
					timeFormat.ReplaceAll("k", "K");
					timeFormat.RemoveAll(" a");
					timeFormat.RemoveAll("a");
				}
			} else {
				if (fLocaleIs24Hr && timeFormat.FindFirst("a") == B_ERROR) {
					timeFormat.ReplaceAll("K", "k");
					timeFormat.ReplaceAll("H", "h");
					timeFormat.Append(" a");
				}
			}
			fLocale.SetTimeFormat(timeFormat.String(), false);
			newMessage.AddString("shortTimeFormat", timeFormat);

			timeFormat = fOriginalLongTimeFormat;
			if (f24HrRadioButton->Value() == 1) {
				if (!fLocaleIs24Hr) {
					timeFormat.ReplaceAll("h", "H");
					timeFormat.ReplaceAll("k", "K");
					timeFormat.RemoveAll(" a");
					timeFormat.RemoveAll("a");
				}
			} else {
				if (fLocaleIs24Hr && timeFormat.FindFirst("a") == B_ERROR) {
					timeFormat.ReplaceAll("K", "k");
					timeFormat.ReplaceAll("H", "h");
					timeFormat.Append(" a");
				}
			}
			fLocale.SetTimeFormat(timeFormat.String(), true);
			newMessage.AddString("longTimeFormat", timeFormat);
			_UpdateExamples();
			Window()->PostMessage(kSettingsContentsModified);
			be_app_messenger.SendMessage(&newMessage);
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}


void
FormatView::Revert()
{
	/*
	TrackerSettings settings;

	settings.SetTimeFormatSeparator(fSeparator);
	settings.SetDateOrderFormat(fFormat);
	settings.SetClockTo24Hr(f24HrClock);
	*/

	// ShowCurrentSettings();
	_SendNotices();
}


void
FormatView::SetLocale(const BLocale& locale)
{
	fLocale = locale;

	fLocale.GetTimeFormat(fOriginalTimeFormat, false);
	fLocale.GetTimeFormat(fOriginalLongTimeFormat, true);

	if (fOriginalTimeFormat.FindFirst("a") != B_ERROR) {
		f12HrRadioButton->SetValue(B_CONTROL_ON);
		fLocaleIs24Hr = false;
	} else {
		f24HrRadioButton->SetValue(B_CONTROL_ON);
		fLocaleIs24Hr = true;
	}

	/*
	FormatSeparator separator = settings.TimeFormatSeparator();

	if (separator >= kNoSeparator && separator < kSeparatorsEnd)
		fSeparatorMenuField->Menu()->ItemAt((int32)separator)->SetMarked(true);
	*/

	_UpdateExamples();
	_ParseDateFormat();
	_ParseCurrencyFormat();
}


void
FormatView::RecordRevertSettings()
{
	/*
	f24HrClock = settings.ClockIs24Hr();
	fSeparator = settings.TimeFormatSeparator();
	fFormat = settings.DateOrderFormat();
	*/
}


// Return true if the Revert button should be enabled (ie some setting was
// changed)
bool
FormatView::IsRevertable() const
{
	FormatSeparator separator;

	BMenuItem* item = fSeparatorMenuField->Menu()->FindMarked();
	if (item) {
		int32 index = fSeparatorMenuField->Menu()->IndexOf(item);
		if (index >= 0)
			separator = (FormatSeparator)index;
		else
			return true;
	} else
		return true;

	// TODO generate ICU string and compare to the initial one
	BString dateFormat ;
		// fYMDRadioButton->Value() ? kYMDFormat :
		//(fDMYRadioButton->Value() ? kDMYFormat : kMDYFormat);

	return f24HrClock != (f24HrRadioButton->Value() > 0)
		|| separator != fSeparator
		|| dateFormat != fDateFormat;
}


void
FormatView::_UpdateExamples()
{
	time_t timeValue = (time_t)time(NULL);
	BString timeFormat;

	fLocale.FormatDate(&timeFormat, timeValue, true);
	fLongDateExampleView->SetText(timeFormat);

	fLocale.FormatDate(&timeFormat, timeValue, false);
	fShortDateExampleView->SetText(timeFormat);

	fLocale.FormatTime(&timeFormat, timeValue, true);
	fLongTimeExampleView->SetText(timeFormat);

	fLocale.FormatTime(&timeFormat, timeValue, false);
	fShortTimeExampleView->SetText(timeFormat);

	status_t Error = fLocale.FormatNumber(&timeFormat, 1234.5678);
	if (Error == B_OK)
		fNumberFormatExampleView->SetText(timeFormat);
	else
		fNumberFormatExampleView->SetText("ERROR");
}


void
FormatView::_SendNotices()
{
	// Make the notification message and send it to the tracker:
	/*
	BMessage notificationMessage;
	notificationMessage.AddInt32("TimeFormatSeparator",
		(int32)settings.TimeFormatSeparator());
	notificationMessage.AddInt32("DateOrderFormat",
		(int32)settings.DateOrderFormat());
	notificationMessage.AddBool("24HrClock", settings.ClockIs24Hr());
	tracker->SendNotices(kDateFormatChanged, &notificationMessage);
	*/
}


void
FormatView::_ParseCurrencyFormat()
{
	BString currencySample;
	int* fieldPos = NULL;
	int fieldCount;
	BNumberElement* fieldID = NULL;
	if (fLocale.FormatMonetary(&currencySample, fieldPos, fieldID, fieldCount,
			-1234.56) != B_OK) {
		fMonetaryView->SetText("ERROR");
		return;
	}

	fMonetaryView->SetText(currencySample);

	for (int i = 0; i < fieldCount; i++) {
		BString currentSymbol;
		currentSymbol.AppendChars(currencySample.CharAt(fieldPos[i]&0xFFFF),
			(fieldPos[i]>>16) - (fieldPos[i]&0xFFFF));
		switch (fieldID[i]) {
			case B_NUMBER_ELEMENT_CURRENCY:
				fCurrencySymbolView->SetText(currentSymbol);
				if (i > fieldCount / 2)
					fCurrencySymbolAfter->SetValue(1);
				else
					fCurrencySymbolBefore->SetValue(1);
				break;
			default:
				break;
		}
	}

	free(fieldPos);
	free(fieldID);
}


//! Get the date format from ICU and set the date fields accordingly
void
FormatView::_ParseDateFormat()
{
	BString dateFormatString;
	fLocale.GetDateFormat(dateFormatString, true);
	const char* dateFormat = dateFormatString.String();

	// Travel through the string and parse it
	const char* parsePointer = dateFormat;
	const char* fieldBegin = dateFormat;

	for (int i = 0; i < 4; i++)
	{
		fieldBegin = parsePointer;
		while (*parsePointer == *(parsePointer + 1))
			parsePointer++;
		parsePointer++;
		BString str;
		str.Append(fieldBegin, parsePointer - fieldBegin);

		fLongDateString[i] = str;

		BMenu* subMenu;
		bool isFound = false;
		for (int subMenuIndex = 0; subMenuIndex < 3; subMenuIndex++) {
			subMenu = fLongDateMenu[i]->Menu()->SubmenuAt(subMenuIndex);
			BMenuItem* item;
			for (int itemIndex = 0; (item = subMenu->ItemAt(itemIndex)) != NULL;
					itemIndex++) {
				if (static_cast<DateMenuItem*>(item)->ICUCode() == str) {
					item->SetMarked(true);
					fLongDateMenu[i]->MenuItem()->SetLabel(item->Label());
					isFound = true;
				} else
					item->SetMarked(false);
			}
		}

		if (!isFound)
			fLongDateMenu[i]->MenuItem()->SetLabel(str.Append("*"));

		fieldBegin = parsePointer;
		while ((!IsSpecialDateChar(*parsePointer)) && *parsePointer != '\0') {
			if (*parsePointer == '\'') {
				parsePointer++;
				while (*parsePointer != '\0' && *parsePointer != '\'')
					parsePointer++;
				if (*parsePointer == '\0')
					break;
			}
			parsePointer++;
		}
		str.Truncate(0);
		str.Append(fieldBegin, parsePointer - fieldBegin);
		fLongDateSeparator[i]->SetText(str);
	}

	// Short date is a bit more tricky, we want to extract the separator
	fLocale.GetDateFormat(dateFormatString, false);
	dateFormat = dateFormatString.String();

	// Travel trough the string and parse it
	parsePointer = dateFormat;
	fieldBegin = dateFormat;

	for (int i = 0; i < 3; i++) {
		fieldBegin = parsePointer;
		while (*parsePointer == *(parsePointer + 1))
			parsePointer++;
		parsePointer++;
		BString str;
		str.Append(fieldBegin, parsePointer - fieldBegin);

		fLongDateString[i] = str;

		BMenu* subMenu;
		bool isFound = false;
		for (int subMenuIndex = 0; subMenuIndex < 3; subMenuIndex++) {
			subMenu = fDateMenu[i]->Menu()->SubmenuAt(subMenuIndex);
			BMenuItem* item;
			for (int itemIndex = 0; (item = subMenu->ItemAt(itemIndex)) != NULL;
					itemIndex++) {
				if (static_cast<DateMenuItem*>(item)->ICUCode() == str) {
					item->SetMarked(true);
					fDateMenu[i]->MenuItem()->SetLabel(item->Label());
					isFound = true;
				} else
					item->SetMarked(false);
			}
		}

		if (!isFound) {
			fDateMenu[i]->MenuItem()->SetLabel(
				str.Append(B_TRANSLATE(" (unknown format)")));
		}

		fieldBegin = parsePointer;
		while ((!IsSpecialDateChar(*parsePointer)) && *parsePointer != '\0') {
			if (*parsePointer == '\'') {
				parsePointer++;
				while (*parsePointer != '\0' && *parsePointer != '\'')
					parsePointer++;
				if (*parsePointer == '\0')
					break;
			}
			parsePointer++;
		}
		if (parsePointer - fieldBegin > 0) {
			str.Truncate(0);
			str.Append(fieldBegin, parsePointer - fieldBegin);
			fSeparatorMenuField->MenuItem()->SetLabel(str);
		}
	}
}


void
FormatView::_UpdateLongDateFormatString()
{
	BString newDateFormat;

	for (int i = 0; i < 4; i++) {
		newDateFormat.Append(fLongDateString[i]);
		newDateFormat.Append(fLongDateSeparator[i]->Text());
	}

	// TODO save this in the settings preflet and make the roster load it back
	fLocale.SetDateFormat(newDateFormat.String());

	newDateFormat.Truncate(0);

	newDateFormat.Append(fDateString[0]);
	newDateFormat.Append(fSeparatorMenuField->MenuItem()->Label());
	newDateFormat.Append(fDateString[1]);
	newDateFormat.Append(fSeparatorMenuField->MenuItem()->Label());
	newDateFormat.Append(fDateString[2]);

	// TODO save this in the settings preflet and make the roster load it back
	fLocale.SetDateFormat(newDateFormat.String(), false);
}
