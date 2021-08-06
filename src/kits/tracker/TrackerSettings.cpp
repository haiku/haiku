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


#include "TrackerSettings.h"

#include <Debug.h>

#include "Tracker.h"
#include "WidgetAttributeText.h"


class TTrackerState : public Settings {
public:
	static TTrackerState* Get();
	void Release();

	void LoadSettingsIfNeeded();
	void SaveSettings(bool onlyIfNonDefault = true);

	TTrackerState();
	~TTrackerState();

private:
	friend class BPrivate::TrackerSettings;

	static void InitIfNeeded();
	TTrackerState(const TTrackerState&);

	BooleanValueSetting* fShowDisksIcon;
	BooleanValueSetting* fMountVolumesOntoDesktop;
	BooleanValueSetting* fDesktopFilePanelRoot;
	BooleanValueSetting* fMountSharedVolumesOntoDesktop;
	BooleanValueSetting* fEjectWhenUnmounting;

	BooleanValueSetting* fShowFullPathInTitleBar;
	BooleanValueSetting* fSingleWindowBrowse;
	BooleanValueSetting* fShowNavigator;
	BooleanValueSetting* fShowSelectionWhenInactive;
	BooleanValueSetting* fTransparentSelection;
	BooleanValueSetting* fSortFolderNamesFirst;
	BooleanValueSetting* fHideDotFiles;
	BooleanValueSetting* fTypeAheadFiltering;
	BooleanValueSetting* fGenerateImageThumbnails;

	ScalarValueSetting* fRecentApplicationsCount;
	ScalarValueSetting* fRecentDocumentsCount;
	ScalarValueSetting* fRecentFoldersCount;

	BooleanValueSetting* fShowVolumeSpaceBar;
	HexScalarValueSetting* fUsedSpaceColor;
	HexScalarValueSetting* fFreeSpaceColor;
	HexScalarValueSetting* fWarningSpaceColor;

	BooleanValueSetting* fDontMoveFilesToTrash;
	BooleanValueSetting* fAskBeforeDeleteFile;

	Benaphore fInitLock;
	bool fInited;
	bool fSettingsLoaded;

	int32 fUseCounter;

	typedef Settings _inherited;
};


static TTrackerState gTrackerState;


rgb_color ValueToColor(int32 value)
{
	rgb_color color;
	color.alpha = static_cast<uchar>((value >> 24L) & 0xff);
	color.red = static_cast<uchar>((value >> 16L) & 0xff);
	color.green = static_cast<uchar>((value >> 8L) & 0xff);
	color.blue = static_cast<uchar>(value & 0xff);

	return color;
}


int32 ColorToValue(rgb_color color)
{
	return color.alpha << 24L | color.red << 16L | color.green << 8L
		| color.blue;
}


//	#pragma mark - TTrackerState


TTrackerState::TTrackerState()
	:
	Settings("TrackerSettings", "Tracker"),
	fShowDisksIcon(NULL),
	fMountVolumesOntoDesktop(NULL),
	fDesktopFilePanelRoot(NULL),
	fMountSharedVolumesOntoDesktop(NULL),
	fEjectWhenUnmounting(NULL),
	fShowFullPathInTitleBar(NULL),
	fSingleWindowBrowse(NULL),
	fShowNavigator(NULL),
	fShowSelectionWhenInactive(NULL),
	fTransparentSelection(NULL),
	fSortFolderNamesFirst(NULL),
	fHideDotFiles(NULL),
	fTypeAheadFiltering(NULL),
	fGenerateImageThumbnails(NULL),
	fRecentApplicationsCount(NULL),
	fRecentDocumentsCount(NULL),
	fRecentFoldersCount(NULL),
	fShowVolumeSpaceBar(NULL),
	fUsedSpaceColor(NULL),
	fFreeSpaceColor(NULL),
	fWarningSpaceColor(NULL),
	fDontMoveFilesToTrash(NULL),
	fAskBeforeDeleteFile(NULL),
	fInited(false),
	fSettingsLoaded(false)
{
}


