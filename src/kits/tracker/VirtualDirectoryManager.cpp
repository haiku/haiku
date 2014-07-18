/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include "VirtualDirectoryManager.h"

#include <errno.h>

#include <new>

#include <Directory.h>
#include <File.h>
#include <StringList.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <DriverSettings.h>
#include <NotOwningEntryRef.h>
#include <Uuid.h>

#include "MimeTypes.h"
#include "Model.h"


namespace BPrivate {

static const size_t kMaxVirtualDirectoryFileSize = 10 * 1024;
static const char* const kTemporaryDefinitionFileBaseDirectoryPath
	= "/tmp/tracker/virtual-directories";


// #pragma mark - VirtualDirectoryManager::Info


class VirtualDirectoryManager::Info {
private:
	typedef BObjectList<Info> InfoList;

public:
	Info(RootInfo* root, Info* parent, const BString& path,
		const node_ref& definitionFileNodeRef,
		const entry_ref& definitionFileEntryRef)
		:
		fRoot(root),
		fParent(parent),
		fPath(path),
		fDefinitionFileNodeRef(definitionFileNodeRef),
		fDefinitionFileEntryRef(definitionFileEntryRef),
		fId(),
		fChildDefinitionsDirectoryRef(-1, -1),
		fChildren(10, true)
	{
	}

	~Info()
	{
	}

	RootInfo* Root() const
	{
		return fRoot;
	}

	Info* Parent() const
	{
		return fParent;
	}

	const char* Name() const
	{
		return fDefinitionFileEntryRef.name;
	}

	const BString& Path() const
	{
		return fPath;
	}

	const node_ref& DefinitionFileNodeRef() const
	{
		return fDefinitionFileNodeRef;
	}

	const entry_ref& DefinitionFileEntryRef() const
	{
		return fDefinitionFileEntryRef;
	}

	const InfoList& Children() const
	{
		return fChildren;
	}

	const BString& Id() const
	{
		return fId;
	}

	void SetId(const BString& id)
	{
		fId = id;
	}

	const node_ref& ChildDefinitionsDirectoryRef() const
	{
		return fChildDefinitionsDirectoryRef;
	}

	void SetChildDefinitionsDirectoryRef(const node_ref& ref)
	{
		fChildDefinitionsDirectoryRef = ref;
	}

	Info* GetChild(const char* name) const
	{
		for (int32 i = 0; Info* child = fChildren.ItemAt(i); i++) {
			if (strcmp(name, child->Name()) == 0)
				return child;
		}
		return NULL;
	}

	Info* CreateChild(const node_ref& definitionFileNodeRef,
		const entry_ref& definitionFileEntryRef)
	{
		BString path;
		if (fPath.IsEmpty()) {
			path = definitionFileEntryRef.name;
		} else {
			path.SetToFormat("%s/%s", fPath.String(),
				definitionFileEntryRef.name);
		}
		if (path.IsEmpty())
			return NULL;

		Info* info = new(std::nothrow) Info(fRoot, this, path,
			definitionFileNodeRef, definitionFileEntryRef);
		if (info == NULL || !fChildren.AddItem(info)) {
			delete info;
			return NULL;
		}
		return info;
	}

	bool DeleteChild(Info* info)
	{
		return fChildren.RemoveItem(info, true);
	}

	void DeleteChildAt(int32 index)
	{
		delete fChildren.RemoveItemAt(index);
	}

private:
	RootInfo*	fRoot;
	Info*		fParent;
	BString		fPath;
	node_ref	fDefinitionFileNodeRef;
	entry_ref	fDefinitionFileEntryRef;
	BString		fId;
	node_ref	fChildDefinitionsDirectoryRef;
	InfoList	fChildren;
};


// #pragma mark - VirtualDirectoryManager::RootInfo


class VirtualDirectoryManager::RootInfo {
public:
	RootInfo(const node_ref& definitionFileNodeRef,
		const entry_ref& definitionFileEntryRef)
		:
		fDirectoryPaths(),
		fInfo(new(std::nothrow) VirtualDirectoryManager::Info(this, NULL,
				BString(), definitionFileNodeRef, definitionFileEntryRef)),
		fFileTime(-1),
		fLastChangeTime(-1)
	{
	}

