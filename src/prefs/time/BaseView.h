#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <Message.h>
#include <View.h>

#include "TimeMessages.h"

class TTimeBaseView: public BView {
	public:
		TTimeBaseView(BRect frmae, const char *name);
		virtual ~TTimeBaseView();
		
		virtual void Pulse();
		
		void ChangeTime(BMessage *message);
		void SetGMTime(bool);
	protected:
		virtual void DispatchMessage();
	private:
		BMessage *f_message;
		bool f_gmtime;
};

#endif //TIMEBASE_H
