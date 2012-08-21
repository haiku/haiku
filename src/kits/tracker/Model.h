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
#ifndef _NU_MODEL_H
#define _NU_MODEL_H


//	Dedicated to BModel


#include <AppFileInfo.h>
#include <Debug.h>
#include <Mime.h>
#include <StorageDefs.h>
#include <String.h>

#include "IconCache.h"
#include "ObjectList.h"


class BPath;
class BHandler;
class BEntry;
class BQuery;

#if __GNUC__ && __GNUC__ < 3
// using std::stat instead of just stat here because of what
// seems to be a gcc bug involving namespace and struct stat interaction
typedef struct std::stat StatStruct;
#else
// on mwcc std isn't turned on but there is no bug either.
// Also seems to be fixed in gcc 3.
typedef struct stat StatStruct;
#endif

namespace BPrivate {

enum {
	kDoesNotSupportType,
	kSuperhandlerModel,
	kModelSupportsSupertype,
	kModelSupportsType,
	kModelSupportsFile
};

class Model {
	public:
		Model();
		Model(const Model &);
		Model(const BEntry* entry, bool open = false, bool writable = false);
		Model(const entry_ref*, bool traverse = false, bool open = false,
			bool writable = false);
		Model(const node_ref* dirNode, const node_ref* node, const char* name,
			bool open = false, bool writable = false);
		~Model();

		Model& operator=(const Model&);

		status_t InitCheck() const;

		status_t SetTo(const BEntry*, bool open = false,
			bool writable = false);
		status_t SetTo(const entry_ref*, bool traverse = false,
			bool open = false, bool writable = false);
		status_t SetTo(const node_ref* dirNode, const node_ref* node,
			const char* name, bool open = false, bool writable = false);

		int CompareFolderNamesFirst(const Model* compareModel) const;

		// node management
		status_t OpenNode(bool writable = false);
			// also used to switch from read-only to writable
		void CloseNode();
		bool IsNodeOpen() const;
		bool IsNodeOpenForWriting() const;

		status_t UpdateStatAndOpenNode(bool writable = false);
			// like OpenNode, called on zombie poses to check if they turned
			// real, starts by rereading the stat structure

		// basic getters
		const char* Name() const;
		const entry_ref* EntryRef() const;
		const node_ref* NodeRef() const;
		const StatStruct* StatBuf() const;

		BNode* Node() const;
			// returns null if not Open
		void GetPath(BPath*) const;
		void GetEntry(BEntry*) const;

		const char* MimeType() const;
		const char* PreferredAppSignature() const;
			// only not-null if not default for type and not self for app
		void SetPreferredAppSignature(const char*);

		void GetPreferredAppForBrokenSymLink(BString &result);
			// special purpose call - if a symlink is unresolvable, it makes
			// sense to be able to get at it's preferred handler which may be
			// different from the Tracker. Used by the network neighborhood.

		// type getters
		bool IsFile() const;
		bool IsDirectory() const;
		bool IsQuery() const;
		bool IsQueryTemplate() const;
		bool IsContainer() const;
		bool IsExecutable() const;
		bool IsSymLink() const;
		bool IsRoot() const;
		bool IsTrash() const;
		bool IsDesktop() const;
		bool IsVolume() const;

		IconSource IconFrom() const;
		void SetIconFrom(IconSource);
			// where is this model getting it's icon from

		void ResetIconFrom();
			// called from the attribute changed calls to force a lookup of
			// a new icon

		// symlink handling calls, mainly used by the IconCache
		const Model* ResolveIfLink() const;
		Model* ResolveIfLink();
			// works on anything
		Model* LinkTo() const;
			// fast, works only on symlinks
		void SetLinkTo(Model*);

		status_t GetLongVersionString(BString &, version_kind);
		status_t GetVersionString(BString &, version_kind);
		status_t AttrAsString(BString &, int64* value,
			const char* attributeName, uint32 attributeType);

