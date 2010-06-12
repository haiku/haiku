//--------------------------------------------------------------------
//	
//	TestMenuBuilder.h
//
//	Written by: Owen Smith
//	
//--------------------------------------------------------------------

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _TestMenuBuilder_h
#define _TestMenuBuilder_h

#include <Menu.h>
#include <Mime.h> /* for icon_size */

//====================================================================
//	CLASS: TestMenuBuilder

class TestMenuBuilder
{		
	//----------------------------------------------------------------
	//	Entry point

public:
	BMenu*			BuildTestMenu(icon_size size);
	
	
	//----------------------------------------------------------------
	//	Implementation member functions

private:
	BBitmap*		LoadTestBitmap(int32 i, int32 j, icon_size size);
};

#endif /* _TestMenuBuilder_h */