TTrackerState::TTrackerState(const TTrackerState&)
	:
	Settings("", ""),
	fShowDisksIcon(NULL),
	fMountVolumesOntoDesktop(NULL),
	fDesktopFilePanelRoot(NULL),
	fMountSharedVolumesOntoDesktop(NULL),
	fEjectWhenUnmounting(NULL),
	fShowFullPathInTitleBar(NULL),
	fSingleWindowBrowse(NULL),
	fShowNavigator(NULL),
	fShowSelectionWhenInactive(NULL),
	fTransparentSelection(NULL),
	fSortFolderNamesFirst(NULL),
	fHideDotFiles(NULL),
	fTypeAheadFiltering(NULL),
	fGenerateImageThumbnails(NULL),
	fRecentApplicationsCount(NULL),
	fRecentDocumentsCount(NULL),
	fRecentFoldersCount(NULL),
	fShowVolumeSpaceBar(NULL),
	fUsedSpaceColor(NULL),
	fFreeSpaceColor(NULL),
	fWarningSpaceColor(NULL),
	fDontMoveFilesToTrash(NULL),
	fAskBeforeDeleteFile(NULL),
	fInited(false),
	fSettingsLoaded(false)
{
	// Placeholder copy constructor to prevent others from accidentally using
	// the default copy constructor.  Note, the DEBUGGER call is for the off
	// chance that a TTrackerState method (or friend) tries to make a copy.
	DEBUGGER("Don't make a copy of this!");
}


TTrackerState::~TTrackerState()
{
}


void
TTrackerState::SaveSettings(bool onlyIfNonDefault)
{
	if (fSettingsLoaded)
		_inherited::SaveSettings(onlyIfNonDefault);
}


void
TTrackerState::LoadSettingsIfNeeded()
{
	if (fSettingsLoaded)
		return;

	// Set default settings before reading from disk

	Add(fShowDisksIcon = new BooleanValueSetting("ShowDisksIcon", false));
	Add(fMountVolumesOntoDesktop
		= new BooleanValueSetting("MountVolumesOntoDesktop", true));
	Add(fMountSharedVolumesOntoDesktop =
		new BooleanValueSetting("MountSharedVolumesOntoDesktop", true));
	Add(fEjectWhenUnmounting
		= new BooleanValueSetting("EjectWhenUnmounting", true));

	Add(fDesktopFilePanelRoot
		= new BooleanValueSetting("DesktopFilePanelRoot", true));
	Add(fShowFullPathInTitleBar
		= new BooleanValueSetting("ShowFullPathInTitleBar", false));
	Add(fShowSelectionWhenInactive
		= new BooleanValueSetting("ShowSelectionWhenInactive", true));
	Add(fTransparentSelection
		= new BooleanValueSetting("TransparentSelection", true));
	Add(fSortFolderNamesFirst
		= new BooleanValueSetting("SortFolderNamesFirst", true));
	Add(fHideDotFiles = new BooleanValueSetting("HideDotFiles", false));
	Add(fTypeAheadFiltering
		= new BooleanValueSetting("TypeAheadFiltering", false));
	Add(fGenerateImageThumbnails
		= new BooleanValueSetting("GenerateImageThumbnails", false));
	Add(fSingleWindowBrowse
		= new BooleanValueSetting("SingleWindowBrowse", false));
	Add(fShowNavigator = new BooleanValueSetting("ShowNavigator", false));

	Add(fRecentApplicationsCount
		= new ScalarValueSetting("RecentApplications", 10, "", ""));
	Add(fRecentDocumentsCount
		= new ScalarValueSetting("RecentDocuments", 10, "", ""));
	Add(fRecentFoldersCount
		= new ScalarValueSetting("RecentFolders", 10, "", ""));

	Add(fShowVolumeSpaceBar
		= new BooleanValueSetting("ShowVolumeSpaceBar", true));

	Add(fUsedSpaceColor
		= new HexScalarValueSetting("UsedSpaceColor", 0xc000cb00, "", ""));
	Add(fFreeSpaceColor
		= new HexScalarValueSetting("FreeSpaceColor", 0xc0ffffff, "", ""));
	Add(fWarningSpaceColor
		= new HexScalarValueSetting("WarningSpaceColor", 0xc0cb0000, "", ""));

	Add(fDontMoveFilesToTrash
		= new BooleanValueSetting("DontMoveFilesToTrash", false));
	Add(fAskBeforeDeleteFile
		= new BooleanValueSetting("AskBeforeDeleteFile", true));

	TryReadingSettings();

	NameAttributeText::SetSortFolderNamesFirst(
		fSortFolderNamesFirst->Value());
	RealNameAttributeText::SetSortFolderNamesFirst(
		fSortFolderNamesFirst->Value());

	fSettingsLoaded = true;
}


