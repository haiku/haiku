/*****************************************************************************/
// Haiku InputServer
//
// This is the InputServerMethod implementation
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2002-2004 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include <InputServerMethod.h>
#include <Menu.h>
#include <Messenger.h>
#include "InputServer.h"
#include "InputServerTypes.h"
#include "remote_icon.h"
/**
 *  Method: BInputServerMethod::BInputServerMethod()
 *   Descr: 
 */
BInputServerMethod::BInputServerMethod(const char *name,
                                       const uchar *icon)
{
	CALLED();
	fOwner = new _BMethodAddOn_(this, name, icon);
}


/**
 *  Method: BInputServerMethod::~BInputServerMethod()
 *   Descr: 
 */
BInputServerMethod::~BInputServerMethod()
{
	CALLED();
	delete fOwner;
}


/**
 *  Method: BInputServerMethod::MethodActivated()
 *   Descr: 
 */
status_t
BInputServerMethod::MethodActivated(bool active)
{
    return B_OK;
}


/**
 *  Method: BInputServerMethod::EnqueueMessage()
 *   Descr: 
 */
status_t
BInputServerMethod::EnqueueMessage(BMessage *message)
{
	return ((InputServer*)be_app)->EnqueueMethodMessage(message);
}


/**
 *  Method: BInputServerMethod::SetName()
 *   Descr: 
 */
status_t
BInputServerMethod::SetName(const char *name)
{
    return fOwner->SetName(name);
}


/**
 *  Method: BInputServerMethod::SetIcon()
 *   Descr: 
 */
status_t
BInputServerMethod::SetIcon(const uchar *icon)
{
    return fOwner->SetIcon(icon);
}


/**
 *  Method: BInputServerMethod::SetMenu()
 *   Descr: 
 */
status_t
BInputServerMethod::SetMenu(const BMenu *menu,
                            const BMessenger target)
{
    return fOwner->SetMenu(menu, target);
}


/**
 *  Method: BInputServerMethod::_ReservedInputServerMethod1()
 *   Descr: 
 */
void
BInputServerMethod::_ReservedInputServerMethod1()
{
}


/**
 *  Method: BInputServerMethod::_ReservedInputServerMethod2()
 *   Descr: 
 */
void
BInputServerMethod::_ReservedInputServerMethod2()
{
}


/**
 *  Method: BInputServerMethod::_ReservedInputServerMethod3()
 *   Descr: 
 */
void
BInputServerMethod::_ReservedInputServerMethod3()
{
}


/**
 *  Method: BInputServerMethod::_ReservedInputServerMethod4()
 *   Descr: 
 */
void
BInputServerMethod::_ReservedInputServerMethod4()
{
}


_BMethodAddOn_::_BMethodAddOn_(BInputServerMethod *method, const char *name,
	const uchar *icon)
	: fMethod(method),
	fMenu(NULL)
{
	fName = strdup(name);
	if (icon != NULL)
		memcpy(fIcon, icon, 16*16*1);
	else
		memset(fIcon, 0x1d, 16*16*1);
}


_BMethodAddOn_::~_BMethodAddOn_()
{
	free(fName);
	delete fMenu;
}


status_t
_BMethodAddOn_::SetName(const char* name)
{
	CALLED();
	if (fName)
		free(fName);
	if (name)
		fName = strdup(name);

	BMessage msg(IS_UPDATE_NAME);
	msg.AddPointer("cookie", fMethod);
	msg.AddString("name", name);
	if (((InputServer*)be_app)->MethodReplicant())
		return ((InputServer*)be_app)->MethodReplicant()->SendMessage(&msg);
	else
		return B_ERROR;
}


status_t
_BMethodAddOn_::SetIcon(const uchar* icon)
{	
	CALLED();

	if (icon != NULL)
		memcpy(fIcon, icon, 16*16*1);
	else
		memset(fIcon, 0x1d, 16*16*1);

	BMessage msg(IS_UPDATE_ICON);
	msg.AddPointer("cookie", fMethod);
	msg.AddData("icon", B_RAW_TYPE, icon, 16*16*1);
	if (((InputServer*)be_app)->MethodReplicant())
		return ((InputServer*)be_app)->MethodReplicant()->SendMessage(&msg);
	else
		return B_ERROR;
}


status_t
_BMethodAddOn_::SetMenu(const BMenu *menu, const BMessenger &messenger)
{
	CALLED();
	fMenu = menu;
	fMessenger = messenger;

	BMessage msg(IS_UPDATE_MENU);
	msg.AddPointer("cookie", fMethod);
	BMessage menuMsg;
	if (menu)
		menu->Archive(&menuMsg);
	msg.AddMessage("menu", &menuMsg);
	msg.AddMessenger("target", messenger);
	if (((InputServer*)be_app)->MethodReplicant())
		return ((InputServer*)be_app)->MethodReplicant()->SendMessage(&msg);
	else
		return B_ERROR;
}


status_t
_BMethodAddOn_::MethodActivated(bool activate)
{
	CALLED();
	if (fMethod) {
		PRINT(("%s cookie %p\n", __PRETTY_FUNCTION__, fMethod));
		if (activate && ((InputServer*)be_app)->MethodReplicant()) {
			BMessage msg(IS_UPDATE_METHOD);
        		msg.AddPointer("cookie", fMethod);
                	((InputServer*)be_app)->MethodReplicant()->SendMessage(&msg);
		}
		return fMethod->MethodActivated(activate);
	}
	return B_ERROR;
}


status_t
_BMethodAddOn_::AddMethod()
{
	PRINT(("%s cookie %p\n", __PRETTY_FUNCTION__, fMethod));
	BMessage msg(IS_ADD_METHOD);
	msg.AddPointer("cookie", fMethod);
	msg.AddString("name", fName);
	msg.AddData("icon", B_RAW_TYPE, fIcon, 16*16*1);
	if (((InputServer*)be_app)->MethodReplicant())
		return ((InputServer*)be_app)->MethodReplicant()->SendMessage(&msg);
	else
		return B_ERROR;
}


KeymapMethod::KeymapMethod()
        : BInputServerMethod("Roman", kRemoteBits)
{

}


KeymapMethod::~KeymapMethod()
{

}

