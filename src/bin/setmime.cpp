/*
 * Copyright 2011 Aleksas Pantechovskis, <alexp.frl@gmail.com>
 * Copyright 2011 Siarzhuk Zharski, <imker@gmx.li>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <iostream>
#include <map>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include <Application.h>
#include <Bitmap.h>
#include <Entry.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <Mime.h>
#include <Path.h>
#include <String.h>


using namespace std;


const char* kUsageMessage = "# setmime:\n"
	"# usage: setmime ((-dump | -dumpSniffRule | -dumpIcon | -dumpAll) "
													"[ <signatureString> ] )\n"
	"#      | (-remove <signatureString> )\n"
	"#      | ( (-set | -force | -add)  <signatureString>\n"
	"#          [ -short <short description> ] [ -long <long description> ]\n"
	"#          [ -preferredApp <preferred app path> ]\n"
	"#          [ -preferredAppSig <preferred app signature> ]\n"
	"#          [ -sniffRule <sniffRule> ]\n"
	"#          [ -extension <file suffix> ]\n"
	"#          [ -attribute <internal name>\n"
	"#             [ -attrName <public name> ] [ -attrType <type code> ]\n"
	"#             [ -attrWidth <display width> ][ -attrAlignment <position>]\n"
	"#             [ -attrViewable <bool flag> ][ -attrEditable <bool flag> ]\n"
	"#             [ -attrExtra <bool flag> ] ]\n"
	"#          [ -miniIcon <256 hex bytes> ]\n"
	"#          [ -largeIcon <1024 hex bytes> ] ... )\n"
	"#      | (-checkSniffRule <sniffRule>\n"
	"#      | -includeApps)\n";

const char* kHelpMessage = "#  -dump prints a specified metamime\n"
	"#  -remove removes specified metamime\n"
	"#  -add adds specified metamime and specified metamime attributes\n"
	"#      that have not yet been defined\n"
	"#  -set adds specified metamime and specified metamime attributes, \n"
	"#      overwrites the existing values of specified metamime attributes\n"
	"#  -force adds specified metamime and specified metamime attributes\n"
	"#      after first erasing all the existing attributes\n"
	"#  -dumpSniffRule prints just the MIME sniffer rule of a "
														"specified metamime\n"
	"#     -dumpIcon prints just the icon information of a specified metamime\n"
	"#  -dumpAll prints all the information, including icons of a "
														"specified metamime\n"
	"#  -checkSniffRule parses a MIME sniffer rule and reports any errors\n"
	"#  -includeApps will include applications\n";

const char* kNeedArgMessage =	"you have to specify any of "
								"-dump[All|Icon|SnifferRule], -add, -set, "
								"-force or -remove";

const char* kWrongModeMessage = "can only specify one of -dump, -dumpAll, "
								"-dumpIcon, -dumpSnifferRule, -remove, "
								"-add, -set, -force or -checkSnifferRule";

const char* kHelpReq		=	"--help";
const char* kDump			=	"-dump";
const char* kDumpSniffRule	=	"-dumpSniffRule";
const char* kDumpIcon		=	"-dumpIcon";
const char* kDumpAll		=	"-dumpAll";
const char* kAdd			=	"-add";
const char* kSet			=	"-set";
const char* kForce			=	"-force";
const char* kRemove			=	"-remove";
const char* kCheckSniffRule	=	"-checkSniffRule";
const char* kShort			=	"-short";
const char* kLong			=	"-long";
const char* kPreferredApp	=	"-preferredApp";
const char* kPreferredAppSig =	"-preferredAppSig";
const char* kSniffRule		=	"-sniffRule";
const char* kMiniIcon		=	"-miniIcon";
const char* kLargeIcon		=	"-largeIcon";
const char* kIncludeApps	=	"-includeApps";
const char* kExtension		=	"-extension";
const char* kAttribute		=	"-attribute";
const char* kAttrName		=	"-attrName";
const char* kAttrType		=	"-attrType";
const char* kAttrWidth		=	"-attrWidth";
const char* kAttrAlignment	=	"-attrAlignment";
const char* kAttrViewable	=	"-attrViewable";
const char* kAttrEditable	=	"-attrEditable";
const char* kAttrExtra		=	"-attrExtra";


const uint32 hash(const char* str)
{
	uint32 h = 0;
	uint32 g = 0;
	for (const char* p = str; *p; p++) {
		h = (h << 4) + (*p & 0xFF);
		g = h & 0xF0000000;
		if (g != 0) {
			h ^= g >> 24;
			h ^= g;
		}
	}
	return h;
}


// the list of all acceptable command-line options
struct CmdOption {

	enum Type {
		kMode,
		kOption,
		kAttrRoot,
		kAttrib,
		kHelp
	};

	const char*	fName;
	Type		fType;
	bool		fNeedArg;
	bool		fNonExclusive;

} gCmdOptions[] = {

	{ kHelpReq,			CmdOption::kHelp },

	{ kDump,			CmdOption::kMode },
	{ kDumpSniffRule,	CmdOption::kMode },
	{ kDumpIcon,		CmdOption::kMode },
	{ kDumpAll,			CmdOption::kMode },
	{ kAdd,				CmdOption::kMode },
	{ kSet,				CmdOption::kMode },
	{ kForce,			CmdOption::kMode },
	{ kRemove,			CmdOption::kMode },
	{ kCheckSniffRule,	CmdOption::kMode },

	{ kShort,			CmdOption::kOption, true },
	{ kLong,			CmdOption::kOption, true },
	{ kPreferredApp,	CmdOption::kOption, true },
	{ kPreferredAppSig,	CmdOption::kOption, true },
	{ kSniffRule,		CmdOption::kOption, true },
	{ kMiniIcon	,		CmdOption::kOption, true },
	{ kLargeIcon,		CmdOption::kOption, true },
	{ kIncludeApps,		CmdOption::kOption, false },
	{ kExtension,		CmdOption::kOption, true, true },
	{ kAttribute,		CmdOption::kAttrRoot, true, true },

	{ kAttrName,		CmdOption::kAttrib, true },
	{ kAttrType,		CmdOption::kAttrib, true },
	{ kAttrWidth,		CmdOption::kAttrib, true },
	{ kAttrAlignment,	CmdOption::kAttrib, true },
	{ kAttrViewable,	CmdOption::kAttrib, true },
	{ kAttrEditable,	CmdOption::kAttrib, true },
	{ kAttrExtra,		CmdOption::kAttrib, true }
};

// the 'hash -> value' map of arguments provided by user
typedef multimap<uint32, const char*>				TUserArgs;
typedef multimap<uint32, const char*>::iterator		TUserArgsI;

// user provided attributes are grouped separately in vector
typedef vector<TUserArgs>			TUserAttrs;
typedef vector<TUserArgs>::iterator	TUserAttrsI;

const uint32 kOpModeUndefined = 0;


// #pragma mark -

// encapsulate the single attribute params
//
struct MimeAttribute
{
	status_t	fStatus;
	BString 	fName;
	BString 	fPublicName;
	int32 		fType;
	bool 		fViewable;
	bool 		fEditable;
	bool 		fExtra;
	int32 		fWidth;
	int32 		fAlignment;

				MimeAttribute(BMessage& msg, int32 index);
				MimeAttribute(TUserArgs& args);

	status_t	InitCheck() { return fStatus; }
	void		Dump();
	void		SyncWith(TUserArgs& args);
	void		StoreInto(BMessage* target);
	const char*	UserArgValue(TUserArgs& map, const char* name);
};


MimeAttribute::MimeAttribute(BMessage& msg, int32 index)
		:
		fStatus(B_NO_INIT),
		fType('CSTR'),
		fViewable(true),
		fEditable(false),
		fExtra(false),
		fWidth(0),
		fAlignment(0)
{
	BString rawPublicName;
	struct attrEntry {
		const char* name;
		type_code	type;
		void*		data;
	} attrEntries[] = {
		{ "attr:name",			B_STRING_TYPE,	&fName },
		{ "attr:public_name",	B_STRING_TYPE,	&rawPublicName },
		{ "attr:type",			B_INT32_TYPE,	&fType },
		{ "attr:viewable",		B_BOOL_TYPE,	&fViewable },
		{ "attr:editable",		B_BOOL_TYPE,	&fEditable },
		{ "attr:extra",			B_BOOL_TYPE,	&fExtra },
		{ "attr:width",			B_INT32_TYPE,	&fWidth },
		{ "attr:alignment",		B_INT32_TYPE,	&fAlignment }
	};

	for (int i = 0; i < sizeof(attrEntries) / sizeof(attrEntries[0]); i++) {
		switch (attrEntries[i].type) {
			case B_STRING_TYPE:
				fStatus = msg.FindString(attrEntries[i].name, index,
								(BString*)attrEntries[i].data);
				break;
			case B_BOOL_TYPE:
				fStatus = msg.FindBool(attrEntries[i].name, index,
								(bool*)attrEntries[i].data);
				break;
			case B_INT32_TYPE:
				fStatus = msg.FindInt32(attrEntries[i].name, index,
								(int32*)attrEntries[i].data);
				break;
		}

		if (fStatus != B_OK)
			return;
	}

	fPublicName.CharacterEscape(rawPublicName, "\'", '\\');
}


MimeAttribute::MimeAttribute(TUserArgs& args)
{
	SyncWith(args);
}


void
MimeAttribute::SyncWith(TUserArgs& args)
{
	const char* value = UserArgValue(args, kAttribute);
	if (value != NULL)
		fName.SetTo(value, B_MIME_TYPE_LENGTH);

	value = UserArgValue(args, kAttrName);
	if (value != NULL)
		fPublicName.SetTo(value, B_MIME_TYPE_LENGTH);

	value = UserArgValue(args, kAttrType);
	if (value != NULL) {
		fType = 0;
		for (int i = 0; i < 4 && value[i] != '\0'; i++) {
			fType <<= 8;
			fType |= (value[i] != '\0' ? value[i] : ' ');
		}

		fType = B_LENDIAN_TO_HOST_INT32(fType);
	}

	value = UserArgValue(args, kAttrWidth);
	if (value != NULL)
		fWidth = atoi(value);

	value = UserArgValue(args, kAttrAlignment);
	if (value != NULL) {
		if (strcasecmp(value, "right") == 0) {
			fAlignment = B_ALIGN_RIGHT;
		} else if (strcasecmp(value, "left") == 0) {
			fAlignment = B_ALIGN_LEFT;
		} else if (strcasecmp(value, "center") == 0) {
			fAlignment = B_ALIGN_CENTER;
		} else
			fAlignment = atoi(value);
	}

	value = UserArgValue(args, kAttrViewable);
	if (value != NULL)
		fViewable = atoi(value) != 0;

	value = UserArgValue(args, kAttrEditable);
	if (value != NULL)
		fEditable = atoi(value) != 0;

	value = UserArgValue(args, kAttrExtra);
	if (value != NULL)
		fExtra = atoi(value) != 0;
}


void
MimeAttribute::Dump()
{
	uint32 type = B_HOST_TO_LENDIAN_INT32(fType);
	const char* alignment = fAlignment == B_ALIGN_RIGHT ? "right"
					: (fAlignment == B_ALIGN_LEFT ? "left" : "center");

	cout << " \\" << endl << "\t" << kAttribute << " \"" << fName << "\" "
				<< kAttrName << " \"" << fPublicName << "\"";

	cout << " \\" << endl << "\t\t" << kAttrType << " '"
				<< (char)((type >> 24) & 0xFF) << (char)((type >> 16) & 0xFF)
				<< (char)((type >> 8) & 0xFF) << (char)(type & 0xFF) << "' "
			<< " " << kAttrWidth << " " << fWidth
			<< " " << kAttrAlignment << " " << alignment;

	cout << " \\" << endl << "\t\t" << kAttrViewable << " " << fViewable
			<< " " << kAttrEditable << " " << fEditable
			<< " " << kAttrExtra << " " << fExtra;
}


void
MimeAttribute::StoreInto(BMessage* target)
{
	struct attrEntry {
		const char* name;
		type_code	type;
		const void*	data;
	} attrEntries[] = {
		{ "attr:name",			B_STRING_TYPE,	fName.String() },
		{ "attr:public_name",	B_STRING_TYPE,	fPublicName.String() },
		{ "attr:type",			B_INT32_TYPE,	&fType },
		{ "attr:viewable",		B_BOOL_TYPE,	&fViewable },
		{ "attr:editable",		B_BOOL_TYPE,	&fEditable },
		{ "attr:extra",			B_BOOL_TYPE,	&fExtra },
		{ "attr:width",			B_INT32_TYPE,	&fWidth },
		{ "attr:alignment",		B_INT32_TYPE,	&fAlignment }
	};

	for (int i = 0; i < sizeof(attrEntries) / sizeof(attrEntries[0]); i++) {
		switch (attrEntries[i].type) {
			case B_STRING_TYPE:
				fStatus = target->AddString(attrEntries[i].name,
								(const char*)attrEntries[i].data);
				break;
			case B_BOOL_TYPE:
				fStatus = target->AddBool(attrEntries[i].name,
								(bool*)attrEntries[i].data);
				break;
			case B_INT32_TYPE:
				fStatus = target->AddInt32(attrEntries[i].name,
								*(int32*)attrEntries[i].data);
				break;
		}

		if (fStatus != B_OK)
			return;
	}
}


const char*
MimeAttribute::UserArgValue(TUserArgs& map, const char* name)
{
	TUserArgsI i = map.find(hash(name));
	if (i == map.end())
		return NULL;
	return i->second != NULL ? i->second : "";
}


// #pragma mark -

// the work-horse of the app - the class encapsulates extended info readed
// from the mime type and do all edit and dump operations
//
class MimeType : public BMimeType {

public:
	class Error : public std::exception
	{
				BString		fWhat;
	public:
							Error(const char* what, ...);
		virtual				~Error() throw() {}
		virtual const char*	what() const throw() { return fWhat.String(); }
	};

					MimeType(char** argv) throw (Error);
					~MimeType();

	void			Process() throw (Error);

private:
	status_t		_InitCheck();
	void			_SetTo(const char* mimetype) throw (Error);
	void			_PurgeProperties();
	void			_Init(char** argv) throw (Error);
	void			_Dump(const char* mimetype) throw (Error);
	void			_DoEdit() throw (Error);
	const char*		_UserArgValue(const char* name);

	status_t		fStatus;
	const char*		fToolName;

	// configurable MimeType properties
	BString			fShort;
	BString			fLong;
	BString			fPrefApp;
	BString			fPrefAppSig;
	BString			fSniffRule;
	BBitmap*		fSmallIcon;
	BBitmap*		fBigIcon;

	map<uint32, BString>		fExtensions;
	map<uint32, MimeAttribute>	fAttributes;

	// user provided arguments
	TUserArgs		fUserArguments;
	TUserAttrs		fUserAttributes;

	// operation mode switches and flags
	uint32			fOpMode;
	bool			fDumpNormal;
	bool			fDumpRule;
	bool			fDumpIcon;
	bool			fDumpAll;
	bool			fDoAdd;
	bool			fDoSet;
	bool			fDoForce;
	bool			fDoRemove;
	bool			fCheckSniffRule;
};


MimeType::Error::Error(const char* what, ...)
{
	const int size = 1024;
	va_list args;
	va_start(args, what);
	vsnprintf(fWhat.LockBuffer(size), size, what, args);
	fWhat.UnlockBuffer();
	va_end(args);
}


MimeType::MimeType(char** argv) throw (Error)
		:
		fStatus(B_NO_INIT),
		fToolName(argv[0]),
		fSmallIcon(NULL),
		fBigIcon(NULL),
		fOpMode(kOpModeUndefined),
		fDumpNormal(false),
		fDumpRule(false),
		fDumpIcon(false),
		fDumpAll(false),
		fDoAdd(false),
		fDoSet(false),
		fDoForce(false),
		fDoRemove(false),
		fCheckSniffRule(false)
{
	_Init(++argv);
}


MimeType::~MimeType()
{
	delete fSmallIcon;
	delete fBigIcon;
}


void
MimeType::_Init(char** argv) throw (Error)
{
	// fill the helper map of options - for quick lookup of arguments
	map<uint32, const CmdOption*> cmdOptionsMap;
	for (size_t i = 0; i < sizeof(gCmdOptions) / sizeof(gCmdOptions[0]); i++)
		cmdOptionsMap.insert(pair<uint32, CmdOption*>(
							hash(gCmdOptions[i].fName), &gCmdOptions[i]));

	// parse the command line arguments
	for (char** arg = argv; *arg; arg++) {
		// non-option arguments are assumed as signature
		if (**arg != '-') {
			if (Type() != NULL)
				throw Error("mime signature already specified: '%s'", Type());

			status_t s = SetTo(*arg);
			continue;
		}

		// check op.modes, options and attribs
		uint32 key = hash(*arg);

		map<uint32, const CmdOption*>::iterator I = cmdOptionsMap.find(key);
		if (I == cmdOptionsMap.end())
			throw Error("unknown option '%s'", *arg);

		switch (I->second->fType) {
			case CmdOption::kHelp:
				cerr << kUsageMessage;
				throw Error(kHelpMessage);

			case CmdOption::kMode:
				// op.modes are exclusive - no simultaneous possible
				if (fOpMode != kOpModeUndefined)
					throw Error(kWrongModeMessage);
				fOpMode = key;

				if (I->second->fType != hash(kCheckSniffRule))
					break;
				// else -> fallthrough, CheckRule works both as mode and Option
			case CmdOption::kOption:
				{
					const char* name = *arg;
					const char* param = NULL;
					if (I->second->fNeedArg) {
						if (!*++arg)
							throw Error("argument required for '%s'", name);
						param = *arg;
					}

					TUserArgsI A = fUserArguments.find(key);
					if (A != fUserArguments.end() && !I->second->fNonExclusive)
						throw Error("option '%s' already specified", name);

					fUserArguments.insert(
							pair<uint32, const char*>(key, param));
				}
				break;

			case CmdOption::kAttrRoot:
				if (!*++arg)
					throw Error("attribute name should be specified");

				fUserAttributes.resize(fUserAttributes.size() + 1);
				fUserAttributes.back().insert(
							pair<uint32, const char*>(key, *arg));
				break;

			case CmdOption::kAttrib:
				{
					const char* name = *arg;
					if (fUserAttributes.size() <= 0)
						throw Error("'%s' allowed only after the '%s' <name>",
								name, kAttribute);

					if (!*++arg)
						throw Error("'%s', argument should be specified", name);

					TUserArgsI A = fUserAttributes.back().find(key);
					if (A != fUserAttributes.back().end())
						throw Error("'%s' for attribute '%s' already specified",
								name, A->second);

					fUserAttributes.back().insert(
							pair<uint32, const char*>(key, *arg));
				}
				break;

			default:
				throw Error("internal error. wrong mode: %d", I->second->fType);
		}
	}

	// check some mutual exclusive conditions
	if (fOpMode == kOpModeUndefined)
		throw Error(kNeedArgMessage);

	if (Type() != NULL && InitCheck() != B_OK)
		throw Error("error instantiating mime for '%s': %s",
							Type(), strerror(InitCheck()));

	fDoAdd = fOpMode == hash(kAdd);
	fDoSet = fOpMode == hash(kSet);
	fDoForce = fOpMode == hash(kForce);
	fDoRemove = fOpMode == hash(kRemove);
	fDumpNormal = fOpMode == hash(kDump);
	fDumpRule = fOpMode == hash(kDumpSniffRule);
	fDumpIcon = fOpMode == hash(kDumpIcon);
	fDumpAll = fOpMode == hash(kDumpAll);
	fCheckSniffRule = fOpMode == hash(kCheckSniffRule);

	if (fDoAdd || fDoSet || fDoForce || fDoRemove) {
		if (Type() == NULL)
			throw Error("signature should be specified");

		if (!IsValid())
			throw Error("mime for '%s' is not valid", Type());

	} else if (fDumpNormal || fDumpRule || fDumpIcon || fDumpAll) {
		if (Type() != NULL) {
			if (!IsValid())
				throw Error("mime for '%s' is not valid", Type());

			if (!IsInstalled())
				throw Error("mime for '%s' is not installed", Type());
		}
	}

	// finally force to load mime-specific fileds
	_SetTo(Type());
}


status_t
MimeType::_InitCheck()
{
	return fStatus != B_OK ? fStatus : BMimeType::InitCheck();
}


void
MimeType::_PurgeProperties()
{
	fShort.Truncate(0);
	fLong.Truncate(0);
	fPrefApp.Truncate(0);
	fPrefAppSig.Truncate(0);
	fSniffRule.Truncate(0);

	delete fSmallIcon;
	fSmallIcon = NULL;

	delete fBigIcon;
	fBigIcon = NULL;

	fExtensions.clear();
	fAttributes.clear();
}


void
MimeType::_SetTo(const char* mimetype) throw (Error)
{
	if (mimetype == NULL)
		return; // iterate all types - nothing to load ATM

	if (BMimeType::SetTo(mimetype) != B_OK)
		throw Error("failed to set mimetype to '%s'", mimetype);

	_PurgeProperties();

	char buffer[B_MIME_TYPE_LENGTH] = { 0 };
	if (GetShortDescription(buffer) == B_OK)
		fShort.SetTo(buffer, B_MIME_TYPE_LENGTH);

	if (GetLongDescription(buffer) == B_OK)
		fLong.SetTo(buffer, B_MIME_TYPE_LENGTH);

	entry_ref ref;
	if (GetAppHint(&ref) == B_OK) {
		BPath path(&ref);
		fPrefApp.SetTo(path.Path(), B_MIME_TYPE_LENGTH);
	}

	if (GetPreferredApp(buffer, B_OPEN) == B_OK)
		fPrefAppSig.SetTo(buffer, B_MIME_TYPE_LENGTH);

	BString rule;
	if (GetSnifferRule(&rule) == B_OK)
		fSniffRule.CharacterEscape(rule.String(), "\'", '\\');

	BMessage exts;
	fExtensions.clear();
	if (GetFileExtensions(&exts) == B_OK) {
		uint32 i = 0;
		const char* ext = NULL;
		while (exts.FindString("extensions", i++, &ext) == B_OK)
			fExtensions.insert(pair<uint32, BString>(hash(ext), ext));
	}

	BMessage attrs;
	fAttributes.clear();
	if (GetAttrInfo(&attrs) == B_OK) {
		for (int index = 0; ; index++) {
			MimeAttribute attr(attrs, index);
			if (attr.InitCheck() != B_OK)
				break;

			fAttributes.insert(
					pair<uint32, MimeAttribute>(hash(attr.fName), attr));
		}
	}

	fSmallIcon = new BBitmap(BRect(0, 0, 15, 15), B_COLOR_8_BIT);
	if (GetIcon(fSmallIcon, B_MINI_ICON) != B_OK) {
		delete fSmallIcon;
		fSmallIcon = NULL;
	}

	fBigIcon = new BBitmap(BRect(0, 0, 31, 31), B_COLOR_8_BIT);
	if (GetIcon(fBigIcon, B_LARGE_ICON) != B_OK) {
		delete fBigIcon;
		fBigIcon = NULL;
	}
}


const char*
MimeType::_UserArgValue(const char* name)
{
	TUserArgsI i = fUserArguments.find(hash(name));
	if (i == fUserArguments.end())
		return NULL;

	return i->second != NULL ? i->second : "";
}


void
MimeType::_Dump(const char* mimetype) throw (Error)
{
	// _Dump can be called as part of all types iteration - so set to required
	if (Type() == NULL || strcasecmp(Type(), mimetype) != 0)
		_SetTo(mimetype);

	// apps have themself as preferred app - use it to handle
	// -includeApps option - do not dump applications info
	if (!fPrefApp.IsEmpty()
		&& fPrefApp.ICompare(mimetype) == 0
		&& _UserArgValue(kIncludeApps) == NULL)
			return;

	cout << fToolName << " -set " << mimetype;

	if (fDumpNormal || fDumpAll) {
		if (!fShort.IsEmpty())
			cout << " " << kShort << " \"" << fShort << "\"";
		if (!fLong.IsEmpty())
			cout << " " << kLong << " \"" << fLong << "\"";
		if (!fPrefApp.IsEmpty())
			cout << " " << kPreferredApp << " " << fPrefApp;
		if (!fPrefAppSig.IsEmpty())
			cout << " " << kPreferredAppSig << " " << fPrefAppSig;
	}

	if (!fDumpIcon && !fSniffRule.IsEmpty())
		cout << " " << kSniffRule << " '" << fSniffRule << "'";

	if (fDumpNormal || fDumpAll)
		for (map<uint32, BString>::iterator i = fExtensions.begin();
				i != fExtensions.end(); i++)
			cout << " " << kExtension << " " << i->second;

	if (fDumpAll)
		for (map<uint32, MimeAttribute>::iterator i = fAttributes.begin();
				i != fAttributes.end(); i++)
			i->second.Dump();

	if (fDumpIcon || fDumpAll)
		cout << " \\" << endl << "Icon dumps are not yet implemented!";

	cout << endl;
}


void
MimeType::_DoEdit() throw (Error)
{
	if (fDoRemove || fDoForce) {
		status_t result = Delete();
		if (result != B_OK)
			throw Error(strerror(result), result);

		if (fDoRemove)
			return;

		_PurgeProperties();
	}

	if (!IsInstalled() && Install() != B_OK)
		throw Error("could not install mimetype '%s'", Type());

	const char* value = _UserArgValue(kShort);
	if (value != NULL && (!fDoAdd || fShort.IsEmpty()))
		if (SetShortDescription(value) != B_OK)
			throw Error("cannot set %s to %s for '%s'", kShort, value, Type());

	value = _UserArgValue(kLong);
	if (value != NULL && (!fDoAdd || fLong.IsEmpty()))
		if (SetLongDescription(value) != B_OK)
			throw Error("cannot set %s to %s for '%s'", kLong, value, Type());

	value = _UserArgValue(kPreferredApp);
	if (value != NULL && (!fDoAdd || fPrefApp.IsEmpty())) {
		entry_ref appHint;
		if (get_ref_for_path(value, &appHint) != B_OK)
			throw Error("%s ref_entry for '%s' couldn't be found for '%s'",
						kPreferredApp, value, Type());

		if (SetAppHint(&appHint) != B_OK)
			throw Error("cannot set %s to %s for '%s'",
					kPreferredApp, value, Type());
	}

	value = _UserArgValue(kPreferredAppSig);
	if (value != NULL && (!fDoAdd || fPrefAppSig.IsEmpty()))
		if (SetPreferredApp(value) != B_OK)
			throw Error("cannot set %s to %s for '%s'",
					kPreferredAppSig, value, Type());

	value = _UserArgValue(kSniffRule);
	if (value != NULL && (!fDoAdd || fSniffRule.IsEmpty()))
		if (SetSnifferRule(value) != B_OK)
			throw Error("cannot set %s to %s for '%s'",
					kSniffRule, value, Type());

	// handle extensions update
	pair<TUserArgsI, TUserArgsI> exts
							= fUserArguments.equal_range(hash(kExtension));
	for (TUserArgsI i = exts.first; i != exts.second; i++) {
		uint32 key = hash(i->second);
		if (fExtensions.find(key) == fExtensions.end())
			fExtensions.insert(pair<uint32, BString>(key, i->second));
	}

	if (exts.first != exts.second) {
		BMessage msg;
		for (map<uint32, BString>::iterator i = fExtensions.begin();
				i != fExtensions.end(); i++)
			if (msg.AddString("extensions", i->second.String()) != B_OK)
				throw Error("extension '%s' couldn't be added",
					i->second.String());

		if (SetFileExtensions(&msg) != B_OK)
			throw Error("set file extensions failed");
	}

	// take care about attribute trees
	for (TUserAttrsI userAttr = fUserAttributes.begin();
			userAttr != fUserAttributes.end(); userAttr++ )
	{
		// search for -attribute "name" in args map
		TUserArgsI attrArgs = userAttr->find(hash(kAttribute));
		if (attrArgs == userAttr->end())
			throw Error("internal error: %s arg not found", kAttribute);

		// check if we already have this attribute cached
		map<uint32, MimeAttribute>::iterator
								attr = fAttributes.find(hash(attrArgs->second));
		if (attr == fAttributes.end()) {
			// add new one
			MimeAttribute mimeAttr(*userAttr);
			fAttributes.insert(
				pair<uint32, MimeAttribute>(hash(mimeAttr.fName), mimeAttr));
		} else if (!fDoAdd)
			attr->second.SyncWith(*userAttr);
	}

	if (fAttributes.size() > 0) {
		BMessage msg;
		for (map<uint32, MimeAttribute>::iterator i = fAttributes.begin();
				i != fAttributes.end(); i++)
		{
			i->second.StoreInto(&msg);
			if (i->second.InitCheck() != B_OK)
				throw Error("storing attributes in message failed");
		}

		if (SetAttrInfo(&msg) != B_OK)
			throw Error("set mimetype attributes failed");
	}
}


void
MimeType::Process() throw (Error)
{
	if (fCheckSniffRule) {
		TUserArgsI I = fUserArguments.find(fOpMode);
		if (I == fUserArguments.end())
			throw Error("sniffer rule is empty");

		BString error;
		status_t result = BMimeType::CheckSnifferRule(I->second, &error);
		cerr <<  (result == B_OK ?
					"sniffer rule is correct" : error.String()) << endl;
		return;
	}

	if (fDoAdd || fDoSet || fDoForce || fDoRemove) {
		_DoEdit();
		return;
	}

	if (fDumpNormal || fDumpRule || fDumpIcon || fDumpAll) {
		if (Type() != NULL) {
			_Dump(Type());
			return;
		}

		BMessage superTypes;
		int32 superCount = 0;
		type_code type = B_INT32_TYPE;
		if (BMimeType::GetInstalledSupertypes(&superTypes) != B_OK
			|| superTypes.GetInfo("super_types", &type, &superCount) != B_OK)
			throw Error("super types enumeration failed");

		for (int32 si = 0; si < superCount; si++) {
			const char* superName = NULL;
			if (superTypes.FindString("super_types", si, &superName) != B_OK)
				throw Error("name for supertype #%d not found", si);

			BMessage types;
			if (BMimeType::GetInstalledTypes(superName, &types) != B_OK)
				throw Error("mimetypes of supertype '%s' not found", superName);

			int32 count = 0;
			if (types.GetInfo("types", &type, &count) != B_OK)
				continue; // no sub-types?

			for (int32 i = 0; i < count; i++) {
				const char* name = NULL;
				if (types.FindString("types", i, &name) != B_OK)
					throw Error("name for type %s/#%d not found", superName, i);

				_Dump(name);
			}
		}
	}
}


int
main(int argc, char** argv)
{
	// AppServer link is required to work with bitmaps
	BApplication app("application/x-vnd.haiku.setmime");

	try {

		if (argc < 2)
			throw MimeType::Error(kNeedArgMessage);

		MimeType mimetype(argv);

		mimetype.Process();

	} catch(exception& exc) {
		cerr << argv[0] << " : " << exc.what() << endl;
		cerr <<	kUsageMessage;
		return B_ERROR;
	}

	return B_OK;
}

