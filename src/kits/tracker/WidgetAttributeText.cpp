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

#include <fs_attr.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <parsedate.h>

#include <Alert.h>
#include <AppFileInfo.h>
#include <Debug.h>
#include <NodeInfo.h>
#include <Path.h>
#include <TextView.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "Attributes.h"
#include "FindPanel.h"
#include "FSUndoRedo.h"
#include "FSUtils.h"
#include "Model.h"
#include "OpenWithWindow.h"
#include "MimeTypes.h"
#include "PoseView.h"
#include "SettingsViews.h"
#include "Utilities.h"
#include "ViewState.h"
#include "WidgetAttributeText.h"


template <class View>
float
TruncStringBase(BString *result, const char *str, int32 length,
	const View *view, float width, uint32 truncMode = B_TRUNCATE_MIDDLE)
{
	// we are using a template version of this call to make sure
	// the right StringWidth gets picked up for BView x BPoseView
	// for max speed and flexibility

	// a standard ellipsis inserting fitting algorithm
	if (view->StringWidth(str, length) <= width)
		*result = str;
	else {
		const char *srcstr[1];
		char *results[1];

		srcstr[0] = str;
		results[0] = result->LockBuffer(length + 3);

		BFont font;
		view->GetFont(&font);
	
	    font.GetTruncatedStrings(srcstr, 1, truncMode, width, results);
		result->UnlockBuffer();
	}
	return view->StringWidth(result->String(), result->Length());
}


WidgetAttributeText *
WidgetAttributeText::NewWidgetText(const Model *model,
	const BColumn *column, const BPoseView *view)
{
	// call this to make the right WidgetAttributeText type for a
	// given column

	const char *attrName = column->AttrName();

	if (strcmp(attrName, kAttrPath) == 0)
		return new PathAttributeText(model, column);
	else if (strcmp(attrName, kAttrMIMEType) == 0)
		return new KindAttributeText(model, column);
	else if (strcmp(attrName, kAttrStatName) == 0)
		return new NameAttributeText(model, column);
	else if (strcmp(attrName, kAttrStatSize) == 0)
		return new SizeAttributeText(model, column);
	else if (strcmp(attrName, kAttrStatModified) == 0)
		return new ModificationTimeAttributeText(model, column);
	else if (strcmp(attrName, kAttrStatCreated) == 0)
		return new CreationTimeAttributeText(model, column);
#ifdef OWNER_GROUP_ATTRIBUTES
	else if (strcmp(attrName, kAttrStatOwner) == 0)
		return new OwnerAttributeText(model, column);
	else if (strcmp(attrName, kAttrStatGroup) == 0)
		return new GroupAttributeText(model, column);
#endif
	else if (strcmp(attrName, kAttrStatMode) == 0)
		return new ModeAttributeText(model, column);
	else if (strcmp(attrName, kAttrOpenWithRelation) == 0)
		return new OpenWithRelationAttributeText(model, column, view);
	else if (strcmp(attrName, kAttrAppVersion) == 0)
		return new AppShortVersionAttributeText(model, column);
	else if (strcmp(attrName, kAttrSystemVersion) == 0)
		return new SystemShortVersionAttributeText(model, column);
	else if (strcmp(attrName, kAttrOriginalPath) == 0)
		return new OriginalPathAttributeText(model, column);

	return new GenericAttributeText(model, column);		
}


WidgetAttributeText::WidgetAttributeText(const Model *model, const BColumn *column)
	:
	fModel(const_cast<Model *>(model)),
	fColumn(column),
	fDirty(true),
	fValueIsDefined(false)
{
	ASSERT(fColumn);
	ASSERT(fColumn->Width() > 0);
}


WidgetAttributeText::~WidgetAttributeText()
{
}


const char *
WidgetAttributeText::FittingText(const BPoseView *view)
{
	if (fDirty || fColumn->Width() != fOldWidth || !fValueIsDefined)
		CheckViewChanged(view);

	ASSERT(!fDirty);
	return fText.String();
}


bool
WidgetAttributeText::CheckViewChanged(const BPoseView *view)
{
	BString newText;
	FitValue(&newText, view);	

	if (newText == fText)
		return false;

	fText = newText;
	return true;
}


float
WidgetAttributeText::TruncString(BString *result, const char *str,
	int32 length, const BPoseView *view, float width, uint32 truncMode)
{
	return TruncStringBase(result, str, length, view, width, truncMode);
}


const char *kSizeFormats[] = {
	"%.2f %s",
	"%.1f %s",
	"%.f %s",
	"%.f%s",
	0
};


template <class View>
float
TruncFileSizeBase(BString *result, int64 value, const View *view, float width)
{
	// ToDo:
	// if slow, replace float divisions with shifts
	// if fast enough, try fitting more decimal places

	// format file size value
	char buffer[1024];
	if (value == kUnknownSize) {
		*result = "-";
		return view->StringWidth("-");
	} else if (value < kKBSize) {
		sprintf(buffer, "%Ld bytes", value);
		if (view->StringWidth(buffer) > width)
			sprintf(buffer, "%Ld B", value);
	} else {
		const char *suffix;
		float floatValue;
		if (value >= kTBSize) {
			suffix = "TB";
			floatValue = (float)value / kTBSize;
		} else if (value >= kGBSize) {
			suffix = "GB";
			floatValue = (float)value / kGBSize;
		} else if (value >= kMBSize) {
			suffix = "MB";
			floatValue = (float)value / kMBSize;
		} else {
			ASSERT(value >= kKBSize);
			suffix = "KB";
			floatValue = (float)value / kKBSize;
		}

		for (int32 index = 0; ; index++) {
			if (!kSizeFormats[index])
				break;

			sprintf(buffer, kSizeFormats[index], floatValue, suffix);

			// strip off an insignificant zero so we don't get readings
			// such as 1.00
			char *period = 0;
			for (char *tmp = buffer; *tmp; tmp++) {
				if (*tmp == '.')
					period = tmp;
			}
			if (period && period[1] && period[2] == '0')
				// move the rest of the string over the insignificant zero
				for (char *tmp = &period[2]; *tmp; tmp++)
					*tmp = tmp[1];

			float resultWidth = view->StringWidth(buffer);
			if (resultWidth <= width) {
				*result = buffer;
				return resultWidth;
			}
		}
	}

	return TruncStringBase(result, buffer, (ssize_t)strlen(buffer), view, width,
		(uint32)B_TRUNCATE_END);
}


