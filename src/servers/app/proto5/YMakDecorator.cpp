/*******************************************************
*   YMakDecorator
*
*   Writen by YNOP.
*    used BeOS Magnify and decor to try to figure out
*    how it looks.
*   Originaly writen on AtheOS as MakDadddy Decor.
*   Hacked around several times, and then ported
*    (by me again) to OBOS app_server Decorator API
*   
*   TODO
*    Fix title issuse (We need a Font width to center by)
*    Fix any issuse that came about via porting 
*     These may have to do with the fact that the origin is
*     differnat for the two API's and also with the fact
*     that we can test anything yet :P
*   Border size should change with Font Height
*
*   @author YNOP (ynop@beunited.org)
*   @version 1 (p5)
*******************************************************/


#include "Layer.h"
#include "DisplayDriver.h"
#include "YMakDecorator.h"

//#define DEBUG_DECOR

#ifdef DEBUG_DECOR
#include <stdio.h>
#endif

// Hardcoded sized .. TOP should be Font based...
#define TOP   22
#define FRAME 6

/*******************************************************
*   @description
*******************************************************/
YMakDecorator::YMakDecorator(Layer *lay, uint32 dflags, window_look wlook):Decorator(lay, dflags, wlook){
   if(flags & B_NO_BORDER_WINDOW_LOOK){
      LeftBorder = 0;
      TopBorder = 0;
      RightBorder = 0;
      BottomBorder = 0;
   }else{
      TopBorder = TOP;
      LeftBorder = FRAME;
      RightBorder = FRAME;
      BottomBorder = FRAME;
   }
   Resize(lay->frame);
}

/*******************************************************
*   @description
*******************************************************/
YMakDecorator::~YMakDecorator(void){
}

/*******************************************************
*   @description
*******************************************************/
click_type YMakDecorator::Clicked(BPoint pt, uint32 buttons){
#ifdef DEBUG_DECOR
printf("YMakDecorator():Clicked(%f,%f)\n",pt.x,pt.y);
printf("ZoomRect:");ZoomRect.PrintToStream();
#endif

   if(CloseRect.Contains(pt)){
      return CLICK_CLOSE;
   }
   if(ZoomRect.Contains(pt)){
      return CLICK_ZOOM;
   }
   //return CLICK_RESIZE_RB;
   //return CLICK_NONE;
   return CLICK_DRAG;
}

/*******************************************************
*   @description
*******************************************************/
void YMakDecorator::Resize(BRect rect){
   frame = rect;
   
   BRect But;
   But.left = frame.left + 0;
   But.right = frame.left + 12;
   But.top = frame.top + 0;
   But.bottom = frame.top + 12;
   
   CloseRect = But;
   CloseRect.left += 4;
   CloseRect.top += 4;
   CloseRect.right += 4;
   CloseRect.bottom += 4;
   
}

/*******************************************************
*   @description  
*******************************************************/
BRect YMakDecorator::GetBorderSize(void){
   // I dont know what this does yet :P
	return bsize;
}

/*******************************************************
*   @description
*******************************************************/
BPoint YMakDecorator::GetMinimumSize(void){
	return minsize;
}

/*******************************************************
*   @description
*******************************************************/
void YMakDecorator::SetFlags(uint32 dflags){
	flags=dflags;
}

/*******************************************************
*   @description
*******************************************************/
void YMakDecorator::UpdateFont(void){
}

/*******************************************************
*   @description
*******************************************************/
void YMakDecorator::UpdateTitle(const char *string){
}

/*******************************************************
*   @description
*******************************************************/
void YMakDecorator::SetFocus(bool bfocused){
	focused=bfocused;
}

/*******************************************************
*   @description
*******************************************************/
void YMakDecorator::SetCloseButton(bool down){
	closestate=down;
}

/*******************************************************
*   @description
*******************************************************/
void YMakDecorator::SetZoomButton(bool down){
	zoomstate=down;
}

