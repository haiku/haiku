/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _AUTO_ICON_H_
#define _AUTO_ICON_H_


#include <SupportDefs.h>

class BBitmap;


class AutoIcon {
	public:
		AutoIcon(const char* signature)
			:
			fSignature(signature),
			fbits(0),
			fBitmap(0)
		{
		}

		AutoIcon(const uchar* bits)
			:
			fSignature(0),
			fbits(bits),
			fBitmap(0)
		{
		}

		~AutoIcon();

	  	operator BBitmap*()
	  	{
	  		return Bitmap();
	  	}

		BBitmap* Bitmap();

	private:
		const char*		fSignature;
		const uchar*	fbits;
		BBitmap*		fBitmap;
};

#endif // _AUTO_ICON_H_