		// Node monitor update call
		void UpdateEntryRef(const node_ref* dirRef, const char* name);
		bool AttrChanged(const char*);
			// returns true if pose needs to update it's icon, etc.
			// pass null to force full update
		bool StatChanged();
			// returns true if pose needs to update it's icon

		status_t WatchVolumeAndMountPoint(uint32, BHandler*);
			// correctly handles boot volume name watching

		bool IsDropTarget(const Model* forDocument = 0,
			bool traverse = false) const;
			// if nonzero <forDocument> passed, mime info is used to
			// resolve if document can be opened
			// if zero, all executables, directories and volumes pass
			// if traverse, dereference symlinks
		bool IsDropTargetForList(const BObjectList<BString>* list) const;
			// <list> contains mime types of all documents about to be handled
			// by model

	#if DEBUG
		void PrintToStream(int32 level = 1, bool deep = false);
		void TrackIconSource(icon_size);
	#endif

		bool IsSuperHandler() const;
		int32 SupportsMimeType(const char* type,
			const BObjectList<BString>* list, bool exactReason = false) const;
			// pass in one string in <type> or a bunch in <list>
			// if <exactReason> false, returns as soon as it figures out that
			// app supports a given type, if true, returns an exact reason

		// get rid of this??
		ssize_t WriteAttr(const char* attr, type_code type, off_t,
			const void* buffer, size_t );
			// cover call, creates a writable node and writes out attributes
			// into it; work around for file nodes not being writeable
		ssize_t WriteAttrKillForeign(const char* attr,
			const char* foreignAttr, type_code type, off_t,
			const void* buffer, size_t);

		bool Mimeset(bool force);
			// returns true if mime type changed

		bool HasLocalizedName() const;

	private:
		status_t OpenNodeCommon(bool writable);
		void SetupBaseType();
		void FinishSettingUpType();
		void DeletePreferredAppVolumeNameLinkTo();
		void CacheLocalizedName();

		status_t FetchOneQuery(const BQuery*, BHandler* target,
			BObjectList<BQuery>*, BVolume*);

		enum CanHandleResult {
			kCanHandle,
			kCannotHandle,
			kNeedToCheckType
		};

		CanHandleResult CanHandleDrops() const;

		enum NodeType {
			kPlainNode,
			kExecutableNode,
			kDirectoryNode,
			kLinkNode,
			kQueryNode,
			kQueryTemplateNode,
			kVolumeNode,
			kRootNode,
			kTrashNode,
			kDesktopNode,
			kUnknownNode
		};

		entry_ref fEntryRef;
		StatStruct fStatBuf;
		BString fMimeType;
			// should use string that may be shared for common types

		// bit of overloading hackery here to save on footprint
		union {
			char* fPreferredAppName;	// used if we are neither a volume
										// nor a symlink
			char* fVolumeName;			// used if we are a volume
			Model* fLinkTo;				// used if we are a symlink
		};

		uint8 fBaseType;
		uint8 fIconFrom;
		bool fWritable;
		BNode* fNode;
		status_t fStatus;
		BString fLocalizedName;
		bool fHasLocalizedName;
		bool fLocalizedNameIsCached;
};


class ModelNodeLazyOpener {
	// a utility open state manager, usefull to allocate on stack
	// and have close up model when done, etc.
	public:
		// consider failing when open does not succeed

		ModelNodeLazyOpener(Model* model, bool writable = false,
			bool openLater = true);
		~ModelNodeLazyOpener();

		bool IsOpen() const;
		bool IsOpenForWriting() const;
		bool IsOpen(bool forWriting) const;
		Model* TargetModel() const;
		status_t OpenNode(bool writable = false);

