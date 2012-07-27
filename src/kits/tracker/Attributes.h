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
#ifndef _ATTRIBUTES_H
#define _ATTRIBUTES_H


namespace BPrivate {

// viewable attributes
#define kAttrStatName						"_stat/name"
#define kAttrRealName						"_stat/realname"
#define kAttrStatSize						"_stat/size"
#define kAttrStatModified					"_stat/modified"
#define kAttrStatCreated					"_stat/created"
#define kAttrStatMode						"_stat/mode"
#define kAttrStatOwner						"_stat/owner"
#define kAttrStatGroup						"_stat/group"
#define kAttrPath							"_trk/path"
#define kAttrOriginalPath					"_trk/original_path"
#define kAttrAppVersion						"_trk/app_version"
#define kAttrSystemVersion					"_trk/system_version"
#define kAttrOpenWithRelation				"_trk/open_with_relation"

// private attributes
#define kAttrWindowFrame					"_trk/windframe"
#define kAttrWindowWorkspace				"_trk/windwkspc"
#define kAttrWindowDecor					"_trk/winddecor"

#define kAttrQueryString					"_trk/qrystr"
#define kAttrQueryVolume					"_trk/qryvol1"

#define kAttrMIMEType						"BEOS:TYPE"
#define kAttrAppSignature					"BEOS:APP_SIG"
#define kAttrPreferredApp					"BEOS:PREF_APP"
#define kAttrLargeIcon						"BEOS:L:STD_ICON"
#define kAttrMiniIcon						"BEOS:M:STD_ICON"
#define kAttrIcon							"BEOS:ICON"

#define kAttrDisksFrame						"_trk/d_windframe"
#define kAttrDisksWorkspace					"_trk/d_windwkspc"

#define kAttrOpenWindows					"_trk/_windows_to_open_"

#define kAttrClippingFile					"_trk/_clipping_file_"


#define kAttrQueryInitialMode				"_trk/qryinitmode"
#define kAttrQueryInitialString				"_trk/qryinitstr"
#define kAttrQueryInitialNumAttrs			"_trk/qryinitnumattrs"
#define kAttrQueryInitialAttrs				"_trk/qryinitattrs"
#define kAttrQueryInitialMime				"_trk/qryinitmime"
#define kAttrQueryLastChange				"_trk/qrylastchange"


#define kAttrQueryMoreOptions_le			"_trk/qrymoreoptions_le"
#define kAttrQueryMoreOptions_be			"_trk/qrymoreoptions"

#define kAttrQueryTemplate					"_trk/queryTemplate"
#define kAttrQueryTemplateName				"_trk/queryTemplateName"
#define kAttrDynamicDateQuery				"_trk/queryDynamicDate"
// attributes that need endian swapping (stored as raw)

#define kAttrPoseInfo_be					"_trk/pinfo"
#define kAttrPoseInfo_le					"_trk/pinfo_le"
#define kAttrDisksPoseInfo_be				"_trk/d_pinfo"
#define kAttrDisksPoseInfo_le				"_trk/d_pinfo_le"
#define kAttrTrashPoseInfo_be				"_trk/t_pinfo"
#define kAttrTrashPoseInfo_le				"_trk/t_pinfo_le"
#define kAttrColumns_be						"_trk/columns"
#define kAttrColumns_le						"_trk/columns_le"
#define kAttrViewState_be					"_trk/viewstate"
#define kAttrViewState_le					"_trk/viewstate_le"
#define kAttrDisksViewState_be				"_trk/d_viewstate"
#define kAttrDisksViewState_le				"_trk/d_viewstate_le"
#define kAttrDesktopViewState_be			"_trk/desk_viewstate"
#define kAttrDesktopViewState_le			"_trk/desk_viewstate_le"
#define kAttrDisksColumns_be				"_trk/d_columns"
#define kAttrDisksColumns_le				"_trk/d_columns_le"

#define kAttrExtendedPoseInfo_be			"_trk/xtpinfo"
#define kAttrExtendedPoseInfo_le			"_trk/xtpinfo_le"
#define kAttrExtendedDisksPoseInfo_be		"_trk/xt_d_pinfo"
#define kAttrExtendedDisksPoseInfo_le		"_trk/xt_d_pinfo_le"

#if B_HOST_IS_LENDIAN
#define kEndianSuffix						"_le"
#define kForeignEndianSuffix				""

#define kAttrDisksPoseInfo 					kAttrDisksPoseInfo_le
#define kAttrDisksPoseInfoForeign 			kAttrDisksPoseInfo_be

#define kAttrTrashPoseInfo 					kAttrTrashPoseInfo_le
#define kAttrTrashPoseInfoForeign 			kAttrTrashPoseInfo_be

#define kAttrPoseInfo						kAttrPoseInfo_le
#define kAttrPoseInfoForeign				kAttrPoseInfo_be

#define kAttrColumns						kAttrColumns_le
#define kAttrColumnsForeign					kAttrColumns_be

#define kAttrViewState						kAttrViewState_le
#define kAttrViewStateForeign				kAttrViewState_be

#define kAttrDisksViewState					kAttrDisksViewState_le
#define kAttrDisksViewStateForeign			kAttrDisksViewState_be

#define kAttrDisksColumns					kAttrDisksColumns_le
#define kAttrDisksColumnsForeign			kAttrDisksColumns_be

#define kAttrDesktopViewState				kAttrDesktopViewState_le
#define kAttrDesktopViewStateForeign		kAttrDesktopViewState_be

#define kAttrQueryMoreOptions 				kAttrQueryMoreOptions_le
#define kAttrQueryMoreOptionsForeign 		kAttrQueryMoreOptions_be
#define kAttrExtendedPoseInfo				kAttrExtendedPoseInfo_le
#define kAttrExtendedPoseInfoForegin		kAttrExtendedPoseInfo_be
#define kAttrExtendedDisksPoseInfo			kAttrExtendedDisksPoseInfo_le
#define kAttrExtendedDisksPoseInfoForegin	kAttrExtendedDisksPoseInfo_be

#else
#define kEndianSuffix						""
#define kForeignEndianSuffix				"_le"

#define kAttrDisksPoseInfo 					kAttrDisksPoseInfo_be
#define kAttrDisksPoseInfoForeign	 		kAttrDisksPoseInfo_le

#define kAttrTrashPoseInfo 					kAttrTrashPoseInfo_be
#define kAttrTrashPoseInfoForeign 			kAttrTrashPoseInfo_le

#define kAttrPoseInfo						kAttrPoseInfo_be
#define kAttrPoseInfoForeign				kAttrPoseInfo_le

#define kAttrColumns						kAttrColumns_be
#define kAttrColumnsForeign					kAttrColumns_le

#define kAttrViewState						kAttrViewState_be
#define kAttrViewStateForeign				kAttrViewState_le

#define kAttrDisksViewState					kAttrDisksViewState_be
#define kAttrDisksViewStateForeign			kAttrDisksViewState_le

#define kAttrDisksColumns					kAttrDisksColumns_be
#define kAttrDisksColumnsForeign			kAttrDisksColumns_le

#define kAttrDesktopViewState				kAttrViewState_be
#define kAttrDesktopViewStateForeign		kAttrViewState_le

#define kAttrQueryMoreOptions 				kAttrQueryMoreOptions_be
#define kAttrQueryMoreOptionsForeign 		kAttrQueryMoreOptions_le
#define kAttrExtendedPoseInfo				kAttrExtendedPoseInfo_be
#define kAttrExtendedPoseInfoForegin		kAttrExtendedPoseInfo_le
#define kAttrExtendedDisksPoseInfo			kAttrExtendedDisksPoseInfo_be
#define kAttrExtendedDisksPoseInfoForegin	kAttrExtendedDisksPoseInfo_le

#endif

} // namespace BPrivate

using namespace BPrivate;

#endif
