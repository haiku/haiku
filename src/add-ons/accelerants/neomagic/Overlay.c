/* Written by Rudolf Cornelissen 05-2002/4-2006 */

/* Note on 'missing features' in BeOS 5.0.3 and DANO:
 * BeOS needs to define more colorspaces! It would be nice if BeOS would support the FourCC 'definitions'
 * of colorspaces. These colorspaces are 32bit words, so it could be simply done (or is it already so?)
 */

#define MODULE_BIT 0x00000400

#include "acc_std.h"

/* define the supported overlay input colorspaces */
static uint32 overlay_colorspaces [] = { (uint32)B_YCbCr422, (uint32)B_NO_COLOR_SPACE };

uint32 OVERLAY_COUNT(const display_mode *dm) 
// This method is never used AFAIK though it *is* exported on R5.0.3 and DANO.
// Does someone know howto invoke it?
{
	LOG(4,("Overlay: count called\n"));

	/* check for NULL pointer */
	if (dm == NULL)
	{
		LOG(4,("Overlay: No display mode specified!\n"));
	}
	/* apparantly overlay count should report the number of 'overlay units' on the card */
	return 1;
}

const uint32 *OVERLAY_SUPPORTED_SPACES(const display_mode *dm)
// This method is never used AFAIK though it *is* exported on R5.0.3 and DANO.
// Does someone know howto invoke it?
{
	LOG(4,("Overlay: supported_spaces called.\n"));

	/* check for NULL pointer */
	if (dm == NULL)
	{
		LOG(4,("Overlay: No display mode specified!\n"));
		return NULL;
	}

	/* assuming interlaced VGA is not supported */
	if (dm->timing.flags & B_TIMING_INTERLACED)
	{
		return NULL;
	}
	/* return a B_NO_COLOR_SPACE terminated list */
	return &overlay_colorspaces[0];
}

uint32 OVERLAY_SUPPORTED_FEATURES(uint32 a_color_space)
// This method is never used AFAIK. On R5.0.3 and DANO it is not even exported!
{
	LOG(4,("Overlay: supported_features: color_space $%08x\n",a_color_space));

	/* check what features are supported for the current overlaybitmap colorspace */
	switch (a_color_space)
	{
	default:
			return 
				( B_OVERLAY_COLOR_KEY 			 |
				  B_OVERLAY_HORIZONTAL_FILTERING |
				  B_OVERLAY_VERTICAL_FILTERING );
	}
}