/*******************************************************
*   @description
*******************************************************/
void YMakDecorator::Draw(BRect update){
   BRect b = frame;
   BRect cOBounds = frame;
   
   rgb_color cB = {0,  0,  0,  255};
   rgb_color cW = {251,251,251,255};
   rgb_color cG = {154,154,154,255};
   rgb_color cD = {113,113,113,255};
   rgb_color cL = {203,203,203,255};
   
   // Debug yellow backgound to make sure we cover everthign :)
   // Take this out in real ver for speed :P
   rgb_color yellow = {255,255,0};
   driver->FillRect(frame,yellow);

   // Stroke outside border
   driver->SetHighColor(cB.red,cB.green,cB.blue,cB.alpha);   
   driver->StrokeRect(b,NULL);
   
   // Stroke inside border
   b = cOBounds;
   b.top += TOP - 1;
   b.bottom -= FRAME-1;
   b.left += FRAME - 1;
   b.right -= FRAME-1;
   driver->StrokeRect(b,NULL);
   
   if(focused){
      // Stroke outer boder (raized)
      b = cOBounds;
      b.left += 1;
      b.right -= 1;
      b.top += 1;
      b.bottom -= 1;
      driver->SetHighColor(cW.red,cW.green,cW.blue,cW.alpha);
      driver->MovePenTo(BPoint(b.left,b.top));driver->StrokeLine(BPoint(b.right,b.top),NULL);

      driver->MovePenTo(BPoint(b.left,b.bottom));driver->StrokeLine(BPoint(b.left,b.top),NULL);
      driver->SetHighColor(cG.red,cG.green,cG.blue,cG.alpha);
      driver->MovePenTo(BPoint(b.right,b.top));driver->StrokeLine(BPoint(b.right,b.bottom),NULL);
      driver->MovePenTo(BPoint(b.right,b.bottom));driver->StrokeLine(BPoint(b.left,b.bottom),NULL);
      
      // Stroke inter border (raized)
      b = cOBounds;
      b.top += TOP - 2;
      b.bottom -= FRAME - 2;
      b.left += FRAME - 2;
      b.right -= FRAME - 2;
      driver->SetHighColor(cG.red,cG.green,cG.blue,cG.alpha);
      driver->MovePenTo(BPoint(b.left,b.top));driver->StrokeLine(BPoint(b.right,b.top),NULL);
      driver->MovePenTo(BPoint(b.left,b.bottom));driver->StrokeLine(BPoint(b.left,b.top),NULL);
      driver->SetHighColor(cW.red,cW.green,cW.blue,cW.alpha);
      driver->MovePenTo(BPoint(b.right,b.top));driver->StrokeLine(BPoint(b.right,b.bottom),NULL);
      driver->MovePenTo(BPoint(b.right,b.bottom));driver->StrokeLine(BPoint(b.left,b.bottom),NULL);
      
      driver->SetHighColor(cL.red,cL.green,cL.blue,cL.alpha);
      // Fill top
      b = cOBounds;
      b.bottom = b.top + TOP - 3;
      b.top += 2;
      b.left += 2;
      b.right -= 2;
      driver->FillRect(b,NULL);
      // Fill left
      b = cOBounds;
      b.left += 2;
      b.right = cOBounds.left + FRAME - 3;
      b.top = cOBounds.top + TOP - 2;
      b.bottom -= 2;
      driver->FillRect(b,NULL);
      // Fill right
      b = cOBounds;
      b.left = b.right - FRAME + 3;
      b.right -= 2;
      b.top = b.top + TOP - 2;
      b.bottom -= 2;
      driver->FillRect(b,NULL);
      // fill bottom
      b = cOBounds;
      b.top = b.bottom - FRAME + 3;
      b.bottom -= 2;
      b.left += FRAME - 2;
      b.right -= FRAME - 2;
      driver->FillRect(b,NULL);
      
      // Stroke the lines .. stoke them  oohh ya .. storkem ..
      // 
      b = cOBounds;
      b.left += 20;
      b.right -= 20;
      b.top += 4;
      b.bottom += 10;
      
      //if((Flags & WND_NO_DEPTH_BUT) == 0){
      //   b.right -= 20;	 
      //}
      
      
      driver->SetHighColor(cW.red,cW.green,cW.blue,cW.alpha);
      driver->MovePenTo(BPoint(b.left,b.top+0));driver->StrokeLine(BPoint(b.right-1,b.top+0),NULL);
      driver->MovePenTo(BPoint(b.left,b.top+2));driver->StrokeLine(BPoint(b.right-1,b.top+2),NULL);
      driver->MovePenTo(BPoint(b.left,b.top+4));driver->StrokeLine(BPoint(b.right-1,b.top+4),NULL);
      driver->MovePenTo(BPoint(b.left,b.top+6));driver->StrokeLine(BPoint(b.right-1,b.top+6),NULL);
      driver->MovePenTo(BPoint(b.left,b.top+8));driver->StrokeLine(BPoint(b.right-1,b.top+8),NULL);
      driver->MovePenTo(BPoint(b.left,b.top+10));driver->StrokeLine(BPoint(b.right-1,b.top+10),NULL);
      
#ifdef DEBUG_DECOR
printf("About to draw high lines\n");
printf("do done\n");
#endif
      driver->SetHighColor(cD.red,cD.green,cD.blue,cD.alpha);
      driver->MovePenTo(BPoint(b.left+1,b.top+1));driver->StrokeLine(BPoint(b.right,b.top+1),NULL);
      driver->MovePenTo(BPoint(b.left+1,b.top+3));driver->StrokeLine(BPoint(b.right,b.top+3),NULL);
      driver->MovePenTo(BPoint(b.left+1,b.top+5));driver->StrokeLine(BPoint(b.right,b.top+5),NULL);
      driver->MovePenTo(BPoint(b.left+1,b.top+7));driver->StrokeLine(BPoint(b.right,b.top+7),NULL);
      driver->MovePenTo(BPoint(b.left+1,b.top+9));driver->StrokeLine(BPoint(b.right,b.top+9),NULL);
      driver->MovePenTo(BPoint(b.left+1,b.top+11));driver->StrokeLine(BPoint(b.right,b.top+11),NULL);
            
   }else{
      driver->SetHighColor(cL.red,cL.green,cL.blue,cL.alpha);
      // Fill top
      b = cOBounds;
      b.bottom = b.top + TOP - 2;
      b.top += 1;
      b.left += 1;
      b.right -= 1;
      driver->FillRect(b,NULL);
      // Fill left side
      b = cOBounds;
      b.left += 1;
      b.right = b.left + FRAME - 2;
      b.top = b.top + TOP - 1;
      b.bottom -= 1;
      driver->FillRect(b,NULL);
      // fill right
      b = cOBounds;
      b.left = b.right - FRAME + 2;
      b.right -= 1;
      b.top = b.top + TOP - 1;
      b.bottom -= 1;
      driver->FillRect(b,NULL);
      // fill bottom
      b = cOBounds;
      b.top = b.bottom - FRAME + 2;
      b.bottom -= 1;
      b.left += FRAME - 1;
      b.right -= FRAME - 1;
      driver->FillRect(b,NULL);
   }

   
/*   if((flags & WND_NO_TITLE) == 0){
      // Stroke The title Text
      driver->SetFgColor(cL.red,cL.green,cL.blue,cL.alpha);
      b.left = 30;
      b.right = 100;
      //Font *font = new Font(DEFAULT_FONT_WINDOW);
      //b.right = font->GetStringWidth(Title.c_str());
      
      int32 len = strlen(Title.c_str());
      b.right = b.left + len * 7;
      
      
      b.top = 2;
      b.bottom = TOP - 3;
      V->FillRect(b);
      if(HasFocus){
         V->SetFgColor(0,0,0,255);
      }else{
	V->SetFgColor(cD); 
      }
      V->SetBgColor(cL);
      V->MovePenTo(34,TOP-7);
      V->DrawString(Title.c_str(), -1 );
   }
*/
   
   
   if(!(flags & B_NOT_CLOSABLE)){
      DrawClose(CloseRect);
   }
   if(!(flags & B_NOT_ZOOMABLE)){
      DrawZoom(ZoomRect);
   }


/*	if(!(flags & B_NOT_RESIZABLE))
*/
}

