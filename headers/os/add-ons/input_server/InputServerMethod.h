/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _INPUTSERVERMETHOD_H
#define _INPUTSERVERMETHOD_H


#include <InputServerFilter.h>


class _BMethodAddOn_;
class AddOnManager;
class BMenu;
class InputServer;


class BInputServerMethod : public BInputServerFilter {
public:
								BInputServerMethod(const char* name,
									const uchar* icon);
	virtual						~BInputServerMethod();

	virtual	status_t			MethodActivated(bool active);

			status_t			EnqueueMessage(BMessage* message);

			status_t			SetName(const char* name);
			status_t			SetIcon(const uchar* icon);
			status_t			SetMenu(const BMenu* menu,
									const BMessenger target);

private:
	// FBC padding
	virtual	void				_ReservedInputServerMethod1();
	virtual	void				_ReservedInputServerMethod2();
	virtual	void				_ReservedInputServerMethod3();
	virtual	void				_ReservedInputServerMethod4();

			uint32				_reserved[4];

private:
			friend class AddOnManager;
			friend class InputServer;

private:
			_BMethodAddOn_*		fOwner;
};

#endif // _INPUTSERVERMETHOD_H
