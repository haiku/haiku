/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef _SETTINGS_VIEWS_H
#define _SETTINGS_VIEWS_H


#include <GroupView.h>

#include "TrackerSettings.h"


const uint32 kSettingsContentsModified = 'Scmo';


class BButton;
class BCheckBox;
class BColorControl;
class BMenuField;
class BRadioButton;
class BStringView;


namespace BPrivate {

class SettingsView : public BGroupView {
public:
			SettingsView(const char* name);
	virtual ~SettingsView();

	virtual void SetDefaults();
	virtual bool IsDefaultable() const;
	virtual void Revert();
	virtual void ShowCurrentSettings();
	virtual void RecordRevertSettings();
	virtual bool IsRevertable() const;

protected:
	typedef BGroupView _inherited;
};


class DesktopSettingsView : public SettingsView {
public:
	DesktopSettingsView();

	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow();

	virtual void SetDefaults();
	virtual bool IsDefaultable() const;
	virtual void Revert();
	virtual void ShowCurrentSettings();
	virtual void RecordRevertSettings();
	virtual bool IsRevertable() const;

private:
	void _SendNotices();

	BRadioButton*	fShowDisksIconRadioButton;
	BRadioButton*	fMountVolumesOntoDesktopRadioButton;
	BCheckBox*		fMountSharedVolumesOntoDesktopCheckBox;

	bool fShowDisksIcon;
	bool fMountVolumesOntoDesktop;
	bool fMountSharedVolumesOntoDesktop;
	bool fIntegrateNonBootBeOSDesktops;
	bool fEjectWhenUnmounting;

	typedef SettingsView _inherited;
};


class WindowsSettingsView : public SettingsView {
public:
	WindowsSettingsView();

	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow();

	virtual void SetDefaults();
	virtual bool IsDefaultable() const;
	virtual void Revert();
	virtual void ShowCurrentSettings();
	virtual void RecordRevertSettings();
	virtual bool IsRevertable() const;

private:
	BCheckBox* fShowFullPathInTitleBarCheckBox;
	BCheckBox* fSingleWindowBrowseCheckBox;
	BCheckBox* fShowNavigatorCheckBox;
	BCheckBox* fOutlineSelectionCheckBox;
	BCheckBox* fSortFolderNamesFirstCheckBox;
	BCheckBox* fHideDotFilesCheckBox;
	BCheckBox* fTypeAheadFilteringCheckBox;
	BCheckBox* fGenerateImageThumbnailsCheckBox;

	bool fShowFullPathInTitleBar;
	bool fSingleWindowBrowse;
	bool fShowNavigator;
	bool fTransparentSelection;
	bool fSortFolderNamesFirst;
	bool fHideDotFiles;
	bool fTypeAheadFiltering;
	bool fGenerateImageThumbnails;

	typedef SettingsView _inherited;
};


class SpaceBarSettingsView : public SettingsView {
public:
	SpaceBarSettingsView();
	virtual ~SpaceBarSettingsView();

	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow();

	virtual void SetDefaults();
	virtual bool IsDefaultable() const;
	virtual void Revert();
	virtual void ShowCurrentSettings();
	virtual void RecordRevertSettings();
	virtual bool IsRevertable() const;

private:
	BCheckBox*		fSpaceBarShowCheckBox;
	BColorControl*	fColorControl;
	BMenuField*		fColorPicker;
	int32			fCurrentColor;

	bool			fSpaceBarShow;
	rgb_color		fUsedSpaceColor;
	rgb_color		fFreeSpaceColor;
	rgb_color		fWarningSpaceColor;

	typedef SettingsView _inherited;
};


class AutomountSettingsPanel : public SettingsView {
public:
								AutomountSettingsPanel();
	virtual						~AutomountSettingsPanel();

	virtual	bool				IsDefaultable() const;
	virtual	void				Revert();
	virtual	void				ShowCurrentSettings();
	virtual void				RecordRevertSettings();
	virtual	bool				IsRevertable() const;

protected:
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

private:
			void				_SendSettings(bool rescan);
			void				_GetSettings(BMessage* reply) const;
			void				_ParseSettings(const BMessage& settings);

			BRadioButton*		fInitialDontMountCheck;
			BRadioButton*		fInitialMountAllBFSCheck;
			BRadioButton*		fInitialMountAllCheck;
			BRadioButton*		fInitialMountRestoreCheck;

			BRadioButton*		fScanningDisabledCheck;
			BRadioButton*		fAutoMountAllBFSCheck;
			BRadioButton*		fAutoMountAllCheck;

			BCheckBox*			fEjectWhenUnmountingCheckBox;

			BButton*			fMountAllNow;

			BMessenger			fTarget;
			BMessage			fInitialSettings;

			typedef SettingsView _inherited;
};


} // namespace BPrivate

using namespace BPrivate;


#endif	// _SETTINGS_VIEWS_H
