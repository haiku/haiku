/*
    OBOS ShowImage 0.1 - 17/02/2002 - 22:22 - Fernando Francisco de Oliveira
*/

#ifndef _ShowImageView_h
#define _ShowImageView_h

#include <View.h>

class OffscreenView;

class ShowImageView : public BView
{
public:
	ShowImageView(BRect r, const char* name, uint32 resizingMode, uint32 flags);
	~ShowImageView();
	
	void SetBitmap(BBitmap* pBitmap);
	
	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void FrameResized(float width, float height);	
	virtual void MessageReceived(BMessage* message);
	
	virtual void MouseDown( BPoint point );
	virtual void MouseUp( BPoint point );
	virtual void MouseMoved( BPoint point, uint32 transit, const BMessage *message );
	virtual void Pulse(void);
	
	void FixupScrollBars();
	
	BMenuBar* 	pBar;	
	
private:
	bool 		Selecting, Selected, PointOn;
	
	BRect       SelectedRect, LastPoint, PointRecOn;
	BPoint 		IniPoint, EndPoint;
	
	BBitmap* 	m_pBitmap;
};

#endif /* _ShowImageView_h */
