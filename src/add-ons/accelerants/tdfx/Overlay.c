/* Overlay.c Framwork Taken from Rudolf Cornelissen (Open)BeOS MGA Driver
   05/09-2002 V0.13 beta1 */ 

#include "GlobalData.h"
#include "generic.h"
#include "voodoo3_accelerant.h"

static uint32 overlay_colorspaces [] = {(uint32)B_YCbCr422, 
#if defined(__POWERPC__) 
	(uint32)B_RGB16_BIG,
#else
	(uint32)B_RGB16,
#endif
	(uint32)B_NO_COLOR_SPACE }; 


// This method is never used AFAIK though it *is* exported on R5.0.3 and DANO. 
// Does someone know howto invoke it? 
// apparantly overlay count should report the number of 'overlay units' on the card

uint32 OVERLAY_COUNT(const display_mode *dm)  { 
	if (dm == NULL) 
		ddpf( ("OVERLAY_COUNT:No DM given\n"));
	return 1; 
} 

// This method is never used AFAIK though it *is* exported on R5.0.3 and DANO. 
// Does someone know howto invoke it? 
const uint32 *OVERLAY_SUPPORTED_SPACES(const display_mode *dm) { 
	if (dm == NULL) {
		ddpf (("OVERLAY_SUPPORTED_SPACES:No DM given\n"));
		return NULL; 
	} 
	/* return a B_NO_COLOR_SPACE terminated list */ 
    return &overlay_colorspaces[0]; 
}

// This method is never used AFAIK. On R5.0.3 and DANO it is not even exported! 
uint32 OVERLAY_SUPPORTED_FEATURES(uint32 a_color_space) { 
	switch (a_color_space){ 
		default: 
			return 
            	( B_OVERLAY_COLOR_KEY | 
                  B_OVERLAY_HORIZONTAL_FILTERING | 
                  B_OVERLAY_VERTICAL_FILTERING ); 
    } 
} 

