// ----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//
//  File Name:		Directory.cpp
//
//	Description:	BVolume class
// ----------------------------------------------------------------------
/*!
	\file Volume.h
	BVolume interface declarations.
*/
#ifndef _VOLUME_H
#define _VOLUME_H

#include <sys/types.h>

#include <fs_info.h>
#include <Mime.h>
#include <StorageDefs.h>
#include <SupportDefs.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

class BDirectory;
class BBitmap;

class BVolume {
public:
	BVolume();
	BVolume(dev_t dev);
	BVolume(const BVolume &vol);
	virtual ~BVolume();

	status_t InitCheck() const;
	status_t SetTo(dev_t dev);
	void Unset();

	dev_t Device() const;

	bool operator==(const BVolume &volume) const;
	bool operator!=(const BVolume &volume) const;
	BVolume &operator=(const BVolume &volume);

private:
//	friend class BVolumeRoster;

	virtual void _TurnUpTheVolume1();
	virtual void _TurnUpTheVolume2();
	virtual void _TurnUpTheVolume3();
	virtual void _TurnUpTheVolume4();
	virtual void _TurnUpTheVolume5();
	virtual void _TurnUpTheVolume6();
	virtual void _TurnUpTheVolume7();
	virtual void _TurnUpTheVolume8();

	dev_t		fDevice;
	status_t	fCStatus;
	int32		_reserved[8];
};

#ifdef USE_OPENBEOS_NAMESPACE
}	// namespace OpenBeOS
#endif

#endif	// _VOLUME_H
