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
	StatusReplicant(BMessage *);
	virtual ~StatusReplicant();

	static StatusReplicant *Instantiate(BMessage *);
	status_t Archive(BMessage *, bool deep) const;

	void AttachedToWindow();
	void MessageReceived(BMessage *);

	void MouseDown(BPoint point);

private:
	typedef std::pair<std::string, int> InterfaceStatus;
	typedef std::vector<InterfaceStatus> InterfaceStatusList;

	void UpdateFromMessage(BMessage *);
	void ShowConfiguration(BMessage *);
	void PrepareMenu(const InterfaceStatusList &);
	void ChangeStatus(int newStatus);

	BPopUpMenu fPopup;
	BBitmap *fBitmaps[kStatusCount];
};

#endif
