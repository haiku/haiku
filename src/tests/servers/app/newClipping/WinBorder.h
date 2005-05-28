#include <Region.h>
#include "Layer.h"

class WinBorder : public Layer
{
public:
							WinBorder(BRect frame, const char* name,
								uint32 rm, uint32 flags, rgb_color c);
							~WinBorder();
private:
			void			set_decorator_region(BRect frame);
	virtual	void			operate_on_visible(BRegion &region);
	virtual	void			set_user_regions(BRegion &reg);

			BRegion			fDecRegion;
};
