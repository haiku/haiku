/*
 * Copyright 2001-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SHELF_H
#define _SHELF_H


#include <Dragger.h>
#include <Handler.h>
#include <List.h>
#include <Locker.h>


class BDataIO;
class BPoint;
class BView;
class BEntry;
class _BZombieReplicantView_;
struct entry_ref;

namespace BPrivate {
	struct replicant_data;
	class ShelfContainerViewFilter;
};


class BShelf : public BHandler {
public:
								BShelf(BView* view, bool allowDrags = true,
									const char* shelfType = NULL);
								BShelf(const entry_ref* ref, BView* view,
									bool allowDrags = true,
									const char* shelfType = NULL);
								BShelf(BDataIO* stream, BView* view,
									bool allowDrags = true,
									const char* shelfType = NULL);
								BShelf(BMessage* archive);
	virtual						~BShelf();

	static BArchivable*			Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				MessageReceived(BMessage* message);
			status_t			Save();
	virtual	void				SetDirty(bool state);
			bool				IsDirty() const;

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

	virtual	status_t			Perform(perform_code code, void* data);

			bool				AllowsDragging() const;
			void				SetAllowsDragging(bool state);
			bool				AllowsZombies() const;
			void				SetAllowsZombies(bool state);
			bool				DisplaysZombies() const;
			void				SetDisplaysZombies(bool state);
			bool				IsTypeEnforced() const;
			void				SetTypeEnforced(bool state);

			status_t			SetSaveLocation(BDataIO* stream);
			status_t			SetSaveLocation(const entry_ref* ref);
			BDataIO*			SaveLocation(entry_ref* ref) const;

			status_t			AddReplicant(BMessage* archive,
									BPoint location);
			status_t			DeleteReplicant(BView* replicant);
			status_t			DeleteReplicant(BMessage* archive);
			status_t			DeleteReplicant(int32 index);
			int32				CountReplicants() const;
			BMessage*			ReplicantAt(int32 index, BView** view = NULL,
									uint32* uid = NULL,
									status_t* perr = NULL) const;
			int32				IndexOf(const BView* replicantView) const;
			int32				IndexOf(const BMessage* archive) const;
			int32				IndexOf(uint32 id) const;

protected:
	virtual	bool				CanAcceptReplicantMessage(
									BMessage* archive) const;
	virtual	bool				CanAcceptReplicantView(BRect,
									BView*, BMessage*) const;
	virtual	BPoint				AdjustReplicantBy(BRect, BMessage*) const;

	virtual	void				ReplicantDeleted(int32 index,
									const BMessage* archive,
									const BView *replicant);

private:
	// FBC padding and forbidden methods
	virtual	void				_ReservedShelf2();
	virtual	void				_ReservedShelf3();
	virtual	void				_ReservedShelf4();
	virtual	void				_ReservedShelf5();
	virtual	void				_ReservedShelf6();
	virtual	void				_ReservedShelf7();
	virtual	void				_ReservedShelf8();

								BShelf(const BShelf& other);
			BShelf&				operator=(const BShelf& other);

private:
	friend class BPrivate::ShelfContainerViewFilter;

			status_t			_Archive(BMessage* data) const;
			void				_InitData(BEntry* entry, BDataIO* stream,
									BView* view, bool allowDrags);
			status_t			_DeleteReplicant(
									BPrivate::replicant_data* replicant);
			status_t			_AddReplicant(BMessage* data,
									BPoint* location, uint32 uniqueID);
			BView*				_GetReplicant(BMessage* data, BView* view,
									const BPoint& point, BDragger*& dragger,
									BDragger::relation& relation);
			_BZombieReplicantView_* _CreateZombie(BMessage *data,
									BDragger *&dragger);

			status_t			_GetProperty(BMessage* message,
									BMessage* reply);
	static	void				_GetReplicantData(BMessage* message,
									BView* view, BView*& replicant,
									BDragger*& dragger,
									BDragger::relation& relation);
	static	BArchivable*		_InstantiateObject(BMessage* archive,
									image_id* image);

private:
			BView*				fContainerView;
			BDataIO*			fStream;
			BEntry*				fEntry;
			BList				fReplicants;
			BPrivate::ShelfContainerViewFilter* fFilter;
			uint32				fGenCount;
			bool				fAllowDragging;
			bool				fDirty;
			bool				fDisplayZombies;
			bool				fAllowZombies;
			bool				fTypeEnforced;

			uint32				_reserved[8];
};

#endif	/* _SHELF_H */