void YMakDecorator::Draw(void)
{
   BRect b = frame;
   BRect cOBounds = frame;
   
   rgb_color cB = {0,  0,  0,  255};
   rgb_color cW = {251,251,251,255};
   rgb_color cG = {154,154,154,255};
   rgb_color cD = {113,113,113,255};
   rgb_color cL = {203,203,203,255};
   
   // Debug yellow backgound to make sure we cover everthign :)
   // Take this out in real ver for speed :P
   rgb_color yellow = {255,255,0};
   driver->FillRect(frame,yellow);

   // Stroke outside border
   driver->SetHighColor(cB.red,cB.green,cB.blue,cB.alpha);   
   driver->StrokeRect(b,NULL);
   
   // Stroke inside border
   b = cOBounds;
   b.top += TOP - 1;
   b.bottom -= FRAME-1;
   b.left += FRAME - 1;
   b.right -= FRAME-1;
   driver->StrokeRect(b,NULL);
   
   if(focused){
      // Stroke outer boder (raized)
      b = cOBounds;
      b.left += 1;
      b.right -= 1;
      b.top += 1;
      b.bottom -= 1;
      driver->SetHighColor(cW.red,cW.green,cW.blue,cW.alpha);
      driver->MovePenTo(BPoint(b.left,b.top));driver->StrokeLine(BPoint(b.right,b.top),NULL);

      driver->MovePenTo(BPoint(b.left,b.bottom));driver->StrokeLine(BPoint(b.left,b.top),NULL);
      driver->SetHighColor(cG.red,cG.green,cG.blue,cG.alpha);
      driver->MovePenTo(BPoint(b.right,b.top));driver->StrokeLine(BPoint(b.right,b.bottom),NULL);
      driver->MovePenTo(BPoint(b.right,b.bottom));driver->StrokeLine(BPoint(b.left,b.bottom),NULL);
      
      // Stroke inter border (raized)
      b = cOBounds;
      b.top += TOP - 2;
      b.bottom -= FRAME - 2;
      b.left += FRAME - 2;
      b.right -= FRAME - 2;
      driver->SetHighColor(cG.red,cG.green,cG.blue,cG.alpha);
      driver->MovePenTo(BPoint(b.left,b.top));driver->StrokeLine(BPoint(b.right,b.top),NULL);
      driver->MovePenTo(BPoint(b.left,b.bottom));driver->StrokeLine(BPoint(b.left,b.top),NULL);
      driver->SetHighColor(cW.red,cW.green,cW.blue,cW.alpha);
      driver->MovePenTo(BPoint(b.right,b.top));driver->StrokeLine(BPoint(b.right,b.bottom),NULL);
      driver->MovePenTo(BPoint(b.right,b.bottom));driver->StrokeLine(BPoint(b.left,b.bottom),NULL);
      
      driver->SetHighColor(cL.red,cL.green,cL.blue,cL.alpha);
      // Fill top
      b = cOBounds;
      b.bottom = b.top + TOP - 3;
      b.top += 2;
      b.left += 2;
      b.right -= 2;
      driver->FillRect(b,NULL);
      // Fill left
      b = cOBounds;
      b.left += 2;
      b.right = cOBounds.left + FRAME - 3;
      b.top = cOBounds.top + TOP - 2;
      b.bottom -= 2;
      driver->FillRect(b,NULL);
      // Fill right
      b = cOBounds;
      b.left = b.right - FRAME + 3;
      b.right -= 2;
      b.top = b.top + TOP - 2;
      b.bottom -= 2;
      driver->FillRect(b,NULL);
      // fill bottom
      b = cOBounds;
      b.top = b.bottom - FRAME + 3;
      b.bottom -= 2;
      b.left += FRAME - 2;
      b.right -= FRAME - 2;
      driver->FillRect(b,NULL);
      
      // Stroke the lines .. stoke them  oohh ya .. storkem ..
      // 
      b = cOBounds;
      b.left += 20;
      b.right -= 20;
      b.top += 4;
      b.bottom += 10;
      
      //if((Flags & WND_NO_DEPTH_BUT) == 0){
      //   b.right -= 20;	 
      //}
      
      
      driver->SetHighColor(cW.red,cW.green,cW.blue,cW.alpha);
      driver->MovePenTo(BPoint(b.left,b.top+0));driver->StrokeLine(BPoint(b.right-1,b.top+0),NULL);
      driver->MovePenTo(BPoint(b.left,b.top+2));driver->StrokeLine(BPoint(b.right-1,b.top+2),NULL);
      driver->MovePenTo(BPoint(b.left,b.top+4));driver->StrokeLine(BPoint(b.right-1,b.top+4),NULL);
      driver->MovePenTo(BPoint(b.left,b.top+6));driver->StrokeLine(BPoint(b.right-1,b.top+6),NULL);
      driver->MovePenTo(BPoint(b.left,b.top+8));driver->StrokeLine(BPoint(b.right-1,b.top+8),NULL);
      driver->MovePenTo(BPoint(b.left,b.top+10));driver->StrokeLine(BPoint(b.right-1,b.top+10),NULL);
      
#ifdef DEBUG_DECOR
printf("About to draw high lines\n");
printf("do done\n");
#endif
      driver->SetHighColor(cD.red,cD.green,cD.blue,cD.alpha);
      driver->MovePenTo(BPoint(b.left+1,b.top+1));driver->StrokeLine(BPoint(b.right,b.top+1),NULL);
      driver->MovePenTo(BPoint(b.left+1,b.top+3));driver->StrokeLine(BPoint(b.right,b.top+3),NULL);
      driver->MovePenTo(BPoint(b.left+1,b.top+5));driver->StrokeLine(BPoint(b.right,b.top+5),NULL);
      driver->MovePenTo(BPoint(b.left+1,b.top+7));driver->StrokeLine(BPoint(b.right,b.top+7),NULL);
      driver->MovePenTo(BPoint(b.left+1,b.top+9));driver->StrokeLine(BPoint(b.right,b.top+9),NULL);
      driver->MovePenTo(BPoint(b.left+1,b.top+11));driver->StrokeLine(BPoint(b.right,b.top+11),NULL);
            
   }else{
      driver->SetHighColor(cL.red,cL.green,cL.blue,cL.alpha);
      // Fill top
      b = cOBounds;
      b.bottom = b.top + TOP - 2;
      b.top += 1;
      b.left += 1;
      b.right -= 1;
      driver->FillRect(b,NULL);
      // Fill left side
      b = cOBounds;
      b.left += 1;
      b.right = b.left + FRAME - 2;
      b.top = b.top + TOP - 1;
      b.bottom -= 1;
      driver->FillRect(b,NULL);
      // fill right
      b = cOBounds;
      b.left = b.right - FRAME + 2;
      b.right -= 1;
      b.top = b.top + TOP - 1;
      b.bottom -= 1;
      driver->FillRect(b,NULL);
      // fill bottom
      b = cOBounds;
      b.top = b.bottom - FRAME + 2;
      b.bottom -= 1;
      b.left += FRAME - 1;
      b.right -= FRAME - 1;
      driver->FillRect(b,NULL);
   }

   
/*   if((flags & WND_NO_TITLE) == 0){
      // Stroke The title Text
      driver->SetFgColor(cL.red,cL.green,cL.blue,cL.alpha);
      b.left = 30;
      b.right = 100;
      //Font *font = new Font(DEFAULT_FONT_WINDOW);
      //b.right = font->GetStringWidth(Title.c_str());
      
      int32 len = strlen(Title.c_str());
      b.right = b.left + len * 7;
      
      
      b.top = 2;
      b.bottom = TOP - 3;
      V->FillRect(b);
      if(HasFocus){
         V->SetFgColor(0,0,0,255);
      }else{
	V->SetFgColor(cD); 
      }
      V->SetBgColor(cL);
      V->MovePenTo(34,TOP-7);
      V->DrawString(Title.c_str(), -1 );
   }
*/
   
   
   if(!(flags & B_NOT_CLOSABLE)){
      DrawClose(CloseRect);
   }
   if(!(flags & B_NOT_ZOOMABLE)){
      DrawZoom(ZoomRect);
   }

}

