#include <Application.h>
#include <DirectWindow.h>

#include <stdio.h>

static const char *
state_to_string(direct_buffer_state state)
{
	//TODO: Return more info like B_CLIPPING_MODIFIED, etc.
			
	switch (state & B_DIRECT_MODE_MASK) {
		case B_DIRECT_START:
			return "B_DIRECT_START";
		case B_DIRECT_STOP:
			return "B_DIRECT_STOP";
		case B_DIRECT_MODIFY:
			return "B_DIRECT_MODIFY";
		default:
			return "other state";
	}
}


static const char *
layout_to_string(buffer_layout layout)
{
	switch (layout) {
		case B_BUFFER_NONINTERLEAVED:
			return "B_BUFFER_NONINTERLEAVED";
		default:
			return "unknown buffer_layout";
	}
}


static const char *
orientation_to_string(buffer_orientation orientation)
{
	switch (orientation) {
		case B_BUFFER_TOP_TO_BOTTOM:
			return "B_BUFFER_TOP_TO_BOTTOM";
		case B_BUFFER_BOTTOM_TO_TOP:
			return "B_BUFFER_BOTTOM_TO_TOP";
		default:
			return "unknown buffer_orientation";
	}
}

class TestWindow : public BDirectWindow {
public:
	TestWindow() : BDirectWindow(BRect(100, 100, 400, 300), "DWInfo", B_DOCUMENT_WINDOW, 0)
	{
		
	}
	
	virtual void DirectConnected(direct_buffer_info *info)
	{
		BRegion region;
		
		printf("\n\n*** DirectConnected() ***\n");
		area_id areaId = area_for(info);
		area_info areaInfo;
		if (areaId >= 0 && get_area_info(areaId, &areaInfo) == B_OK)
			printf("area size: %ld\n", areaInfo.size);
				
		printf("buffer state: %s\n", state_to_string(info->buffer_state));
		printf("bits: %p\n", info->bits);
		printf("pci_bits: %p\n", info->pci_bits);
		printf("bytes_per_row: %ld\n", info->bytes_per_row);
		printf("bits_per_pixel: %lu\n", info->bits_per_pixel);
		printf("pixel_format: %d\n", info->pixel_format);
		printf("buffer_layout: %s\n", layout_to_string(info->layout));
		printf("buffer_orientation: %s\n", orientation_to_string(info->orientation));
		
		printf("\nCLIPPING INFO:\n");
		printf("clipping_rects count: %ld\n", info->clip_list_count);
		
		printf("- window_bounds:\n");
		region.Set(info->window_bounds);
		region.PrintToStream();
		
		region.MakeEmpty();
		for (uint32 i = 0; i < info->clip_list_count; i++)
			region.Include(info->clip_list[i]);
		
		printf("- clip_list:\n");
		region.PrintToStream();
	}
	
	virtual bool QuitRequested()
	{
		be_app->PostMessage(B_QUIT_REQUESTED);
		return BDirectWindow::QuitRequested();
	}
};



int main()
{
	BApplication app("application/x-vnd.DWInfo");
	
	(new TestWindow())->Show();
	
	app.Run();
}