float
WidgetAttributeText::TruncFileSize(BString *result, int64 value,
	const BPoseView *view, float width)
{
	return TruncFileSizeBase(result, value, view, width);
}

/*
const char *kTimeFormats[] = {
	"%A, %B %d %Y, %I:%M:%S %p",	// Monday, July 09 1997, 05:08:15 PM
	"%a, %b %d %Y, %I:%M:%S %p",	// Mon, Jul 09 1997, 05:08:15 PM
	"%a, %b %d %Y, %I:%M %p",		// Mon, Jul 09 1997, 05:08 PM
	"%b %d %Y, %I:%M %p",			// Jul 09 1997, 05:08 PM
	"%m/%d/%y, %I:%M %p",			// 07/09/97, 05:08 PM
	"%m/%d/%y",						// 07/09/97
	NULL
};
*/

status_t
TimeFormat(BString &string, int32 index, FormatSeparator separator,
	DateOrder order, bool clockIs24Hour)
{
	if (index >= 6)
		return B_ERROR;

	BString clockString;
	BString dateString;
	
	if (index <= 1)
		if (clockIs24Hour)
			clockString = "%H:%M:%S";
		else
			clockString = "%I:%M:%S %p";
	else
		if (clockIs24Hour)
			clockString = "%H:%M";
		else
			clockString = "%I:%M %p";

	if (index <= 3) {
		switch (order) {
			case kYMDFormat:
				dateString = "%Y %! %d";
				break;
			case kDMYFormat:
				dateString = "%d %! %Y";
				break;
			case kMDYFormat:
				// Fall through
			case kDateFormatEnd:
				// Fall through
			default:
				dateString = "%! %d %Y";
				break;
		}
		if (index == 0)
			dateString.Replace('!', 'B', 1);
		else
			dateString.Replace('!', 'b', 1);	
	} else {
		switch (order) {
			case kYMDFormat:
				dateString = "%y!%m!%d";
				break;
			case kDMYFormat:
				dateString = "%d!%m!%y";
				break;
			case kMDYFormat:
				// Fall through
			case kDateFormatEnd:
				// Fall through
			default:
				dateString = "%m!%d!%y";
				break;
		}

		char separatorArray[] = {' ', '-', '/', '\\', '.'};

		if (separator == kNoSeparator)
			dateString.ReplaceAll("!", "");
		else
			dateString.ReplaceAll('!', separatorArray[separator-1]);
	}

	if (index == 0)
		string = "%A, ";
	else if (index < 3)
		string = "%a, ";
	else
		string = "";

	string << dateString;

	if (index < 5)
		string << ", " << clockString;

	return B_OK;
}


template <class View>
float
TruncTimeBase(BString *result, int64 value, const View *view, float width)
{
	TrackerSettings settings;
	FormatSeparator separator = settings.TimeFormatSeparator();
	DateOrder order = settings.DateOrderFormat();
	bool clockIs24hr = settings.ClockIs24Hr();

	float resultWidth = 0;
	char buffer[256];

	time_t timeValue = (time_t)value;
	tm timeData;
	// use reentrant version of localtime to avoid having to have a semaphore
	// (localtime uses a global structure to do it's conversion)
	localtime_r(&timeValue, &timeData);

	BString timeFormat;

	for (int32 index = 0; ; index++) {
		if (TimeFormat(timeFormat, index, separator, order, clockIs24hr) != B_OK)
			break;
		strftime(buffer, 256, timeFormat.String(), &timeData);
		resultWidth = view->StringWidth(buffer);
		if (resultWidth <= width)
			break;
	}
	if (resultWidth > width)
		// even the shortest format string didn't do it, insert ellipsis
		resultWidth = TruncStringBase(result, buffer, (ssize_t)strlen(buffer), view, width);
	else
		*result = buffer;

	return resultWidth;
}


float
WidgetAttributeText::TruncTime(BString *result, int64 value, const BPoseView *view,
	float width)
{
	return TruncTimeBase(result, value, view, width);
}


float
WidgetAttributeText::CurrentWidth() const
{
	return fTruncatedWidth;
}


float
WidgetAttributeText::Width(const BPoseView *pose)
{
	FittingText(pose);
	return CurrentWidth();
}


void
WidgetAttributeText::SetUpEditing(BTextView *)
{
	ASSERT(fColumn->Editable());
}


bool
WidgetAttributeText::CommitEditedText(BTextView *)
{
	// can't do anything here at this point
	TRESPASS();
	return false;
}


