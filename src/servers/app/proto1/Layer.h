#ifndef	_LAYER_H_
#define	_LAYER_H_

#include <GraphicsDefs.h>
#include <Region.h>
#include <Rect.h>

class ServerBitmap;
class ServerWindow;

class Layer
{
public:
	Layer(ServerWindow *Win, const char *Name, BRect &Frame, int32 Flags);
	Layer(ServerBitmap *Bitmap);
	virtual ~Layer(void);
	
	void Initialize(void);
	void Show(void);
	void Hide(void);
	bool IsVisible(void);
	void Update(bool ForceUpdate);
	
	bool IsBackground(void);
	
	virtual void SetFrame(const BRect &Rect);
	BRect Frame(void);
	BRect Bounds(void);
	
	void Invalidate(const BRect &Rect);
	void AddRegion(BRegion *Region);
	void RemoveRegions(void);
	void UpdateRegions(bool ForceUpdate=true, bool Root=true);
	
	// Render functions:
	virtual void Draw(const BRect &Rect);
	
	const char *name;
	int32 handle;
	ServerWindow *window;	// Window associated with this. NULL for background layer
	ServerBitmap *bitmap;	// Blasted to screen on redraw. We render to this
	
	BRect frame;
	
	BRegion *visible,	// Everything currently visible
			*full,		// The complete region
			*update;	// Area to be updated at next redraw
	
	int32 flags;		// Various flags for behavior
	
	// Render state:
	BPoint penpos;
	rgb_color high, low, erase;
	
	int32 drawingmode;
};
	
	Layer *FindLayer(int32 Handle);
	
#endif
