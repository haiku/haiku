/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hugo Santos, hugosantos@gmail.com
 */
#ifndef _STATUS_REPLICANT_H
#define _STATUS_REPLICANT_H


#include "NetServer.h"

#include <PopUpMenu.h>
#include <View.h>

#include <string>
#include <utility>
#include <vector>


static const uint32 kRegisterStatusReplicant = 'rnsr';
static const uint32 kStatusUpdate = 'stup';
extern const char *kStatusReplicant;


class StatusReplicant : public BView {
	public:
		StatusReplicant();
		StatusReplicant(BMessage *archive);
		virtual ~StatusReplicant();

		static StatusReplicant *Instantiate(BMessage *archive);
		virtual status_t Archive(BMessage *archive, bool deep) const;

		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		virtual void Draw(BRect updateRect);
		virtual void MouseDown(BPoint point);

	private:
		typedef std::pair<std::string, int> InterfaceStatus;
		typedef std::vector<InterfaceStatus> InterfaceStatusList;

		void _Init(bool isReplicant = true);
		void _UpdateFromMessage(BMessage *message);
		void _ShowConfiguration(BMessage *message);
		void _PrepareMenu(const InterfaceStatusList &list);
		void _ChangeStatus(int newStatus);

		BPopUpMenu	fPopUp;
		BBitmap*	fBitmaps[kStatusCount];
		int32		fStatus;
};

#endif	// _STATUS_REPLICANT_H