status_t 
WidgetAttributeText::AttrAsString(const Model *model, BString *result,
	const char *attrName, int32 attrType,
	float width, BView *view, int64 *resultingValue)
{
	int64 value;
	
	status_t error = model->InitCheck();
	if (error != B_OK)
		return error;

	switch (attrType) {
		case B_TIME_TYPE: 
			if (strcmp(attrName, kAttrStatModified) == 0)
				value = model->StatBuf()->st_mtime;
			else if (strcmp(attrName, kAttrStatCreated) == 0)
				value = model->StatBuf()->st_crtime;
			else {
				TRESPASS();
				// not yet supported
				return B_ERROR;
			}
			TruncTimeBase(result, value, view, width);
			if (resultingValue)
				*resultingValue = value;
			return B_OK;

		case B_STRING_TYPE:
			if (strcmp(attrName, kAttrPath) == 0) {
				BEntry entry(model->EntryRef());
				BPath path;
				BString tmp;

				if (entry.InitCheck() == B_OK && entry.GetPath(&path) == B_OK) {
					tmp = path.Path();
					TruncateLeaf(&tmp);
				} else
					tmp = "-";
			
				if (width > 0)
					TruncStringBase(result, tmp.String(), tmp.Length(), view, width);
				else
					*result = tmp.String();
					
				return B_OK;
			}
			break;
		
		case kSizeType:
//			TruncFileSizeBase(result, model->StatBuf()->st_size, view, width);
			return B_OK;
			break;
			
		default:
			TRESPASS();
			// not yet supported
			return B_ERROR;
		
	}
	
	TRESPASS();
	return B_ERROR;
}


bool 
WidgetAttributeText::IsEditable() const
{
	return fColumn->Editable() && !BVolume(fModel->StatBuf()->st_dev).IsReadOnly();
}


void
WidgetAttributeText::SetDirty(bool value)
{
	fDirty = value;
}


//	#pragma mark -


StringAttributeText::StringAttributeText(const Model *model, const BColumn *column)
	:	WidgetAttributeText(model, column),
	fValueDirty(true)
{
}


const char *
StringAttributeText::Value()
{
	if (fValueDirty)
		ReadValue(&fFullValueText);

	return fFullValueText.String();
}


bool
StringAttributeText::CheckAttributeChanged()
{
	BString newString;
	ReadValue(&newString);

	if (newString == fFullValueText)
		return false;
	
	fFullValueText = newString;
	fDirty = true;		// have to redo fitted string
	return true;
}


void
StringAttributeText::FitValue(BString *result, const BPoseView *view)
{
	if (fValueDirty)
		ReadValue(&fFullValueText);
	fOldWidth = fColumn->Width();

	fTruncatedWidth = TruncString(result, fFullValueText.String(), fFullValueText.Length(),
		view, fOldWidth);
	fDirty = false;
}


float
StringAttributeText::PreferredWidth(const BPoseView *pose) const
{
	return pose->StringWidth(fFullValueText.String());
}


int
StringAttributeText::Compare(WidgetAttributeText &attr, BPoseView *)
{
	StringAttributeText *compareTo =
		dynamic_cast<StringAttributeText *>(&attr);
	ASSERT(compareTo);

	if (fValueDirty)
		ReadValue(&fFullValueText);

	return strcasecmp(fFullValueText.String(), compareTo->Value());
}


bool
StringAttributeText::CommitEditedText(BTextView *textView)
{
	ASSERT(fColumn->Editable());
	const char *text = textView->Text();

	if (fFullValueText == text)
		// no change
		return false;

	if (textView->TextLength() == 0)
		// cannot do an empty name
		return false;

	// cause re-truncation
	fDirty = true;

	if (!CommitEditedTextFlavor(textView))
		return false;

	// update text and width in this widget
	fFullValueText = text;

	return true;
}


//	#pragma mark -


ScalarAttributeText::ScalarAttributeText(const Model *model,
	const BColumn *column)
	:	WidgetAttributeText(model, column),
		fValueDirty(true)
{
}


int64
ScalarAttributeText::Value() 
{
	if (fValueDirty)
		fValue = ReadValue();
	return fValue;
}


bool
ScalarAttributeText::CheckAttributeChanged()
{
	int64 newValue = ReadValue();
	if (newValue == fValue)
		return false;
	
	fValue = newValue;
	fDirty = true;		// have to redo fitted string
	return true;
}


float
ScalarAttributeText::PreferredWidth(const BPoseView *pose) const
{
	BString widthString;
	widthString << fValue;
	return pose->StringWidth(widthString.String());
}


int
ScalarAttributeText::Compare(WidgetAttributeText &attr, BPoseView *)
{
	ScalarAttributeText *compareTo =
		dynamic_cast<ScalarAttributeText *>(&attr);
	ASSERT(compareTo);
		// make sure we're not comparing apples and oranges

	if (fValueDirty)
		fValue = ReadValue();

	return fValue > compareTo->Value() ? (fValue == compareTo->Value() ? 0 : -1) : 1 ;
}


//	#pragma mark -


PathAttributeText::PathAttributeText(const Model *model, const BColumn *column)
	:	StringAttributeText(model, column)
{
}


void
PathAttributeText::ReadValue(BString *result)
{
	// get the path
	BEntry entry(fModel->EntryRef());
	BPath path;

	if (entry.InitCheck() == B_OK && entry.GetPath(&path) == B_OK) {
		*result = path.Path();
		TruncateLeaf(result);
	} else
		*result = "-";
	fValueDirty = false;
}


//	#pragma mark -


OriginalPathAttributeText::OriginalPathAttributeText(const Model *model, const BColumn *column)
	:	StringAttributeText(model, column)
{
}


void
OriginalPathAttributeText::ReadValue(BString *result)
{
	BEntry entry(fModel->EntryRef());
	BPath path;

	// get the original path
	if (entry.InitCheck() == B_OK && FSGetOriginalPath(&entry, &path) == B_OK)
		*result = path.Path();
	else
		*result = "-";
	fValueDirty = false;
}


//	#pragma mark -


KindAttributeText::KindAttributeText(const Model *model, const BColumn *column)
	:	StringAttributeText(model, column)
{
}


void
KindAttributeText::ReadValue(BString *result)
{
	BMimeType mime;
	char desc[B_MIME_TYPE_LENGTH];

	// get the mime type
	if (mime.SetType(fModel->MimeType()) != B_OK)
		*result = "Unknown";
	// get the short mime type description
	else if (mime.GetShortDescription(desc) == B_OK)
		*result = desc;
	else
		*result = fModel->MimeType();
	fValueDirty = false;
}