//	#pragma mark - TrackerSettings


TrackerSettings::TrackerSettings()
{
	gTrackerState.LoadSettingsIfNeeded();
}


void
TrackerSettings::SaveSettings(bool onlyIfNonDefault)
{
	gTrackerState.SaveSettings(onlyIfNonDefault);
}


bool
TrackerSettings::ShowDisksIcon()
{
	return gTrackerState.fShowDisksIcon->Value();
}


void
TrackerSettings::SetShowDisksIcon(bool enabled)
{
	gTrackerState.fShowDisksIcon->SetValue(enabled);
}


bool
TrackerSettings::DesktopFilePanelRoot()
{
	return gTrackerState.fDesktopFilePanelRoot->Value();
}


void
TrackerSettings::SetDesktopFilePanelRoot(bool enabled)
{
	gTrackerState.fDesktopFilePanelRoot->SetValue(enabled);
}


bool
TrackerSettings::MountVolumesOntoDesktop()
{
	return gTrackerState.fMountVolumesOntoDesktop->Value();
}


void
TrackerSettings::SetMountVolumesOntoDesktop(bool enabled)
{
	gTrackerState.fMountVolumesOntoDesktop->SetValue(enabled);
}


bool
TrackerSettings::MountSharedVolumesOntoDesktop()
{
	return gTrackerState.fMountSharedVolumesOntoDesktop->Value();
}


void
TrackerSettings::SetMountSharedVolumesOntoDesktop(bool enabled)
{
	gTrackerState.fMountSharedVolumesOntoDesktop->SetValue(enabled);
}


bool
TrackerSettings::EjectWhenUnmounting()
{
	return gTrackerState.fEjectWhenUnmounting->Value();
}


void
TrackerSettings::SetEjectWhenUnmounting(bool enabled)
{
	gTrackerState.fEjectWhenUnmounting->SetValue(enabled);
}


bool
TrackerSettings::ShowVolumeSpaceBar()
{
	return gTrackerState.fShowVolumeSpaceBar->Value();
}


void
TrackerSettings::SetShowVolumeSpaceBar(bool enabled)
{
	gTrackerState.fShowVolumeSpaceBar->SetValue(enabled);
}


rgb_color
TrackerSettings::UsedSpaceColor()
{
	return ValueToColor(gTrackerState.fUsedSpaceColor->Value());
}


void
TrackerSettings::SetUsedSpaceColor(rgb_color color)
{
	gTrackerState.fUsedSpaceColor->ValueChanged(ColorToValue(color));
}


rgb_color
TrackerSettings::FreeSpaceColor()
{
	return ValueToColor(gTrackerState.fFreeSpaceColor->Value());
}


void
TrackerSettings::SetFreeSpaceColor(rgb_color color)
{
	gTrackerState.fFreeSpaceColor->ValueChanged(ColorToValue(color));
}


rgb_color
TrackerSettings::WarningSpaceColor()
{
	return ValueToColor(gTrackerState.fWarningSpaceColor->Value());
}


void
TrackerSettings::SetWarningSpaceColor(rgb_color color)
{
	gTrackerState.fWarningSpaceColor->ValueChanged(ColorToValue(color));
}


bool
TrackerSettings::ShowFullPathInTitleBar()
{
	return gTrackerState.fShowFullPathInTitleBar->Value();
}


