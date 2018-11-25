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
#include "ViewState.h"


enum {
	kForceLargeIcon = 0x1,
	kForceMiniIcon = 0x2,
	kForceShortDescription = 0x4,
	kForceLongDescription = 0x8,
	kForcePreferredApp = 0x10
};


static const char* kAttrName = "META:name";
static const char* kAttrCompany = "META:company";
static const char* kAttrAddress = "META:address";
static const char* kAttrCity = "META:city";
static const char* kAttrState = "META:state";
static const char* kAttrZip = "META:zip";
static const char* kAttrCountry = "META:country";
static const char* kAttrHomePhone = "META:hphone";
static const char* kAttrWorkPhone = "META:wphone";
static const char* kAttrFax = "META:fax";
static const char* kAttrEmail = "META:email";
static const char* kAttrURL = "META:url";
static const char* kAttrGroup = "META:group";
static const char* kAttrNickname = "META:nickname";

static const char* kNetPositiveSignature = "application/x-vnd.Be-NPOS";
static const char* kPeopleSignature = "application/x-vnd.Be-PEPL";

static const BRect kDefaultFrame(40, 40, 695, 350);


struct ColumnData
{
	const char*	title;
	float		offset;
	float		width;
	alignment	align;
	const char*	attributeName;
	uint32		attrType;
	bool		statField;
	bool		editable;
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


//	#pragma mark - ExtraAttributeLazyInstaller


ExtraAttributeLazyInstaller::ExtraAttributeLazyInstaller(const char* type)
	:
	fMimeType(type),
	fDirty(false)
{
	if (fMimeType.InitCheck() != B_OK
		|| fMimeType.GetAttrInfo(&fExtraAttrs) != B_OK) {
		fExtraAttrs = BMessage();
	}
}


ExtraAttributeLazyInstaller::~ExtraAttributeLazyInstaller()
{
	if (fMimeType.InitCheck() == B_OK && fDirty
		&& fMimeType.SetAttrInfo(&fExtraAttrs) != B_OK) {
		fExtraAttrs = BMessage();
	}
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


// #pragma mark - static functions


static void
InstallTemporaryBackgroundImages(BNode* node, BMessage* message)
{
	ssize_t size = message->FlattenedSize();
	try {
		ThrowIfNotSize(size);
		char* buffer = new char[(size_t)size];
		message->Flatten(buffer, size);
		node->WriteAttr(kBackgroundImageInfo, B_MESSAGE_TYPE, 0, buffer,
			(size_t)size);
		delete[] buffer;
	} catch (...) {
		;
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


static size_t
mkColumnsBits(BMallocIO& stream, const ColumnData* src, int32 nelm,
	const char* context)
{
	for (int32 i = 0; i < nelm; i++) {
		BColumn c(
			B_TRANSLATE_CONTEXT(src[i].title, context),
			src[i].offset, src[i].width, src[i].align, src[i].attributeName,
			src[i].attrType, src[i].statField, src[i].editable);
		c.ArchiveToStream(&stream);
	}

	return stream.Position();
}


// #pragma mark - TrackerInitialState


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
	// the following templates are in big endian and we rely on the Tracker
	// translation support to swap them on little endian machines
	//
	// in case there is an attribute (B_RECT_TYPE) that gets swapped by the media
	// (unzip, file system endianness swapping, etc., the correct endianness for
	// the correct machine has to be used here

	static AttributeTemplate sDefaultQueryTemplate[] =
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
			0,
			NULL
		}
	};

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Default Query Columns"

	static const ColumnData defaultQueryColumns[] =
	{
		{ B_TRANSLATE_MARK("Name"), 40, 145, B_ALIGN_LEFT, "_stat/name",
			B_STRING_TYPE, true, true },
		{ B_TRANSLATE_MARK("Path"), 200, 225, B_ALIGN_LEFT, "_trk/path",
			B_STRING_TYPE, false, false },
		{ B_TRANSLATE_MARK("Size"), 440, 41, B_ALIGN_LEFT, "_stat/size",
			B_OFF_T_TYPE, true, false },
		{ B_TRANSLATE_MARK("Modified"), 496, 138, B_ALIGN_LEFT, "_stat/modified",
			B_TIME_TYPE, true, false }
	};


	static AttributeTemplate sBookmarkQueryTemplate[] =
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
			0,
			NULL
		}
	};


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Bookmark Query Columns"


	static const ColumnData bookmarkQueryColumns[] =
	{
		{ B_TRANSLATE_MARK("Title"), 40, 171, B_ALIGN_LEFT, "META:title",
			B_STRING_TYPE, false, true },
		{ B_TRANSLATE_MARK("URL"), 226, 287, B_ALIGN_LEFT, kAttrURL,
			B_STRING_TYPE, false, true },
		{ B_TRANSLATE_MARK("Keywords"), 528, 130, B_ALIGN_LEFT, "META:keyw",
			B_STRING_TYPE, false, true }
	};


	static AttributeTemplate sPersonQueryTemplate[] =
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
			0,
			NULL
		},
	};


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Person Query Columns"


	static const ColumnData personQueryColumns[] =
	{
		{ B_TRANSLATE_MARK("Name"), 40, 115, B_ALIGN_LEFT, "_stat/name",
			B_STRING_TYPE, true, true },
		{ B_TRANSLATE_MARK("Work Phone"), 170, 90, B_ALIGN_LEFT, kAttrWorkPhone,
			B_STRING_TYPE, false, true },
		{ B_TRANSLATE_MARK("E-mail"), 275, 93, B_ALIGN_LEFT, kAttrEmail,
			B_STRING_TYPE, false, true },
		{ B_TRANSLATE_MARK("Company"), 383, 120, B_ALIGN_LEFT, kAttrCompany,
			B_STRING_TYPE, false, true }
	};


	static AttributeTemplate sEmailQueryTemplate[] =
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
			0,
			NULL
		},
	};


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Email Query Columns"


	static const ColumnData emailQueryColumns[] =
	{
		{ B_TRANSLATE_MARK("Subject"), 40, 110, B_ALIGN_LEFT, "MAIL:subject",
			B_STRING_TYPE, false, false },
		{ B_TRANSLATE_MARK("From"), 165, 153, B_ALIGN_LEFT, "MAIL:from",
			B_STRING_TYPE, false, false },
		{ B_TRANSLATE_MARK("When"), 333, 120, B_ALIGN_LEFT, "MAIL:when",
			B_STRING_TYPE, false, false },
		{ B_TRANSLATE_MARK("Status"), 468, 50, B_ALIGN_RIGHT, "MAIL:status",
			B_STRING_TYPE, false, true }
	};


	BNode node;
	BString query(kQueryTemplates);
	query += "/application_octet-stream";

	if (!BContainerWindow::DefaultStateSourceNode(query.String(),
			&node, false)) {
		if (BContainerWindow::DefaultStateSourceNode(query.String(),
				&node, true)) {
			BMallocIO stream;
			size_t n = mkColumnsBits(stream,
					defaultQueryColumns, 4, "Default Query Columns");
			sDefaultQueryTemplate[2].fSize = n;
			sDefaultQueryTemplate[2].fBits = (const char*)stream.Buffer();

			AttributeStreamFileNode fileNode(&node);
			AttributeStreamTemplateNode tmp(sDefaultQueryTemplate, 3);
			fileNode << tmp;
		}
	}

	(query = kQueryTemplates) += "/application_x-vnd.Be-bookmark";
	if (!BContainerWindow::DefaultStateSourceNode(query.String(),
			&node, false)) {
		if (BContainerWindow::DefaultStateSourceNode(query.String(),
				&node, true)) {
			BMallocIO stream;
			size_t n = mkColumnsBits(stream,
				bookmarkQueryColumns, 3, "Bookmark Query Columns");
			sBookmarkQueryTemplate[2].fSize = n;
			sBookmarkQueryTemplate[2].fBits = (const char*)stream.Buffer();

			AttributeStreamFileNode fileNode(&node);
			AttributeStreamTemplateNode tmp(sBookmarkQueryTemplate, 3);
			fileNode << tmp;
		}
	}

	(query = kQueryTemplates) += "/application_x-person";
	if (!BContainerWindow::DefaultStateSourceNode(query.String(),
			&node, false)) {
		if (BContainerWindow::DefaultStateSourceNode(query.String(),
				&node, true)) {
			BMallocIO stream;
			size_t n = mkColumnsBits(stream,
				personQueryColumns, 4, "Person Query Columns");
			sPersonQueryTemplate[2].fSize = n;
			sPersonQueryTemplate[2].fBits = (const char*)stream.Buffer();

			AttributeStreamFileNode fileNode(&node);
			AttributeStreamTemplateNode tmp(sPersonQueryTemplate, 3);
			fileNode << tmp;
		}
	}

	(query = kQueryTemplates) += "/text_x-email";
	if (!BContainerWindow::DefaultStateSourceNode(query.String(),
			&node, false)) {
		if (BContainerWindow::DefaultStateSourceNode(query.String(),
				&node, true)) {
			BMallocIO stream;
			size_t n = mkColumnsBits(stream,
				emailQueryColumns, 4, "Email Query Columns");
			sEmailQueryTemplate[2].fSize = n;
			sEmailQueryTemplate[2].fBits = (const char*)stream.Buffer();

			AttributeStreamFileNode fileNode(&node);
			AttributeStreamTemplateNode tmp(sEmailQueryTemplate, 3);
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
