#ifndef	_OBVIEW_H_
#define	_OBVIEW_H_

#include <BeBuild.h>
#include <InterfaceDefs.h>
#include <Font.h>
#include <Handler.h>
#include <Rect.h>
#include <View.h>

class BMessage;
class BString;
class BWindow;
class OBWindow;

class OBView : public BHandler 
{
public:
OBView(	BRect frame,const char *name,uint32 resizeMask,uint32 flags);
virtual ~OBView();

virtual	void			AttachedToWindow();
virtual	void			AllAttached();
virtual	void			DetachedFromWindow();
virtual	void			AllDetached();

virtual	void			MessageReceived(BMessage *msg);

void			AddChild(OBView *child, OBView *before = NULL);
bool			RemoveChild(OBView *child);
bool			RemoveSelf();
int32			CountChildren() const;
OBView			*ChildAt(int32 index) const;
OBView			*NextSibling() const;
OBView			*PreviousSibling() const;

OBWindow			*Window() const;

virtual	void			Draw(BRect updateRect);
virtual	void			MouseDown(BPoint where);
virtual	void			MouseUp(BPoint where);
virtual	void			MouseMoved(	BPoint where,uint32 code,const BMessage *a_message);
virtual	void			WindowActivated(bool state);
virtual	void			KeyDown(const char *bytes, int32 numBytes);
virtual	void			KeyUp(const char *bytes, int32 numBytes);
virtual	void			Pulse();
virtual	void			FrameMoved(BPoint new_position);
virtual	void			FrameResized(float new_width, float new_height);

virtual	void			TargetedByScrollView(BScrollView *scroll_view);
void BeginRectTracking(	BRect startRect,uint32 style = B_TRACK_WHOLE_RECT);
void EndRectTracking();

void GetMouse(BPoint *location,uint32 *buttons,bool checkMessageQueue=true);

OBView			*FindView(const char *name);
OBView			*Parent() const;
BRect			Bounds() const;
BRect			Frame() const;
void			ConvertToScreen(BPoint* pt) const;
BPoint			ConvertToScreen(BPoint pt) const;
void			ConvertFromScreen(BPoint* pt) const;
BPoint			ConvertFromScreen(BPoint pt) const;
void			ConvertToScreen(BRect *r) const;
BRect			ConvertToScreen(BRect r) const;
void			ConvertFromScreen(BRect *r) const;
BRect			ConvertFromScreen(BRect r) const;
void			ConvertToParent(BPoint *pt) const;
BPoint			ConvertToParent(BPoint pt) const;
void			ConvertFromParent(BPoint *pt) const;
BPoint			ConvertFromParent(BPoint pt) const;
void			ConvertToParent(BRect *r) const;
BRect			ConvertToParent(BRect r) const;
void			ConvertFromParent(BRect *r) const;
BRect			ConvertFromParent(BRect r) const;
BPoint			LeftTop() const;

void			GetClippingRegion(BRegion *region) const;
virtual	void			ConstrainClippingRegion(BRegion *region);

virtual	void			SetDrawingMode(drawing_mode mode);
drawing_mode 	DrawingMode() const;

void			SetBlendingMode(source_alpha srcAlpha, alpha_function alphaFunc);
void	 		GetBlendingMode(source_alpha *srcAlpha, alpha_function *alphaFunc) const;

virtual	void			SetPenSize(float size);
float			PenSize() const;

virtual	void			SetViewColor(rgb_color c);
void			SetViewColor(uchar r, uchar g, uchar b, uchar a = 255);
rgb_color		ViewColor() const;

virtual	void			SetHighColor(rgb_color a_color);
void			SetHighColor(uchar r, uchar g, uchar b, uchar a = 255);
rgb_color		HighColor() const;

virtual	void			SetLowColor(rgb_color a_color);
void			SetLowColor(uchar r, uchar g, uchar b, uchar a = 255);
rgb_color		LowColor() const;

void			Invalidate(BRect invalRect);
void			Invalidate(const BRegion *invalRegion);
void			Invalidate();

virtual	void			SetFlags(uint32 flags);
uint32			Flags() const;
virtual	void			SetResizingMode(uint32 mode);
uint32			ResizingMode() const;
void			MoveBy(float dh, float dv);
void			MoveTo(BPoint where);
void			MoveTo(float x, float y);
void			ResizeBy(float dh, float dv);
void			ResizeTo(float width, float height);
virtual	void			MakeFocus(bool focusState = true);
bool			IsFocus() const;
	
virtual	void Show();
virtual	void Hide();
bool IsHidden() const;
bool IsHidden(const OBView* looking_from) const;
	
void			Flush() const;
void			Sync() const;

void			StrokeRect(BRect r, pattern p = B_SOLID_HIGH);
void			FillRect(BRect r, pattern p = B_SOLID_HIGH);
void			MovePenTo(BPoint pt);
void			MovePenTo(float x, float y);
void			MovePenBy(float x, float y);
BPoint			PenLocation() const;
void			StrokeLine(	BPoint toPt,pattern p = B_SOLID_HIGH);
void			StrokeLine(	BPoint pt0,BPoint pt1,pattern p = B_SOLID_HIGH);

protected:
	friend OBWindow;

int32 GetServerToken(void);
void CallAttached(OBView *view);
void CallDetached(OBView *view);
OBView *FindViewInChildren(const char *name);
void AddedToWindow(void);

int32			id;
uint32			viewflags, resizeflags;
float			origin_h;
float			origin_v;
OBWindow*		owner;

OBView*			parent;
OBView*			uppersibling;
OBView*			lowersibling;
OBView*			topchild;
OBView*			bottomchild;

uint16 			hidelevel;
bool topview;
bool has_focus;

BRect			vframe;
BRegion *clipregion;

rgb_color highcol;
rgb_color lowcol;
rgb_color viewcol;
};

#endif
