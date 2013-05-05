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

// ToDo:
// add code to initialize a subset of the mime database, including
// important sniffer rules


#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>
#include <InterfaceDefs.h>
#include <Locale.h>
#include <Message.h>
#include <Node.h>
#include <Path.h>
#include <Screen.h>
#include <VolumeRoster.h>

#include <fs_attr.h>
#include <fs_index.h>

#include "pr_server.h"

#include "Attributes.h"
#include "AttributeStream.h"
#include "BackgroundImage.h"
#include "Bitmaps.h"
#include "ContainerWindow.h"
#include "MimeTypes.h"
#include "FSUtils.h"
#include "QueryContainerWindow.h"
#include "Tracker.h"


enum {
	kForceLargeIcon = 0x1,
	kForceMiniIcon = 0x2,
	kForceShortDescription = 0x4,
	kForceLongDescription = 0x8,
	kForcePreferredApp = 0x10
};


const char* kAttrName = "META:name";
const char* kAttrCompany = "META:company";
const char* kAttrAddress = "META:address";
const char* kAttrCity = "META:city";
const char* kAttrState = "META:state";
const char* kAttrZip = "META:zip";
const char* kAttrCountry = "META:country";
const char* kAttrHomePhone = "META:hphone";
const char* kAttrWorkPhone = "META:wphone";
const char* kAttrFax = "META:fax";
const char* kAttrEmail = "META:email";
const char* kAttrURL = "META:url";
const char* kAttrGroup = "META:group";
const char* kAttrNickname = "META:nickname";

const char* kNetPositiveSignature = "application/x-vnd.Be-NPOS";
const char* kPeopleSignature = "application/x-vnd.Be-PEPL";

// the following templates are in big endian and we rely on the Tracker
// translation support to swap them on little endian machines
//
// in case there is an attribute (B_RECT_TYPE) that gets swapped by the media
// (unzip, file system endianness swapping, etc., the correct endianness for
// the correct machine has to be used here

const BRect kDefaultFrame(40, 40, 695, 350);
const int32 kDefaultQueryTemplateCount = 3;

const AttributeTemplate kDefaultQueryTemplate[] =
	/* /boot/home/config/settings/Tracker/DefaultQueryTemplates/
		application_octet-stream */
{
	{
		// default frame
		kAttrWindowFrame,
		B_RECT_TYPE,
		16,
		(const char*)&kDefaultFrame
	},
	{
		// attr: _trk/viewstate
		kAttrViewState_be,
		B_RAW_TYPE,
		49,
		"o^\365R\000\000\000\012Tlst\000\000\000\000\000\000\000\000\000\000"
		"\000\000\000\000\000\000\000\000\000\000\357\323\335RCSTR\000\000"
		"\000\000\000\000\000\000\000"
	},
	{
		// attr: _trk/columns
		kAttrColumns_be,
		B_RAW_TYPE,
		223,
		"O\362VR\000\000\000\025\000\000\000\004Name\000B \000\000C\021\000"
		"\000\000\000\000\000\000\000\000\012_stat/name\000\357\323\335RCST"
		"R\001\001O\362VR\000\000\000\025\000\000\000\004Path\000CH\000\000"
		"Ca\000\000\000\000\000\000\000\000\000\011_trk/path\000\357_\174RC"
		"STR\000\000O\362VR\000\000\000\025\000\000\000\004Size\000C\334\000"
		"\000B$\000\000\000\000\000\001\000\000\000\012_stat/size\000\317\317"
		"\306TOFFT\001\000O\362VR\000\000\000\025\000\000\000\010Modified\000"
		"C\370\000\000C\012\000\000\000\000\000\000\000\000\000\016_stat/mo"
		"dified\000]KmETIME\001\000"
	}
};

