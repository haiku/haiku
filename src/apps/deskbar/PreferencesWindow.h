/*
 * Copyright 2009 Haiku, Inc.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */
#ifndef _PREFERENCES_WINDOW_H
#define _PREFERENCES_WINDOW_H


#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <RadioButton.h>
#include <Slider.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>


const uint32 kConfigShow = 'show';
const uint32 kConfigClose = 'canc';
const uint32 kUpdateRecentCounts = 'upct';
const uint32 kEditMenuInTracker = 'mtrk';
const uint32 kStateChanged = 'stch';


class PreferencesWindow : public BWindow
{
public:
							PreferencesWindow(BRect frame);
							~PreferencesWindow();

	virtual void			MessageReceived(BMessage* message);
	virtual void			WindowActivated(bool active);

private:
			void			_UpdateRecentCounts();
			void			_EnableDisableDependentItems();
			void			_UpdateTimeFormatRadioButtonLabels();

			BBox*			fMenuBox;
			BBox*			fAppsBox;
			BBox*			fClockBox;
			BBox*			fWindowBox;

			BCheckBox*		fMenuRecentDocuments;
			BCheckBox*		fMenuRecentApplications;
			BCheckBox*		fMenuRecentFolders;

			BTextControl*	fMenuRecentDocumentCount;
			BTextControl*	fMenuRecentApplicationCount;
			BTextControl*	fMenuRecentFolderCount;

			BCheckBox*		fAppsSort;
			BCheckBox*		fAppsSortTrackerFirst;
			BCheckBox*		fAppsShowExpanders;
			BCheckBox*		fAppsExpandNew;
			BCheckBox*		fAppsHideLabels;
			BSlider*		fAppsIconSizeSlider;

			BCheckBox*		fWindowAlwaysOnTop;
			BCheckBox*		fWindowAutoRaise;
			BCheckBox*		fWindowAutoHide;

			BCheckBox*		fShowTime;
			BRadioButton*	fTimeInterval24HourRadioButton;
			BRadioButton*	fTimeInterval12HourRadioButton;

			BRadioButton*	fTimeFormatLongRadioButton;
			BRadioButton*	fTimeFormatMediumRadioButton;
			BRadioButton*	fTimeFormatShortRadioButton;
};

#endif	// _PREFERENCES_WINDOW_H

