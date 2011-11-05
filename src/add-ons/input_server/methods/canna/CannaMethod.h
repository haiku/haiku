//
// CannaIM - Canna-based Input Method Add-on for BeOS R4
//

#ifndef _CANNAMETHOD_H
#define _CANNAMETHOD_H

#include <Messenger.h>
#include <InputServerMethod.h>

#if __POWERPC__
#pragma export on
#endif
extern "C" _EXPORT BInputServerMethod* instantiate_input_method();
#if __POWERPC__
#pragma export off
#endif

class CannaLooper;

class CannaMethod : public BInputServerMethod
{
public:
					CannaMethod();
	virtual			~CannaMethod();
	status_t		MethodActivated( bool active );
	status_t		InitCheck();
	filter_result	Filter(BMessage *message, BList *outList);
private:
	BMessenger		cannaLooper;
	void			ReadSettings();
	void			WriteSettings();
};

#endif
