// ----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  File Name:		Directory.cpp
//
//	Description:	BVolume class
// ----------------------------------------------------------------------

#ifndef __sk_volume_h__
#define __sk_volume_h__

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <sys/types.h>

#ifndef _FS_INFO_H
#include <fs_info.h>
//#include "fs_info.h"
#endif
#ifndef _STORAGE_DEFS_H
#include <StorageDefs.h>
//#include "StorageDefs.h"
#endif
#ifndef _MIME_H
#include <Mime.h>
#endif
#ifndef _SUPPORT_DEFS_H
#include <SupportDefs.h>
#endif

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif


class	BDirectory;
class	BBitmap;

class BVolume {
	public:
							BVolume(void);
							BVolume(dev_t dev);
							BVolume(const BVolume& vol);

		virtual				~BVolume(void);

		status_t			InitCheck(void) const;

		status_t			SetTo(dev_t dev);
		void				Unset(void);

		dev_t				Device(void) const;

		status_t			GetRootDirectory(BDirectory* dir) const;

		off_t				Capacity(void) const;
		off_t				FreeBytes(void) const;

		status_t			GetName(char* name) const;
		status_t			SetName(const char* name);

		status_t			GetIcon(
								BBitmap*	icon,
								icon_size	which) const;
		
		bool				IsRemovable(void) const;
		bool				IsReadOnly(void) const;
		bool				IsPersistent(void) const;
		bool				IsShared(void) const;
		bool				KnowsMime(void) const;
		bool				KnowsAttr(void) const;
		bool				KnowsQuery(void) const;
		
		bool				operator==(const BVolume& vol) const;
		bool				operator!=(const BVolume& vol) const;
		BVolume&			operator=(const BVolume& vol);

	private:

	friend class BVolumeRoster;

	virtual	void			_TurnUpTheVolume1();
	virtual	void			_TurnUpTheVolume2();
	
	#if !_PR3_COMPATIBLE_
	virtual	void			_TurnUpTheVolume3();
	virtual	void			_TurnUpTheVolume4();
	virtual	void			_TurnUpTheVolume5();
	virtual	void			_TurnUpTheVolume6();
	virtual	void			_TurnUpTheVolume7();
	virtual	void			_TurnUpTheVolume8();
	#endif

		dev_t				fDev;
		status_t			fCStatus;

	#if !_PR3_COMPATIBLE_
		int32				_reserved[8];
	#endif
};


#ifdef USE_OPENBEOS_NAMESPACE
}
#endif

#endif
