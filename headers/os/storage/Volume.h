/*
 * Copyright 2002-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VOLUME_H
#define _VOLUME_H


#include <sys/types.h>

#include <fs_info.h>
#include <Mime.h>
#include <StorageDefs.h>
#include <SupportDefs.h>

class BDirectory;
class BBitmap;


class BVolume {
public:
							BVolume();
							BVolume(dev_t device);
							BVolume(const BVolume& volume);
	virtual					~BVolume();

			status_t		InitCheck() const;
			status_t		SetTo(dev_t device);
			void			Unset();

			dev_t			Device() const;

			status_t		GetRootDirectory(BDirectory* directory) const;

			off_t			Capacity() const;
			off_t			FreeBytes() const;
			off_t			BlockSize() const;

			status_t		GetName(char* name) const;
			status_t		SetName(const char* name);

			status_t		GetIcon(BBitmap* icon, icon_size which) const;
			status_t		GetIcon(uint8** _data, size_t* _size,
								type_code* _type) const;

			bool			IsRemovable() const;
			bool			IsReadOnly() const;
			bool			IsPersistent() const;
			bool			IsShared() const;
			bool			KnowsMime() const;
			bool			KnowsAttr() const;
			bool			KnowsQuery() const;

			bool			operator==(const BVolume& volume) const;
			bool			operator!=(const BVolume& volume) const;
			BVolume&		operator=(const BVolume& volume);

private:
	virtual void			_TurnUpTheVolume1();
	virtual void			_TurnUpTheVolume2();
	virtual void			_TurnUpTheVolume3();
	virtual void			_TurnUpTheVolume4();
	virtual void			_TurnUpTheVolume5();
	virtual void			_TurnUpTheVolume6();
	virtual void			_TurnUpTheVolume7();
	virtual void			_TurnUpTheVolume8();

			dev_t			fDevice;
			status_t		fCStatus;
			int32			_reserved[8];
};

#endif	// _VOLUME_H
