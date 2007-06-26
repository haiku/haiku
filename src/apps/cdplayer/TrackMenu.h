#ifndef TRACKMENU_H
#define TRACKMENU_H

#include <Invoker.h>
#include <View.h>

class TrackMenu : public BView, public BInvoker {
	public:
		TrackMenu(const BRect &frame, const char *name, BMessage *msg,
			const int32 &resize = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			const int32 &flags = B_WILL_DRAW);
		~TrackMenu();
	
		void AttachedToWindow();
		void MessageReceived(BMessage *msg);
		void Draw(BRect update);
		void MouseDown(BPoint pt);
		void MouseUp(BPoint pt);
		void MouseMoved(BPoint pt, uint32 transit, const BMessage *msg);

		int32 CountItems() const { return fCount; }
		void SetItemCount(const int32 &count);

		int32 Value() const { return fCurrentItem; }
		void SetValue(const int32 &value);

	private:
		int32 ItemAt(const BPoint &pt);

		int32 fCurrentItem;
		int32 fCount;
		BRect fItemRect;

		bool fIsTracking;

		float fFontHeight;
};

#endif	// TRACKMENU_H
