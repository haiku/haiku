/*
 * Copyright 2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef PPP_DESKBAR_REPLICANT__H
#define PPP_DESKBAR_REPLICANT__H

#include <View.h>
#include <PPPDefs.h>

class BPopUpMenu;
class PPPStatusWindow;


class PPPDeskbarReplicant : public BView {
	public:
		PPPDeskbarReplicant(ppp_interface_id id);
		PPPDeskbarReplicant(BMessage *message);
		virtual ~PPPDeskbarReplicant();
		
		static PPPDeskbarReplicant *Instantiate(BMessage *data);
		virtual status_t Archive(BMessage *data, bool deep = true) const;
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		
		virtual void MouseDown(BPoint point);
		virtual void MouseUp(BPoint point);
		
		virtual void Draw(BRect updateRect);

	private:
		void Init();

	private:
		PPPStatusWindow *fWindow;
		BPopUpMenu *fContextMenu;
		ppp_interface_id fID;
		int32 fLastButtons;
};


#endif
