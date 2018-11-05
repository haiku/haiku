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
#ifndef _UTILITIES_H
#define _UTILITIES_H


#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>

#include <Bitmap.h>
#include <ByteOrder.h>
#include <DataIO.h>
#include <Directory.h>
#include <Entry.h>
#include <GraphicsDefs.h>
#include <Looper.h>
#include <MenuItem.h>
#include <MessageFilter.h>
#include <Mime.h>
#include <NaturalCompare.h>
#include <ObjectList.h>
#include <Point.h>
#include <Path.h>
#include <String.h>
#include <StringView.h>


class BMessage;
class BTextView;
class BView;
class BVolume;

namespace BPrivate {

class Benaphore;
class BPose;
class BPoseView;

// global variables
static const rgb_color kBlack = {0, 0, 0, 255};
static const rgb_color kWhite = {255, 255, 255 ,255};

const int64 kHalfKBSize = 512;
const int64 kKBSize = 1024;
const int64 kMBSize = 1048576;
const int64 kGBSize = 1073741824;
const int64 kTBSize = kGBSize * kKBSize;

const int32 kMiniIconSeparator = 3;

const color_space kDefaultIconDepth = B_RGBA32;


extern bool gLocalizedNamePreferred;


// misc typedefs, constants and structs

class PeriodicUpdatePoses {
	// Periodically updated poses (ones with a volume space bar) register
	// themselfs in this global list. This way they can be iterated over instead
	// of sending around update messages.
public:
	PeriodicUpdatePoses();
	~PeriodicUpdatePoses();

	typedef bool (*PeriodicUpdateCallback)(BPose* pose, void* cookie);

	void AddPose(BPose* pose, BPoseView* poseView,
		PeriodicUpdateCallback callback, void* cookie);
	bool RemovePose(BPose* pose, void** cookie);

	void DoPeriodicUpdate(bool forceRedraw);

private:
	struct periodic_pose {
		BPose*					pose;
		BPoseView*				pose_view;
		PeriodicUpdateCallback	callback;
		void*					cookie;
	};

	Benaphore* fLock;
	BObjectList<periodic_pose> fPoseList;
};

extern PeriodicUpdatePoses gPeriodicUpdatePoses;


class PoseInfo {
	// PoseInfo is the structure that gets saved as attributes for every node
	// on disk, defining the node's position and visibility
public:
	static void EndianSwap(void* castToThis);
	void PrintToStream();

	bool fInvisible;
	ino_t fInitedDirectory;
		// For a location to be valid, fInitedDirectory has to contain
		// the inode of the items parent directory. This makes it
		// impossible to for instance zip up files and extract them in
		// the same location. This should probably be reworked.
		// Tracker could strip the file location attributes when dropping
		// files into a closed folder.
	BPoint fLocation;
};


class ExtendedPoseInfo {
	// extends PoseInfo adding workspace support; used for desktop
	// poses only
public:
	size_t Size() const;
	static size_t Size(int32);
	size_t SizeWithHeadroom() const;
	static size_t SizeWithHeadroom(size_t);
	bool HasLocationForFrame(BRect) const;
	BPoint LocationForFrame(BRect) const;
	bool SetLocationForFrame(BPoint, BRect);

	static void EndianSwap(void* castToThis);
	void PrintToStream();

	uint32 fWorkspaces;
	bool fInvisible;
	bool fShowFromBootOnly;
	bool fReservedBool1;
	bool fReservedBool2;
	int32 fReservedInt1;
	int32 fReservedInt2;
	int32 fReservedInt3;
	int32 fReservedInt4;
	int32 fReservedInt5;

	int32 fNumFrames;
	struct FrameLocation {
		BPoint fLocation;
		BRect fFrame;
		uint32 fWorkspaces;
	};

