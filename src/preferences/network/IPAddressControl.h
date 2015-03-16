/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 */
#ifndef IP_ADDRESS_CONTROL_H
#define IP_ADDRESS_CONTROL_H


#include <TextControl.h>


class IPAddressControl : public BTextControl {
public:
								IPAddressControl(int family, const char* label,
									const char* name);
	virtual						~IPAddressControl();

			bool				AllowEmpty() const;
			void				SetAllowEmpty(bool empty);

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			void				_UpdateMark();

private:
			int					fFamily;
			bool				fAllowEmpty;
};


#endif // IP_ADDRESS_CONTROL_H
