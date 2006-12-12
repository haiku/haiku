/*******************************************************************************
/
/	File:			PopUpMenu.h
/
/   Description:    BPopUpMenu represents a menu that pops up when you
/                   activate it.
/
/	Copyright 1994-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _POP_UP_MENU_H
#define _POP_UP_MENU_H

#include <BeBuild.h>
#include <Menu.h>

/*----------------------------------------------------------------*/
/*----- BPopUpMenu class -----------------------------------------*/

class BPopUpMenu : public BMenu
{
public:
					BPopUpMenu(	const char *title,
								bool radioMode = true,
								bool autoRename = true,
								menu_layout layout = B_ITEMS_IN_COLUMN
								);
					BPopUpMenu(BMessage *data);
virtual				~BPopUpMenu();
virtual status_t	Archive(BMessage *data, bool deep = true) const;
static	BArchivable	*Instantiate(BMessage *data);

		BMenuItem	*Go(BPoint where,
						bool delivers_message = false,
						bool open_anyway = false,
						bool async = false);
		BMenuItem	*Go(BPoint where,
						bool delivers_message,
						bool open_anyway,
						BRect click_to_open,
						bool async = false);

virtual void		MessageReceived(BMessage *msg);
virtual	void		MouseDown(BPoint pt);
virtual	void		MouseUp(BPoint pt);
virtual	void		MouseMoved(BPoint pt, uint32 code, const BMessage *msg);
virtual	void		AttachedToWindow();
virtual	void		DetachedFromWindow();
virtual	void		FrameMoved(BPoint new_position);
virtual	void		FrameResized(float new_width, float new_height);

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

		void		SetAsyncAutoDestruct(bool state);
		bool		AsyncAutoDestruct() const;

protected:
virtual	BPoint		ScreenLocation();

virtual	void		_ReservedPopUpMenu1();
virtual	void		_ReservedPopUpMenu2();
virtual	void		_ReservedPopUpMenu3();

		BPopUpMenu	&operator=(const BPopUpMenu &);

/*----- Private or reserved -----------------------------------------*/
private:
		BMenuItem	*_Go(BPoint where, bool autoInvoke, bool startOpened,
							BRect *specialRect, bool async);
		BMenuItem	*_StartTrack(BPoint where, bool autoInvoke, bool startOpened, BRect *specialRect);
		BMenuItem	*_WaitMenu(void *data);

static	int32	_thread_entry(void *);

		BPoint		fWhere;
		bool		fUseWhere;
		bool		fAutoDestruct;
		bool		_fUnusedBool1;
		bool		_fUnusedBool2;
		thread_id	fTrackThread;
		uint32		_reserved[3];
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _POP_UP_MENU_H */
