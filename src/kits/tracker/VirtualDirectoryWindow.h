/*
 * Copyright 2013-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */
#ifndef VIRTUAL_DIRECTORY_WINDOW_H
#define VIRTUAL_DIRECTORY_WINDOW_H


#include "ContainerWindow.h"


namespace BPrivate {

class VirtualDirectoryPoseView;

class VirtualDirectoryWindow : public BContainerWindow {
public:
								VirtualDirectoryWindow(
									LockingList<BWindow>* windowList,
									uint32 containerWindowFlags,
									window_look look = B_DOCUMENT_WINDOW_LOOK,
									window_feel feel = B_NORMAL_WINDOW_FEEL,
									uint32 flags = B_WILL_ACCEPT_FIRST_CLICK
										| B_NO_WORKSPACE_ACTIVATION,
									uint32 workspace = B_CURRENT_WORKSPACE);

	VirtualDirectoryPoseView*	PoseView() const;

protected:
	virtual	void				CreatePoseView(Model* model);
	virtual	BPoseView*			NewPoseView(Model* model, uint32 viewMode);
	virtual	void				AddWindowMenu(BMenu* menu);
	virtual	void				AddWindowContextMenus(BMenu* menu);

private:
			typedef BContainerWindow _inherited;
};

} // namespace BPrivate


#endif	// VIRTUAL_DIRECTORY_WINDOW_H