//	#pragma mark -


NameAttributeText::NameAttributeText(const Model *model, const BColumn *column)
	:	StringAttributeText(model, column)
{
}


int
NameAttributeText::Compare(WidgetAttributeText &attr, BPoseView *)
{
	NameAttributeText *compareTo = dynamic_cast<NameAttributeText *>(&attr);

	ASSERT(compareTo);

	if (fValueDirty)
		ReadValue(&fFullValueText);

	if (NameAttributeText::sSortFolderNamesFirst) 
		return fModel->CompareFolderNamesFirst(attr.TargetModel());

	return strcasecmp(fFullValueText.String(), compareTo->Value());
}


void
NameAttributeText::ReadValue(BString *result)
{
#ifdef DEBUG
// x86 support :-)
	if ((modifiers() & B_CAPS_LOCK) != 0) {
		if (fModel->IsVolume()) {
			BVolumeRoster roster;
			roster.Rewind();
			BVolume volume;
			char device = 'A';
			while (roster.GetNextVolume(&volume) == B_OK) {
				char name[256];
				if (volume.GetName(name) == B_OK
					&& strcmp(name, fModel->Name()) == 0) {
					*result += device;
					*result += ':';
					fValueDirty = false;
					return;
				}
				device++;
			}
		}
		const char *modelName = fModel->Name();
		bool hasDot = strstr(".", modelName) != 0;
		for (int32 index = 0; index < 8; index++) {
			if (!modelName[index] || modelName[index] == '.')
				break;
			*result += toupper(modelName[index]);
		}
		if (hasDot) {
			modelName = strstr(".", modelName);
			for (int32 index = 0; index < 4; index++) {
				if (!modelName[index])
					break;
				*result += toupper(modelName[index]);
			}
		} else if (fModel->IsExecutable())
			*result += ".EXE";

	} else
#endif
	*result = fModel->Name();

	fValueDirty = false;
}


void
NameAttributeText::FitValue(BString *result, const BPoseView *view)
{
	if (fValueDirty)
		ReadValue(&fFullValueText);
	fOldWidth = fColumn->Width();
	fTruncatedWidth = TruncString(result, fFullValueText.String(), fFullValueText.Length(),
		view, fOldWidth, B_TRUNCATE_END);
	fDirty = false;
}


void
NameAttributeText::SetUpEditing(BTextView *textView)
{
	DisallowFilenameKeys(textView);
	
	textView->SetMaxBytes(B_FILE_NAME_LENGTH);
	textView->SetText(fFullValueText.String(), fFullValueText.Length());
}


bool
NameAttributeText::CommitEditedTextFlavor(BTextView *textView)
{
	const char *text = textView->Text();

	BEntry entry(fModel->EntryRef());
	if (entry.InitCheck() != B_OK)
		return false;

	BDirectory	parent;
	if (entry.GetParent(&parent) != B_OK)
		return false;

	bool removeExisting = false;
	if (parent.Contains(text)) {
		BAlert *alert = new BAlert("", "That name is already taken. "
				"Please type another one.", "Replace other file", "OK", NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT);

		alert->SetShortcut(0, 'r');

		if (alert->Go())
			return false;

		removeExisting = true;
	}

	// ToDo:
	// use model-flavor specific virtuals for all of these special
	// renamings
	status_t result;
	if (fModel->IsVolume()) {
		BVolume	volume(fModel->NodeRef()->device);
		result = volume.InitCheck();
		if (result == B_OK) {
			RenameVolumeUndo undo(volume, text);

			result = volume.SetName(text);
			if (result != B_OK)
				undo.Remove();
		}
	} else {
		if (fModel->IsQuery()) {
			BModelWriteOpener opener(fModel);
			ASSERT(fModel->Node());
			MoreOptionsStruct::SetQueryTemporary(fModel->Node(), false);
		}

		RenameUndo undo(entry, text);

		result = entry.Rename(text, removeExisting);
		if (result != B_OK)
			undo.Remove();
	}

	return result == B_OK;
}

bool NameAttributeText::sSortFolderNamesFirst = false;

void
NameAttributeText::SetSortFolderNamesFirst(bool enabled)
{
	NameAttributeText::sSortFolderNamesFirst = enabled;	
}


//	#pragma mark -

#ifdef OWNER_GROUP_ATTRIBUTES

OwnerAttributeText::OwnerAttributeText(const Model *model, const BColumn *column)
	:	StringAttributeText(model, column)
{
}


void
OwnerAttributeText::ReadValue(BString *result)
{
	uid_t nodeOwner = fModel->StatBuf()->st_uid;
	BString user;
	
	if (nodeOwner == 0) {
		if (getenv("USER") != NULL)
			user << getenv("USER");
		else
			user << "root";
	} else
		user << nodeOwner;
	*result = user.String();	

	fValueDirty = false;
}


GroupAttributeText::GroupAttributeText(const Model *model, const BColumn *column)
	:	StringAttributeText(model, column)
{
}


void
GroupAttributeText::ReadValue(BString *result)
{
	gid_t nodeGroup = fModel->StatBuf()->st_gid;
	BString group;

	if (nodeGroup == 0) {
		if (getenv("GROUP") != NULL)
			group << getenv("GROUP");
		else
			group << "0";
	} else
		group << nodeGroup;
	*result = group.String();	

	fValueDirty = false;
}

#endif  /* OWNER_GROUP_ATTRIBUTES */

ModeAttributeText::ModeAttributeText(const Model *model, const BColumn *column)
	:	StringAttributeText(model, column)
{
}


