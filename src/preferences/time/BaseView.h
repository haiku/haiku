#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <Message.h>
#include <View.h>


class TTimeBaseView: public BView {
	public:
		TTimeBaseView(BRect frmae, const char *name);
		virtual ~TTimeBaseView();
		
		virtual void Pulse();
		virtual void AttachedToWindow();

		void ChangeTime(BMessage *);
		void SetGMTime(bool);
	protected:
		virtual void DispatchMessage();
	private:
		BMessage *fMessage;
		bool fIsGMT;
};

#endif //TIMEBASE_H
