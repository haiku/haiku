#include <GraphicsDefs.h>

#include "GlobalData.h"
#include "generic.h"

#define STICKY_REGS 1

#define WRITE_REG(x, y) outw(x, y)

#define wait_for_slots(num) WaitQueue(num)

/*#define wait_for_slots(numSlots) \
	{ \
	uint32 FifoReg; \
	\
	do \
	{ \
		READ_REG(FIFO_STAT, FifoReg); \
	} \
	while ((FifoReg & 0xFFFF) >> (16 - numSlots)); \
	}
*/
void SCREEN_TO_SCREEN_BLIT(engine_token *et, blit_params *list, uint32 count) {

	uint32 Offset, Pitch;
	uint32 BppEncoding;
	uint32 XDir;
	uint32 YDir;

	uint32 src_top, src_left;
	uint32 dest_top, dest_left;
	uint32 width, height;

	uint32 scissor_x = (si->dm.virtual_width - 1) << 16;
	uint32 scissor_y = (si->dm.virtual_height - 1) << 16;

	// Perform required calculations and do the blit.
	// start of frame buffer
	Offset = 1024;
	Offset = Offset >> 3; // qword offset of the frame buffer in card memory; had
		// better be qword-aligned.
	
	Pitch = si->dm.virtual_width;
	Pitch = (Pitch + 7) >> 3; // frame buffer stride in pixels*8
	
	// encode pixel format
	switch (si->dm.space & ~0x3000)
	{
	case B_CMAP8:
		BppEncoding = 0x2;
		break;
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE:
		BppEncoding = 0x3;
		break;
	case B_RGB16_BIG:
	case B_RGB16_LITTLE:
		BppEncoding = 0x4;
		break;
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
	default:
		BppEncoding = 0x6;
	}
	
	BppEncoding = (BppEncoding << 28) | (BppEncoding << 16) | (BppEncoding << 8)
		| (BppEncoding << 4) | BppEncoding;
	
#if STICKY_REGS > 0
	wait_for_slots(10);

	WRITE_REG(SC_LEFT_RIGHT, scissor_x);
	WRITE_REG(SC_TOP_BOTTOM, scissor_y);
	WRITE_REG(DP_WRITE_MASK, 0xFFFFFFFF);
	WRITE_REG(DP_PIX_WIDTH, BppEncoding);
	WRITE_REG(DP_MIX, 0x00070007); // ROP = SRC
	WRITE_REG(DP_SRC, 0x00000300);
	WRITE_REG(CLR_CMP_CNTL, 0x0);

	// Pitch and offset had better not be out of range.
	WRITE_REG(SRC_OFF_PITCH, (Pitch << 22) | Offset);
	WRITE_REG(DST_OFF_PITCH, (Pitch << 22) | Offset);
	WRITE_REG(SRC_CNTL, 0x0);
	/* update fifo count */
	si->engine.count += 10;
#endif
	
	/* program the blit */
	while (count--) {
		src_left = list->src_left;
		src_top = list->src_top;
		dest_left = list->dest_left;
		dest_top = list->dest_top;
		width = list->width + 1; /* + 1 because it seems to be non-inclusive */
		height = list->height + 1;

		// Adjust direction of blitting to allow for overlapping source and destination.
		if (src_left < dest_left)
		{
			XDir = 0; // right to left
			src_left += (width - 1);
			dest_left += (width - 1);
		}
		else
			XDir = 1; // left to right

		if (src_top < dest_top)
		{
			YDir = 0; // bottom to top
			src_top += (height - 1);
			dest_top += (height - 1);
		}
		else
			YDir = 1; // top to bottom


		// Set up the rectangle blit

		// Critital section - accessing card registers.
		//lock_card();


#if STICKY_REGS > 0
		wait_for_slots(5);
#else
		wait_for_slots(15);

		WRITE_REG(SC_LEFT_RIGHT, scissor_x);
		WRITE_REG(SC_TOP_BOTTOM, scissor_y);
		WRITE_REG(DP_WRITE_MASK, 0xFFFFFFFF);
		WRITE_REG(DP_PIX_WIDTH, BppEncoding);
		WRITE_REG(DP_MIX, 0x00070007); // ROP = SRC
		WRITE_REG(DP_SRC, 0x00000300);
		WRITE_REG(CLR_CMP_CNTL, 0x0);

		// Pitch and offset had better not be out of range.
		WRITE_REG(SRC_OFF_PITCH, (Pitch << 22) | Offset);
		WRITE_REG(DST_OFF_PITCH, (Pitch << 22) | Offset);
		WRITE_REG(SRC_CNTL, 0x0);
#endif

		WRITE_REG(SRC_WIDTH1, width);
		WRITE_REG(SRC_Y_X, ((uint32)src_left << 16) | src_top);

		// Pitch and offset had better not be out of range.
		WRITE_REG(DST_CNTL, (YDir << 1) | XDir);
		WRITE_REG(DST_Y_X, ((uint32)dest_left << 16) | dest_top);
		WRITE_REG(DST_HEIGHT_WIDTH, ((uint32)width << 16) | height); // This triggers drawing.

#if STICKY_REGS
		/* update fifo count */
		si->engine.count += 5;
#else
		/* update fifo count */
		si->engine.count += 15;
#endif
		/* next one */
		list++;
	}
}

