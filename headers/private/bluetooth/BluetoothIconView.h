/*
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */

#ifndef _BLUETOOTH_ICON_VIEW_H_
#define _BLUETOOTH_ICON_VIEW_H_

#include <View.h>
#include <Bitmap.h>
#include <MimeType.h>
#include <IconUtils.h>

namespace Bluetooth {

class BluetoothIconView : public BView {
public:
						BluetoothIconView();
						~BluetoothIconView();

	void 				Draw(BRect rect);

private:
	static BBitmap*		fBitmap;
	static int32		fRefCount;
};

}

#endif /* _BLUETOOTH_ICON_VIEW_H_ */