const overlay_buffer *ALLOCATE_OVERLAY_BUFFER(color_space cs, uint16 width, uint16 height) { 

	int offset = 0;                 // used to determine next buffer to create 
	uint32 adress, adress2, temp32; // used to calculate buffer adresses
	uint32 oldsize = 0;             // used to 'squeeze' new buffers between already existing ones
	int cnt;                        


	/* acquire the shared benaphore */ 
	AQUIRE_BEN(si->overlay.lock) 

	/* find first empty slot (room for another buffer?) */ 
	for (offset = 0; offset < MAXBUFFERS; offset++) 
	if (si->overlay.myBuffer[offset].buffer == NULL) break; 

	/* I just could make a lookup on overlay_colorspaces instead but its overhead I thing */
 
	if (offset < MAXBUFFERS) { 
		switch (cs) { 
			case B_YCbCr422:
#if defined(__POWERPC__) 
			case B_RGB16_BIG:			
#else
			case B_RGB16:
#endif
				si->overlay.myBuffer[offset].width = width; 
                si->overlay.myBuffer[offset].bytes_per_row = 2 * si->overlay.myBuffer[offset].width; 
                if (si->overlay.myBuffer[offset].width > 2048) { 
    	            // release the shared benaphore  
					RELEASE_BEN(si->overlay.lock) 
					return NULL; 
				}
            	break; 
            default: 
	                 // release the shared benaphore */ 
					RELEASE_BEN(si->overlay.lock) 
					return NULL; 
		}

		// store slopspace (in pixels) for each bitmap for use by 'overlay unit' (BES) 
		si->overlay.myBufInfo[offset].slopspace = si->overlay.myBuffer[offset].width - width; 

		si->overlay.myBuffer[offset].space = cs; 
		si->overlay.myBuffer[offset].height = height; 
        
		// we define the overlay buffers to reside 'in the back' of the cards RAM 
 
		// calculate first free RAM adress in card: 
		adress2 = (((uint32)((uint8*)si->fbc.frame_buffer)) +   		
                    (si->fbc.bytes_per_row * si->dm.virtual_height));       
		// calculate 'preliminary' buffer size including slopspace 
		oldsize = si->overlay.myBufInfo[offset].size; 
		si->overlay.myBufInfo[offset].size = 
		si->overlay.myBuffer[offset].bytes_per_row * si->overlay.myBuffer[offset].height; 

		// calculate virtual memory adress that would be needed for a new bitmap

		adress = (((uint32)((uint8*)si->framebuffer)) + (si->mem_size)); 
		for (cnt = 0; cnt <= offset; cnt++) adress -= si->overlay.myBufInfo[cnt].size; 

		// the > G200 scalers require buffers to be aligned to 16 byte pages cardRAM offset, G200 can do with 
		// 8 byte pages cardRAM offset. Compatible settings used, has no real downside consequences here 

		// Check if we need to modify the buffers starting adress and thus the size
		// calculate 'would be' cardRAM offset 
		temp32 = (adress - ((uint32)((vuint32 *)si->framebuffer))); 

		// check if it is aligned //
		if (temp32 != (temp32 & ~3)) { 
			// update the (already calculated) buffersize to get it aligned 
			si->overlay.myBufInfo[offset].size += (temp32 - (temp32 & ~3)); 
			// update the (already calculated) adress to get it aligned 
			adress -= (temp32 - (temp32  & ~3)); 
        } 

		// First check now if buffer to be defined is 'last one' in memory (speaking backwards): 
		// this is done to prevent a large buffer getting created in the space a small buffer 
		// occupied earlier, if not all buffers created were deleted. 
		
		for (cnt = offset; cnt < MAXBUFFERS; cnt++) { 
			if (si->overlay.myBuffer[cnt].buffer != NULL) {
				// Check if the new buffer would fit into the space the single old one used here
				if (si->overlay.myBufInfo[offset].size <= oldsize) { 
					// It does, so we reset to the old size and adresses to prevent the space from shrinking
					// if we get here again...
					adress -= (oldsize - si->overlay.myBufInfo[offset].size); 
					si->overlay.myBufInfo[offset].size = oldsize; 
					// force exiting the FOR loop
					cnt = MAXBUFFERS; 
				} else { 
					// Reset to the old size to prevent the space from 'growing' if we get here again...
					si->overlay.myBufInfo[offset].size = oldsize; 
					// release the shared benaphore 
					RELEASE_BEN(si->overlay.lock) 
					return NULL; 
				} 
			} 
		} 
		// check if we have enough space to setup this new bitmap 
		// (preventing overlap of desktop RAMspace & overlay bitmap RAMspace here) 
		if (adress < adress2) {
			// nope, sorry
			RELEASE_BEN(si->overlay.lock) 
			return NULL; 
		} 
		// continue buffer setup
		si->overlay.myBuffer[offset].buffer = (void *) adress; 

		// calculate physical memory adress (for dma use) 
		adress = (((uint32)((uint8*)si->framebuffer_pci)) + (si->mem_size)); 
		for (cnt = 0; cnt <= offset; cnt++) 
			adress -= si->overlay.myBufInfo[cnt].size; 
		// this adress is already aligned to the scaler's requirements (via the already modified sizes)

		si->overlay.myBuffer[offset].buffer_dma = (void *) adress; 

		// release the shared benaphore
		RELEASE_BEN(si->overlay.lock)
        return &si->overlay.myBuffer[offset]; 
    } else {
		// sorry, no more room for buffers */ 
        RELEASE_BEN(si->overlay.lock) 
        return NULL; 
    } 
} 

status_t RELEASE_OVERLAY_BUFFER(const overlay_buffer *ob) { 
	int offset = 0; 
	if (ob != NULL) { 
		//Find the buffer
		for (offset = 0; offset < MAXBUFFERS; offset++) 
		if (si->overlay.myBuffer[offset].buffer == ob->buffer) break; 

	    if (offset < MAXBUFFERS) { // delete current buffer 
			si->overlay.myBuffer[offset].buffer = NULL;             
        	si->overlay.myBuffer[offset].buffer_dma = NULL;         
                        
	        return B_OK; 
    	} else { 
			return B_ERROR; 
    	} 
    } else { 
		return B_ERROR; 
    }       
} 

