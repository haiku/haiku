/******************************************************************************
/
/	File:			InputServerFilter.h
/
/	Description:	Add-on class for input_server filters.
/
/	Copyright 1998, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _INPUTSERVERFILTER_H
#define _INPUTSERVERFILTER_H

#include <BeBuild.h>
#include <MessageFilter.h>
#include <SupportDefs.h>


class BInputServerFilter {
public:
							BInputServerFilter();
	virtual					~BInputServerFilter();

	virtual status_t		InitCheck();

	virtual filter_result	Filter(BMessage *message, BList *outList);
	
			status_t		GetScreenRegion(BRegion *region) const;

private:
	virtual void			_ReservedInputServerFilter1();
	virtual void			_ReservedInputServerFilter2();
	virtual void			_ReservedInputServerFilter3();
	virtual void			_ReservedInputServerFilter4();
	uint32					_reserved[4];
};


#endif
