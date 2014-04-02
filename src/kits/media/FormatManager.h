/*
 * Copyright 2004-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Axel Dörfler
 *		Marcus Overhagen
 */
#ifndef _FORMAT_MANAGER_H
#define _FORMAT_MANAGER_H


#include <Locker.h>
#include <ObjectList.h>
#include <pthread.h>

#include "MetaFormat.h"


class FormatManager {
public:
								~FormatManager();

			void				GetFormats(bigtime_t lastUpdate, BMessage& reply);
			status_t			MakeFormatFor(
									const media_format_description* descriptions,
									int32 descriptionCount,
									media_format& format, uint32 flags,
									void* _reserved);
			void				RemoveFormat(const media_format& format);

			static FormatManager* GetInstance();

private:
								FormatManager();
			static void			CreateInstance();
private:
	typedef BPrivate::media::meta_format meta_format;

			BObjectList<meta_format> fList;
			BLocker				fLock;
			bigtime_t			fLastUpdate;
			int32				fNextCodecID;

			static FormatManager* sInstance;
			static pthread_once_t	sInitOnce;
};

#endif // _FORMAT_MANAGER_H