void
ModeAttributeText::ReadValue(BString *result)
{
	mode_t mode = fModel->StatBuf()->st_mode;
	mode_t baseMask = 00400;
	char buffer[11];

	char *scanner = buffer;

	if (S_ISDIR(mode))
		*scanner++ = 'd';
	else if (S_ISLNK(mode))
		*scanner++ = 'l';
	else if (S_ISBLK(mode))
		*scanner++ = 'b';
	else if (S_ISCHR(mode))
		*scanner++ = 'c';
	else
		*scanner++ = '-';

	for (int32 index = 0; index < 9; index++) {
		*scanner++ = (mode & baseMask) ? "rwx"[index % 3] : '-';
		baseMask >>= 1;
	}

	*scanner = 0;
	*result = buffer;

	fValueDirty = false;
}


//	#pragma mark -


SizeAttributeText::SizeAttributeText(const Model *model, const BColumn *column)
	:	ScalarAttributeText(model, column)
{
}


int64
SizeAttributeText::ReadValue()
{
	fValueDirty = false;
	// get the size
	
	if (fModel->IsVolume()) {
		BVolume volume(fModel->NodeRef()->device);

		return volume.Capacity();
	}

	if (fModel->IsDirectory() || fModel->IsQuery()
		|| fModel->IsQueryTemplate() || fModel->IsSymLink())
		return kUnknownSize;

	fValueIsDefined = true;

	return fModel->StatBuf()->st_size;
}


void
SizeAttributeText::FitValue(BString *result, const BPoseView *view)
{
	if (fValueDirty)
		fValue = ReadValue();
	fOldWidth = fColumn->Width();
	fTruncatedWidth = TruncFileSize(result, fValue, view, fOldWidth);
	fDirty = false;
}


float
SizeAttributeText::PreferredWidth(const BPoseView *pose) const
{
	if (fValueIsDefined) {
		BString widthString;
		TruncFileSize(&widthString, fValue, pose, 100000);
		return pose->StringWidth(widthString.String());
	}
	return pose->StringWidth("-");
}


//	#pragma mark -


TimeAttributeText::TimeAttributeText(const Model *model, const BColumn *column)
	:	ScalarAttributeText(model, column)
{
}


float
TimeAttributeText::PreferredWidth(const BPoseView *pose) const
{
	BString widthString;
	TruncTimeBase(&widthString, fValue, pose, 100000);
	return pose->StringWidth(widthString.String());
}


void
TimeAttributeText::FitValue(BString *result, const BPoseView *view)
{
	if (fValueDirty)
		fValue = ReadValue();
	fOldWidth = fColumn->Width();
	fTruncatedWidth = TruncTime(result, fValue, view, fOldWidth);
	fDirty = false;
}


CreationTimeAttributeText::CreationTimeAttributeText(const Model *model,
	const BColumn *column)
	:	TimeAttributeText(model, column)
{
}


int64
CreationTimeAttributeText::ReadValue()
{
	fValueDirty = false;
	fValueIsDefined = true;
	return fModel->StatBuf()->st_crtime;
}

ModificationTimeAttributeText::ModificationTimeAttributeText(const Model *model,
	const BColumn *column)
	:	TimeAttributeText(model, column)
{
}


int64
ModificationTimeAttributeText::ReadValue()
{
	fValueDirty = false;
	fValueIsDefined = true;
	return fModel->StatBuf()->st_mtime;
}


//	#pragma mark -

const int32 kGenericReadBufferSize = 1024;

GenericAttributeText::GenericAttributeText(const Model *model, const BColumn *column)
	:	WidgetAttributeText(model, column),
	fValueDirty(true)
{
}


bool
GenericAttributeText::CheckAttributeChanged()
{
	GenericValueStruct tmpValue = fValue;
	BString tmpString(fFullValueText);
	ReadValue();

	// fDirty could already be true, in that case we mustn't set it to
	// false, even if the attribute text hasn't changed
	bool changed = (fValue.int64t != tmpValue.int64t) || (tmpString != fFullValueText);
	if (changed)
		fDirty = true;

	return fDirty;
}


float
GenericAttributeText::PreferredWidth(const BPoseView *pose) const
{
	return pose->StringWidth(fFullValueText.String());
}


void
GenericAttributeText::ReadValue()
{
	BModelOpener opener(const_cast<Model *>(fModel));
	
	ssize_t length = 0;
	fFullValueText = "-";
	fValue.int64t = 0;
	fValueIsDefined = false;
	fValueDirty = false;

	if (!fModel->Node())
		return;

	switch (fColumn->AttrType()) {
		case B_STRING_TYPE:
		{
			char buffer[kGenericReadBufferSize];
			length = fModel->Node()->ReadAttr(fColumn->AttrName(),
				fColumn->AttrType(), 0, buffer, kGenericReadBufferSize - 1);

			if (length > 0) {
				buffer[length] = '\0';
				// make sure the buffer is null-terminated even if we
				// didn't read the whole attribute in or it wasn't to
				// begin with

				fFullValueText = buffer;
				fValueIsDefined = true;
			}
			break;
		}

		case B_SSIZE_T_TYPE:
		case B_TIME_TYPE:
		case B_OFF_T_TYPE:
		case B_FLOAT_TYPE:
		case B_BOOL_TYPE:
		case B_CHAR_TYPE:
		case B_INT8_TYPE:
		case B_INT16_TYPE:
		case B_INT32_TYPE:
		case B_INT64_TYPE:
		case B_UINT8_TYPE:
		case B_UINT16_TYPE:
		case B_UINT32_TYPE:
		case B_UINT64_TYPE:
		case B_DOUBLE_TYPE:
		{
			// read in the numerical bit representation and attach it
			// with a type, depending on the bytes that could be read
			attr_info info;
			GenericValueStruct tmp;
			if (fModel->Node()->GetAttrInfo(fColumn->AttrName(), &info) == B_OK) {
				if (info.size && info.size <= sizeof(int64)) {
					length = fModel->Node()->ReadAttr(fColumn->AttrName(),
						fColumn->AttrType(), 0, &tmp, (size_t)info.size);
				}

				// We used tmp as a block of memory, now set the correct fValue:

				if (length == info.size) {
					if (fColumn->AttrType() == B_FLOAT_TYPE
						|| fColumn->AttrType() == B_DOUBLE_TYPE) {
						// filter out special float/double types
						switch (info.size) {
							case sizeof(float):
								fValueIsDefined = true;
								fValue.floatt = tmp.floatt;
								break;

							case sizeof(double):
								fValueIsDefined = true;
								fValue.doublet = tmp.doublet;
								break;

							default:
								TRESPASS();
						}
					} else {
						// handle the standard data types
						switch (info.size) {
							case sizeof(char):	// Takes care of bool, too.
								fValueIsDefined = true;
								fValue.int8t = tmp.int8t;
								break;

							case sizeof(int16):
								fValueIsDefined = true;
								fValue.int16t = tmp.int16t;
								break;

							case sizeof(int32):	// Takes care of time_t, too.
								fValueIsDefined = true;
								fValue.int32t = tmp.int32t;
								break;

							case sizeof(int64):	// Taked care of off_t, too.
								fValueIsDefined = true;
								fValue.int64t = tmp.int64t;
								break;

							default:
								TRESPASS();
						}
					}
				}
			}
			break;
		}
	}
}


