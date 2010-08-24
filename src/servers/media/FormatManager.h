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

#include "MetaFormat.h"


class FormatManager {
public:
								FormatManager();
								~FormatManager();

			void				LoadState();
			void				SaveState();

			void				GetFormats(BMessage& message);
			void				MakeFormatFor(BMessage& message);
			void				RemoveFormat(const media_format& format);

private:
	typedef BPrivate::media::meta_format meta_format;

			BObjectList<meta_format> fList;
			BLocker				fLock;
			bigtime_t			fLastUpdate;
			int32				fNextCodecID;
};

#endif // _FORMAT_MANAGER_H
