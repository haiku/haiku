#include <OS.h>
#include <Region.h>
#include <Rect.h>
#include <GraphicsDefs.h>

class MyView;

class Layer
{
public:
							Layer(BRect frame, const char* name, uint32 rm, rgb_color c);
	virtual					~Layer();

			void			AddLayer(Layer* layer);
			bool			RemLayer(Layer* layer);

			void			MoveBy(float dx, float dy);
			void			ResizeBy(float dx, float dy);
			void			ScrollBy(float dx, float dy);

	virtual	void			MovedByHook(float dx, float dy) { }
	virtual	void			ResizedByHook(float dx, float dy) { }
	virtual	void			ScrolledByHook(float dx, float dy) { }

			Layer*			VirtualBottomChild() const;
			Layer*			VirtualTopChild() const;
			Layer*			VirtualUpperSibling() const;
			Layer*			VirtualLowerSibling() const;

			void			RebuildVisibleRegions(	const BRegion &invalid,
													const Layer *startFrom);
			void			ConvertToScreen2(BRect* rect);
			MyView*			GetRootLayer() const;
			void			SetRootLayer(MyView* view) { fView = view; }
			bool			IsVisuallyHidden() const;

			BRegion*		Visible() { return &fVisible; }
			BRegion*		FullVisible() { return &fFullVisible; }

			BRect			Frame() const { return fFrame; }
			BRect			Bounds() const { BRect r(fFrame);
											r.OffsetTo(fOrigin);
											return r; }
			const char*		Name() const { return fName; }
			Layer*			Parent() const { return fParent; }
			void			PrintToStream() const;

			rgb_color		HighColor() const { return fColor; }

			void			rebuild_visible_regions(const BRegion &invalid,
													const BRegion &parentLocalVisible,
													const Layer *startFrom);
private:
			void			set_user_regions(BRegion &reg);
			void			clear_visible_regions();
			void			resize_layer_frame_by(float x, float y);

			char			fName[50];
			BRegion			fVisible;
			BRegion			fFullVisible;

			BRect			fFrame;
			BPoint			fOrigin;
			uint32			fResizeMode;
			rgb_color		fColor;

			Layer*			fBottom;
			Layer*			fUpper;
			Layer*			fLower;
			Layer*			fTop;
			Layer*			fParent;

			MyView*		fView;
};
