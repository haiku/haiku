
#ifndef __LAYER_H__
#define __LAYER_H__

#include <OS.h>
#include <Region.h>
#include <Rect.h>
#include <GraphicsDefs.h>

class MyView;

class Layer
{
public:
							Layer(BRect frame, const char* name, uint32 rm, uint32 flags, rgb_color c);
	virtual					~Layer();

			void			AddLayer(Layer* layer);
			bool			RemLayer(Layer* layer);

			void			MoveBy(float dx, float dy);
			void			ResizeBy(float dx, float dy);
			void			ScrollBy(float dx, float dy);

			bool			IsHidden() const;
			void			Hide();
			void			Show();

			void			Invalidate(	const BRegion &invalid,
										const Layer *startFrom = NULL);

	virtual	void			MovedByHook(float dx, float dy) { }
	virtual	void			ResizedByHook(float dx, float dy, bool automatic) { }
	virtual	void			ScrolledByHook(float dx, float dy) { }

			Layer*			BottomChild() const;
			Layer*			TopChild() const;
			Layer*			UpperSibling() const;
			Layer*			LowerSibling() const;

			void			RebuildVisibleRegions(	const BRegion &invalid,
													const Layer *startFrom);
			void			ConvertToScreen2(BRect* rect) const;
			void			ConvertToScreen2(BRegion* reg) const; 
			MyView*			GetRootLayer() const;
			void			SetRootLayer(MyView* view) { fView = view; }

			BRegion*		Visible() { return &fVisible; }
			BRegion*		FullVisible() { return &fFullVisible; }
			void			GetWantedRegion(BRegion &reg);

			BRect			Frame() const { return fFrame; }
			BRect			Bounds() const { BRect r(fFrame);
											r.OffsetTo(fOrigin);
											return r; }
			const char*		Name() const { return fName; }
			Layer*			Parent() const { return fParent; }
			void			PrintToStream() const;
			bool			IsTopLayer() const { return fView? true: false; }

			rgb_color		HighColor() const { return fColor; }

			void			rebuild_visible_regions(const BRegion &invalid,
													const BRegion &parentLocalVisible,
													const Layer *startFrom);
protected:
			rgb_color		fColor;

private:
	virtual	bool			alter_visible_for_children(BRegion &region);
	virtual	void			get_user_regions(BRegion &reg);

			void			clear_visible_regions();
			void			resize_layer_frame_by(float x, float y);
			void			rezize_layer_redraw_more(BRegion &reg, float dx, float dy);
			void			resize_layer_full_update_on_resize(BRegion &reg, float dx, float dy);

			char			fName[50];
			BRegion			fVisible;
			BRegion			fFullVisible;

			BRect			fFrame;
			BPoint			fOrigin;
			uint32			fResizeMode;

			Layer*			fBottom;
			Layer*			fUpper;
			Layer*			fLower;
			Layer*			fTop;
			Layer*			fParent;
	mutable	Layer*			fCurrent;

			uint32			fFlags;
			bool			fHidden;

			MyView*			fView;
};

#endif