const overlay_buffer *ALLOCATE_OVERLAY_BUFFER(color_space cs, uint16 width, uint16 height)
{
	int offset = 0;					/* used to determine next buffer to create */
	uint32 adress, adress2, temp32;	/* used to calculate buffer adresses */
	uint32 oldsize = 0;				/* used to 'squeeze' new buffers between already existing ones */
	int cnt;						/* loopcounter */

	/* acquire the shared benaphore */
	AQUIRE_BEN(si->overlay.lock)

	LOG(4,("Overlay: cardRAM_start = $%p\n",((uint8*)si->framebuffer)));
	LOG(4,("Overlay: cardRAM_start_DMA = $%p\n",((uint8*)si->framebuffer_pci)));
	LOG(4,("Overlay: cardRAM_size = %dKb\n",si->ps.memory_size));

	/* find first empty slot (room for another buffer?) */
	for (offset = 0; offset < MAXBUFFERS; offset++)
	{
		if (si->overlay.myBuffer[offset].buffer == NULL) break;
	}

	LOG(4,("Overlay: Allocate_buffer offset = %d\n",offset));

	if (offset < MAXBUFFERS)
	/* setup new scaler input buffer */
	{
		switch (cs)
		{
			case B_YCbCr422:
					/* check if slopspace is needed: NeoMagic cards can do with ~x0007 */
					if (width == (width & ~0x0007))
					{
						si->overlay.myBuffer[offset].width = width;
					}
					else
					{
						si->overlay.myBuffer[offset].width = (width & ~0x0007) + 8;
					}
					si->overlay.myBuffer[offset].bytes_per_row = 2 * si->overlay.myBuffer[offset].width;

					/* check if the requested horizontal pitch is supported */
					//fixme?...
					if (si->overlay.myBuffer[offset].width > 2048)
					{
						LOG(4,("Overlay: Sorry, requested buffer pitch not supported, aborted\n"));

						/* release the shared benaphore */
						RELEASE_BEN(si->overlay.lock)

						return NULL;
					}
					break;
			default:
					/* unsupported colorspace! */
					LOG(4,("Overlay: Sorry, colorspace $%08x not supported, aborted\n",cs));

					/* release the shared benaphore */
					RELEASE_BEN(si->overlay.lock)

					return NULL;
					break;
		}

		/* check if the requested buffer width is supported */
		if (si->overlay.myBuffer[offset].width > 1024)
		{
			LOG(4,("Overlay: Sorry, requested buffer width not supported, aborted\n"));

			/* release the shared benaphore */
			RELEASE_BEN(si->overlay.lock)

			return NULL;
		}
		/* check if the requested buffer height is supported */
		if (height > 1024)
		{
			LOG(4,("Overlay: Sorry, requested buffer height not supported, aborted\n"));

			/* release the shared benaphore */
			RELEASE_BEN(si->overlay.lock)

			return NULL;
		}

		/* store slopspace (in pixels) for each bitmap for use by 'overlay unit' (BES) */
		si->overlay.myBufInfo[offset].slopspace = si->overlay.myBuffer[offset].width - width;

		si->overlay.myBuffer[offset].space = cs;
		si->overlay.myBuffer[offset].height = height;
	
		/* we define the overlay buffers to reside 'in the back' of the cards RAM */
		/* NOTE to app programmers:
		 * Beware that an app using overlay needs to track workspace switches and screenprefs
		 * changes. If such an action is detected, the app needs to reset it's pointers to the 
		 * newly created overlay bitmaps, which will be assigned by BeOS automatically after such
		 * an event. (Also the app needs to respect the new overlay_constraints that will be applicable!)
		 *
		 * It is entirely possible that new bitmaps may *not* be re-setup at all, or less of them
		 * than previously setup by the app might be re-setup. This is due to cardRAM restraints then.
		 * This means that the app should also check for NULL pointers returned by the bitmaps,
		 * and if this happens, it needs to fallback to single buffered overlay or even fallback to
		 * bitmap output for the new situation. */

		/* Another NOTE for app programmers:
		 * A *positive* side-effect of assigning the first overlay buffer exactly at the end of the
		 * cardRAM is that apps that try to write beyond the buffer's space get a segfault immediately.
		 * This *greatly* simplifies tracking such errors! 
		 * Of course such errors may lead to strange effects in the app or driver behaviour if they are
		 * not hunted down and removed.. */

		/* calculate first free RAM adress in card:
		 * Driver setup is as follows: 
		 * card base: 		- screen memory,
		 * free space:      - used for overlay,
		 * card top:        - hardware cursor bitmap (if used) */
		adress2 = (((uint32)((uint8*)si->fbc.frame_buffer)) +	/* cursor not yet included here */
			(si->fbc.bytes_per_row * si->dm.virtual_height));	/* size in bytes of screen(s) */
		LOG(4,("Overlay: first free cardRAM virtual adress $%08x\n", adress2));

		/* calculate 'preliminary' buffer size including slopspace */
		oldsize = si->overlay.myBufInfo[offset].size;
		si->overlay.myBufInfo[offset].size = 
			si->overlay.myBuffer[offset].bytes_per_row * si->overlay.myBuffer[offset].height;

		/* calculate virtual memory adress that would be needed for a new bitmap */
		adress = (((uint32)((uint8*)si->framebuffer)) + (si->ps.memory_size * 1024));
		/* take cursor into account (if it exists) */
		if(si->settings.hardcursor) adress -= si->ps.curmem_size;
		LOG(4,("Overlay: last free cardRAM virtual adress $%08x\n", (adress - 1)));
		for (cnt = 0; cnt <= offset; cnt++)
		{
			adress -= si->overlay.myBufInfo[cnt].size;
		}

		/* the > G200 scalers require buffers to be aligned to 16 byte pages cardRAM offset, G200 can do with
		 * 8 byte pages cardRAM offset. Compatible settings used, has no real downside consequences here */

		/* Check if we need to modify the buffers starting adress and thus the size */
		/* calculate 'would be' cardRAM offset */
		temp32 = (adress - ((uint32)((vuint32 *)si->framebuffer)));
		/* check if it is aligned */
		if (temp32 != (temp32 & 0xfffffff0))
		{
			/* update the (already calculated) buffersize to get it aligned */
			si->overlay.myBufInfo[offset].size += (temp32 - (temp32 & 0xfffffff0));
			/* update the (already calculated) adress to get it aligned */
			adress -= (temp32 - (temp32 & 0xfffffff0));
		}
		LOG(4,("Overlay: new buffer needs virtual adress $%08x\n", adress));

		/* First check now if buffer to be defined is 'last one' in memory (speaking backwards):
		 * this is done to prevent a large buffer getting created in the space a small buffer 
		 * occupied earlier, if not all buffers created were deleted.
		 * Note also that the app can delete the buffers in any order desired. */

		/* NOTE to app programmers: 
		 * If you are going to delete a overlay buffer you created, you should delete them *all* and
		 * then re-create only the new ones needed. This way you are sure not to get unused memory-
		 * space in between your overlay buffers for instance, so cardRAM is used 'to the max'. 
		 * If you don't, you might not get a buffer at all if you are trying to set up a larger one
		 * than before. 
		 * (Indeed: not all buffers *have* to be of the same type and size...) */

		for (cnt = offset; cnt < MAXBUFFERS; cnt++)
		{
			if (si->overlay.myBuffer[cnt].buffer != NULL)
			{
				/* Check if the new buffer would fit into the space the single old one used here */
				if (si->overlay.myBufInfo[offset].size <= oldsize)
				{
					/* It does, so we reset to the old size and adresses to prevent the space from shrinking
					 * if we get here again... */
					adress -= (oldsize - si->overlay.myBufInfo[offset].size);
					si->overlay.myBufInfo[offset].size = oldsize;
					LOG(4,("Overlay: 'squeezing' in buffer:\n"
						   "Overlay: resetting it to virtual adress $%08x and size $%08x\n", adress,oldsize));
					/* force exiting the FOR loop */
					cnt = MAXBUFFERS;
				}
				else
				{
					/* nogo, sorry */
					LOG(4,("Overlay: Other buffer(s) exist after this one:\n"
						   "Overlay: not enough space to 'squeeze' this one in, aborted\n"));

					/* Reset to the old size to prevent the space from 'growing' if we get here again... */
					si->overlay.myBufInfo[offset].size = oldsize;

					/* release the shared benaphore */
					RELEASE_BEN(si->overlay.lock)

					return NULL;
				}
			}
		}

		/* check if we have enough space to setup this new bitmap 
		 * (preventing overlap of desktop RAMspace & overlay bitmap RAMspace here) */
		if (adress < adress2)
		/* nope, sorry */
		{
			LOG(4,("Overlay: Sorry, no more space for buffers: aborted\n"));

			/* release the shared benaphore */
			RELEASE_BEN(si->overlay.lock)

			return NULL;
		}
		/* continue buffer setup */		
		si->overlay.myBuffer[offset].buffer = (void *) adress;

		/* calculate physical memory adress (for dma use) */
		adress = (((uint32)((uint8*)si->framebuffer_pci)) + (si->ps.memory_size * 1024));
		/* take cursor into account (if it exists) */
		if(si->settings.hardcursor) adress -= si->ps.curmem_size;
		for (cnt = 0; cnt <= offset; cnt++)
		{
			adress -= si->overlay.myBufInfo[cnt].size;
		}
		/* this adress is already aligned to the scaler's requirements (via the already modified sizes) */
		si->overlay.myBuffer[offset].buffer_dma = (void *) adress;

		LOG(4,("Overlay: New buffer: addr $%08x, dma_addr $%08x, color space $%08x\n",
			(uint32)((uint8*)si->overlay.myBuffer[offset].buffer),
			(uint32)((uint8*)si->overlay.myBuffer[offset].buffer_dma), cs));
		LOG(4,("Overlay: New buffer's size is $%08x\n", si->overlay.myBufInfo[offset].size));
			
		/* release the shared benaphore */
		RELEASE_BEN(si->overlay.lock)

		return &si->overlay.myBuffer[offset];
	}
	else
	/* sorry, no more room for buffers */
	{	
		LOG(4,("Overlay: Sorry, no more space for buffers: aborted\n"));

		/* release the shared benaphore */
		RELEASE_BEN(si->overlay.lock)

		return NULL;
	}
}