void
GenericAttributeText::FitValue(BString *result, const BPoseView *view)
{
	if (fValueDirty)
		ReadValue();

	fOldWidth = fColumn->Width();

	if (!fValueIsDefined) {
		*result = "-";
		fTruncatedWidth = TruncString(result, fFullValueText.String(),
			fFullValueText.Length(), view, fOldWidth);
		fDirty = false;
		return;
	}

	char buffer[256];

	switch (fColumn->AttrType()) {
		case B_SIZE_T_TYPE:
			TruncFileSizeBase(result, fValue.int32t, view, fOldWidth);
			return;

		case B_SSIZE_T_TYPE:
			if (fValue.int32t > 0) {
				TruncFileSizeBase(result, fValue.int32t, view, fOldWidth);
				return;
			}
			sprintf(buffer, "%s", strerror(fValue.int32t));
			fFullValueText = buffer;
			break;

		case B_STRING_TYPE:
			fTruncatedWidth = TruncString(result, fFullValueText.String(),
				fFullValueText.Length(), view, fOldWidth);
			fDirty = false;
			return;
			
		case B_OFF_T_TYPE:
			// as a side effect update the fFullValueText to the string representation
			// of value
			TruncFileSize(&fFullValueText, fValue.off_tt, view, 100000); 
			fTruncatedWidth = TruncFileSize(result, fValue.off_tt, view, fOldWidth);
			fDirty = false;
			return;

		case B_TIME_TYPE:
			// as a side effect update the fFullValueText to the string representation
			// of value
			TruncTime(&fFullValueText, fValue.time_tt, view, 100000); 
			fTruncatedWidth = TruncTime(result, fValue.time_tt, view, fOldWidth);
			fDirty = false;
			return;

		case B_BOOL_TYPE:
			// For now use true/false, would be nice to be able to set
			// the value text
			
 			sprintf(buffer, "%s", fValue.boolt ? "true" : "false");
			fFullValueText = buffer;
			break;

		case B_CHAR_TYPE:
			// Make sure no non-printable characters are displayed:
			if (!isprint(fValue.uint8t)) {
				*result = "-";
				fTruncatedWidth = TruncString(result, fFullValueText.String(),
					fFullValueText.Length(), view, fOldWidth);
				fDirty = false;
				return;
			}
			
			sprintf(buffer, "%c", fValue.uint8t);
			fFullValueText = buffer;
			break;

		case B_INT8_TYPE:
			sprintf(buffer, "%d", fValue.int8t);
			fFullValueText = buffer;
			break;

		case B_UINT8_TYPE:
			sprintf(buffer, "%d", fValue.uint8t);
			fFullValueText = buffer;
			break;

		case B_INT16_TYPE:
			sprintf(buffer, "%d", fValue.int16t);
			fFullValueText = buffer;
			break;

		case B_UINT16_TYPE:
			sprintf(buffer, "%d", fValue.uint16t);
			fFullValueText = buffer;
			break;

		case B_INT32_TYPE:
			sprintf(buffer, "%ld", fValue.int32t);
			fFullValueText = buffer;
			break;

		case B_UINT32_TYPE:
			sprintf(buffer, "%ld", fValue.uint32t);
			fFullValueText = buffer;
			break;

		case B_INT64_TYPE:
			sprintf(buffer, "%Ld", fValue.int64t);
			fFullValueText = buffer;
			break;

		case B_UINT64_TYPE:
			sprintf(buffer, "%Ld", fValue.uint64t);
			fFullValueText = buffer;
			break;
		
		case B_FLOAT_TYPE:
			if (fabs(fValue.floatt) >= 10000
				|| fabs(fValue.floatt) < 0.01)
			// The %f conversion can possibly overflow 'buffer' if the value is
			// too big, since it doesn't print in exponent form. Ever.
				sprintf(buffer, "%.3e", fValue.floatt);
			else
				sprintf(buffer, "%.3f", fValue.floatt);
			fFullValueText = buffer;
			break;
			
		case B_DOUBLE_TYPE:
			if (fabs(fValue.doublet) >= 10000
				|| fabs(fValue.doublet) < 0.01)
			// The %f conversion can possibly overflow 'buffer' if the value is
			// too big, since it doesn't print in exponent form. Ever.
				sprintf(buffer, "%.3e", fValue.doublet);
			else
				sprintf(buffer, "%.3f", fValue.doublet);
			fFullValueText = buffer;
			break;
			
		default:
			*result = "-";
			fTruncatedWidth = TruncString(result, fFullValueText.String(),
				fFullValueText.Length(), view, fOldWidth);
			fDirty = false;
			return;
	}
	fTruncatedWidth = TruncString(result, buffer, (ssize_t)strlen(buffer), view, fOldWidth);
	fDirty = false;
}


