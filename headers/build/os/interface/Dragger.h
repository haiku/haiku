/*******************************************************************************
/
/	File:			Dragger.h
/
/   Description:    BDragger represents a replicant "handle." 
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _DRAGGER_H
#define _DRAGGER_H

#include <BeBuild.h>
#include <Locker.h>
#include <List.h>
#include <View.h>

class BBitmap;
class BMessage;
class BShelf;

/*----------------------------------------------------------------*/
/*----- BDragger class -------------------------------------------*/

class BDragger : public BView {
public:
					BDragger(BRect bounds,
								BView *target,
								uint32 rmask = B_FOLLOW_NONE,
								uint32 flags = B_WILL_DRAW);
					BDragger(BMessage *data);

virtual				~BDragger();
static	BArchivable	*Instantiate(BMessage *data);
virtual	status_t	Archive(BMessage *data, bool deep = true) const;

virtual void		AttachedToWindow();
virtual void		DetachedFromWindow();
virtual void		Draw(BRect update);
virtual void		MouseDown(BPoint where);
virtual	void		MouseUp(BPoint pt);
virtual	void		MouseMoved(BPoint pt, uint32 code, const BMessage *msg);
virtual void		MessageReceived(BMessage *msg);
virtual	void		FrameMoved(BPoint new_position);
virtual	void		FrameResized(float new_width, float new_height);

static	status_t	ShowAllDraggers();			/* system wide!*/
static	status_t	HideAllDraggers();			/* system wide!*/
static	bool		AreDraggersDrawn();

virtual BHandler	*ResolveSpecifier(BMessage *msg,
									int32 index,
									BMessage *specifier,
									int32 form,
									const char *property);
virtual status_t	GetSupportedSuites(BMessage *data);
virtual status_t	Perform(perform_code d, void *arg);

virtual void		ResizeToPreferred();
virtual void		GetPreferredSize(float *width, float *height);
virtual void		MakeFocus(bool state = true);
virtual void		AllAttached();
virtual void		AllDetached();

		status_t	SetPopUp(BPopUpMenu *context_menu);
		BPopUpMenu	*PopUp() const;

		bool		InShelf() const;
		BView		*Target() const;

virtual	BBitmap		*DragBitmap(BPoint *offset, drawing_mode *mode);

protected:
		bool		IsVisibilityChanging() const;

/*----- Private or reserved -----------------------------------------*/
private:

friend class _TContainerViewFilter_;
friend class _rep_data_;
friend class BShelf;
friend void _toggle_handles_(bool);

//+virtual	void		_ReservedDragger1();
virtual	void		_ReservedDragger2();
virtual	void		_ReservedDragger3();
virtual	void		_ReservedDragger4();

		BDragger	&operator=(const BDragger &);

		void		ListManage(bool);
		status_t	determine_relationship();
		status_t	SetViewToDrag(BView *target);
		void		SetShelf(BShelf *);
		void		SetZombied(bool state);
		void		BuildDefaultPopUp();
		void		ShowPopUp(BView *target, BPoint where);
static	bool		sVisible;
static	bool		sInited;
static	BLocker		sLock;
static	BList		sList;

		enum relation {
			TARGET_UNKNOWN,
			TARGET_IS_CHILD,
			TARGET_IS_PARENT,
			TARGET_IS_SIBLING
		};

		BView		*fTarget;
		relation	fRelation;
		BShelf		*fShelf;
		bool		fTransition;
		bool		fIsZombie;
		char		fErrCount;
		bool		_unused2;
		BBitmap		*fBitmap;
		BPopUpMenu	*fPopUp;
		uint32		_reserved[3];	/* was 4 */
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _DRAGGER_H */