const AttributeTemplate kBookmarkQueryTemplate[] =
	/* /boot/home/config/settings/Tracker/DefaultQueryTemplates/
		application_x-vnd.Be-bookmark */
{
	{
		// default frame
		kAttrWindowFrame,
		B_RECT_TYPE,
		16,
		(const char*)&kDefaultFrame
	},
	{
		// attr: _trk/viewstate
		kAttrViewState_be,
		B_RAW_TYPE,
		49,
		"o^\365R\000\000\000\012Tlst\000\000\000\000\000\000\000\000\000\000"
		"\000\000\000\000\000\000\000\000\000\000w\373\175RCSTR\000\000\000"
		"\000\000\000\000\000\000"
	},
	{
		// attr: _trk/columns
		kAttrColumns_be,
		B_RAW_TYPE,
		163,
		"O\362VR\000\000\000\025\000\000\000\005Title\000B \000\000C+\000\000"
		"\000\000\000\000\000\000\000\012META:title\000w\373\175RCSTR\000\001"
		"O\362VR\000\000\000\025\000\000\000\003URL\000Cb\000\000C\217\200"
		"\000\000\000\000\000\000\000\000\010META:url\000\343[TRCSTR\000\001O"
		"\362VR\000\000\000\025\000\000\000\010Keywords\000D\004\000\000C\002"
		"\000\000\000\000\000\000\000\000\000\011META:keyw\000\333\363\334"
		"RCSTR\000\001"
	}
};

const AttributeTemplate kPersonQueryTemplate[] =
	/* /boot/home/config/settings/Tracker/DefaultQueryTemplates/
		application_x-vnd.Be-bookmark */
{
	{
		// default frame
		kAttrWindowFrame,
		B_RECT_TYPE,
		16,
		(const char*)&kDefaultFrame
	},
	{
		// attr: _trk/viewstate
		kAttrViewState_be,
		B_RAW_TYPE,
		49,
		"o^\365R\000\000\000\012Tlst\000\000\000\000\000\000\000\000\000\000"
		"\000\000\000\000\000\000\000\000\000\000\357\323\335RCSTR\000\000"
		"\000\000\000\000\000\000\000"
	},
	{
		// attr: _trk/columns
		kAttrColumns_be,
		B_RAW_TYPE,
		230,
		"O\362VR\000\000\000\025\000\000\000\004Name\000B \000\000B\346\000"
		"\000\000\000\000\000\000\000\000\012_stat/name\000\357\323\335RCST"
		"R\001\001O\362VR\000\000\000\025\000\000\000\012Work Phone\000C*\000"
		"\000B\264\000\000\000\000\000\000\000\000\000\013META:wphone\000C_"
		"uRCSTR\000\001O\362VR\000\000\000\025\000\000\000\006E-mail\000C\211"
		"\200\000B\272\000\000\000\000\000\000\000\000\000\012META:email\000"
		"sW\337RCSTR\000\001O\362VR\000\000\000\025\000\000\000\007Company"
		"\000C\277\200\000B\360\000\000\000\000\000\000\000\000\000\014"
		"META:company\000CS\174RCSTR\000\001"
	},
};

const AttributeTemplate kEmailQueryTemplate[] =
	/* /boot/home/config/settings/Tracker/DefaultQueryTemplates/
		text_x-email */
{
	{
		// default frame
		kAttrWindowFrame,
		B_RECT_TYPE,
		16,
		(const char*)&kDefaultFrame
	},
	{
		// attr: _trk/viewstate
		kAttrViewState_be,
		B_RAW_TYPE,
		49,
		"o^\365R\000\000\000\012Tlst\000\000\000\000\000\000\000\000\000\000"
		"\000\000\000\000\000\000\000\000\000\000\366_\377ETIME\000\000\000"
		"\000\000\000\000\000\000"
	},
	{
		// attr: _trk/columns
		kAttrColumns_be,
		B_RAW_TYPE,
		222,
		"O\362VR\000\000\000\025\000\000\000\007Subject\000B \000\000B\334"
		"\000\000\000\000\000\000\000\000\000\014MAIL:subject\000\343\173\337"
		"RCSTR\000\000O\362VR\000\000\000\025\000\000\000\004From\000C%\000"
		"\000C\031\000\000\000\000\000\000\000\000\000\011MAIL:from\000\317"
		"s_RCSTR\000\000O\362VR\000\000\000\025\000\000\000\004When\000C\246"
		"\200\000B\360\000\000\000\000\000\000\000\000\000\011MAIL:when\000"
		"\366_\377ETIME\000\000O\362VR\000\000\000\025\000\000\000\006Status"
		"\000C\352\000\000BH\000\000\000\000\000\001\000\000\000\013"
		"MAIL:status\000G\363\134RCSTR\000\001"
	},
};