int
GenericAttributeText::Compare(WidgetAttributeText &attr, BPoseView *)
{
	GenericAttributeText *compareTo =
		dynamic_cast<GenericAttributeText *>(&attr);
	ASSERT(compareTo);

	if (fValueDirty)
		ReadValue();
	if (compareTo->fValueDirty)
		compareTo->ReadValue();
	
	// Sort undefined values last, regardless of the other value:
	if (fValueIsDefined == false || compareTo->fValueIsDefined == false)
		return fValueIsDefined < compareTo->fValueIsDefined ?
			(fValueIsDefined == compareTo->fValueIsDefined ? 0 : -1) : 1;

	switch (fColumn->AttrType()) {
		case B_STRING_TYPE:
			return fFullValueText.ICompare(compareTo->fFullValueText);

		case B_CHAR_TYPE:
			{
				char vStr[2] = { static_cast<char>(fValue.uint8t), 0 };
				char cStr[2] = { static_cast<char>(compareTo->fValue.uint8t), 0};
				
				BString valueStr(vStr);
				BString compareToStr(cStr);
				
				return valueStr.ICompare(compareToStr);
			}

		case B_FLOAT_TYPE:
			return fValue.floatt > compareTo->fValue.floatt ?
				(fValue.floatt == compareTo->fValue.floatt ? 0 : -1) : 1;

		case B_DOUBLE_TYPE:
			return fValue.doublet > compareTo->fValue.doublet ?
				(fValue.doublet == compareTo->fValue.doublet ? 0 : -1) : 1;

		case B_BOOL_TYPE:
			return fValue.boolt > compareTo->fValue.boolt ?
				(fValue.boolt == compareTo->fValue.boolt ? 0 : -1) : 1;

		case B_UINT8_TYPE:
			return fValue.uint8t > compareTo->fValue.uint8t ?
				(fValue.uint8t == compareTo->fValue.uint8t ? 0 : -1) : 1;

		case B_INT8_TYPE:
			return fValue.int8t > compareTo->fValue.int8t ?
					(fValue.int8t == compareTo->fValue.int8t ? 0 : -1) : 1;

		case B_UINT16_TYPE:
			return fValue.uint16t > compareTo->fValue.uint16t ?
				(fValue.uint16t == compareTo->fValue.uint16t ? 0 : -1) : 1;

		case B_INT16_TYPE:
			return fValue.int16t > compareTo->fValue.int16t ?
				(fValue.int16t == compareTo->fValue.int16t ? 0 : -1) : 1;

		case B_UINT32_TYPE:
			return fValue.uint32t > compareTo->fValue.uint32t ?
				(fValue.uint32t == compareTo->fValue.uint32t ? 0 : -1) : 1;

		case B_TIME_TYPE:
			// time_t typedef'd to a long, i.e. a int32
		case B_INT32_TYPE:
			return fValue.int32t > compareTo->fValue.int32t ?
				(fValue.int32t == compareTo->fValue.int32t ? 0 : -1) : 1;

		case B_OFF_T_TYPE:
			// off_t typedef'd to a long long, i.e. a int64
		case B_INT64_TYPE:
			return fValue.int64t > compareTo->fValue.int64t ?
				(fValue.int64t == compareTo->fValue.int64t ? 0 : -1) : 1;

		case B_UINT64_TYPE:
		default:
			return fValue.uint64t > compareTo->fValue.uint64t ?
				(fValue.uint64t == compareTo->fValue.uint64t ? 0 : -1) : 1;
	}
	return 0;
}


bool
GenericAttributeText::CommitEditedText(BTextView *textView)
{
	ASSERT(fColumn->Editable());
	const char *text = textView->Text();

	if (fFullValueText == text)
		// no change
		return false;

	if (!CommitEditedTextFlavor(textView))
		return false;

	// update text and width in this widget
	fFullValueText = text;
	// cause re-truncation
	fDirty = true;
	fValueDirty = true;

	return true;
}


void
GenericAttributeText::SetUpEditing(BTextView *textView)
{
	textView->SetMaxBytes(kGenericReadBufferSize - 1);
	textView->SetText(fFullValueText.String(), fFullValueText.Length());
}


