/******************************************************************************
/
/	File:			InputServerMethod.h
/
/	Description:	Add-on class for input_server methods.
/
/	Copyright 1998, Be Incorporated, All Rights Reserved.
/
/******************************************************************************/

#ifndef _INPUTSERVERMETHOD_H
#define _INPUTSERVERMETHOD_H

#include <BeBuild.h>
#include <InputServerFilter.h>
#include <SupportDefs.h>


class _BMethodAddOn_;


class BInputServerMethod : public BInputServerFilter {
public:
							BInputServerMethod(const char		*name,
											   const uchar		*icon);
	virtual					~BInputServerMethod();

	virtual status_t		MethodActivated(bool active);

	status_t				EnqueueMessage(BMessage *message);

	status_t				SetName(const char *name);
	status_t				SetIcon(const uchar *icon);
	status_t				SetMenu(const BMenu *menu, const BMessenger target);

private:
	_BMethodAddOn_*			fOwner;
	
	virtual void			_ReservedInputServerMethod1();
	virtual void			_ReservedInputServerMethod2();
	virtual void			_ReservedInputServerMethod3();
	virtual void			_ReservedInputServerMethod4();
	uint32					_reserved[4];
};


#endif