namespace BPrivate {

class ExtraAttributeLazyInstaller {
public:
	ExtraAttributeLazyInstaller(const char* type);
	~ExtraAttributeLazyInstaller();

	bool AddExtraAttribute(const char* publicName, const char* name,
		uint32 type, bool viewable, bool editable, float width,
		int32 alignment, bool extra);

	status_t InitCheck() const;
public:
	BMimeType fMimeType;
	BMessage fExtraAttrs;
	bool fDirty;
};

}	// namespace BPrivate


ExtraAttributeLazyInstaller::ExtraAttributeLazyInstaller(const char* type)
	:
	fMimeType(type),
	fDirty(false)
{
	if (fMimeType.InitCheck() == B_OK)
		fMimeType.GetAttrInfo(&fExtraAttrs);
}


ExtraAttributeLazyInstaller::~ExtraAttributeLazyInstaller()
{
	if (fMimeType.InitCheck() == B_OK && fDirty)
		fMimeType.SetAttrInfo(&fExtraAttrs);
}


bool
ExtraAttributeLazyInstaller::AddExtraAttribute(const char* publicName,
	const char* name, uint32 type, bool viewable, bool editable, float width,
	int32 alignment, bool extra)
{
	for (int32 index = 0; ; index++) {
		const char* oldPublicName;
		if (fExtraAttrs.FindString("attr:public_name", index, &oldPublicName)
				!= B_OK) {
			break;
		}

		if (strcmp(oldPublicName, publicName) == 0)
			// already got this extra atribute, no work left
			return false;
	}

	fExtraAttrs.AddString("attr:public_name", publicName);
	fExtraAttrs.AddString("attr:name", name);
	fExtraAttrs.AddInt32("attr:type", (int32)type);
	fExtraAttrs.AddBool("attr:viewable", viewable);
	fExtraAttrs.AddBool("attr:editable", editable);
	fExtraAttrs.AddInt32("attr:width", (int32)width);
	fExtraAttrs.AddInt32("attr:alignment", alignment);
	fExtraAttrs.AddBool("attr:extra", extra);

	fDirty = true;
	return true;
}


// #pragma mark -


static void
InstallTemporaryBackgroundImages(BNode* node, BMessage* message)
{
	int32 size = message->FlattenedSize();

	try {
		char* buffer = new char[size];
		message->Flatten(buffer, size);
		node->WriteAttr(kBackgroundImageInfo, B_MESSAGE_TYPE, 0, buffer,
			(size_t)size);
		delete[] buffer;
	} catch (...) {
	}
}


static void
AddTemporaryBackgroundImages(BMessage* message, const char* imagePath,
	BackgroundImage::Mode mode, BPoint offset, uint32 workspaces,
	bool textWidgetOutlines)
{
	message->AddString(kBackgroundImageInfoPath, imagePath);
	message->AddInt32(kBackgroundImageInfoWorkspaces, (int32)workspaces);
	message->AddInt32(kBackgroundImageInfoMode, mode);
	message->AddBool(kBackgroundImageInfoTextOutline, textWidgetOutlines);
	message->AddPoint(kBackgroundImageInfoOffset, offset);
}


// #pragma mark -


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TrackerInitialState"