bool
GenericAttributeText::CommitEditedTextFlavor(BTextView *textView)
{
	BNode node(fModel->EntryRef());

	if (node.InitCheck() != B_OK)
		return false;

	uint32 type = fColumn->AttrType();

	if (type != B_STRING_TYPE
		&& type != B_UINT64_TYPE
		&& type != B_UINT32_TYPE
		&& type != B_UINT16_TYPE
		&& type != B_UINT8_TYPE
		&& type != B_INT64_TYPE
		&& type != B_INT32_TYPE
		&& type != B_INT16_TYPE
		&& type != B_INT8_TYPE
		&& type != B_OFF_T_TYPE
		&& type != B_TIME_TYPE
		&& type != B_FLOAT_TYPE
		&& type != B_DOUBLE_TYPE
		&& type != B_CHAR_TYPE
		&& type != B_BOOL_TYPE) {
		(new BAlert("", "Sorry, you cannot edit that attribute.",
			"Cancel", 0, 0, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		return false;
	}

	const char *columnName = fColumn->AttrName();
	ssize_t size = 0;

	switch (type) {
		case B_STRING_TYPE:
			size = fModel->WriteAttr(columnName,
				type, 0, textView->Text(), (size_t)(textView->TextLength() + 1));
			break;

		case B_BOOL_TYPE:
			{
				bool value = strncasecmp(textView->Text(), "0", 1) != 0
					&& strncasecmp(textView->Text(), "off", 2) != 0
					&& strncasecmp(textView->Text(), "no", 3) != 0
					&& strncasecmp(textView->Text(), "false", 4) != 0
					&& strlen(textView->Text()) != 0;

				size = fModel->WriteAttr(columnName, type, 0, &value, sizeof(bool));
				break;	
			}

		case B_CHAR_TYPE:
			{
				char ch;
				sscanf(textView->Text(), "%c", &ch);
				//Check if we read the start of a multi-byte glyph:
				if (!isprint(ch)) {
					(new BAlert("", "Sorry, The 'Character' attribute cannot store a multi-byte glyph.",
						"Cancel", 0, 0, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
					return false;
				}
				
				size = fModel->WriteAttr(columnName, type, 0, &ch, sizeof(char));
				break;
			}

		case B_FLOAT_TYPE:
			{
				float floatVal;

				if (sscanf(textView->Text(), "%f", &floatVal) == 1) {
					fValueIsDefined = true;
					fValue.floatt = floatVal;
					size = fModel->WriteAttr(columnName, type, 0, &floatVal, sizeof(float));
				} else
					// If the value was already defined, it's on disk. Otherwise not.
					return fValueIsDefined;
				break;
			}
			
		case B_DOUBLE_TYPE:
			{
				double doubleVal;

				if (sscanf(textView->Text(), "%lf", &doubleVal) == 1) {
					fValueIsDefined = true;
					fValue.doublet = doubleVal;
					size = fModel->WriteAttr(columnName, type, 0, &doubleVal, sizeof(double));
				} else
					// If the value was already defined, it's on disk. Otherwise not.
					return fValueIsDefined;
				break;
			}
			
		case B_TIME_TYPE:
		case B_OFF_T_TYPE:
		case B_UINT64_TYPE:
		case B_UINT32_TYPE:
		case B_UINT16_TYPE:
		case B_UINT8_TYPE:
		case B_INT64_TYPE:
		case B_INT32_TYPE:
		case B_INT16_TYPE:
		case B_INT8_TYPE:
			{
				GenericValueStruct tmp;
				size_t scalarSize = 0;
				
				switch (type) {
					case B_TIME_TYPE:
						tmp.time_tt = parsedate(textView->Text(), time(0));
						scalarSize = sizeof(time_t);
						break;
						
					// do some size independent conversion on builtin types
					case B_OFF_T_TYPE:
						tmp.off_tt = StringToScalar(textView->Text());
						scalarSize = sizeof(off_t);
						break;
						
					case B_UINT64_TYPE:
					case B_INT64_TYPE:
						tmp.int64t = StringToScalar(textView->Text());
						scalarSize = sizeof(int64);
						break;
						
					case B_UINT32_TYPE:
					case B_INT32_TYPE:
						tmp.int32t = (int32)StringToScalar(textView->Text());
						scalarSize = sizeof(int32);
						break;
						
					case B_UINT16_TYPE:
					case B_INT16_TYPE:
						tmp.int16t = (int16)StringToScalar(textView->Text());
						scalarSize = sizeof(int16);
						break;
						
					case B_UINT8_TYPE:
					case B_INT8_TYPE:
						tmp.int8t = (int8)StringToScalar(textView->Text());
						scalarSize = sizeof(int8);
						break;
						
					default:
						TRESPASS();
				
				}
				
				size = fModel->WriteAttr(columnName, type, 0, &tmp, scalarSize);
				break;
			}
	}

	if (size < 0) {
		(new BAlert("", "There was an error writing the attribute.",
			"Cancel", 0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();

		fValueIsDefined = false;
		return false;
	}

	fValueIsDefined = true;
	return true;
}


//	#pragma mark -


OpenWithRelationAttributeText::OpenWithRelationAttributeText(const Model *model,
	const BColumn *column, const BPoseView *view)
	:	ScalarAttributeText(model, column),
		fPoseView(view)
{
}


int64 
OpenWithRelationAttributeText::ReadValue()
{
	fValueDirty = false;

	const OpenWithPoseView *view = dynamic_cast<const OpenWithPoseView *>(fPoseView);

	if (view) {
		fValue = view->OpenWithRelation(fModel);
		fValueIsDefined = true;
	}

	return fValue;
}


void 
OpenWithRelationAttributeText::FitValue(BString *result, const BPoseView *view)
{
	if (fValueDirty)
		ReadValue();

	ASSERT(view == fPoseView);
	const OpenWithPoseView *launchWithView =
		dynamic_cast<const OpenWithPoseView *>(view);

	if (launchWithView)
		launchWithView->OpenWithRelationDescription(fModel, &fRelationText);

	fOldWidth = fColumn->Width();
	fTruncatedWidth = TruncString(result, fRelationText.String(), fRelationText.Length(),
		view, fOldWidth, B_TRUNCATE_END);
	fDirty = false;
}


VersionAttributeText::VersionAttributeText(const Model *model,
	const BColumn *column, bool app)
	:	StringAttributeText(model, column),
		fAppVersion(app)
{
}


void 
VersionAttributeText::ReadValue(BString *result)
{
	fValueDirty = false;

	BModelOpener opener(fModel);
	BFile *file = dynamic_cast<BFile *>(fModel->Node());
	if (file) {
		BAppFileInfo info(file);
		version_info version;
		if (info.InitCheck() == B_OK
			&& info.GetVersionInfo(&version,
				fAppVersion ? B_APP_VERSION_KIND : B_SYSTEM_VERSION_KIND) == B_OK) {
			*result = version.short_info;
			return;
		}
	}
	*result = "-";
}

