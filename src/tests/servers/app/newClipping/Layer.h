#include <OS.h>
#include <Region.h>
#include <Rect.h>
#include <GraphicsDefs.h>

class BView;

class Layer
{
public:
							Layer(BRect frame, const char* name, uint32 rm, rgb_color c);
	virtual					~Layer();

			void			AddLayer(Layer* layer);
			bool			RemLayer(Layer* layer);

			Layer*			VirtualBottomChild() const;
			Layer*			VirtualTopChild() const;
			Layer*			VirtualUpperSibling() const;
			Layer*			VirtualLowerSibling() const;

			void			RebuildVisibleRegions(	const BRegion &invalid,
													const Layer *startFrom);
			void			ConvertToScreen2(BRect* rect);
			BView*			GetRootLayer() const;
			void			SetRootLayer(BView* view) { fView = view; }

			BRegion*		Visible() { return &fVisible; }
			BRegion*		FullVisible() { return &fFullVisible; }

			BRect			Frame() const { return fFrame; }
			BRect			Bounds() const { BRect r(fFrame);
											r.OffsetTo(0,0);
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

			char			fName[50];
			BRegion			fVisible;
			BRegion			fFullVisible;

			BRect			fFrame;
			uint32			fRM;
			rgb_color		fColor;

			Layer*			fBottom;
			Layer*			fUpper;
			Layer*			fLower;
			Layer*			fTop;
			Layer*			fParent;

			BView*		fView;
};
