#ifndef _LAYER_H_
#define _LAYER_H_

#include <GraphicsDefs.h>
#include <Rect.h>
#include <Region.h>
#include <List.h>
#include <String.h>
#include <OS.h>
#include <Locker.h>
#include "RGBColor.h"

class ServerWindow;
class RootLayer;
//class MyDriver;
class DisplayDriver;
class LayerData;

class Layer
{
public:
								Layer(BRect frame, const char *name, int32 token, uint32 resize, 
										uint32 flags, DisplayDriver *driver);
	virtual						~Layer(void);

			void				AddChild(Layer *child, RootLayer *rootLayer = NULL);
			void				RemoveChild(Layer *child);
			void				RemoveSelf();
			bool				HasChild(Layer* layer);
			RootLayer*			GetRootLayer() const { return fRootLayer; }

			uint32				CountChildren(void) const;
			Layer*				FindLayer(const int32 token);
			Layer*				LayerAt(const BPoint &pt);
			bool				IsTopLayer() { return fIsTopLayer; }

	virtual	Layer*				VirtualTopChild() const;
	virtual	Layer*				VirtualLowerSibling() const;
	virtual	Layer*				VirtualUpperSibling() const;
	virtual	Layer*				VirtualBottomChild() const;

			const char*			GetName(void) const { return (_name)?_name->String():NULL; }
	
			void				FullInvalidate(const BRect &rect);
			void				FullInvalidate(const BRegion &region);
			void				Invalidate(const BRegion &region);

	virtual	void				RebuildFullRegion(void);
			void				StartRebuildRegions( const BRegion& reg, Layer *target, uint32 action, BPoint& pt);
			void				RebuildRegions( const BRegion& reg, uint32 action, BPoint pt, BPoint ptOffset);
			uint32				ResizeOthers(float x, float y,  BPoint coords[], BPoint *ptOffset);

			void				Redraw(const BRegion& reg, Layer *startFrom=NULL);

			void				EmptyGlobals();

	virtual	void				Draw(const BRect &r);

			void				SendViewMovedMsg();
			void				SendViewResizedMsg();

	virtual	void				Show(void);
	virtual	void				Hide(void);
			bool				IsHidden(void) const;

			BRect				Bounds(void) const;
			BRect				Frame(void) const;

	virtual	void				MoveBy(float x, float y);
	virtual	void				ResizeBy(float x, float y);

			BRect				ConvertToParent(BRect rect);
			BRegion				ConvertToParent(BRegion *reg);
			BRect				ConvertFromParent(BRect rect);
			BRegion				ConvertFromParent(BRegion *reg);
			BRegion				ConvertToTop(BRegion *reg);
			BRect				ConvertToTop(BRect rect);
			BRegion				ConvertFromTop(BRegion *reg);
			BRect				ConvertFromTop(BRect rect);

			DisplayDriver*		GetDisplayDriver() const { return fDriver; }

			void				PruneTree(void);

			void				PrintToStream(void);
			void				PrintNode(void);
			void				PrintTree();

	// server "private" - should not be used
			void				SetRootLayer(RootLayer* rl){ fRootLayer = rl; }
			void				SetServerWindow(ServerWindow *win);
			void				SetAsTopLayer(bool option) { fIsTopLayer = option; }

private:
			void				RequestClientUpdate(const BRegion &reg, Layer *startFrom);
			void				RequestDraw(const BRegion &reg, Layer *startFrom, bool redraw=false);
			ServerWindow*		SearchForServerWindow() const;

protected:
	friend class RootLayer;
	friend class WinBorder;
	friend class Screen;
	friend class ServerWindow;
	friend class Desktop;
	friend class Workspace;

			BRect				_frame;
			BPoint				_boundsLeftTop;
			Layer				*_parent,
								*_uppersibling,
								*_lowersibling,
								*_topchild,
								*_bottomchild;

	mutable	Layer				*fCurrent;

			BRegion				_visible,
								_fullVisible,
								_full,
								fUpdateReg;

			BRegion				*clipToPicture;
			bool				clipToPictureInverse;

			ServerWindow		*_serverwin;
			BString				*_name;	
			int32				_view_token;
			int32				_level;
			uint32				_flags;
			uint32				_resize_mode;
			bool				_hidden;
			bool				_is_updating;
			bool				fIsTopLayer;
			
			DisplayDriver		*fDriver;
			LayerData			*_layerdata;
//			RGBColor			fBackColor;

			RootLayer*			fRootLayer;
};

#endif