status_t RELEASE_OVERLAY_BUFFER(const overlay_buffer *ob)
/* Note that the user can delete the buffers in any order desired! */
{
	int offset = 0;

	if (ob != NULL)
	{
		/* find the buffer */
		for (offset = 0; offset < MAXBUFFERS; offset++)
		{
			if (si->overlay.myBuffer[offset].buffer == ob->buffer) break;
		}

		if (offset < MAXBUFFERS)
		/* delete current buffer */
		{
			si->overlay.myBuffer[offset].buffer = NULL;		
			si->overlay.myBuffer[offset].buffer_dma = NULL;		
			
			LOG(4,("Overlay: Release_buffer offset = %d, buffer released\n",offset));

			return B_OK;
		}
		else
		{
			/* this is no buffer of ours! */
			LOG(4,("Overlay: Release_overlay_buffer: not ours, aborted!\n"));

			return B_ERROR;
		}
	}
	else
	/* no buffer specified! */
	{
		LOG(4,("Overlay: Release_overlay_buffer: no buffer specified, aborted!\n"));

		return B_ERROR;
	}	
}

status_t GET_OVERLAY_CONSTRAINTS
	(const display_mode *dm, const overlay_buffer *ob, overlay_constraints *oc)
{
	int offset = 0;

	LOG(4,("Overlay: Get_overlay_constraints called\n"));

	/* check for NULL pointers */
	if ((dm == NULL) || (ob == NULL) || (oc == NULL))
	{
		LOG(4,("Overlay: Get_overlay_constraints: Null pointer(s) detected!\n"));
		return B_ERROR;
	}
	
	/* find the buffer */
	for (offset = 0; offset < MAXBUFFERS; offset++)
	{
		if (si->overlay.myBuffer[offset].buffer == ob->buffer) break;
	}

	if (offset < MAXBUFFERS)
	{
		/* scaler input (values are in pixels) */
		oc->view.h_alignment = 0;
		oc->view.v_alignment = 0;

		switch (ob->space)
		{
			case B_YCbCr422:
					/* NeoMagic cards can work with 7. 
					 * Note: this has to be in sync with the slopspace setup during buffer allocation.. */
					//fixme: >= NM2200 use 16 instead of 8???
					oc->view.width_alignment = 7;
					break;
			default:
					/* we should not be here, but set the worst-case value just to be safe anyway */
					oc->view.width_alignment = 31;
					break;
		}

		oc->view.height_alignment = 0;
		oc->view.width.min = 1;
		oc->view.height.min = 2; /* two fields */
		oc->view.width.max = ob->width;
		oc->view.height.max = ob->height;

		/* scaler output restrictions */
		oc->window.h_alignment = 0;
		oc->window.v_alignment = 0;
		oc->window.width_alignment = 0;
		oc->window.height_alignment = 0;
		oc->window.width.min = 2;
		/* G200-G550 can output upto and including 2048 pixels in width */
		//fixme...
		if (dm->virtual_width > 2048)
		{
			oc->window.width.max = 2048;
		}
		else
		{
			oc->window.width.max = dm->virtual_width;
		}
		oc->window.height.min = 2;
		/* G200-G550 can output upto and including 2048 pixels in height */
		//fixme...
		if (dm->virtual_height > 2048)
		{
			oc->window.height.max = 2048;
		}
		else
		{
			oc->window.height.max = dm->virtual_height;
		}

		/* NeoMagic scaling restrictions */
		/* Note: official max is 8.0, and NM BES does not support downscaling! */
		oc->h_scale.min = 1.0;
		oc->h_scale.max = 8.0;
		oc->v_scale.min = 1.0;
		oc->v_scale.max = 8.0;

		return B_OK;
	}
	else
	{
		/* this is no buffer of ours! */
		LOG(4,("Overlay: Get_overlay_constraints: buffer is not ours, aborted!\n"));

		return B_ERROR;
	}
}

