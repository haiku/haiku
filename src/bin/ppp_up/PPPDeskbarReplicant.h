/*
 * Copyright 2004-2005, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef PPP_DESKBAR_REPLICANT__H
#define PPP_DESKBAR_REPLICANT__H

#include <View.h>
#include <PPPDefs.h>

class StatusWindow;


class PPPDeskbarReplicant : public BView {
	public:
		PPPDeskbarReplicant(ppp_interface_id id);
		PPPDeskbarReplicant(BMessage *message);
		virtual ~PPPDeskbarReplicant();
		
		static PPPDeskbarReplicant *Instantiate(BMessage *data);
		virtual status_t Archive(BMessage *data, bool deep = true) const;
		
		virtual void MouseDown(BPoint point);
		virtual void MouseUp(BPoint point);
		
		virtual void Draw(BRect updateRect);

	private:
		void Init();

	private:
		StatusWindow *fWindow;
		ppp_interface_id fID;
		int32 fLastButtons;
};


#endif
