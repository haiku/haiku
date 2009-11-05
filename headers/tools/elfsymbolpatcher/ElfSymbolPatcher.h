//------------------------------------------------------------------------------
//	Copyright (c) 2003, Ingo Weinhold
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ElfSymbolPatcher.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Interface declaration of classes used for patching ELF
//					symbols. Central class is ElfSymbolPatcher. It is a kind of
//					roster, managing all necessary infos (loaded images) and
//					being able to fill ElfSymbolPatchInfos with life.
//					An ElfSymbolPatchInfo represents a symbol and is able to
//					patch/restore it. An ElfSymbolPatchGroup bundles several
//					ElfSymbolPatchInfos and can update their data, e.g.
//					when images are loaded/unloaded. It uses a
//					ElfSymbolPatcher internally and provides a more convenient
//					API for the user.
//------------------------------------------------------------------------------

#ifndef ELF_SYMBOL_PATCHER_H
#define ELF_SYMBOL_PATCHER_H

#include <List.h>
#include <String.h>


namespace SymbolPatcher {

class ElfImage;
class ElfSymbolPatcher;
class ElfSymbolPatchGroup;

// ElfSymbolPatchInfo
class ElfSymbolPatchInfo {
public:
								ElfSymbolPatchInfo();
								~ElfSymbolPatchInfo();

			status_t			InitCheck() const;

			const char*			GetSymbolName() const;
			void*				GetOriginalAddress() const;
			image_id			GetOriginalAddressImage() const;

			status_t			Patch(void* newAddress);
			status_t			Restore();

private:
			class Entry;

private:
			void				Unset();
			status_t			SetSymbolName(const char* name);
			void				SetOriginalAddress(void* address,
												   image_id image);
			status_t			CreateEntry(image_id image, BList* targets);
			bool				DeleteEntry(image_id image);
			Entry*				EntryAt(int32 index);
			Entry*				EntryFor(image_id image);

private:
			friend class ElfSymbolPatcher;
			friend class ElfSymbolPatchGroup;

			BString				fSymbolName;
			void*				fOriginalAddress;
			image_id			fOriginalAddressImage;
			BList				fEntries;
};

// ElfSymbolPatcher
class ElfSymbolPatcher {
public:
			class UpdateAdapter;

public:
								ElfSymbolPatcher();
								~ElfSymbolPatcher();

			status_t			InitCheck() const;
			status_t			Update(UpdateAdapter* updateAdapter = NULL);
			void				Unload();

			status_t			GetSymbolPatchInfo(const char* symbolName,
												   ElfSymbolPatchInfo* info);
			status_t			UpdateSymbolPatchInfo(ElfSymbolPatchInfo* info,
													  ElfImage* image);

private:
			status_t			_Init();
			void				_Cleanup();

			ElfImage*			_ImageAt(int32 index) const;
			ElfImage*			_ImageForID(image_id id) const;

private:
			BList				fImages;
			status_t			fInitStatus;
};

// UpdateAdapter
class ElfSymbolPatcher::UpdateAdapter {
public:
								UpdateAdapter();
	virtual						~UpdateAdapter();

	virtual	void				ImageAdded(ElfImage* image);
	virtual	void				ImageRemoved(ElfImage* image);
};


// ElfSymbolPatchGroup
class ElfSymbolPatchGroup : private ElfSymbolPatcher::UpdateAdapter {
public:
								ElfSymbolPatchGroup(
									ElfSymbolPatcher* patcher = NULL);
								~ElfSymbolPatchGroup();

			ElfSymbolPatcher*	GetPatcher() const	{ return fPatcher; }

			status_t			AddPatch(const char* symbolName,
										 void* newAddress,
										 void** originalAddress);

			void				RemoveAllPatches();

			status_t			Patch();
			status_t			Restore();

			status_t			Update();

private:
			class PatchInfo : public ElfSymbolPatchInfo {
			public:
				void*			fNewAddress;
			};

private:
	virtual	void				ImageAdded(ElfImage* image);
	virtual	void				ImageRemoved(ElfImage* image);

private:
			ElfSymbolPatcher*	fPatcher;
			BList				fPatchInfos;
			bool				fOwnsPatcher;
			bool				fPatched;
};

} // namespace SymbolPatcher

using namespace SymbolPatcher;


#endif	// ELF_SYMBOL_PATCHER_H