	private:
		Model* fModel;
		bool fWasOpen;
		bool fWasOpenForWriting;
};

// handy flavors of openers
class BModelOpener : public ModelNodeLazyOpener {
	public:
		BModelOpener(Model* model)
		:	ModelNodeLazyOpener(model, false, false)
		{
		}
};

class BModelWriteOpener : public ModelNodeLazyOpener {
	public:
		BModelWriteOpener(Model* model)
		:	ModelNodeLazyOpener(model, true, false)
		{
		}
};


#if DEBUG
// #define CHECK_OPEN_MODEL_LEAKS
#endif

#ifdef CHECK_OPEN_MODEL_LEAKS
void DumpOpenModels(bool extensive);
void InitOpenModelDumping();
#endif

// inlines follow -----------------------------------

inline const char*
Model::MimeType() const
{
	return fMimeType.String();
}


inline const entry_ref*
Model::EntryRef() const
{
	return &fEntryRef;
}


inline const node_ref*
Model::NodeRef() const
{
	// the stat structure begins with a node_ref
	return (node_ref*)&fStatBuf;
}


inline BNode*
Model::Node() const
{
	return fNode;
}


inline const StatStruct*
Model::StatBuf() const
{
	return &fStatBuf;
}


inline IconSource
Model::IconFrom() const
{
	return (IconSource)fIconFrom;
}


inline void
Model::SetIconFrom(IconSource from)
{
	fIconFrom = from;
}


inline Model*
Model::LinkTo() const
{
	ASSERT(IsSymLink());
	return fLinkTo;
}


inline bool
Model::IsFile() const
{
	return fBaseType == kPlainNode
		|| fBaseType == kQueryNode
		|| fBaseType == kQueryTemplateNode
		|| fBaseType == kExecutableNode;
}


inline bool
Model::IsVolume() const
{
	return fBaseType == kVolumeNode;
}


inline bool
Model::IsDirectory() const
{
	return fBaseType == kDirectoryNode
		|| fBaseType == kVolumeNode
		|| fBaseType == kRootNode
		|| fBaseType == kTrashNode
		|| fBaseType == kDesktopNode;
}


inline bool
Model::IsQuery() const
{
	return fBaseType == kQueryNode;
}


inline bool
Model::IsQueryTemplate() const
{
	return fBaseType == kQueryTemplateNode;
}


inline bool
Model::IsContainer() const
{
	// I guess as in should show container window -
	// volumes show the volume window
	return IsQuery() || IsDirectory();
}


inline bool
Model::IsRoot() const
{
	return fBaseType == kRootNode;
}


inline bool
Model::IsTrash() const
{
	return fBaseType == kTrashNode;
}


inline bool
Model::IsDesktop() const
{
	return fBaseType == kDesktopNode;
}


inline bool
Model::IsExecutable() const
{
	return fBaseType == kExecutableNode;
}


inline bool
Model::IsSymLink() const
{
	return fBaseType == kLinkNode;
}


inline bool
Model::HasLocalizedName() const
{
	return fHasLocalizedName;
}


inline
ModelNodeLazyOpener::ModelNodeLazyOpener(Model* model, bool writable,
	bool openLater)
	:	fModel(model),
		fWasOpen(model->IsNodeOpen()),
		fWasOpenForWriting(model->IsNodeOpenForWriting())
{
	if (!openLater)
		OpenNode(writable);
}


inline
ModelNodeLazyOpener::~ModelNodeLazyOpener()
{
	if (!fModel->IsNodeOpen())
		return;
	if (!fWasOpen)
		fModel->CloseNode();
	else if (!fWasOpenForWriting)
		fModel->OpenNode();
}


inline bool
ModelNodeLazyOpener::IsOpen() const
{
	return fModel->IsNodeOpen();
}


inline bool
ModelNodeLazyOpener::IsOpenForWriting() const
{
	return fModel->IsNodeOpenForWriting();
}


inline bool
ModelNodeLazyOpener::IsOpen(bool forWriting) const
{
	return forWriting ? fModel->IsNodeOpenForWriting() : fModel->IsNodeOpen();
}


inline Model*
ModelNodeLazyOpener::TargetModel() const
{
	return fModel;
}


inline status_t
ModelNodeLazyOpener::OpenNode(bool writable)
{
	if (writable) {
		if (!fModel->IsNodeOpenForWriting())
			return fModel->OpenNode(true);
	} else if (!fModel->IsNodeOpen())
		return fModel->OpenNode();

	return B_OK;
}

} // namespace BPrivate

#endif	// _NU_MODEL_H