	FrameLocation fLocations[0];
};

// misc functions
void DisallowMetaKeys(BTextView*);
void DisallowFilenameKeys(BTextView*);


bool ValidateStream(BMallocIO*, uint32, int32 version);


bool SecondaryMouseButtonDown(int32 modifiers, int32 buttons);
uint32 HashString(const char* string, uint32 seed);
uint32 AttrHashString(const char* string, uint32 type);


class OffscreenBitmap {
	// a utility class for setting up offscreen bitmaps
	public:
		OffscreenBitmap(BRect bounds);
		OffscreenBitmap();
		~OffscreenBitmap();

		BView* BeginUsing(BRect bounds);
		void DoneUsing();
		BBitmap* Bitmap() const;
			// blit this to your view when you are done rendering
		BView* View() const;
			// use this to render your image

	private:
		void NewBitmap(BRect frame);
		BBitmap* fBitmap;
};


// bitmap functions
extern void FadeRGBA32Horizontal(uint32* bits, int32 width, int32 height,
	int32 from, int32 to);
extern void FadeRGBA32Vertical(uint32* bits, int32 width, int32 height,
	int32 from, int32 to);


class FlickerFreeStringView : public BStringView {
	// adds support for offscreen bitmap drawing for string views that update
	// often this would be better implemented as an option of BStringView
public:
	FlickerFreeStringView(BRect bounds, const char* name,
		const char* text, uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		uint32 flags = B_WILL_DRAW);
	FlickerFreeStringView(BRect bounds, const char* name,
		const char* text, BBitmap* existingOffscreen,
		uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		uint32 flags = B_WILL_DRAW);
	virtual ~FlickerFreeStringView();
	virtual void Draw(BRect);
	virtual void AttachedToWindow();
	virtual void SetViewColor(rgb_color);
	virtual void SetLowColor(rgb_color);

private:
	OffscreenBitmap* fBitmap;
	rgb_color fViewColor;
	rgb_color fLowColor;
	BBitmap* fOriginalBitmap;

	typedef BStringView _inherited;
};


class DraggableIcon : public BView {
	// used to determine a save location for a file
public:
	DraggableIcon(BRect rect, const char* name, const char* mimeType,
		icon_size which, const BMessage* message, BMessenger target,
		uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		uint32 flags = B_WILL_DRAW);
	virtual ~DraggableIcon();

	static BRect PreferredRect(BPoint offset, icon_size which);
	void SetTarget(BMessenger);

protected:
	virtual void AttachedToWindow();
	virtual void MouseDown(BPoint);
	virtual void Draw(BRect);

	virtual bool DragStarted(BMessage* dragMessage);

protected:
	BBitmap* fBitmap;
	BMessage fMessage;
	BMessenger fTarget;
};


class PositionPassingMenuItem : public BMenuItem {
public:
	PositionPassingMenuItem(const char* title, BMessage*,
		char shortcut = 0, uint32 modifiers = 0);
	PositionPassingMenuItem(BMenu*, BMessage*);
	PositionPassingMenuItem(BMessage* data);

	static BArchivable* Instantiate(BMessage* data);

protected:
	virtual status_t Invoke(BMessage* = 0);
		// appends the invoke location for NewFolder, etc. to use

private:
	typedef BMenuItem _inherited;
};


class Benaphore {
	// aka benaphore
public:
	Benaphore(const char* name = "Light Lock")
	:	fSemaphore(create_sem(0, name)),
		fCount(1)
	{
	}

	~Benaphore()
	{
		delete_sem(fSemaphore);
	}

	bool Lock()
	{
		if (atomic_add(&fCount, -1) <= 0)
			return acquire_sem(fSemaphore) == B_OK;

		return true;
	}

	void Unlock()
	{
		if (atomic_add(&fCount, 1) < 0)
			release_sem(fSemaphore);
	}

	bool IsLocked() const
	{
		return fCount <= 0;
	}

private:
	sem_id fSemaphore;
	int32 fCount;
};


class SeparatorLine : public BView {
public:
	SeparatorLine(BPoint, float, bool vertical, const char* name = "");
	virtual void Draw(BRect bounds);
};


class TitledSeparatorItem : public BMenuItem {
public:
	TitledSeparatorItem(const char*);
	virtual ~TitledSeparatorItem();