status_t GET_OVERLAY_CONSTRAINTS (const display_mode *dm, const overlay_buffer *ob, overlay_constraints *oc) { 

	int offset = 0; 
	// Check for NULL Pointers
    if ((dm == NULL) || (ob == NULL) || (oc == NULL)) { 
		return B_ERROR; 
    } 
        
	// find the buffer
	for (offset = 0; offset < MAXBUFFERS; offset++) 
    if (si->overlay.myBuffer[offset].buffer == ob->buffer) break; 

	if (offset < MAXBUFFERS) { 
    	// scaler input (values are in pixels)
        oc->view.h_alignment = 3; 
        oc->view.v_alignment = 3; 

        switch (ob->space) { 
			default: 
				// we should not be here, but set the worst-case value just to be safe anyway
	            oc->view.width_alignment = 3; 
                break; 
		} 

		oc->view.height_alignment = 3; 
		oc->view.width.min = 1; 
		oc->view.height.min = 2;  // two fields
		oc->view.width.max = ob->width; 
		oc->view.height.max = ob->height; 

		// scaler output restrictions
		oc->window.h_alignment = 3; 
		oc->window.v_alignment = 3; 
		oc->window.width_alignment = 3; 
		oc->window.height_alignment = 3; 
		oc->window.width.min = 2; 
		if (dm->virtual_width > 2048) oc->window.width.max = 2048; else
			oc->window.width.max = dm->virtual_width; 

		oc->window.height.min = 2; 

		if (dm->virtual_height > 2048) oc->window.height.max = 2048; else
			oc->window.height.max = dm->virtual_height; 

		oc->h_scale.min = 1.0f; 
		oc->h_scale.max = 4.0f; 
		oc->v_scale.min = 1.0f; 
		oc->v_scale.max = 4.0f; 
		return B_OK; 
	} else { 
 		return B_ERROR; 
    } 
} 

overlay_token ALLOCATE_OVERLAY(void) 
{ 
	uint32 tmpToken; 
	// come up with a token
	tmpToken = 0x12345678; 
	
	// acquire the shared benaphore 
	AQUIRE_BEN(si->overlay.lock) 
	// overlay unit already in use? 
    if (si->overlay.myToken == NULL) {
	    // overlay unit is available
		si->overlay.myToken = &tmpToken; 
		// release the shared benaphore
		RELEASE_BEN(si->overlay.lock) 
		return si->overlay.myToken; 
	} else {
		RELEASE_BEN(si->overlay.lock) 
		return NULL; 
    } 
} 

status_t RELEASE_OVERLAY(overlay_token ot) { 

	// is this call for real?
	if ((ot == NULL) || (si->overlay.myToken == NULL) || (ot != si->overlay.myToken)) { 
		return B_ERROR; 
    } else { 
		// Do really something here!
		voodoo3_stop_overlay();
		si->overlay.myToken = NULL; 
       	return B_OK; 
    } 
} 

status_t CONFIGURE_OVERLAY (overlay_token ot, const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov) { 
	
	int offset = 0; // used for buffer index


	// in BeOS R5.0.3 (maybe DANO works different): 
	// ov' is the output size on the desktop: does not change if clipped. 
	// h_start and v_start are always 0, width and heigth are the size of the window. 
	// Because the width and height can also be found in '*ow', '*ov' is not used.

	if ((ow == NULL) || (ov == NULL)) {
		return B_OK; 
	}       

	if (ob == NULL) { 
		return B_ERROR; 
	}       

    // is this call done by the app that owns us? */ 
	if ((ot == NULL) || (si->overlay.myToken == NULL) || (ot != si->overlay.myToken)) { 
	    // nope, abort //
		return B_ERROR; 
	} else {
        // call is for real
		for (offset = 0; offset < MAXBUFFERS; offset++) 
		if (si->overlay.myBuffer[offset].buffer == ob->buffer) break; 

		if (offset < MAXBUFFERS) { 
			voodoo3_display_overlay(ow, ob, ov);
			return B_OK;
		} else { 
			return B_ERROR; 
        } 
    }
}
