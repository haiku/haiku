#ifndef _YNOP_MAK_DECORATOR_H_
#define _YNOP_MAK_DECORATOR_H_

#include "Decorator.h"

class YMakDecorator: public Decorator{
public:
   YMakDecorator(Layer *lay, uint32 dflags, window_look wlook);
   ~YMakDecorator(void);
   
   click_type Clicked(BPoint pt, uint32 buttons);
   void Resize(BRect rect);
   BRect GetBorderSize(void);
   BPoint GetMinimumSize(void);
   void SetTitle(const char *newtitle);
   void SetFlags(uint32 flags);
   void SetLook(window_look wlook);
   void UpdateFont(void);
   void UpdateTitle(const char *string);
   void SetFocus(bool focused);
   void SetCloseButton(bool down);
   void SetZoomButton(bool down);
   void Draw(BRect update);
   void Draw(void);
   void DrawZoom(BRect r);
   void DrawClose(BRect r);
   void CalculateBorders(void);
private:
   BRect frame;
   
   bool zoomstate;
   bool closestate;

  BRect   CloseRect;
  BRect   ZoomRect;
  
  int	 LeftBorder;
  int	 TopBorder;
  int	 RightBorder;
  int	 BottomBorder;
/*
  bool	 HasFocus;
  bool	 CloseState;
  bool	 ZoomState;
  bool	 DepthState;
*/
};

#endif