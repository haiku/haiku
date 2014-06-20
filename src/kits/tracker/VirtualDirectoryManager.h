/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */
#ifndef _VIRTUAL_DIRECTORY_MANAGER_H
#define _VIRTUAL_DIRECTORY_MANAGER_H


#include <dirent.h>

#include <map>

#include <Locker.h>
#include <Node.h>


class BStringList;


namespace BPrivate {

class Model;


class VirtualDirectoryManager {
public:
	static	VirtualDirectoryManager* Instance();

			bool				Lock()		{ return fLock.Lock(); }
			void				Unlock()	{ fLock.Unlock(); }

			status_t			ResolveDirectoryPaths(
									const node_ref& definitionFileNodeRef,
									const entry_ref& definitionFileEntryRef,
									BStringList& _directoryPaths,
									node_ref* _definitionFileNodeRef = NULL,
									entry_ref* _definitionFileEntryRef = NULL);

			bool				GetDefinitionFileChangeTime(
									const node_ref& definitionFileRef,
									bigtime_t& _time) const;

			bool				GetRootDefinitionFile(
									const node_ref& definitionFileRef,
									node_ref& _rootDefinitionFileRef);
			bool				GetSubDirectoryDefinitionFile(
									const node_ref& baseDefinitionRef,
									const char* subDirName,
									entry_ref& _entryRef, node_ref& _nodeRef);
			bool				GetParentDirectoryDefinitionFile(
									const node_ref& subDirDefinitionRef,
									entry_ref& _entryRef, node_ref& _nodeRef);

			status_t			TranslateDirectoryEntry(
									const node_ref& definitionFileRef,
									dirent* buffer);
			status_t			TranslateDirectoryEntry(
									const node_ref& definitionFileRef,
									entry_ref& entryRef, node_ref& _nodeRef);

			bool				DefinitionFileChanged(
									const node_ref& definitionFileRef);
									// returns whether the directory still
									// exists
			status_t			DirectoryRemoved(
									const node_ref& definitionFileRef);

	static	bool				GetEntry(const BStringList& directoryPaths,
									const char* name, entry_ref* _ref,
						 			struct stat* _st);

private:
			class Info;
			class RootInfo;

			typedef std::map<node_ref, Info*> NodeRefInfoMap;

private:
								VirtualDirectoryManager();

			Info*				_InfoForNodeRef(const node_ref& nodeRef) const;

			bool				_AddInfo(Info* info);
			void				_RemoveInfo(Info* info);

			void				_UpdateTree(RootInfo* root);
			void				_UpdateTree(Info* info);

			void				_RemoveDirectory(Info* info);

			status_t			_ResolveUnknownDefinitionFile(
									const node_ref& definitionFileNodeRef,
									const entry_ref& definitionFileEntryRef,
									Info*& _info);
			status_t			_CreateRootInfo(
									const node_ref& definitionFileNodeRef,
									const entry_ref& definitionFileEntryRef,
									Info*& _info);
			status_t			_ReadSubDirectoryDefinitionFileInfo(
									const entry_ref& entryRef,
									entry_ref& _rootDefinitionFileEntryRef,
									BString& _subDirPath);

private:
			BLocker				fLock;
			NodeRefInfoMap		fInfos;
};

} // namespace BPrivate


#endif	// _VIRTUAL_DIRECTORY_MANAGER_H