	~RootInfo()
	{
		delete fInfo;
	}

	status_t InitCheck() const
	{
		return fInfo != NULL ? B_OK : B_NO_MEMORY;
	}

	bigtime_t FileTime() const
	{
		return fFileTime;
	}

	bigtime_t LastChangeTime() const
	{
		return fLastChangeTime;
	}

	const BStringList& DirectoryPaths() const
	{
		return fDirectoryPaths;
	}

	status_t ReadDefinition(bool* _changed = NULL)
	{
		// open the definition file
		BFile file;
		status_t error = file.SetTo(&fInfo->DefinitionFileEntryRef(),
			B_READ_ONLY);
		if (error != B_OK)
			return error;

		struct stat st;
		error = file.GetStat(&st);
		if (error != B_OK)
			return error;

		bigtime_t fileTime = st.st_mtim.tv_sec;
		fileTime *= 1000000;
		fileTime += st.st_mtim.tv_nsec / 1000;
		if (fileTime == fFileTime) {
			if (_changed != NULL)
				*_changed = false;

			return B_OK;
		}

		if (node_ref(st.st_dev, st.st_ino) != fInfo->DefinitionFileNodeRef())
			return B_ENTRY_NOT_FOUND;

		// read the contents
		off_t fileSize = st.st_size;
		if (fileSize > (off_t)kMaxVirtualDirectoryFileSize)
			return B_BAD_VALUE;

		char* buffer = new(std::nothrow) char[fileSize + 1];
		if (buffer == NULL)
			return B_NO_MEMORY;
		ArrayDeleter<char> bufferDeleter(buffer);

		ssize_t bytesRead = file.ReadAt(0, buffer, fileSize);
		if (bytesRead < 0)
			return bytesRead;

		buffer[bytesRead] = '\0';

		// parse it
		BStringList oldDirectoryPaths(fDirectoryPaths);
		fDirectoryPaths.MakeEmpty();

		BDriverSettings driverSettings;
		error = driverSettings.SetToString(buffer);
		if (error != B_OK)
			return error;

		BDriverParameterIterator it
			= driverSettings.ParameterIterator("directory");
		while (it.HasNext()) {
			BDriverParameter parameter = it.Next();
			for (int32 i = 0; i < parameter.CountValues(); i++)
				fDirectoryPaths.Add(parameter.ValueAt(i));
		}

		// update file time and check whether something has changed
		fFileTime = fileTime;

		bool changed = fDirectoryPaths != oldDirectoryPaths;
		if (changed || fLastChangeTime < 0)
			fLastChangeTime = fFileTime;

		if (_changed != NULL)
			*_changed = changed;

		return B_OK;
	}