	virtual void SetEnabled(bool state);

protected:
	virtual void GetContentSize(float* width, float* height);
	virtual void Draw();

private:
	typedef BMenuItem _inherited;
};


class LooperAutoLocker {
public:
	LooperAutoLocker(BHandler* handler)
	:	fHandler(handler),
		fHasLock(handler->LockLooper())
	{
	}

	~LooperAutoLocker()
	{
		if (fHasLock)
			fHandler->UnlockLooper();
	}

	bool operator!() const
	{
		return !fHasLock;
	}

	bool IsLocked() const
	{
		return fHasLock;
	}

private:
	BHandler* fHandler;
	bool fHasLock;
};


class ShortcutFilter : public BMessageFilter {
public:
	ShortcutFilter(uint32 shortcutKey, uint32 shortcutModifier,
		uint32 shortcutWhat, BHandler* target);

protected:
	filter_result Filter(BMessage*, BHandler**);

private:
	uint32 fShortcutKey;
	uint32 fShortcutModifier;
	uint32 fShortcutWhat;
	BHandler* fTarget;
};


// iterates over all the refs in a message
entry_ref* EachEntryRef(BMessage*, entry_ref* (*)(entry_ref*, void*),
	void* passThru = 0);
const entry_ref* EachEntryRef(const BMessage*,
	const entry_ref* (*)(const entry_ref*, void*), void* passThru = 0);

entry_ref* EachEntryRef(BMessage*, entry_ref* (*)(entry_ref*, void*),
	void* passThru, int32 maxCount);
const entry_ref* EachEntryRef(const BMessage*,
	const entry_ref* (*)(const entry_ref*, void*), void* passThru,
	int32 maxCount);


bool ContainsEntryRef(const BMessage*, const entry_ref*);
int32 CountRefs(const BMessage*);

BMenuItem* EachMenuItem(BMenu* menu, bool recursive,
	BMenuItem* (*func)(BMenuItem*));
const BMenuItem* EachMenuItem(const BMenu* menu, bool recursive,
	BMenuItem* (*func)(const BMenuItem*));

int64 StringToScalar(const char* text);
	// string to num, understands kB, MB, etc.

int32 ListIconSize();

// misc calls
void EmbedUniqueVolumeInfo(BMessage* message, const BVolume* volume);
status_t MatchArchivedVolume(BVolume* volume, const BMessage* message,
	int32 index = 0);
void TruncateLeaf(BString* string);

void StringFromStream(BString*, BMallocIO*, bool endianSwap = false);
void StringToStream(const BString*, BMallocIO*);
int32 ArchiveSize(const BString*);

extern void EnableNamedMenuItem(BMenu* menu, const char* itemName, bool on);
extern void MarkNamedMenuItem(BMenu* menu, const char* itemName, bool on);
extern void EnableNamedMenuItem(BMenu* menu, uint32 commandName, bool on);
extern void MarkNamedMenuItem(BMenu* menu, uint32 commandName, bool on);
extern void DeleteSubmenu(BMenuItem* submenuItem);

extern bool BootedInSafeMode();


inline rgb_color
Color(int32 r, int32 g, int32 b, int32 alpha = 255)
{
	rgb_color result;
	result.red = (uchar)r;
	result.green = (uchar)g;
	result.blue = (uchar)b;
	result.alpha = (uchar)alpha;

	return result;
}

void PrintToStream(rgb_color color);

template <class InitCheckable>
void
ThrowOnInitCheckError(InitCheckable* item)
{
	if (item == NULL)
		throw (status_t)B_ERROR;

	status_t result = item->InitCheck();
	if (result != B_OK)
		throw (status_t)result;
}

#if DEBUG
#	define ThrowOnError(x) _ThrowOnError(x, __FILE__, __LINE__)
#	define ThrowIfNotSize(x) _ThrowIfNotSize(x, __FILE__, __LINE__)
#	define ThrowOnAssert(x) _ThrowOnAssert(x, __FILE__, __LINE__)
#else
#	define ThrowOnError(x) _ThrowOnError(x, NULL, 0)
#	define ThrowIfNotSize(x) _ThrowIfNotSize(x, NULL, 0)
#	define ThrowOnAssert(x) _ThrowOnAssert(x, NULL, 0)
#endif

void _ThrowOnError(status_t, const char*, int32);
void _ThrowIfNotSize(ssize_t, const char*, int32);
void _ThrowOnAssert(bool, const char*, int32);

// stub calls that work around BAppFile info inefficiency
status_t GetAppSignatureFromAttr(BFile*, char*);
status_t GetAppIconFromAttr(BFile* file, BBitmap* icon, icon_size which);
status_t GetFileIconFromAttr(BNode* node, BBitmap* icon, icon_size which);

// debugging
void HexDump(const void* buffer, int32 length);

#if xDEBUG

inline void
PrintRefToStream(const entry_ref* ref, const char* trailer = "\n")
{
	if (ref == NULL) {
		PRINT(("NULL entry_ref%s", trailer));
		return;
	}

	BPath path;
	BEntry entry(ref);
	entry.GetPath(&path);
	PRINT(("%s%s", path.Path(), trailer));
}


inline void
PrintEntryToStream(const BEntry* entry, const char* trailer = "\n")
{
	if (entry == NULL) {
		PRINT(("NULL entry%s", trailer));
		return;
	}

	BPath path;
	entry->GetPath(&path);
	PRINT(("%s%s", path.Path(), trailer));
}


inline void
PrintDirToStream(const BDirectory* dir, const char* trailer = "\n")
{
	if (dir == NULL) {
		PRINT(("NULL entry_ref%s", trailer));
		return;
	}

	BPath path;
	BEntry entry;
	dir->GetEntry(&entry);
	entry.GetPath(&path);
	PRINT(("%s%s", path.Path(), trailer));
}

#else

inline void PrintRefToStream(const entry_ref*, const char* = 0) {}
inline void PrintEntryToStream(const BEntry*, const char* = 0) {}
inline void PrintDirToStream(const BDirectory*, const char* = 0) {}

#endif

#ifdef xDEBUG

