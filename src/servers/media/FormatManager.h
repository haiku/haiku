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

		status_t	RegisterDecoder(const media_format_description *descriptions,
						int32 descriptionCount, media_format *format, uint32 flags);
		status_t	RegisterEncoder(const media_format_description *descriptions,
						int32 descriptionCount, media_format *format, uint32 flags);

		void		UnregisterDecoder(media_format &format);
		void		UnregisterEncoder(media_format &format);
		
	private:
		status_t	RegisterDescriptions(const media_format_description *descriptions,
						int32 descriptionCount, media_format *format, uint32 flags,
						bool encoder);

	private:
		typedef BPrivate::media::meta_format meta_format;

		BObjectList<meta_format> fDecoderFormats;
		BObjectList<meta_format> fEncoderFormats;
		BObjectList<meta_format> fList;
		BLocker		fLock;
		bigtime_t	fLastUpdate;
		int32		fNextCodecID;
};

#endif // _FORMAT_MANAGER_H