bool
TTracker::InstallMimeIfNeeded(const char* type, int32 bitsID,
	const char* shortDescription, const char* longDescription,
	const char* preferredAppSignature, uint32 forceMask)
{
	// used by InitMimeTypes - checks if a metamime of a given <type> is
	// installed and if it has all the specified attributes; if not, the
	// whole mime type is installed and all attributes are set; nulls can
	// be passed for attributes that don't matter; returns true if anything
	// had to be changed

	BBitmap vectorIcon(BRect(0, 0, 31, 31), B_BITMAP_NO_SERVER_LINK,
		B_RGBA32);
	BBitmap largeIcon(BRect(0, 0, 31, 31), B_BITMAP_NO_SERVER_LINK, B_CMAP8);
	BBitmap miniIcon(BRect(0, 0, 15, 15), B_BITMAP_NO_SERVER_LINK, B_CMAP8);
	char tmp[B_MIME_TYPE_LENGTH];

	BMimeType mime(type);
	bool installed = mime.IsInstalled();

	if (!installed
		|| (bitsID >= 0 && ((forceMask & kForceLargeIcon)
			|| mime.GetIcon(&vectorIcon, B_LARGE_ICON) != B_OK))
		|| (bitsID >= 0 && ((forceMask & kForceLargeIcon)
			|| mime.GetIcon(&largeIcon, B_LARGE_ICON) != B_OK))
		|| (bitsID >= 0 && ((forceMask & kForceMiniIcon)
			|| mime.GetIcon(&miniIcon, B_MINI_ICON) != B_OK))
		|| (shortDescription && ((forceMask & kForceShortDescription)
			|| mime.GetShortDescription(tmp) != B_OK))
		|| (longDescription && ((forceMask & kForceLongDescription)
			|| mime.GetLongDescription(tmp) != B_OK))
		|| (preferredAppSignature && ((forceMask & kForcePreferredApp)
			|| mime.GetPreferredApp(tmp) != B_OK))) {

		if (!installed)
			mime.Install();

		if (bitsID >= 0) {
			const uint8* iconData;
			size_t iconSize;
			if (GetTrackerResources()->
					GetIconResource(bitsID, &iconData, &iconSize) == B_OK)
				mime.SetIcon(iconData, iconSize);

			if (GetTrackerResources()->
					GetIconResource(bitsID, B_LARGE_ICON, &largeIcon) == B_OK)
				mime.SetIcon(&largeIcon, B_LARGE_ICON);

			if (GetTrackerResources()->
					GetIconResource(bitsID, B_MINI_ICON, &miniIcon) == B_OK)
				mime.SetIcon(&miniIcon, B_MINI_ICON);
		}

		if (shortDescription)
			mime.SetShortDescription(shortDescription);

		if (longDescription)
			mime.SetLongDescription(longDescription);

		if (preferredAppSignature)
			mime.SetPreferredApp(preferredAppSignature);

		return true;
	}
	return false;
}