void hardware_rectangle(engine_token *et, uint32 colorIndex, fill_rect_params *list, uint32 count, uint32 ROP) {

	uint32 Offset;
	uint32 Pitch;
	uint32 Y;
	uint32 X;
	uint32 Width;
	uint32 Height;
	uint32 Colour;
	uint32 BppEncoding;

#if STICKY_REGS > 0
	uint32 scissor_x = (si->dm.virtual_width - 1) << 16;
	uint32 scissor_y = (si->dm.virtual_height - 1) << 16;
#endif

	// Perform required calculations and do the blit.
	// start of frame buffer
	Offset = 1024;
	Offset = Offset >> 3; // qword offset of the frame buffer in card memory; had
		// better be qword-aligned.
	
	Pitch = si->dm.virtual_width;
	Pitch = (Pitch + 7) >> 3; // frame buffer stride in pixels*8

	// Don't need this - rectangle is in frame buffer co-ordinates.
//	X += (uint32) (CardInfo.Display.DisplayXPos);
//	Y += (uint32) (CardInfo.Display.DisplayYPos);

	// Fill colour dword with specified colour, though only ls should be necessary.
	switch (si->dm.space & ~0x3000)
	{
	case B_CMAP8:
		BppEncoding = 0x2;
		colorIndex &= 0xFF;
		Colour = colorIndex | (colorIndex << 8) | (colorIndex << 16) | (colorIndex << 24);
		break;
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE:
		BppEncoding = 0x3;
		colorIndex &= 0xFFFF;
		Colour = colorIndex | (colorIndex << 16);
		break;
	case B_RGB16_BIG:
	case B_RGB16_LITTLE:
		BppEncoding = 0x4;
		colorIndex &= 0xFFFF;
		Colour = colorIndex | (colorIndex << 16);
		break;
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
	default:
		BppEncoding = 0x6;
		Colour = colorIndex;
	}

	BppEncoding = (BppEncoding << 28) | (BppEncoding << 16) | (BppEncoding << 8) | (BppEncoding << 4) | BppEncoding;

#if STICKY_REGS > 0
	wait_for_slots(11);
	// This seems to be needed even though we aren't using a source trajectory.
	WRITE_REG(SRC_CNTL, 0x0);
	
	// Pitch and offset had better not be out of range.
	WRITE_REG(DST_OFF_PITCH, (Pitch << 22) | Offset);
	
	WRITE_REG(DP_FRGD_CLR, Colour);
	WRITE_REG(DP_WRITE_MASK, 0xFFFFFFFF);
	WRITE_REG(DP_PIX_WIDTH, BppEncoding);
	WRITE_REG(DP_MIX, ROP);
	WRITE_REG(DP_SRC, 0x00000100);
	
	WRITE_REG(CLR_CMP_CNTL, 0x0);
	WRITE_REG(GUI_TRAJ_CNTL, 0x3);
	WRITE_REG(SC_LEFT_RIGHT, scissor_x);
	WRITE_REG(SC_TOP_BOTTOM, scissor_y);
	/* update fifo count */
	si->engine.count += 11;
#endif

	while (count--) {

		X = list->left;
		Y = list->top;
		Width = list->right;
		Width = Width - X + 1;
		Height = list->bottom;
		Height = Height - Y + 1;

#if STICKY_REGS > 0
		wait_for_slots(2);
#else
		wait_for_slots(13);
		// This seems to be needed even though we aren't using a source trajectory.
		WRITE_REG(SRC_CNTL, 0x0);
		
		// Pitch and offset had better not be out of range.
		WRITE_REG(DST_OFF_PITCH, (Pitch << 22) | Offset);
		
		WRITE_REG(DP_FRGD_CLR, Colour);
		WRITE_REG(DP_WRITE_MASK, 0xFFFFFFFF);
		WRITE_REG(DP_PIX_WIDTH, BppEncoding);
		WRITE_REG(DP_MIX, ROP);
		WRITE_REG(DP_SRC, 0x00000100);
		
		WRITE_REG(CLR_CMP_CNTL, 0x0);
		WRITE_REG(GUI_TRAJ_CNTL, 0x3);

		WRITE_REG(SC_LEFT_RIGHT, ((X + Width - 1) << 16) | X);
		WRITE_REG(SC_TOP_BOTTOM, ((Y + Height - 1) << 16) | Y);
#endif
		
		// X and Y should have been clipped to frame buffer, and so should be in range.
		WRITE_REG(DST_Y_X, (X << 16) | Y);
		WRITE_REG(DST_HEIGHT_WIDTH, (Width << 16) | Height); // This triggers drawing.

#if STICKY_REGS
		/* update fifo count */
		si->engine.count += 2;
#else
		/* update fifo count */
		si->engine.count += 13;
#endif

		/* next rect */
		list++;
	}

}

