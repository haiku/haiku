/*
    OBOS ShowImage 0.1 - 17/02/2002 - 22:22 - Fernando Francisco de Oliveira
*/

#ifndef _ShowImageView_h
#define _ShowImageView_h

#include <View.h>

class ShowImageView : public BView
{
public:
	ShowImageView(BRect r, const char* name, uint32 resizingMode, uint32 flags);
	~ShowImageView();
	
	void SetBitmap(BBitmap* pBitmap);
	BBitmap *GetBitmap();
	
	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void FrameResized(float width, float height);	
	virtual void MessageReceived(BMessage* message);
	
	void FixupScrollBars();
	
	BMenuBar* 	pBar;	
	
private:
	
	BBitmap* 	m_pBitmap;
};

#endif /* _ShowImageView_h */