void
TTracker::InitMimeTypes()
{
	InstallMimeIfNeeded(B_APP_MIME_TYPE, R_AppIcon, "Be Application",
		"Generic Be application executable.", kTrackerSignature);

	InstallMimeIfNeeded(B_FILE_MIMETYPE, R_FileIcon,
		"Generic file", "Generic document file.", kTrackerSignature);

	InstallMimeIfNeeded(B_VOLUME_MIMETYPE, R_HardDiskIcon,
		"Be Volume", "Disk volume.", kTrackerSignature);

	InstallMimeIfNeeded(B_QUERY_MIMETYPE, R_QueryDirIcon,
		"Be Query", "Query to locate items on disks.", kTrackerSignature);

	InstallMimeIfNeeded(B_QUERY_TEMPLATE_MIMETYPE, R_QueryTemplateIcon,
		"Be Query template", "", kTrackerSignature);

	InstallMimeIfNeeded(B_LINK_MIMETYPE, R_BrokenLinkIcon, "Symbolic link",
		"Link to another item in the file system.", kTrackerSignature);

	InstallMimeIfNeeded(B_ROOT_MIMETYPE, R_RootIcon,
		"Be Root", "File system root.", kTrackerSignature);

	InstallMimeIfNeeded(B_BOOKMARK_MIMETYPE, R_BookmarkIcon,
		"Bookmark", "Bookmark for a web page.", kNetPositiveSignature);

	{
		// install a couple of extra fields for bookmark

		ExtraAttributeLazyInstaller installer(B_BOOKMARK_MIMETYPE);
		installer.AddExtraAttribute("URL", "META:url", B_STRING_TYPE,
			true, true, 170, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Keywords", "META:keyw", B_STRING_TYPE,
			true, true, 130, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Title", "META:title", B_STRING_TYPE,
			true, true, 130, B_ALIGN_LEFT, false);
	}

	InstallMimeIfNeeded(B_PERSON_MIMETYPE, R_PersonIcon,
		"Person", "Contact information for a person.", kPeopleSignature);

	{
		ExtraAttributeLazyInstaller installer(B_PERSON_MIMETYPE);
		installer.AddExtraAttribute("Contact name", kAttrName, B_STRING_TYPE,
			true, true, 120, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Company", kAttrCompany, B_STRING_TYPE,
			true, true, 120, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Address", kAttrAddress, B_STRING_TYPE,
			true, true, 120, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("City", kAttrCity, B_STRING_TYPE,
			true, true, 90, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("State", kAttrState, B_STRING_TYPE,
			true, true, 50, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Zip", kAttrZip, B_STRING_TYPE,
			true, true, 50, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Country", kAttrCountry, B_STRING_TYPE,
			true, true, 120, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("E-mail", kAttrEmail, B_STRING_TYPE,
			true, true, 120, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Home phone", kAttrHomePhone,
			B_STRING_TYPE, true, true, 90, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Work phone", kAttrWorkPhone,
			B_STRING_TYPE, true, true, 90, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Fax", kAttrFax, B_STRING_TYPE,
			true, true, 90, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("URL", kAttrURL, B_STRING_TYPE,
			true, true, 120, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Group", kAttrGroup, B_STRING_TYPE,
			true, true, 120, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Nickname", kAttrNickname, B_STRING_TYPE,
			true, true, 120, B_ALIGN_LEFT, false);
	}

	InstallMimeIfNeeded(B_PRINTER_SPOOL_MIMETYPE, R_SpoolFileIcon,
		"Printer spool", "Printer spool file.", "application/x-vnd.Be-PRNT");

	{
#if B_BEOS_VERSION_DANO
		ExtraAttributeLazyInstaller installer(B_PRINTER_SPOOL_MIMETYPE);
		installer.AddExtraAttribute("Status", PSRV_SPOOL_ATTR_STATUS,
			B_STRING_TYPE, true, false, 60, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Page count", PSRV_SPOOL_ATTR_PAGECOUNT,
			B_INT32_TYPE, true, false, 40, B_ALIGN_RIGHT, false);
		installer.AddExtraAttribute("Description",
			PSRV_SPOOL_ATTR_DESCRIPTION, B_STRING_TYPE, true, true, 100,
			B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Printer name", PSRV_SPOOL_ATTR_PRINTER,
			B_STRING_TYPE, true, false, 80, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Job creator type",
			PSRV_SPOOL_ATTR_MIMETYPE, B_ASCII_TYPE, true, false, 60,
			B_ALIGN_LEFT, false);
#else
		ExtraAttributeLazyInstaller installer(B_PRINTER_SPOOL_MIMETYPE);
		installer.AddExtraAttribute("Page count", "_spool/Page Count",
			B_INT32_TYPE, true, false, 40, B_ALIGN_RIGHT, false);
		installer.AddExtraAttribute("Description", "_spool/Description",
			B_ASCII_TYPE, true, true, 100, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Printer name", "_spool/Printer",
			B_ASCII_TYPE, true, false, 80, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Job creator type", "_spool/MimeType",
			B_ASCII_TYPE, true, false, 60, B_ALIGN_LEFT, false);
#endif
	}

	InstallMimeIfNeeded(B_PRINTER_MIMETYPE, R_GenericPrinterIcon,
		"Printer", "Printer queue.", kTrackerSignature);
		// application/x-vnd.Be-PRNT
		// for now set tracker as a default handler for the printer because we
		// just want to open it as a folder
#if B_BEOS_VERSION_DANO
	{
		ExtraAttributeLazyInstaller installer(B_PRINTER_MIMETYPE);
		installer.AddExtraAttribute("Driver", PSRV_PRINTER_ATTR_DRV_NAME,
			B_STRING_TYPE, true, false, 120, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Transport",
			PSRV_PRINTER_ATTR_TRANSPORT, B_STRING_TYPE, true, false,
			60, B_ALIGN_RIGHT, false);
		installer.AddExtraAttribute("Connection",
			PSRV_PRINTER_ATTR_CNX, B_STRING_TYPE, true, false,
			40, B_ALIGN_LEFT, false);
		installer.AddExtraAttribute("Description",
			PSRV_PRINTER_ATTR_COMMENTS, B_STRING_TYPE, true, true,
			140, B_ALIGN_LEFT, false);
	}
#endif
}


void
TTracker::InstallIndices()
{
	BVolumeRoster roster;
	BVolume volume;

	roster.Rewind();
	while (roster.GetNextVolume(&volume) == B_OK) {
		if (volume.IsReadOnly() || !volume.IsPersistent()
			|| !volume.KnowsAttr() || !volume.KnowsQuery())
			continue;
		InstallIndices(volume.Device());
	}
}


void
TTracker::InstallIndices(dev_t device)
{
	fs_create_index(device, kAttrQueryLastChange, B_INT32_TYPE, 0);
	fs_create_index(device, "_trk/recentQuery", B_INT32_TYPE, 0);
}


void
TTracker::InstallDefaultTemplates()
{
	BNode node;
	BString query(kQueryTemplates);
	query += "/application_octet-stream";

	if (!BContainerWindow::DefaultStateSourceNode(query.String(),
			&node, false)) {
		if (BContainerWindow::DefaultStateSourceNode(query.String(),
				&node, true)) {
			AttributeStreamFileNode fileNode(&node);
			AttributeStreamTemplateNode tmp(kDefaultQueryTemplate, 3);
			fileNode << tmp;
		}
	}

	(query = kQueryTemplates) += "/application_x-vnd.Be-bookmark";
	if (!BContainerWindow::DefaultStateSourceNode(query.String(),
			&node, false)) {
		if (BContainerWindow::DefaultStateSourceNode(query.String(),
				&node, true)) {
			AttributeStreamFileNode fileNode(&node);
			AttributeStreamTemplateNode tmp(kBookmarkQueryTemplate, 3);
			fileNode << tmp;
		}
	}

	(query = kQueryTemplates) += "/application_x-person";
	if (!BContainerWindow::DefaultStateSourceNode(query.String(),
			&node, false)) {
		if (BContainerWindow::DefaultStateSourceNode(query.String(),
				&node, true)) {
			AttributeStreamFileNode fileNode(&node);
			AttributeStreamTemplateNode tmp(kPersonQueryTemplate, 3);
			fileNode << tmp;
		}
	}

	(query = kQueryTemplates) += "/text_x-email";
	if (!BContainerWindow::DefaultStateSourceNode(query.String(),
			&node, false)) {
		if (BContainerWindow::DefaultStateSourceNode(query.String(),
				&node, true)) {
			AttributeStreamFileNode fileNode(&node);
			AttributeStreamTemplateNode tmp(kEmailQueryTemplate, 3);
			fileNode << tmp;
		}
	}
}


void
TTracker::InstallTemporaryBackgroundImages()
{
	// make the large Haiku Logo the default background

	BPath path;
	status_t status = find_directory(B_SYSTEM_DATA_DIRECTORY, &path);
	if (status < B_OK) {
		// TODO: this error shouldn't be shown to the regular user
		BString errorMessage(B_TRANSLATE("At %func \nfind_directory() "
			"failed. \nReason: %error"));
		errorMessage.ReplaceFirst("%func", __PRETTY_FUNCTION__);
		errorMessage.ReplaceFirst("%error", strerror(status));
		BAlert* alert = new BAlert("AlertError", errorMessage.String(),
			B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		return;
	}
	path.Append("artwork");

	BString defaultBackgroundImage("/HAIKU logo - white on blue - big.png");

	BDirectory dir;
	if (FSGetBootDeskDir(&dir) == B_OK) {
		// install a default background if there is no background defined yet
		attr_info info;
		if (dir.GetAttrInfo(kBackgroundImageInfo, &info) != B_OK) {
			BScreen screen(B_MAIN_SCREEN_ID);
			BPoint logoPos;
			logoPos.x
				= floorf((screen.Frame().Width() - 605) * (sqrtf(5) - 1) / 2);
			logoPos.y = floorf((screen.Frame().Height() - 190) * 0.9);
			BMessage message;
			AddTemporaryBackgroundImages(&message,
				(BString(path.Path()) << defaultBackgroundImage).String(),
				BackgroundImage::kAtOffset, logoPos, 0xffffffff, false);
			::InstallTemporaryBackgroundImages(&dir, &message);
		}
	}
}
