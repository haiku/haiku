// Structs necessary for our hook functions
#include <GraphicsDefs.h>

typedef struct
{	int16 x1,y1,x2,y2;
	uint8 color;
} indexed_color_line;

typedef struct
{	int16 x1,y1,x2,y2;
	rgb_color color;
} rgb_color_line;

int32 define_cursor(uchar *xormask, uchar *andmask, int32 width,
		int32 height, int32 hotx, int32 hoty);
int32 move_cursor(int32 screenx, int32 screeny);
int32 show_cursor(bool flag);
int32 draw_line_with_8_bit_depth(int32 startx, int32 endx,
		int32 starty, int32 endy, uint8 colorindex, bool cliptorect,
		int16 clipleft, int16 cliptop, int16 clipright, int16 clipbottom);
int32 draw_line_with_32_bit_depth(int32 startx, int32 endx,
		int32 starty, int32 endy, uint32 color, bool cliptorect,
		int16 clipleft, int16 cliptop, int16 clipright, int16 clipbottom);
int32 draw_rect_with_8_bit_depth(int32 left, int32 top, int32 right,
		int32 bottom, uint8 colorindex);
int32 draw_rect_with_32_bit_depth(int32 left, int32 top, int32 right,
		int32 bottom, uint32 color);
int32 blit(int32 sourcex, int32 sourcey, int32 destinationx,
		int32 destinationy, int32 width, int32 height);
int32 draw_array_with_8_bit_depth(indexed_color_line *array, int32 numitems,
		bool cliptorect, int16 clipleft, int16 cliptop, 
		int16 clipright, int16 clipbottom);
int32 draw_array_with_32_bit_depth(rgb_color_line *array, int32 numitems,
		bool cliptorect, int16 clipleft, int16 cliptop, 
		int16 clipright, int16 clipbottom);
int32 sync(void);
int32 invert_rect(int32 left, int32 top, int32 right, int32 bottom);
int32 draw_line_with_16_bit_depth(int32 startx, int32 endx,
		int32 starty, int32 endy, uint16 color, bool cliptorect,
		int16 clipleft, int16 cliptop, int16 clipright, int16 clipbottom);
int32 draw_rect_with_16_bit_depth(int32 left, int32 top, int32 right,
		int32 bottom, uint16 color);


// Not officially-defined hook functions which should be here
typedef struct
{	int16 x1,y1,x2,y2;
	uint16 color;
} high_color_line;

int32 draw_array_with_16_bit_depth(high_color_line *array, int32 numitems,
		bool cliptorect, int16 clipleft, int16 cliptop, 
		int16 clipright, int16 clipbottom);

int32 draw_frect_with_8_bit_depth(int32 left, int32 top, int32 right,
		int32 bottom, uint8 colorindex);
int32 draw_frect_with_16_bit_depth(int32 left, int32 top, int32 right,
		int32 bottom, uint16 color);
int32 draw_frect_with_32_bit_depth(int32 left, int32 top, int32 right,
		int32 bottom, uint32 color);