	VirtualDirectoryManager::Info* Info() const
	{
		return fInfo;
	}

private:
	typedef std::map<BString, VirtualDirectoryManager::Info*> InfoMap;

private:
	BStringList						fDirectoryPaths;
	VirtualDirectoryManager::Info*	fInfo;
	bigtime_t						fFileTime;
										// actual file modified time
	bigtime_t						fLastChangeTime;
										// last time something actually changed
};


// #pragma mark - VirtualDirectoryManager


VirtualDirectoryManager::VirtualDirectoryManager()
	:
	fLock("virtual directory manager")
{
}


/*static*/ VirtualDirectoryManager*
VirtualDirectoryManager::Instance()
{
	static VirtualDirectoryManager* manager
		= new(std::nothrow) VirtualDirectoryManager;
	return manager;
}


status_t
VirtualDirectoryManager::ResolveDirectoryPaths(
	const node_ref& definitionFileNodeRef,
	const entry_ref& definitionFileEntryRef, BStringList& _directoryPaths,
	node_ref* _definitionFileNodeRef, entry_ref* _definitionFileEntryRef)
{
	Info* info = _InfoForNodeRef(definitionFileNodeRef);
	if (info == NULL) {
		status_t error = _ResolveUnknownDefinitionFile(definitionFileNodeRef,
			definitionFileEntryRef, info);
		if (error != B_OK)
			return error;
	}

	const BString& subDirectory = info->Path();
	const BStringList& rootDirectoryPaths = info->Root()->DirectoryPaths();
	if (subDirectory.IsEmpty()) {
		_directoryPaths = rootDirectoryPaths;
	} else {
		_directoryPaths.MakeEmpty();
		int32 count = rootDirectoryPaths.CountStrings();
		for (int32 i = 0; i < count; i++) {
			BString path = rootDirectoryPaths.StringAt(i);
			_directoryPaths.Add(path << '/' << subDirectory);
		}
	}

	if (_definitionFileEntryRef != NULL) {
		*_definitionFileEntryRef = info->DefinitionFileEntryRef();
		if (_definitionFileEntryRef->name == NULL)
			return B_NO_MEMORY;
	}

	if (_definitionFileNodeRef != NULL)
		*_definitionFileNodeRef = info->DefinitionFileNodeRef();

	return B_OK;
}


bool
VirtualDirectoryManager::GetDefinitionFileChangeTime(
	const node_ref& definitionFileRef, bigtime_t& _time) const
{
	Info* info = _InfoForNodeRef(definitionFileRef);
	if (info == NULL)
		return false;

	_time = info->Root()->LastChangeTime();
	return true;
}


bool
VirtualDirectoryManager::GetRootDefinitionFile(
	const node_ref& definitionFileRef, node_ref& _rootDefinitionFileRef)
{
	Info* info = _InfoForNodeRef(definitionFileRef);
	if (info == NULL)
		return false;

	_rootDefinitionFileRef = info->Root()->Info()->DefinitionFileNodeRef();
	return true;
}


bool
VirtualDirectoryManager::GetSubDirectoryDefinitionFile(
	const node_ref& baseDefinitionRef, const char* subDirName,
	entry_ref& _entryRef, node_ref& _nodeRef)
{
	Info* parentInfo = _InfoForNodeRef(baseDefinitionRef);
	if (parentInfo == NULL)
		return false;

	Info* info = parentInfo->GetChild(subDirName);
	if (info == NULL)
		return false;

	_entryRef = info->DefinitionFileEntryRef();
	_nodeRef = info->DefinitionFileNodeRef();
	return _entryRef.name != NULL;
}


bool
VirtualDirectoryManager::GetParentDirectoryDefinitionFile(
	const node_ref& subDirDefinitionRef, entry_ref& _entryRef,
	node_ref& _nodeRef)
{
	Info* info = _InfoForNodeRef(subDirDefinitionRef);
	if (info == NULL)
		return false;

	Info* parentInfo = info->Parent();
	if (parentInfo == NULL)
		return false;

	_entryRef = parentInfo->DefinitionFileEntryRef();
	_nodeRef = parentInfo->DefinitionFileNodeRef();
	return _entryRef.name != NULL;
}


status_t
VirtualDirectoryManager::TranslateDirectoryEntry(
	const node_ref& definitionFileRef, dirent* buffer)
{
	NotOwningEntryRef entryRef(buffer->d_pdev, buffer->d_pino, buffer->d_name);
	node_ref nodeRef(buffer->d_dev, buffer->d_ino);

	status_t result = TranslateDirectoryEntry(definitionFileRef, entryRef,
		nodeRef);
	if (result != B_OK)
		return result;

	buffer->d_pdev = entryRef.device;
	buffer->d_pino = entryRef.directory;
	buffer->d_dev = nodeRef.device;
	buffer->d_ino = nodeRef.node;

	return B_OK;
}


status_t
VirtualDirectoryManager::TranslateDirectoryEntry(
	const node_ref& definitionFileRef, entry_ref& _entryRef, node_ref& _nodeRef)
{
	Info* parentInfo = _InfoForNodeRef(definitionFileRef);
	if (parentInfo == NULL)
		return B_BAD_VALUE;

	// get the info for the entry
	Info* info = parentInfo->GetChild(_entryRef.name);
	if (info == NULL) {
		// If not done yet, create a directory for the parent, where we can
		// place the new definition file.
		if (parentInfo->Id().IsEmpty()) {
			BString id = BUuid().SetToRandom().ToString();
			if (id.IsEmpty())
				return B_NO_MEMORY;

			BPath path(kTemporaryDefinitionFileBaseDirectoryPath, id);
			status_t error = path.InitCheck();
			if (error != B_OK)
				return error;

			error = create_directory(path.Path(),
				S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
			if (error != B_OK)
				return error;

			struct stat st;
			if (lstat(path.Path(), &st) != 0)
				return errno;

			parentInfo->SetId(id);
			parentInfo->SetChildDefinitionsDirectoryRef(
				node_ref(st.st_dev, st.st_ino));
		}

		// create the definition file
		const node_ref& directoryRef
			= parentInfo->ChildDefinitionsDirectoryRef();
		NotOwningEntryRef entryRef(directoryRef, _entryRef.name);

		BFile definitionFile;
		status_t error = definitionFile.SetTo(&entryRef,
			B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
		if (error != B_OK)
			return error;

		node_ref nodeRef;
		error = definitionFile.GetNodeRef(&nodeRef);
		if (error != B_OK)
			return error;

		BNodeInfo nodeInfo(&definitionFile);
		error = nodeInfo.SetType(kVirtualDirectoryMimeType);
		if (error != B_OK)
			return error;

		// create the info
		info = parentInfo->CreateChild(nodeRef, entryRef);
		if (info == NULL || !_AddInfo(info))
			return B_NO_MEMORY;

		// Write some info into the definition file that helps us to find the
		// root definition file. This is only necessary when definition file
		// entry refs are transferred between applications. Then the receiving
		// application may need to find the root definition file and resolve
		// the subdirectories.
		const entry_ref& rootEntryRef
			= parentInfo->Root()->Info()->DefinitionFileEntryRef();
		BString definitionFileContent;
		definitionFileContent.SetToFormat(
			"root {\n"
			"  device %" B_PRIdDEV "\n"
			"  directory %" B_PRIdINO "\n"
			"  name \"%s\"\n"
			"}\n"
			"subdir \"%s\"\n",
			rootEntryRef.device, rootEntryRef.directory, rootEntryRef.name,
			info->Path().String());
		// failure is not nice, but not mission critical for this application
		if (!definitionFileContent.IsEmpty()) {
			definitionFile.WriteAt(0,
				definitionFileContent.String(), definitionFileContent.Length());
		}
	}

	const entry_ref& entryRef = info->DefinitionFileEntryRef();
	_nodeRef = info->DefinitionFileNodeRef();
	_entryRef.device = entryRef.device;
	_entryRef.directory = entryRef.directory;

	return B_OK;
}


bool
VirtualDirectoryManager::DefinitionFileChanged(
	const node_ref& definitionFileRef)
{
	Info* info = _InfoForNodeRef(definitionFileRef);
	if (info == NULL)
		return false;

	_UpdateTree(info->Root());

	return _InfoForNodeRef(definitionFileRef) != NULL;
}


status_t
VirtualDirectoryManager::DirectoryRemoved(const node_ref& definitionFileRef)
{
	Info* info = _InfoForNodeRef(definitionFileRef);
	if (info == NULL)
		return B_ENTRY_NOT_FOUND;

	_RemoveDirectory(info);

	// delete the info
	if (info->Parent() == NULL)
		delete info->Root();
	else
		info->Parent()->DeleteChild(info);

	return B_OK;
}


/*static*/ bool
VirtualDirectoryManager::GetEntry(const BStringList& directoryPaths,
	const char* name, entry_ref* _ref, struct stat* _st)
{
	int32 count = directoryPaths.CountStrings();
	for (int32 i = 0; i < count; i++) {
		BPath path;
		if (path.SetTo(directoryPaths.StringAt(i), name) != B_OK)
			continue;

		struct stat st;
		if (lstat(path.Path(), &st) == 0) {
			if (_ref != NULL) {
				if (get_ref_for_path(path.Path(), _ref) != B_OK)
					return false;
			}
			if (_st != NULL)
				*_st = st;
			return true;
		}
	}

	return false;
}


VirtualDirectoryManager::Info*
VirtualDirectoryManager::_InfoForNodeRef(const node_ref& nodeRef) const
{
	NodeRefInfoMap::const_iterator it = fInfos.find(nodeRef);
	return it != fInfos.end() ? it->second : NULL;
}


bool
VirtualDirectoryManager::_AddInfo(Info* info)
{
	try {
		fInfos[info->DefinitionFileNodeRef()] = info;
		return true;
	} catch (...) {
		return false;
	}
}


void
VirtualDirectoryManager::_RemoveInfo(Info* info)
{
	NodeRefInfoMap::iterator it = fInfos.find(info->DefinitionFileNodeRef());
	if (it != fInfos.end())
		fInfos.erase(it);
}


void
VirtualDirectoryManager::_UpdateTree(RootInfo* root)
{
	bool changed = false;
	status_t result = root->ReadDefinition(&changed);
	if (result != B_OK) {
		DirectoryRemoved(root->Info()->DefinitionFileNodeRef());
		return;
	}

	if (!changed)
		return;

	_UpdateTree(root->Info());
}


void
VirtualDirectoryManager::_UpdateTree(Info* info)
{
	const BStringList& directoryPaths = info->Root()->DirectoryPaths();

	int32 childCount = info->Children().CountItems();
	for (int32 i = childCount -1; i >= 0; i--) {
		Info* childInfo = info->Children().ItemAt(i);
		struct stat st;
		if (GetEntry(directoryPaths, childInfo->Path(), NULL, &st)
			&& S_ISDIR(st.st_mode)) {
			_UpdateTree(childInfo);
		} else {
			_RemoveDirectory(childInfo);
			info->DeleteChildAt(i);
		}
	}
}


void
VirtualDirectoryManager::_RemoveDirectory(Info* info)
{
	// recursively remove the subdirectories
	for (int32 i = 0; Info* child = info->Children().ItemAt(i); i++)
		_RemoveDirectory(child);

	// remove the directory for the child definition file
	if (!info->Id().IsEmpty()) {
		BPath path(kTemporaryDefinitionFileBaseDirectoryPath, info->Id());
		if (path.InitCheck() == B_OK)
			rmdir(path.Path());
	}

	// unless this is the root directory, remove the definition file
	if (info != info->Root()->Info())
		BEntry(&info->DefinitionFileEntryRef()).Remove();

	_RemoveInfo(info);
}


status_t
VirtualDirectoryManager::_ResolveUnknownDefinitionFile(
	const node_ref& definitionFileNodeRef,
	const entry_ref& definitionFileEntryRef, Info*& _info)
{
	// This is either a root definition file or a subdir definition file
	// created by another application. We'll just try to read the info from the
	// file that a subdir definition file would contain. If that fails, we
	// assume a root definition file.
	entry_ref entryRef;
	BString subDirPath;
	if (_ReadSubDirectoryDefinitionFileInfo(definitionFileEntryRef, entryRef,
			subDirPath) != B_OK) {
		return _CreateRootInfo(definitionFileNodeRef, definitionFileEntryRef,
			_info);
	}

	if (subDirPath.IsEmpty())
		return B_BAD_VALUE;

	// get the root definition file node ref
	node_ref nodeRef;
	status_t error = BEntry(&entryRef).GetNodeRef(&nodeRef);
	if (error != B_OK)
		return error;

	// resolve/create the root info
	Info* info = _InfoForNodeRef(nodeRef);
	if (info == NULL) {
		error = _CreateRootInfo(nodeRef, entryRef, info);
		if (error != B_OK)
			return error;
	} else if (info->Root()->Info() != info)
		return B_BAD_VALUE;

	const BStringList& rootDirectoryPaths = info->Root()->DirectoryPaths();

	// now we can traverse the subdir path and resolve all infos along the way
	int32 nextComponentOffset = 0;
	while (nextComponentOffset < subDirPath.Length()) {
		int32 componentEnd = subDirPath.FindFirst('/', nextComponentOffset);
		if (componentEnd >= 0) {
			// skip duplicate '/'s
			if (componentEnd == nextComponentOffset + 1) {
				nextComponentOffset = componentEnd;
				continue;
			}
			nextComponentOffset = componentEnd + 1;
		} else {
			componentEnd = subDirPath.Length();
			nextComponentOffset = componentEnd;
		}

		BString entryPath(subDirPath, componentEnd);
		if (entryPath.IsEmpty())
			return B_NO_MEMORY;

		struct stat st;
		if (!GetEntry(rootDirectoryPaths, entryPath, &entryRef, &st))
			return B_ENTRY_NOT_FOUND;

		if (!S_ISDIR(st.st_mode))
			return B_BAD_VALUE;

		error = TranslateDirectoryEntry(info->DefinitionFileNodeRef(), entryRef,
			nodeRef);
		if (error != B_OK)
			return error;

		info = _InfoForNodeRef(nodeRef);
	}

	_info = info;

	return B_OK;
}


status_t
VirtualDirectoryManager::_CreateRootInfo(const node_ref& definitionFileNodeRef,
	const entry_ref& definitionFileEntryRef, Info*& _info)
{
	RootInfo* root = new(std::nothrow) RootInfo(definitionFileNodeRef,
		definitionFileEntryRef);
	if (root == NULL || root->InitCheck() != B_OK) {
		delete root;
		return B_NO_MEMORY;
	}
	ObjectDeleter<RootInfo> rootDeleter(root);

	status_t error = root->ReadDefinition();
	if (error != B_OK)
		return error;

	if (!_AddInfo(root->Info()))
		return B_NO_MEMORY;

	rootDeleter.Detach();
	_info = root->Info();

	return B_OK;
}


status_t
VirtualDirectoryManager::_ReadSubDirectoryDefinitionFileInfo(
	const entry_ref& entryRef, entry_ref& _rootDefinitionFileEntryRef,
	BString& _subDirPath)
{
	BDriverSettings driverSettings;
	status_t error = driverSettings.Load(entryRef);
	if (error != B_OK)
		return error;

	const char* subDirPath = driverSettings.GetParameterValue("subdir");
	if (subDirPath == NULL || subDirPath[0] == '\0')
		return B_BAD_DATA;

	BDriverParameter rootParameter;
	if (!driverSettings.FindParameter("root", &rootParameter))
		return B_BAD_DATA;

	const char* name = rootParameter.GetParameterValue("name");
	dev_t device = rootParameter.GetInt32ParameterValue("device", -1, -1);
	ino_t directory = rootParameter.GetInt64ParameterValue("directory");
	if (name == NULL || name[0] == '\0' || device < 0)
		return B_BAD_DATA;

	_rootDefinitionFileEntryRef = entry_ref(device, directory, name);
	_subDirPath = subDirPath;

	return !_subDirPath.IsEmpty() && _rootDefinitionFileEntryRef.name != NULL
		? B_OK : B_NO_MEMORY;
}

} // namespace BPrivate
