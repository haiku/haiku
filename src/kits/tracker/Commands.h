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
#ifndef _COMMANDS_H
#define _COMMANDS_H


#include "PublicCommands.h"

#include <MountServer.h>

namespace BPrivate {

// external app messages

const uint32 kGetInfo = 'Tinf';
const uint32 kMoveToTrash = 'Ttrs';
const uint32 kDelete = 'Tdel';
const uint32 kRestoreFromTrash = 'Tres';
const uint32 kIdentifyEntry = 'Tidt';
const uint32 kOpenSelection = 'Tosl';
const uint32 kOpenSelectionWith = 'Tosu';
const uint32 kCloseAllWindows = 'Tall';
const uint32 kCloseWindowAndChildren = 'Tcwc';
const uint32 kCloseAllInWorkspace = 'Tciw';

// end external app messages

const uint32 kOpenPreviouslyOpenWindows = 'Topw';
const uint32 kRestoreState = 'Trst';

const uint32 kCutMoreSelectionToClipboard = 'Tmvm';
const uint32 kCopyMoreSelectionToClipboard = 'Tcpm';
const uint32 kPasteLinksFromClipboard = 'Tplc';
const uint32 kCancelSelectionToClipboard = 'Tesc';
const uint32 kClipboardPosesChanged = 'Tcpc';

const uint32 kEditItem = 'Tedt';
const uint32 kEditQuery = 'Qedt';
const uint32 kNewFolder = 'Tnwf';
const uint32 kNewEntryFromTemplate = 'Tnwe';
const uint32 kCopySelectionTo = 'Tcsl';
const uint32 kMoveSelectionTo = 'Tmsl';
const uint32 kCreateLink = 'Tlnk';
const uint32 kCreateRelativeLink = 'Trln';
const uint32 kDuplicateSelection = 'Tdsl';
const uint32 kLoadAddOn = 'Tlda';
const uint32 kEmptyTrash = 'Tetr';
const uint32 kAddPrinter = 'Tadp';
const uint32 kMakeActivePrinter = 'Tmap';
const uint32 kRestartDeskbar = 'DBar';

const uint32 kRunAutomounterSettings = 'Tram';

const uint32 kOpenParentDir = 'Topt';
const uint32 kOpenDir = 'Topd';
const uint32 kCleanup = 'Tcln';
const uint32 kCleanupAll = 'Tcla';

const uint32 kArrangeBy = 'ARBY';
const uint32 kArrangeReverseOrder = 'ARRO';

const uint32 kResizeToFit = 'Trtf';
const uint32 kSelectMatchingEntries = 'Tsme';
const uint32 kShowSelectionWindow = 'Tssw';
const uint32 kShowSettingsWindow = 'Tstw';
const uint32 kInvertSelection = 'Tisl';

const uint32 kCancelButton = 'Tcnl';
const uint32 kDefaultButton = 'Tact';
const uint32 kPauseButton = 'Tpaw';
const uint32 kStopButton = 'Tstp';
const uint32 kCopyAttributes = 'Tcat';
const uint32 kPasteAttributes = 'Tpat';
const uint32 kAttributeItem = 'Tatr';
const uint32 kMIMETypeItem = 'Tmim';
const uint32 kAddCurrentDir = 'Tadd';
const uint32 kEditFavorites	= 'Tedf';
const uint32 kSwitchDirectory = 'Tswd';
const uint32 kQuitTracker = 'Tqit';

const uint32 kSwitchToHome = 'Tswh';

const uint32 kTestIconCache = 'TicC';

// Observers and Notifiers:

// Settings-changed messages:
const uint32 kDisksIconChanged = 'Dicn';
const uint32 kDesktopIntegrationChanged = 'Dint';
const uint32 kShowDisksIconChanged = 'Sdic';
const uint32 kVolumesOnDesktopChanged = 'Codc';
const uint32 kEjectWhenUnmountingChanged = 'Ewum';

const uint32 kWindowsShowFullPathChanged = 'Wsfp';
const uint32 kSingleWindowBrowseChanged = 'Osmw';
const uint32 kShowNavigatorChanged = 'Snvc';
const uint32 kShowSelectionWhenInactiveChanged = 'Sswi';
const uint32 kTransparentSelectionChanged = 'Trse';
const uint32 kSortFolderNamesFirstChanged = 'Sfnf';
const uint32 kTypeAheadFilteringChanged = 'Tafc';

const uint32 kDesktopFilePanelRootChanged = 'Dfpr';
const uint32 kFavoriteCountChanged = 'Fvct';
const uint32 kFavoriteCountChangedExternally = 'Fvcx';

const uint32 kDateFormatChanged = 'Date';

const uint32 kUpdateVolumeSpaceBar = 'UpSB';
const uint32 kShowVolumeSpaceBar = 'ShSB';
const uint32 kSpaceBarColorChanged = 'SBcc';

const uint32 kDontMoveFilesToTrashChanged = 'STdm';
const uint32 kAskBeforeDeleteFileChanged = 'STad';

} // namespace BPrivate

using namespace BPrivate;

#endif
