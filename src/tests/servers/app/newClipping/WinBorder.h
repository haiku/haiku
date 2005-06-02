#include <Region.h>
#include "Layer.h"

class WinBorder : public Layer
{
public:
							WinBorder(BRect frame, const char* name,
								uint32 rm, uint32 flags, rgb_color c);
							~WinBorder();

	virtual	void			MovedByHook(float dx, float dy);
	virtual	void			ResizedByHook(float dx, float dy, bool automatic);

private:
			void			set_decorator_region(BRect frame);
	virtual	bool			alter_visible_for_children(BRegion &region);
	virtual	void			get_user_regions(BRegion &reg);

			BRegion			fDecRegion;
			bool			fRebuildDecRegion;
};
