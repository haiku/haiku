//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

/*
 * AccessHelper.h
 *
 * The AccessHelper class automatically increases the given variable by
 * one on construction and decreases it by one on destruction. If a NULL
 * pointer is given, it will do nothing.
 *
 */


#ifndef _ACCESS_HELPER__H
#define _ACCESS_HELPER__H

#include <OS.h>
#include <SupportDefs.h>


class AccessHelper {
	private:
		// copies are not allowed!
		AccessHelper(const AccessHelper& copy);
		AccessHelper& operator= (const AccessHelper& copy);

	public:
		AccessHelper(vint32 *access) : fAccess(access)
		{
			if(fAccess)
				atomic_add(fAccess, 1);
		}
		
		~AccessHelper()
		{
			if(fAccess)
				atomic_add(fAccess, -1);
		}

	private:
	vint32 *fAccess;
};

#endif
