/*******************************************************************************
/
/	File:			Shelf.h
/
/   Description:    BShelf stores replicant views that are dropped onto it
/
/	Copyright 1996-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _SHELF_H
#define _SHELF_H

#include <BeBuild.h>
#include <Handler.h>
#include <List.h>

class BDataIO;
class BView;

class BEntry;
extern "C" void  _ReservedShelf1__6BShelfFv(BShelf *const, int32,
	const BMessage*, const BView*);

/*----------------------------------------------------------------*/
/*----- BShelf class ---------------------------------------------*/

class BShelf : public BHandler {
public:
						BShelf(BView *view,
								bool allow_drags = true,
								const char *shelf_type = NULL);
						BShelf(const entry_ref *ref,
								BView *view,
								bool allow_drags = true,
								const char *shelf_type = NULL);
						BShelf(BDataIO *stream,
								BView *view,
								bool allow_drags = true,
								const char *shelf_type = NULL);
						BShelf(BMessage *data);
virtual					~BShelf();

virtual	status_t		Archive(BMessage *data, bool deep = true) const;
static	BArchivable		*Instantiate(BMessage *data);

virtual	void			MessageReceived(BMessage *msg);
		status_t		Save();
virtual	void			SetDirty(bool state);
		bool			IsDirty() const;

virtual BHandler		*ResolveSpecifier(BMessage *msg,
										int32 index,
										BMessage *specifier,
										int32 form,
										const char *property);
virtual status_t		GetSupportedSuites(BMessage *data);

virtual status_t		Perform(perform_code d, void *arg);
		bool			AllowsDragging() const;
		void			SetAllowsDragging(bool state);
		bool			AllowsZombies() const;
		void			SetAllowsZombies(bool state);
		bool			DisplaysZombies() const;
		void			SetDisplaysZombies(bool state);
		bool			IsTypeEnforced() const;
		void			SetTypeEnforced(bool state);

		status_t		SetSaveLocation(BDataIO *data_io);
		status_t		SetSaveLocation(const entry_ref *ref);
		BDataIO			*SaveLocation(entry_ref *ref) const;

		status_t		AddReplicant(BMessage *data, BPoint location);
		status_t		DeleteReplicant(BView *replicant);
		status_t		DeleteReplicant(BMessage *data);
		status_t		DeleteReplicant(int32 index);
		int32			CountReplicants() const;
		BMessage		*ReplicantAt(int32 index,
									BView **view = NULL,
									uint32 *uid = NULL,
									status_t *perr = NULL) const;
		int32			IndexOf(const BView *replicant_view) const;
		int32			IndexOf(const BMessage *archive) const;
		int32			IndexOf(uint32 id) const;

protected:
virtual bool 			CanAcceptReplicantMessage(BMessage*) const;
virtual bool 			CanAcceptReplicantView(BRect, BView*, BMessage*) const;
virtual BPoint 			AdjustReplicantBy(BRect, BMessage*) const;

virtual	void			ReplicantDeleted(int32 index,
										const BMessage *archive,
										const BView *replicant);

/*----- Private or reserved -----------------------------------------*/
private:
friend class _TContainerViewFilter_;
friend void  _ReservedShelf1__6BShelfFv(BShelf *const, int32,
				const BMessage*, const BView*);
						
virtual	void			_ReservedShelf2();
virtual	void			_ReservedShelf3();
virtual	void			_ReservedShelf4();
virtual	void			_ReservedShelf5();

#if !_PR3_COMPATIBLE_
virtual	void			_ReservedShelf6();
virtual	void			_ReservedShelf7();
virtual	void			_ReservedShelf8();
#endif
						BShelf(const BShelf&);
		BShelf			&operator=(const BShelf &);

		status_t		_Archive(BMessage *data) const;
		void			InitData(BEntry *entry,
								BDataIO *stream,
								BView *view,
								bool allow_drags);
		status_t		RealAddReplicant(BMessage *data,
										BPoint *loc,
										uint32 uid);
		status_t		GetProperty(BMessage *msg, BMessage *reply);

		BView			*fContainerView;
		BDataIO			*fStream;
		BEntry			*fEntry;
		BList			fReplicants;
		_TContainerViewFilter_	*fFilter;
		uint32			fGenCount;
		bool			fAllowDragging;
		bool			fDirty;
		bool			fDisplayZombies;
		bool			fAllowZombies;
		bool			fTypeEnforced;

		uint32			_reserved[3];	/* was 5 */
#if !_PR3_COMPATIBLE_
		uint32			_more_reserved[5];
#endif
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _SHELF_H */