overlay_token ALLOCATE_OVERLAY(void)
{
	uint32 tmpToken;
	LOG(4,("Overlay: Allocate_overlay called: "));

	/* come up with a token */
	tmpToken = 0x12345678;

	/* acquire the shared benaphore */
	AQUIRE_BEN(si->overlay.lock)

	/* overlay unit already in use? */
	if (si->overlay.myToken == NULL)
	/* overlay unit is available */
	{
		LOG(4,("succesfull\n"));

		si->overlay.myToken = &tmpToken;

		/* release the shared benaphore */
		RELEASE_BEN(si->overlay.lock)

		return si->overlay.myToken;
	}
	else
	/* sorry, overlay unit is occupied */
	{
		LOG(4,("failed: already in use!\n"));

		/* release the shared benaphore */
		RELEASE_BEN(si->overlay.lock)

		return NULL;
	}
}

status_t RELEASE_OVERLAY(overlay_token ot)
{
	LOG(4,("Overlay: Release_overlay called: "));

	/* is this call for real? */
	if ((ot == NULL) || (si->overlay.myToken == NULL) || (ot != si->overlay.myToken))
	/* nope, abort */
	{
		LOG(4,("failed, not in use!\n"));

		return B_ERROR;
	}
	else
	/* call is for real */
	{

		nm_release_bes();

		LOG(4,("succesfull\n"));

		si->overlay.myToken = NULL;
		return B_OK;
	}
}

