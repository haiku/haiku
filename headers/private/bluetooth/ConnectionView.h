/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2021, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */

#ifndef _CONNECTION_VIEW_H_
#define _CONNECTION_VIEW_H_

#include <Window.h>
#include <View.h>
#include <StringView.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Font.h>
#include <String.h>

namespace Bluetooth {

class BluetoothIconView;

class ConnectionView : public BView {
public:
						ConnectionView(BRect frame,
							BString device, BString address);

	void				Pulse();

private:
	BString				strMessage;
	BluetoothIconView*	fIcon;
	BStringView* 		fMessage;
	BStringView*		fDeviceLabel;
	BStringView*		fDeviceText;
	BStringView*		fAddressLabel;
	BStringView*		fAddressText;
};

}

#endif /* _CONNECTION_VIEW_H_ */
