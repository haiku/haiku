#ifndef _FORMAT_MANAGER_H
#define _FORMAT_MANAGER_H


#include <ObjectList.h>
#include <Locker.h>

#include "MetaFormat.h"


class FormatManager {
	public:
		FormatManager();
		~FormatManager();

		void		LoadState();
		void		SaveState();

		void		GetFormats(BMessage &message);
		void		MakeFormatFor(BMessage &message);

	private:
		typedef BPrivate::media::meta_format meta_format;

		BObjectList<meta_format> fList;
		BLocker		fLock;
		bigtime_t	fLastUpdate;
		int32		fNextCodecID;
};

#endif // _FORMAT_MANAGER_H