void FILL_RECTANGLE(engine_token *et, uint32 colorIndex, fill_rect_params *list, uint32 count) {
	hardware_rectangle(et, colorIndex, list, count, 0x00070007); // ROP = SRC
}

void INVERT_RECTANGLE(engine_token *et, fill_rect_params *list, uint32 count) {
	hardware_rectangle(et, 0xFFFFFFFF, list, count, 0x00000000); // ROP = !DST
}

void FILL_SPAN(engine_token *et, uint32 colorIndex, uint16 *list, uint32 count) {
	uint32 Offset;
	uint32 Pitch;
	uint32 Y;
	uint32 X;
	uint32 Width;
	uint32 Height;
	uint32 Colour;
	uint32 BppEncoding;


#if STICKY_REGS > 0
	uint32 scissor_x = (si->dm.virtual_width - 1) << 16;
	uint32 scissor_y = (si->dm.virtual_height - 1) << 16;
#endif

	// Perform required calculations and do the blit.
	// start of frame buffer
	Offset = 1024;
	Offset = Offset >> 3; // qword offset of the frame buffer in card memory; had
		// better be qword-aligned.
	
	Pitch = si->dm.virtual_width;
	Pitch = (Pitch + 7) >> 3; // frame buffer stride in pixels*8

	// Fill colour dword with specified colour, though only ls should be necessary.
	switch (si->dm.space & ~0x3000)
	{
	case B_CMAP8:
		BppEncoding = 0x2;
		colorIndex &= 0xFF;
		Colour = colorIndex | (colorIndex << 8) | (colorIndex << 16) | (colorIndex << 24);
		break;
	case B_RGB15_BIG:
	case B_RGBA15_BIG:
	case B_RGB15_LITTLE:
	case B_RGBA15_LITTLE:
		BppEncoding = 0x3;
		colorIndex &= 0xFFFF;
		Colour = colorIndex | (colorIndex << 16);
		break;
	case B_RGB16_BIG:
	case B_RGB16_LITTLE:
		BppEncoding = 0x4;
		colorIndex &= 0xFFFF;
		Colour = colorIndex | (colorIndex << 16);
		break;
	case B_RGB32_BIG:
	case B_RGBA32_BIG:
	case B_RGB32_LITTLE:
	case B_RGBA32_LITTLE:
	default:
		BppEncoding = 0x6;
		Colour = colorIndex;
	}

	BppEncoding = (BppEncoding << 28) | (BppEncoding << 16) | (BppEncoding << 8) | (BppEncoding << 4) | BppEncoding;


#if STICKY_REGS > 0
	wait_for_slots(11);
	// This seems to be needed even though we aren't using a source trajectory.
	WRITE_REG(SRC_CNTL, 0x0);
	
	// Pitch and offset had better not be out of range.
	WRITE_REG(DST_OFF_PITCH, (Pitch << 22) | Offset);
	
	WRITE_REG(DP_FRGD_CLR, Colour);
	WRITE_REG(DP_WRITE_MASK, 0xFFFFFFFF);
	WRITE_REG(DP_PIX_WIDTH, BppEncoding);
	WRITE_REG(DP_MIX, 0x00070007); // ROP = SRC
	WRITE_REG(DP_SRC, 0x00000100);
	
	WRITE_REG(CLR_CMP_CNTL, 0x0);
	WRITE_REG(GUI_TRAJ_CNTL, 0x3);
	WRITE_REG(SC_LEFT_RIGHT, scissor_x);
	WRITE_REG(SC_TOP_BOTTOM, scissor_y);
	/* update fifo count */
	si->engine.count += 11;
#endif
	/* span lines are always one pixel tall */
	Height = 1;
	while (count--) {

		Y = (uint32)*list++;
		X = (uint32)*list++;
		Width = (uint32)*list++;
		Width = Width - X + 1;

#if STICKY_REGS > 0
		wait_for_slots(2);
#else
		wait_for_slots(13);
		// This seems to be needed even though we aren't using a source trajectory.
		WRITE_REG(SRC_CNTL, 0x0);
		
		// Pitch and offset had better not be out of range.
		WRITE_REG(DST_OFF_PITCH, (Pitch << 22) | Offset);
		
		WRITE_REG(DP_FRGD_CLR, Colour);
		WRITE_REG(DP_WRITE_MASK, 0xFFFFFFFF);
		WRITE_REG(DP_PIX_WIDTH, BppEncoding);
		WRITE_REG(DP_MIX, 0x00070007); // ROP = SRC
		WRITE_REG(DP_SRC, 0x00000100);
		
		WRITE_REG(CLR_CMP_CNTL, 0x0);
		WRITE_REG(GUI_TRAJ_CNTL, 0x3);

		WRITE_REG(SC_LEFT_RIGHT, ((X + Width - 1) << 16) | X);
		WRITE_REG(SC_TOP_BOTTOM, ((Y + Height - 1) << 16) | Y);
#endif

		// X and Y should have been clipped to frame buffer, and so should be in range.
		WRITE_REG(DST_Y_X, (X << 16) | Y);
		WRITE_REG(DST_HEIGHT_WIDTH, (Width << 16) | Height); // This triggers drawing.

#if STICKY_REGS
		/* update fifo count */
		si->engine.count += 2;
#else
		/* update fifo count */
		si->engine.count += 13;
#endif

	}

}
