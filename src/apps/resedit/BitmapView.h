#ifndef BMPVIEW_H
#define BMPVIEW_H

#include <Bitmap.h>
#include <Message.h>
#include <Invoker.h>
#include <PopUpMenu.h>
#include <String.h>
#include <View.h>

enum
{
	M_BITMAP_ADDED = 'mbma',
	M_BITMAP_REMOVED = 'mbmr'
};

class BitmapView : public BView, public BInvoker
{
public:
	BitmapView(BRect frame, const char *name, BMessage *mod, BBitmap *bitmap, 
				const char *label = NULL, border_style = B_PLAIN_BORDER,
				int32 resize = B_FOLLOW_LEFT | B_FOLLOW_TOP, int32 flags = B_WILL_DRAW);
	~BitmapView(void);
	virtual void AttachedToWindow(void);
	
	virtual void Draw(BRect rect);
	virtual void MessageReceived(BMessage *msg);
	virtual void MouseDown(BPoint pt);
	virtual void FrameResized(float w, float h);
	
	virtual void SetBitmap(BBitmap *bitmap);
	BBitmap *GetBitmap(void) const { return fBitmap; }

	bool IsEnabled(void) const { return fEnabled; }
	virtual void SetEnabled(bool value);
	
//	const char *Label(void) const { return fLabel.String(); }
//	virtual void SetLabel(const char *label);
	
	border_style Style(void) const { return fBorderStyle; }
	virtual void SetStyle(border_style style);
	
	bool IsFixedSize(void) const { return fFixedSize; }
	void SetFixedSize(bool isfixed);
	
	bool AcceptsDrops(void) const { return fAcceptDrops; }
	virtual void SetAcceptDrops(bool accept);
	
	bool AcceptsPaste(void) const { return fAcceptPaste; }
	virtual void SetAcceptPaste(bool accept);
	
	bool ConstrainsDrops(void) const { return fConstrainDrops; }
	virtual void SetConstrainDrops(bool value);
	
	void MaxBitmapSize(float *width, float *height) const;
	virtual void SetMaxBitmapSize(const float &width, const float &height);
	
	bool IsBitmapRemovable(void) const { return fRemovableBitmap; }
	void SetBitmapRemovable(bool isremovable);
	
	void RemoveBitmap(void);
	void PasteBitmap(void);
	
private:
	void CalculateBitmapRect(void);
	void ConstrainBitmap(void);
	
	bool ClipboardHasBitmap(void);
	BBitmap *BitmapFromClipboard(void);
	
	float fNoPhotoWidths[4];
	BPoint fNoPhotoOffsets[4];
	
	BBitmap *fBitmap;
	BRect fBitmapRect;
	bool fEnabled;
	BString fLabel;
	border_style fBorderStyle;
	bool fFixedSize;
	bool fAcceptDrops;
	bool fAcceptPaste;
	bool fConstrainDrops;
	float fMaxWidth, fMaxHeight;
	bool fRemovableBitmap;
	BPopUpMenu *fPopUpMenu;
	uint32 fMouseButtons;
	BMenuItem *fPasteItem;
	BMenuItem *fRemoveItem;
};

BRect ScaleRectToFit(const BRect &from, const BRect &to);

#endif