/*******************************************************
*   @description
*******************************************************/
void YMakDecorator::DrawZoom(BRect r){
//	if(zoomstate)
}

/*******************************************************
*   @description
*******************************************************/
void YMakDecorator::DrawClose(BRect r){
    BRect b = CloseRect;

  // if((Flags & WND_NO_CLOSE_BUT)){ return; }
//   if(!HasFocus){ return; }
   
   
   b.left += 1;
   b.right -= 1;
   b.top += 1;
   b.bottom -= 1;
   driver->SetHighColor(32,32,32,255);
   /*V->DrawLine(Point(b.left,b.top),Point(b.right,b.top));
   V->DrawLine(Point(b.right,b.top),Point(b.right,b.bottom));
   V->DrawLine(Point(b.left,b.bottom),Point(b.left,b.top));
   V->DrawLine(Point(b.right,b.bottom),Point(b.left,b.bottom));*/
   driver->StrokeRect(b,NULL);
   
   
   b = CloseRect;
   
   // top left higlith
   driver->SetHighColor(136,136,136,255);
   driver->MovePenTo(BPoint(b.left,b.top));driver->StrokeLine(BPoint(b.right-1,b.top),NULL);
   driver->MovePenTo(BPoint(b.left,b.top+1));driver->StrokeLine(BPoint(b.left,b.bottom-1),NULL);
      
   // right bottom shadow
   driver->SetHighColor(251,251,251,255);
   driver->MovePenTo(BPoint(b.right,b.bottom));driver->StrokeLine(BPoint(b.left+1,b.bottom),NULL);
   driver->MovePenTo(BPoint(b.right,b.bottom));driver->StrokeLine(BPoint(b.right,b.top+1),NULL);
   
   if(closestate){
      b = CloseRect;
      b.left += 2;
      b.right -= 2;
      b.top += 2;
      b.bottom -= 2;
      
      // Button down
      driver->SetHighColor(200,200,200,255);
      driver->FillRect(b,NULL);
   }else{
      b = CloseRect;
      b.left += 2;
      b.right -= 2;
      b.top += 2;
      b.bottom -= 2;
      
      // top left
      driver->SetHighColor(240,240,240,255);
      driver->MovePenTo(BPoint(b.left,b.top));driver->StrokeLine(BPoint(b.right,b.top),NULL);
      driver->MovePenTo(BPoint(b.left,b.top));driver->StrokeLine(BPoint(b.left,b.bottom),NULL);
      
      // bottom right
      driver->SetHighColor(136,136,136,255);
      driver->MovePenTo(BPoint(b.right,b.bottom));driver->StrokeLine(BPoint(b.left+1,b.bottom),NULL);
      driver->MovePenTo(BPoint(b.right,b.bottom));driver->StrokeLine(BPoint(b.right,b.top+1),NULL);

      // fill insdie (Hacked - Mack is a gradiant in the button :P!!)
      driver->SetHighColor(203,203,203,255);
      b.left += 1;
      b.right -= 1;
      b.top += 1;
      b.bottom -= 1;
      driver->FillRect(b,NULL);
   }
}

/*******************************************************
*   @description
*******************************************************/
void YMakDecorator::SetLook(window_look wlook){
   look = wlook;
}

/*******************************************************
*   @description
*******************************************************/
void YMakDecorator::CalculateBorders(void){
	switch(look)
	{
		case B_NO_BORDER_WINDOW_LOOK:
		{
//			bsize.Set(0,0,0,0);
			break;
		}
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
		case B_BORDERED_WINDOW_LOOK:
		{
//			bsize.top=18;
			break;
		}
		case B_MODAL_WINDOW_LOOK:
		case B_FLOATING_WINDOW_LOOK:
		{
//			bsize.top=15;
			break;
		}
		default:
		{
			break;
		}
	}
}