status_t CONFIGURE_OVERLAY
	(overlay_token ot, const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov)
{
	int offset = 0; /* used for buffer index */

	LOG(4,("Overlay: Configure_overlay called: "));

	/* Note:
	 * When a Workspace switch, screen prefs change, or overlay app shutdown occurs, BeOS will
	 * release all overlay buffers. The buffer currently displayed at that moment, may need some
	 * 'hardware releasing' in the CONFIGURE_OVERLAY routine. This is why CONFIGURE_OVERLAY gets
	 * called one more time then, with a null pointer for overlay_window and overlay_view, while
	 * the currently displayed overlay_buffer is given.
	 * The G200-G550 do not need to do anything on such an occasion, so we simply return if we
	 * get called then. */
	if ((ow == NULL) || (ov == NULL))
	{
		LOG(4,("output properties changed\n"));

		return B_OK;
	}	

	/* Note:
	 * If during overlay use the screen prefs are changed, or the workspace has changed, it
	 * may be that we were not able to re-allocate the requested overlay buffers (or only partly)
	 * due to lack of cardRAM. If the app does not respond properly to this, we might end up 
	 * with a NULL pointer instead of a overlay_buffer to work with here. 
	 * Of course, we need to abort then to prevent the system from 'going down'.
	 * The app will probably crash because it will want to write into this non-existant buffer
	 * at some point. */
	if (ob == NULL)
	{
		LOG(4,("no overlay buffer specified\n"));

		return B_ERROR;
	}	

	/* is this call done by the app that owns us? */
	if ((ot == NULL) || (si->overlay.myToken == NULL) || (ot != si->overlay.myToken))
	/* nope, abort */
	{
		LOG(4,("failed\n"));

		return B_ERROR;
	}
	else
	/* call is for real */
	{
		/* find the buffer's offset */
		for (offset = 0; offset < MAXBUFFERS; offset++)
		{
			if (si->overlay.myBuffer[offset].buffer == ob->buffer) break;
		}

		if (offset < MAXBUFFERS)
		{
			LOG(4,("succesfull, switching to buffer %d\n", offset));

			nm_configure_bes(ob, ow, ov, offset);

			return B_OK;
		}
		else
		{
			/* this is no buffer of ours! */
			LOG(4,("buffer is not ours, aborted!\n"));

			return B_ERROR;
		}
	}
}
