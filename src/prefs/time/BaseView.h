#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <Message.h>
#include <View.h>

#include "TimeMessages.h"

class TTimeBaseView: public BView
{
	public:
		TTimeBaseView(BRect frmae, const char *name);
		virtual ~TTimeBaseView();
		
		virtual void Pulse();
	private:
		BMessage *f_message;
};
#endif //TIMEBASE_H
