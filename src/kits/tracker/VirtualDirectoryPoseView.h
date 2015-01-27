/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */
#ifndef _VIRTUAL_DIRECTORY_POSE_VIEW_H
#define _VIRTUAL_DIRECTORY_POSE_VIEW_H


#include <StringList.h>

#include "PoseView.h"


namespace BPrivate {

class VirtualDirectoryEntryList;


class VirtualDirectoryPoseView : public BPoseView {
public:
								VirtualDirectoryPoseView(Model* model);
	virtual						~VirtualDirectoryPoseView();

	virtual	void				MessageReceived(BMessage* message);

protected:
	virtual	void				AttachedToWindow();
	virtual	void				RestoreState(AttributeStreamNode* node);
	virtual	void				RestoreState(const BMessage& message);
	virtual	void				SavePoseLocations(BRect* frameIfDesktop = NULL);
	virtual	void				SetViewMode(uint32 newMode);
	virtual	EntryListBase*		InitDirentIterator(const entry_ref* ref);

	virtual	void				StartWatching();
	virtual	void				StopWatching();

	virtual	bool				FSNotification(const BMessage* message);

private:
			typedef BPoseView _inherited;

private:
			bool				_EntryCreated(const BMessage* message);
			bool				_EntryRemoved(const BMessage* message);
			bool				_EntryMoved(const BMessage* message);
			bool				_NodeStatChanged(const BMessage* message);

			void				_DispatchEntryCreatedOrRemovedMessage(
									int32 opcode, const node_ref& nodeRef,
									const entry_ref& entryRef,
									const char* path = NULL,
									bool dispatchToSuperClass = true);

			bool				_GetEntry(const char* name, entry_ref& _ref,
									struct stat* _st = NULL);

			status_t			_UpdateDirectoryPaths();
									// manager must be locked

private:
			BStringList			fDirectoryPaths;
			node_ref			fRootDefinitionFileRef;
			bigtime_t			fFileChangeTime;
			bool				fIsRoot;
};


} // namespace BPrivate


#endif	// _VIRTUAL_DIRECTORY_POSE_VIEW_H
