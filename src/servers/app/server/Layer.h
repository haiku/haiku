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

enum
{
	B_LAYER_NONE		= 0x00001000UL,
	B_LAYER_MOVE		= 0x00002000UL,
	B_LAYER_SIMPLE_MOVE	= 0x00004000UL,
	B_LAYER_RESIZE		= 0x00008000UL,
	B_LAYER_MASK_RESIZE	= 0x00010000UL,
};

enum
{
	B_LAYER_CHILDREN_DEPENDANT = 0x1000U,
};

class ServerWindow;
class RootLayer;
class DisplayDriver;
class LayerData;

class Layer
{
public:
	Layer(BRect frame, const char *name, int32 token, uint32 resize, 
			uint32 flags, DisplayDriver *driver);
	virtual ~Layer(void);

	void AddChild(Layer *child, ServerWindow *serverWin);
	void RemoveChild(Layer *child);
	void RemoveSelf(void);
	bool HasChild(Layer *layer);
	RootLayer *GetRootLayer(void) const { return fRootLayer; }
	
	uint32 CountChildren(void) const;
	Layer *FindLayer(const int32 token);
	Layer *LayerAt(const BPoint &pt);
	bool IsTopLayer(void) { return fIsTopLayer; }
	
	virtual	Layer *VirtualTopChild(void) const;
	virtual	Layer *VirtualLowerSibling(void) const;
	virtual	Layer *VirtualUpperSibling(void) const;
	virtual	Layer *VirtualBottomChild(void) const;
	
	const char *GetName(void) const { return (fName)?fName->String():NULL; }
	
	void FullInvalidate(const BRect &rect);
	void FullInvalidate(const BRegion &region);
	void Invalidate(const BRegion &region);
	void RebuildAndForceRedraw( const BRegion& reg, Layer *target);
	
	virtual	void RebuildFullRegion(void);
	void StartRebuildRegions( const BRegion& reg, Layer *target, uint32 action, BPoint& pt);
	void RebuildRegions( const BRegion& reg, uint32 action, BPoint pt, BPoint ptOffset);
	uint32 ResizeOthers(float x, float y,  BPoint coords[], BPoint *ptOffset);
	
	void Redraw(const BRegion& reg, Layer *startFrom=NULL);
	
	void EmptyGlobals(void);
	
	virtual	void Draw(const BRect &r);
	
	void SendViewMovedMsg(void);
	void SendViewResizedMsg(void);
	
	virtual	void Show(bool invalidate=true);
	virtual	void Hide(bool invalidate=true);
	bool IsHidden(void) const;
	
	BRect Bounds(void) const;
	BRect Frame(void) const;
	
	virtual	void MoveBy(float x, float y);
	virtual	void ResizeBy(float x, float y);
	
	BRect ConvertToParent(BRect rect);
	BRegion ConvertToParent(BRegion *reg);
	BRect ConvertFromParent(BRect rect);
	BRegion ConvertFromParent(BRegion *reg);
	BRegion ConvertToTop(BRegion *reg);
	BRect ConvertToTop(BRect rect);
	BRegion ConvertFromTop(BRegion *reg);
	BRect ConvertFromTop(BRect rect);
	
	DisplayDriver *GetDisplayDriver(void) const { return fDriver; }
	ServerWindow *Window(void) const { return fServerWin; }

	void PruneTree(void);
	
	void PrintToStream(void);
	void PrintNode(void);
	void PrintTree(void);
	
	// server "private" - should not be used
	void SetRootLayer(RootLayer *rl){ fRootLayer = rl; }
	void SetServerWindow(ServerWindow *win);
	void SetAsTopLayer(bool option) { fIsTopLayer = option; }
	

protected:
	friend class RootLayer;
	friend class WinBorder;
	friend class Screen;
	friend class ServerWindow;
	friend class Desktop;
	friend class Workspace;

	BRect fFrame;
	BPoint fBoundsLeftTop;
	Layer *fParent;
	Layer *fUpperSibling;
	Layer *fLowerSibling;
	Layer *fTopChild;
	Layer *fBottomChild;
	
	mutable	Layer *fCurrent;
	
	BRegion fVisible;
	BRegion	fFullVisible;
	BRegion	fFull;
	BRegion	fUpdateReg;
	
	BRegion *clipToPicture;
	bool clipToPictureInverse;
	
	ServerWindow *fServerWin;
	BString *fName;	
	int32 fViewToken;
	int32 fLevel;
	uint32 fFlags;
	uint32 fResizeMode;
	bool fHidden;
	bool fIsUpdating;
	bool fIsTopLayer;
	uint16 fAdFlags;
	
	DisplayDriver *fDriver;
	LayerData *fLayerData;
	//RGBColor fBackColor;
	
	RootLayer *fRootLayer;

private:
	void RequestDraw(const BRegion &reg, Layer *startFrom);
	ServerWindow *SearchForServerWindow(void) const;

};

#endif
