// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered
//  by the Haiku license.
//
//
//  File:			MethodReplicant.h
//  Authors:		Jérôme Duval,
//
//  Description:	Input Server
//  Created:		October 13, 2004
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef METHOD_REPLICANT_H_
#define METHOD_REPLICANT_H_

#include <PopUpMenu.h>
#include <View.h>
#include "MethodMenuItem.h"

#define REPLICANT_CTL_NAME "MethodReplicant"

class _EXPORT MethodReplicant;

class MethodReplicant : public BView {
	public:
		MethodReplicant(const char* signature);

		MethodReplicant(BMessage *);
		// BMessage * based constructor needed to support archiving
		virtual ~MethodReplicant();

		// archiving overrides
		static MethodReplicant *Instantiate(BMessage *data);
		virtual	status_t Archive(BMessage *data, bool deep = true) const;

		virtual void AttachedToWindow();

		// misc BView overrides
		virtual void MouseDown(BPoint);
		virtual void MouseUp(BPoint);

		virtual void Draw(BRect);

		virtual void MessageReceived(BMessage *);
	private:
		BBitmap *fSegments;
		char *fSignature;
		BPopUpMenu fMenu;

		void UpdateMethod(BMessage *);
		void UpdateMethodIcon(BMessage *);
		void UpdateMethodMenu(BMessage *);
		void UpdateMethodName(BMessage *);
		void AddMethod(BMessage *message);
		void RemoveMethod(BMessage *message);
		MethodMenuItem *FindItemByCookie(void *cookie);
};

#endif
