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

//	Icon cache is used for drawing node icons; it caches icons
//	and reuses them for successive draws

#include <Debug.h>

#include "PoseList.h"
#include "Pose.h"


BPose*
PoseList::FindPose(const node_ref* node, int32* resultingIndex) const
{
	int32 count = CountItems();
	for (int32 index = 0; index < count; index++) {
		BPose* pose = ItemAt(index);
		ASSERT(pose->TargetModel());
		if (*pose->TargetModel()->NodeRef() == *node) {
			if (resultingIndex)
				*resultingIndex = index;

			return pose;
		}
	}
	return NULL;
}


BPose*
PoseList::FindPose(const entry_ref* entry, int32* resultingIndex) const
{
	int32 count = CountItems();
	for (int32 index = 0; index < count; index++) {
		BPose* pose = ItemAt(index);
		ASSERT(pose->TargetModel());
		if (*pose->TargetModel()->EntryRef() == *entry) {
			if (resultingIndex)
				*resultingIndex = index;

			return pose;
		}
	}
	return NULL;
}


BPose*
PoseList::FindPose(const Model* model, int32* resultingIndex) const
{
	return FindPose(model->NodeRef(), resultingIndex);
}


BPose*
PoseList::DeepFindPose(const node_ref* node, int32* resultingIndex) const
{
	int32 count = CountItems();
	for (int32 index = 0; index < count; index++) {
		BPose* pose = ItemAt(index);
		Model* model = pose->TargetModel();
		if (*model->NodeRef() == *node) {
			if (resultingIndex)
				*resultingIndex = index;

			return pose;
		}
		// if model is a symlink, try matching node with the target
		// of the link
		if (model->IsSymLink()) {
			model = model->LinkTo();
			if (model && *model->NodeRef() == *node) {
				if (resultingIndex)
					*resultingIndex = index;

				return pose;
			}
		}
	}

	return NULL;
}


PoseList *
PoseList::FindAllPoses(const node_ref *node) const
{
	int32 count = CountItems();
	PoseList *result = new PoseList(5, false);
	for (int32 index = 0; index < count; index++) {
		BPose *pose = ItemAt(index);
		Model *model = pose->TargetModel();
		if (*model->NodeRef() == *node) {
			result->AddItem(pose, 0);
			continue;
		}
		
		if (!model->IsSymLink())
			continue;

		model = model->LinkTo();
		if (model && *model->NodeRef() == *node) {
			result->AddItem(pose);
			continue;
		}
		
		if (!model) {
			model = new Model(pose->TargetModel()->EntryRef(), true);
			if (*model->NodeRef() == *node)
				result->AddItem(pose);
			delete model;
		}
	}
	return result;
}