void
TrackerSettings::SetShowFullPathInTitleBar(bool enabled)
{
	gTrackerState.fShowFullPathInTitleBar->SetValue(enabled);
}


bool
TrackerSettings::SortFolderNamesFirst()
{
	return gTrackerState.fSortFolderNamesFirst->Value();
}


void
TrackerSettings::SetSortFolderNamesFirst(bool enabled)
{
	gTrackerState.fSortFolderNamesFirst->SetValue(enabled);
	NameAttributeText::SetSortFolderNamesFirst(enabled);
	RealNameAttributeText::SetSortFolderNamesFirst(enabled);
}


bool
TrackerSettings::HideDotFiles()
{
	return gTrackerState.fHideDotFiles->Value();
}


void
TrackerSettings::SetHideDotFiles(bool hide)
{
	gTrackerState.fHideDotFiles->SetValue(hide);
}


bool
TrackerSettings::TypeAheadFiltering()
{
	return gTrackerState.fTypeAheadFiltering->Value();
}


void
TrackerSettings::SetTypeAheadFiltering(bool enabled)
{
	gTrackerState.fTypeAheadFiltering->SetValue(enabled);
}


bool
TrackerSettings::GenerateImageThumbnails()
{
	return gTrackerState.fGenerateImageThumbnails->Value();
}


void
TrackerSettings::SetGenerateImageThumbnails(bool enabled)
{
	gTrackerState.fGenerateImageThumbnails->SetValue(enabled);
}


bool
TrackerSettings::ShowSelectionWhenInactive()
{
	return gTrackerState.fShowSelectionWhenInactive->Value();
}


void
TrackerSettings::SetShowSelectionWhenInactive(bool enabled)
{
	gTrackerState.fShowSelectionWhenInactive->SetValue(enabled);
}


bool
TrackerSettings::TransparentSelection()
{
	return gTrackerState.fTransparentSelection->Value();
}


void
TrackerSettings::SetTransparentSelection(bool enabled)
{
	gTrackerState.fTransparentSelection->SetValue(enabled);
}


bool
TrackerSettings::SingleWindowBrowse()
{
	return gTrackerState.fSingleWindowBrowse->Value();
}


void
TrackerSettings::SetSingleWindowBrowse(bool enabled)
{
	gTrackerState.fSingleWindowBrowse->SetValue(enabled);
}


bool
TrackerSettings::ShowNavigator()
{
	return gTrackerState.fShowNavigator->Value();
}


void
TrackerSettings::SetShowNavigator(bool enabled)
{
	gTrackerState.fShowNavigator->SetValue(enabled);
}


void
TrackerSettings::RecentCounts(int32* applications, int32* documents,
	int32* folders)
{
	if (applications != NULL)
		*applications = gTrackerState.fRecentApplicationsCount->Value();

	if (documents != NULL)
		*documents = gTrackerState.fRecentDocumentsCount->Value();

	if (folders != NULL)
		*folders = gTrackerState.fRecentFoldersCount->Value();
}


void
TrackerSettings::SetRecentApplicationsCount(int32 count)
{
	gTrackerState.fRecentApplicationsCount->ValueChanged(count);
}


void
TrackerSettings::SetRecentDocumentsCount(int32 count)
{
	gTrackerState.fRecentDocumentsCount->ValueChanged(count);
}


void
TrackerSettings::SetRecentFoldersCount(int32 count)
{
	gTrackerState.fRecentFoldersCount->ValueChanged(count);
}


bool
TrackerSettings::DontMoveFilesToTrash()
{
	return gTrackerState.fDontMoveFilesToTrash->Value();
}


void
TrackerSettings::SetDontMoveFilesToTrash(bool enabled)
{
	gTrackerState.fDontMoveFilesToTrash->SetValue(enabled);
}


bool
TrackerSettings::AskBeforeDeleteFile()
{
	return gTrackerState.fAskBeforeDeleteFile->Value();
}


void
TrackerSettings::SetAskBeforeDeleteFile(bool enabled)
{
	gTrackerState.fAskBeforeDeleteFile->SetValue(enabled);
}