	extern FILE* logFile;

	inline void PrintToLogFile(const char* format, ...)
	{
		va_list ap;
		va_start(ap, fmt);
		vfprintf(logFile, fmt, ap);
		va_end(ap);
	}

#define WRITELOG(_ARGS_)													\
	if (logFile == 0) 														\
		logFile = fopen("/var/log/tracker.log", "a+");						\
																			\
	if (logFile != 0) {														\
		thread_info info;													\
		get_thread_info(find_thread(NULL), &info);							\
		PrintToLogFile("[t %Ld] \"%s\" (%s:%i) ", system_time(),			\
			info.name, __FILE__, __LINE__);									\
		PrintToLogFile _ARGS_;												\
		PrintToLogFile("\n");												\
		fflush(logFile);													\
	}

#else

#define WRITELOG(_ARGS_)

#endif

// fancy casting macros

template <typename NewType, typename OldType>
inline NewType assert_cast(OldType castedPointer) {
	ASSERT(dynamic_cast<NewType>(castedPointer) != NULL);
	return static_cast<NewType>(castedPointer);
}

// B_SWAP_INT32 have broken signedness, simple cover calls to fix that
// should fix up in ByteOrder.h

inline int32 SwapInt32(int32 value)
	{ return (int32)B_SWAP_INT32((uint32)value); }
inline uint32 SwapUInt32(uint32 value) { return B_SWAP_INT32(value); }
inline int64 SwapInt64(int64 value)
	{ return (int64)B_SWAP_INT64((uint64)value); }
inline uint64 SwapUInt64(uint64 value) { return B_SWAP_INT64(value); }


extern const float kExactMatchScore;
float ComputeTypeAheadScore(const char* text, const char* match,
	bool wordMode = false);

} // namespace BPrivate


#endif	// _UTILITIES_H
