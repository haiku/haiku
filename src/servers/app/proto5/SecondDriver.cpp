/*******************************************************
*   SecondDriver
*   
*   Second driver is a Drvier abstraction layer for the 
*    OBOS app_server.  It provides access to the Graphic
*    HW via the low level api calls.  This provides a 
*    way to run the app_server on real HW.
*   This driver also makes it easy for you to run the 
*    OBOS app_server in parallel with the Be app_server
*    when two or more video cards are installed in your
*    computer.
*
*   @author YNOP (ynop@beunited.org)
*   @version beta1 (p5)
*   
*   History:
*   -  Added Ellipse (Stroke & Fill)
*      Ran though test app to make sure worked
*      Fixed HorizLine to used fill_span instead of fill_rect :P
*      Updated the Cursor code to work again.
*      Made new cursor (Be-hand like)
*   -  Added StrokeBezir
*      Added HorizLine for speed
*   -  Added HW acceleration methods
*   -  Brez Line added
*   -  Created by YNOP
*
*   Todo:
*      Clean up src and add more comments :P
*      Redo StrokeLine to use PenSize
*      Redo StrokeLine to use pattern
*      Mouse Cursor seems to be broken in proto5 ?
*      Add in a way to read app_server_settings file so we can get the real settings
*      Make display mode settings more flexable
*      Look into FreeType for DrawString :P fun fun fun
*     
*   Todo methods:
*      Blit() DrawBitmap() DrawString() FillArc() FillBezier() FillPolygon()
*      FillRegion() FillRoundRect() FillShape() FillTriangle()
*      StrokeArc() StrokeRoundRect() StrokeShpae()
*
*******************************************************/
#include <Accelerant.h>
#include <OS.h>

#include <stdio.h>

#include "PortLink.h"
#include "ServerProtocol.h"
#include "ServerBitmap.h"
#include "ServerCursor.h"
#include "DebugTools.h"
#include "SecondDriver.h"
#include "SecondDriverHelper.h"

/*******************************************************
*   @description
*******************************************************/
SecondDriver::SecondDriver(void):DisplayDriver(){
   SetupBezier();
   hide_cursor = false;
   fd = -1;
   image = -1;
   
   gah = NULL;
   bits = NULL;
   bpr = -1;
   
   scs = NULL;
   mc = NULL;
   sc = NULL;
   
   et = NULL;
   re = NULL;
   
   s2sb = NULL;
   fr = NULL;
   ir = NULL;
   s2stb = NULL;
   fs = NULL;
   
}

