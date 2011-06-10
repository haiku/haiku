/*
 * Copyright 2011, Rene Gollent, rene@gollent.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef MEMORY_VIEW_H
#define MEMORY_VIEW_H


#include <View.h>

#include "Team.h"


class MemoryView : public BView {
public:
	class Listener;

public:
								MemoryView(
									Team* team, Listener* listener);
	virtual						~MemoryView();

	static MemoryView*			Create(Team* team, Listener* listener);
									// throws

			void				UnsetListener();

			void				SetTargetAddress(target_addr_t address);

			void				TargetAddressChanged(target_addr_t address);

	virtual void				TargetedByScrollView(BScrollView* scrollView);

	virtual void				Draw(BRect rect);
	virtual void				MessageReceived(BMessage* message);

private:
	void						_Init();

private:
	Team*						fTeam;
	target_addr_t				fTargetAddress;
	Listener*					fListener;
};


class MemoryView::Listener {
public:
	virtual						~Listener();

	virtual void				SetTargetAddressRequested(
									target_addr_t address) = 0;
};


#endif // MEMORY_VIEW_H
