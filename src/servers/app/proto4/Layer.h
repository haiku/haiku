#ifndef	_LAYER_H_
#define	_LAYER_H_

#include <GraphicsDefs.h>
#include <Region.h>
#include <Rect.h>

class ServerWindow;

class Layer
{
public:
	Layer(ServerWindow *Win, const char *Name, Layer *Parent, BRect &Frame, int32 Flags);
	virtual ~Layer(void);
	
	void Initialize(void);
	void Show(void);
	void Hide(void);
	bool IsVisible(void);
	void Update(bool ForceUpdate);
	
	void AddChild(Layer *child, bool topmost=true);
	void RemoveChild(Layer *child);
	void RemoveSelf(void);
	void SetAddedData(uint8 count);
	
	virtual void SetFrame(const BRect &Rect);
	BRect Frame(void);
	BRect Bounds(void);
	
	void Invalidate(const BRect &rect);
	void Invalidate(bool recursive);
	void AddRegion(BRegion *Region);
	void RemoveRegions(void);
	void UpdateRegions(bool ForceUpdate=true, bool Root=true);
	void SetDirty(bool flag);
	void MarkModified(BRect rect);
	
	// Render functions:
	virtual void Draw(const BRect &Rect);
	
	const char *name;
	int32 handle;
	ServerWindow *window;	// Window associated with this. NULL for background layer
	
	BRect frame;
	
	BRegion *visible,	// Everything currently visible
			*full,		// The complete region
			*update;	// Area to be updated at next redraw
	
	int32 flags;		// Various flags for behavior
	uint8 hidecount;	// tracks the number of hide calls
	bool isdirty;		// has regions which are invalid
	
	// The whole set of layers is actually a tree
	Layer *topchild, *bottomchild;
	Layer *uppersibling, *lowersibling;
	Layer *parent;
	
	// Render state:
	BPoint penpos;
	rgb_color high, low, erase;
	
	int32 drawingmode;
};
	
#endif