/*******************************************************
*   @description
*******************************************************/
SecondDriver::~SecondDriver(void){
   if(is_initialized){
   }
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::Initialize(void){
   et = NULL;
   bits = NULL;
   
   // Find a driver
   fd = get_device();
   if(fd < 0){
      printf("No Device settings found.\n");
      fd =  find_device();
   }else{
      printf("Device read from settings file\n");
   }
   if(fd < 0){
      printf("No device find, trying fallback stub\n");
      fd = fallback_device();
   }
   if(fd < 0){
      printf("Didn't find device :(\n");
      is_initialized = false;
   }
   /* load the accelerant */
   image = load_accelerant(fd, &gah);
   if (image >= 0){
      // Set up the display to a valid resolution
      if(get_and_set_mode(gah,&dm) == B_OK){
         // Here we are all good
         if(dm.space == B_CMAP8){
            set_palette(gah);
         }
         get_frame_buffer(gah, &fbc);
         bits = (uint32*)fbc.frame_buffer/*_dma*/;
         bpr = fbc.bytes_per_row/4; // sould be by depth
         
         if(dm.flags & B_HARDWARE_CURSOR){
            // Lets check out tha hardwar cursoer stuff :P
            printf("Mode supports Hardware cursors\n");
            scs = (set_cursor_shape)gah(B_SET_CURSOR_SHAPE,NULL);
            mc = (move_cursor)gah(B_MOVE_CURSOR,NULL);
            sc = (show_cursor)gah(B_SHOW_CURSOR,NULL);
         }
   
         // Ok. now lets see if we can add some spiffy
         // 2D acceleration to this :)
         accelerant_engine_count aec = (accelerant_engine_count)gah(B_ACCELERANT_ENGINE_COUNT,NULL);
         uint32 count = aec();
         if(count > 0){
            acquire_engine ae = (acquire_engine)gah(B_ACQUIRE_ENGINE,NULL);
            re = (release_engine)gah(B_RELEASE_ENGINE,NULL);
            if(ae(B_2D_ACCELERATION,1000,&st,&et) == B_OK){
               // Now lets see if we have some functions
               printf("You should be verry happy. You have 2D hardware Acceleration\n");
               s2sb = (screen_to_screen_blit)gah(B_SCREEN_TO_SCREEN_BLIT,NULL);
               fr = (fill_rectangle)gah(B_FILL_RECTANGLE,NULL);
               ir = (invert_rectangle)gah(B_INVERT_RECTANGLE,NULL);
               s2stb = (screen_to_screen_transparent_blit)gah(B_SCREEN_TO_SCREEN_TRANSPARENT_BLIT,NULL);
               fs = (fill_span)gah(B_FILL_SPAN,NULL);
            }
         }
         is_initialized = true;
      }else{
         // failed to set mode
         is_initialized = false;
      }
   }else{
      // image failed to load
      is_initialized = false;
   }
   
   if(is_initialized){
      printf("\n\tSecond Driver by YNOP (ynop@beunited.org)\n");
      printf("\tSecond Driver is copyrigth YNOP 2002\n");
      printf("\tHave fun, its all set up :)\n\n");
   }
   
   SetCursor((ServerCursor*)NULL);
   ShowCursor();
   int32 h = GetHeight();
   int32 w = GetWidth();
   MoveCursorTo(w/2,h/2);
   
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::Shutdown(void){
   printf("\tShuting down SecondDriver\n");
   if(et){
      if(re(et,&st) != B_OK){
         printf("Failed to release engine\n");
      }
   }
   
   uninit_accelerant ua = (uninit_accelerant)gah(B_UNINIT_ACCELERANT,NULL);
   if(ua){ ua();}
   
   unload_add_on(image);
   
   // clsoe the file now...
   close(fd);
   
   
   is_initialized=false;
}

/*******************************************************
*   @description
*******************************************************/
bool SecondDriver::IsInitialized(void){
   return is_initialized;
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::SafeMode(void){
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::Reset(void){
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::SetScreen(uint32 space){
  // Clear(51,102,152);
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::Clear(uint8 red, uint8 green, uint8 blue){
   rgb_color r;
   r.red = red;
   r.blue = blue;
   r.green = green;   
   Clear(r);
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::Clear(rgb_color col){
   highcol = col;
   FillRect(BRect(0,0,GetHeight()-1,GetWidth()-1),NULL);
}

/*******************************************************
*   @description
*******************************************************/
int32 SecondDriver::GetHeight(void){
   // Gets the height of the current mode
   return dm.virtual_height;
}

/*******************************************************
*   @description
*******************************************************/
int32 SecondDriver::GetWidth(void){
   // Gets the width of the current mode
   return dm.virtual_width;
}

/*******************************************************
*   @description
*******************************************************/
int SecondDriver::GetDepth(void){
   // Gets the color depth of the current mode
   return 0;
}

/*******************************************************
*   @description
*******************************************************/
#ifdef PROTO_4
 void SecondDriver::Blit(BPoint loc, ServerBitmap *src, ServerBitmap *dest)
#else
 void SecondDriver::Blit(BRect src, BRect dest)
#endif
{
   printf("Calling Blit (not wirten yet)\n");
   if(s2sb){
      printf("Accelerated Blit\n");
   }else{
   
   }
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::DrawBitmap(ServerBitmap *bitmap){
   printf("Calling DrawBitmap (not writen yet)\n");
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::DrawChar(char c, BPoint point){
	char string[2];
	string[0]=c;
	string[1]='\0';
	DrawString(string,2,point);
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::DrawString(char *string, int length, BPoint point){
   printf("DrawString is not writen yet\n");
   
   // i would say that libtruetype would do some good herer
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::FillArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern){
   printf("FillArc is not writen yet\n");
   StrokeArc(centerx,centery,xradius,yradius,angle,span,pattern);
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::FillBezier(BPoint *points, uint8 *pattern){
   printf("FillBezier is not writen yet\n");
   StrokeBezier(points,pattern);
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::FillEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern){
   MidpointEllipse(centerx,centery,x_radius,y_radius,true,pattern);
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::FillPolygon(int *x, int *y, int numpoints, bool is_closed){
   printf("FillPolygon is not writen yet\n");
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::FillRect(BRect rect, uint8 *pattern){
   if(fr){
      // usieng accelertated 2d file
      union{
         uint8 bytes[4];
         uint32 word;
      }c1;

      //printf("Accelerated Fill\n");
      fill_rect_params frp;
      frp.left =(uint16) rect.left;
      frp.top =(uint16) rect.top;
      frp.right =(uint16) rect.right;
      frp.bottom =(uint16) rect.bottom;
      c1.bytes[0] = highcol.blue;
      c1.bytes[1] = highcol.green;
      c1.bytes[2] = highcol.red;
      fr(et,c1.word,&frp,1);
   }else{
      for(int32 i = 0;i < rect.Height()-1;i++){
         MovePenTo(BPoint(rect.left,rect.top+i));
         StrokeLine(BPoint(rect.right,rect.top+i),pattern);
         //HorizLine(rect.left,rect.right,rect.top+i); // uses accel :P
      }
   }
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::FillRect(BRect rect, rgb_color col){
   rgb_color tmpc = highcol;
   highcol = col;
   FillRect(rect, NULL);
   highcol = tmpc;
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::FillRegion(BRegion *region){
   printf("RillRegion is not writen yet\n");
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::FillRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern){
   printf("FillRoundRect is not writen yet\n");
   StrokeRoundRect(rect,xradius,yradius,pattern);
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::FillShape(BShape *shape){
   printf("FillShape is not writen yet\n");
   StrokeShape(shape);
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::FillTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern){
   printf("FillTriangle is not writen yet\n");
   StrokeTriangle(first,second,third,rect,pattern);
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::HideCursor(void){
   if(sc){
      sc(false);
      cursor_visible = false;
   }else{
      printf("Shoftware Hide Cursor\n");
   }
}

/*******************************************************
*   @description
*******************************************************/
bool SecondDriver::IsCursorHidden(void){
   return cursor_visible;
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::ObscureCursor(void){
   // Hides cursor until mouse is moved
   HideCursor();
   show_on_move = true;
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::MoveCursorTo(float x, float y){
   if(show_on_move){
      ShowCursor();
   }
   if(mc){
      mc((short unsigned int)x,(short unsigned int)y);
   }else{
      printf("Software move Cursor\n");
   }   
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::MovePenTo(BPoint pt){
   penpos = pt;
}

/*******************************************************
*   @description
*******************************************************/
BPoint SecondDriver::PenPosition(void){
   return penpos;
}

/*******************************************************
*   @description
*******************************************************/
float SecondDriver::PenSize(void){
   return pensize;
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::SetCursor(int32 value){
   printf("Setcursoer value?\n");
   // Uh what is this all about?
}




//0 - 1 = black
//0 - 0 = what
//1 - 0 = trans
//1 - 1 = invert

// Pointy cursor
/*uint8 andMask[] = {
   63,255,15,255,131,255,128,127,192,31,192,31,224,63,224,63,
   224,31,240,15,243,7,255,131,255,193,255,224,255,240,255,248
};
uint8 xorMask[] = {
   192,0,176,0,76,0,67,128,32,96,32,32,16,64,16,64,
   16,32,11,16,12,136,0,68,0,34,0,17,0,9,0,7
};*/
// BeOS like cursor
uint8 andMask[] = {
   255,255,255,255,199,255,195,255,195,255,224,31,224,3,240,1,
   240,0,192,0,128,0,128,0,192,0,240,0,252,1,254,7
};
uint8 xorMask[] = {
   0,0,0,0,56,0,36,0,36,0,19,224,18,92,9,42,
   8,1,60,0,76,0,66,0,48,0,12,0,2,0,1,0
};

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::SetCursor(ServerCursor *cursor){
   printf("Setting cursor\n");
   current_cursor = cursor;
//   uint8 *andMask;
 //  uint8 *xorMask;
   if(scs){
//      scs(cursor->width,cursor->height,(short unsigned int)cursor->hotspot.x,(short unsigned int)cursor->hotspot.y,andMask,xorMask);
      scs(16,16,3,3,andMask,xorMask);
   }else{
      printf("Setting software cursor\n");
   }
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::SetPenSize(float size){
   pensize = size;
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::SetHighColor(uint8 r,uint8 g,uint8 b,uint8 a=255){
   highcol.red = r;
   highcol.green = g;
   highcol.blue = b;
   highcol.alpha = a;   
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::SetLowColor(uint8 r,uint8 g,uint8 b,uint8 a=255){
   lowcol.red = r;
   lowcol.green = g;
   lowcol.blue = b;
   lowcol.alpha = a;
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::SetPixel(int x, int y, uint8 *pattern){
   union{
      uint8 bytes[4];
      uint32 word;
   }c1;
   
   c1.bytes[0] = highcol.blue; 
   c1.bytes[1] = highcol.green; 
   c1.bytes[2] = highcol.red; 
   c1.bytes[3] = highcol.alpha; 
 
   *(bits + x + y*bpr) = c1.word;
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::ShowCursor(void){
   printf("\tShow Cursor\n");
   show_on_move = false;
   cursor_visible = true;
   if(sc){
      sc(true);
   }else{
      printf("Software show cursor\n");
   }
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::StrokeArc(int centerx, int centery, int xradius, int yradius, float angle, float span, uint8 *pattern){
   printf("StrokeArch is not writen yet\n");
}


/*******************************************************
*   @description
*******************************************************/
void SecondDriver::StrokeBezier(BPoint *pts, uint8 *pattern){
   BPoint segment[SEGMENTS + 1];
   computeSegments(pts[0], pts[1],pts[2],pts[3], segment);
    
   MovePenTo(segment[0]);
   
   for(int32 s = 1 ; s <= SEGMENTS ; ++s ){
     if(segment[s].x != segment[s - 1].x || segment[s].y != segment[s - 1].y ) {
        StrokeLine(segment[s],pattern);
     }
   } 
} 

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::StrokeEllipse(float centerx, float centery, float x_radius, float y_radius,uint8 *pattern){
   MidpointEllipse(centerx,centery,x_radius,y_radius,false,pattern);
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::StrokeLine(BPoint point, uint8 *pattern){
   if(penpos.y == point.y){
      HorizLine(penpos.x,point.x,point.y,pattern);
      return;
   }
      
   int oct = 0;
   int xoff = (int32)penpos.x;
   int yoff = (int32)penpos.y; 
   int32 x2 = (int32)point.x-xoff;
   int32 y2 = (int32)point.y-yoff; 
   int32 x1 = 0;
   int32 y1 = 0;
//   if(y2==0){ if (x1>x2){int t=x1;x1=x2;x2=t;}HorizLine(x1+xoff,x2+xoff,y1+yoff); return;}
   if(y2<0){ y2 = -y2; oct+=4; }//bit2=1
   if(x2<0){ x2 = -x2; oct+=2;}//bit1=1
   if(x2<y2){ int t=x2; x2=y2; y2=t; oct+=1;}//bit0=1
   int x=x1,
       y=y1,
       sum=x2-x1,
       Dx=2*(x2-x1),
       Dy=2*(y2-y1);
   for(int i=0; i <= x2-x1; i++){ 
      switch(oct){
         case 0:SetPixel(( x)+xoff,( y)+yoff,pattern);break;
         case 1:SetPixel(( y)+xoff,( x)+yoff,pattern);break;
         case 3:SetPixel((-y)+xoff,( x)+yoff,pattern);break;
         case 2:SetPixel((-x)+xoff,( y)+yoff,pattern);break;
         case 6:SetPixel((-x)+xoff,(-y)+yoff,pattern);break;
         case 7:SetPixel((-y)+xoff,(-x)+yoff,pattern);break;
         case 5:SetPixel(( y)+xoff,(-x)+yoff,pattern);break;
         case 4:SetPixel(( x)+xoff,(-y)+yoff,pattern);break;
      }
      x++;
      sum-=Dy;
      if(sum < 0){
         y++;
         sum += Dx;
      }
   }

   MovePenTo(point); // ends up in last position
}

void SecondDriver::StrokeLine(BPoint pt1, BPoint pt2, rgb_color col)
{
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::StrokePolygon(int *x, int *y, int numpoints, bool is_closed){
   BPoint first(x[0],y[0]);
   MovePenTo(first);
   for(int32 i = 1; i < numpoints; i++){
      StrokeLine(BPoint(x[i],y[i]),NULL);
   }
   if(is_closed){
      //StrokeLine();
   }
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::StrokeRect(BRect rect,uint8 *pattern){
   MovePenTo(BPoint(rect.left,rect.top));
   StrokeLine(BPoint(rect.right,rect.top),pattern);
   StrokeLine(BPoint(rect.right,rect.bottom),pattern);
   StrokeLine(BPoint(rect.left,rect.bottom),pattern);
   StrokeLine(BPoint(rect.left,rect.top),pattern);
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::StrokeRect(BRect rect,rgb_color col){
   rgb_color tmpC = highcol;
   highcol = col;
   StrokeRect(rect,NULL);
   highcol = tmpC;
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::StrokeRoundRect(BRect rect,float xradius, float yradius, uint8 *pattern){
   printf("StroekRoundRect is not writen yet\n");
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::StrokeShape(BShape *shape){
   printf("StrokeShape is not writen yet\n");
}

/*******************************************************
*   @description
*******************************************************/
void SecondDriver::StrokeTriangle(BPoint first, BPoint second, BPoint third, BRect rect, uint8 *pattern){
   MovePenTo(first);
   StrokeLine(second,pattern);
   StrokeLine(third,pattern);
   StrokeLine(first,pattern);
}


/*******************************************************
*   
*   Everthign below here is not part of the Driver API
*    however we added it because it makes life easyer for
*    everone :)
*
*******************************************************/


/*******************************************************
*   @description
*******************************************************/
void SecondDriver::HorizLine(int32 x1,int32 x2,int32 y, uint8 *pattern){
   // we can do this forst as ponts are cached
   MovePenTo(BPoint(x2,y)); // ends up in last position
   // Swap so x1 < x2 or the fill wil not work
   if(x1 > x2){ int32 t=x1;x1=x2;x2=t; }
   if(fr){
      union{
         uint8 bytes[4];
         uint32 word;
      }c1;

      //printf("Accelerated HorizLine\n");
      uint16 list[3];
      list[0] = y;
      list[1] = x1;
      list[2] = x2;
      c1.bytes[0] = highcol.blue;
      c1.bytes[1] = highcol.green;
      c1.bytes[2] = highcol.red;
      fs(et,c1.word,list,1);
   }else{
      //software hline
      printf("\t!!HorizLine without HWaccel is a waist\n");
      for(int32 i = x1; i <= x2;i++){
         SetPixel(i,y,pattern);
      }
   }

}

//void EllipsePoints(float cx,float cy,int32 x, int32 y){
//   view->StrokeLine(BPoint(x+cent.x,y+cent.y),BPoint(x+cent.x,y+cent.y));
//   view->StrokeLine(BPoint(-x+cent.x,y+cent.y),BPoint(-x+cent.x,y+cent.y));
//   view->StrokeLine(BPoint(x+cent.x,-y+cent.y),BPoint(x+cent.x,-y+cent.y));
//   view->StrokeLine(BPoint(-x+cent.x,-y+cent.y),BPoint(-x+cent.x,-y+cent.y));
#define EllipsePoints(cx,cy,x,y,pattern){ \
   SetPixel( x+cx, y+cx,pattern); \
   SetPixel(-x+cx, y+cx,pattern); \
   SetPixel( x+cx,-y+cx,pattern); \
   SetPixel(-x+cx,-y+cx,pattern); \
}


//void EllipsePointsFill(BPoint cent,int32 x, int32 y,pattern){
//   view->StrokeLine(BPoint(-x+cent.x,y+cent.y),BPoint(x+cent.x,y+cent.y));
//   view->StrokeLine(BPoint(-x+cent.x,-y+cent.y),BPoint(x+cent.x,-y+cent.y));
  
  #define EllipsePointsFill(cx,cy,x,y,pattern){ \
   HorizLine(-x+cx,x+cx, y+cy, pattern);   \
   HorizLine(-x+cx,x+cx, -y+cy, pattern);  \
}


/*******************************************************
*   
*******************************************************/
void SecondDriver::MidpointEllipse(float centx,float centy,int32 xrad, int32 yrad,bool fill,uint8 *pattern){
   if(xrad < 0){ xrad = -xrad; }
   if(yrad < 0){ yrad = -yrad; }
   double d2;
   int32 x = 0;
   int32 y = yrad;
   
   int32 xradSqr = xrad*xrad;
   int32 yradSqr = yrad*yrad;
   
   double dl = yradSqr - (xradSqr*yrad) + (0.25 *xradSqr);
   
   if(fill){ EllipsePointsFill(centx,centy,x,y,pattern); }else{ EllipsePoints(centx,centy,x,y,pattern); }
   
   // Test gradient if still in region 1
   while((xradSqr*(y-.5)) > (yradSqr*(x+1))){
      if(dl < 0){  // Select E
         dl += yradSqr*(2*x + 3);
      }else{  // Select SE
         dl += yradSqr*(2*x + 3) + xradSqr*(-2*y + 2);
         y--;
      }
      x++;
      if(fill){ EllipsePointsFill(centx,centy,x,y,pattern); }else{ EllipsePoints(centx,centy,x,y,pattern); }
   }
   
   d2 = (yradSqr*(x + .5)*(x + .5)) + (xradSqr*(y-1)*(y-1)) - xradSqr*yradSqr;
   while(y > 0){
      if(d2 < 0){ // Select SE
         d2 += yradSqr*(2*x + 2) + xradSqr*(-2*y + 3);
         x++;
      }else{  // Select E
         d2 += xradSqr*(-2*y + 3);
      }
      y--;
      if(fill){ EllipsePointsFill(centx,centy,x,y,pattern); }else{ EllipsePoints(centx,centy,x,y,pattern); }
   }
}



#ifdef DEBUG_DRIVER_MODULE
#undef DEBUG_DRIVER_MODULE
#endif