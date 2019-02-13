/*
	Copyright (c) 2002, Thomas Kurschel


	Part of Radeon accelerant

	Overlay interface
*/

#include "GlobalData.h"
#include "radeon_interface.h"
#include "video_overlay.h"
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include "overlay_regs.h"
#include "generic.h"

// we could add support of planar modes and YUV modes
// but I neither know how planar modes are defined nor
// whether there is any program that makes use of them
static uint32 overlay_colorspaces [] = 
{ 
	B_RGB15, B_RGB16, B_RGB32, B_YCbCr422, 0
};


// public function: number of overlay units
uint32 OVERLAY_COUNT( const display_mode *dm )
{
	SHOW_FLOW0( 3, "" );

	(void) dm;

	return 1;
}


// public function: return list of supported overlay colour spaces 
//	dm - display mode where overlay is to be used
const uint32 *OVERLAY_SUPPORTED_SPACES( const display_mode *dm )
{
	SHOW_FLOW0( 3, "" );

	(void) dm;

	return overlay_colorspaces;
}


// public function: returns supported features 
//	color_space - overlay's colour space
uint32 OVERLAY_SUPPORTED_FEATURES( uint32 color_space )
{
	SHOW_FLOW0( 3, "" );

	(void) color_space;

	return 
		B_OVERLAY_COLOR_KEY |
		B_OVERLAY_HORIZONTAL_FILTERING |
		B_OVERLAY_VERTICAL_FILTERING;
}


// public function: allocates overlay buffer
//	cs - overlay's colour space
//	width, height - width and height of overlay buffer
const overlay_buffer *ALLOCATE_OVERLAY_BUFFER( color_space cs, uint16 width, uint16 height )
{
	virtual_card *vc = ai->vc;
	shared_info *si = ai->si;
	radeon_alloc_mem am;
	overlay_buffer_node *node;
	overlay_buffer *buffer;
	status_t result;
	uint ati_space, test_reg, bpp;

	SHOW_FLOW0( 3, "" );

	switch( cs ) {
	case B_RGB15:
		SHOW_FLOW0( 3, "RGB15" );
		bpp = 2;
		ati_space = RADEON_SCALER_SOURCE_15BPP >> 8;
		test_reg = 0;
		break;
	case B_RGB16:
		SHOW_FLOW0( 3, "RGB16" );
		bpp = 2; 
		ati_space = RADEON_SCALER_SOURCE_16BPP >> 8;
		test_reg = 0;
		break;
	case B_RGB32:
		SHOW_FLOW0( 3, "RGB32" );
		bpp = 4; 
		ati_space = RADEON_SCALER_SOURCE_32BPP >> 8;
		test_reg = 0;
		break;
	case B_YCbCr422:
		SHOW_FLOW0( 3, "YCbCr422" );
		bpp = 2;
		// strange naming convention: VYUY has to be read backward,
		// i.e. you get (low to high address) YUYV, which is what we want!
		ati_space = RADEON_SCALER_SOURCE_VYUY422 >> 8;
		test_reg = 0;
		break;
	// YUV12 is planar pixel format consisting of two or three planes
	// I have no clue whether and how this format is used in BeOS
	// (don't even know how it is defined officially)
/*	case B_YUV12:
		SHOW_FLOW0( 3, "YUV12" );
		bpp = 2;
		uvpp = 1;
		ati_space = RADEON_SCALER_SOURCE_YUV12 >> 8;
		testreg = 0;
		break;*/
	default:
		SHOW_FLOW( 3, "Unsupported format (%x)", (int)cs );
		return NULL;
	}

	node = malloc( sizeof( overlay_buffer_node ));
	if( node == NULL )
		return NULL;

	node->ati_space = ati_space;
	node->test_reg = test_reg;

	ACQUIRE_BEN( si->engine.lock );

	// alloc graphics mem
	buffer = &node->buffer;

	buffer->space = cs;
	buffer->width = width;
	buffer->height = height;
	buffer->bytes_per_row = (width * bpp + 0xf) & ~0xf;

	am.magic = RADEON_PRIVATE_DATA_MAGIC;
	am.size = buffer->bytes_per_row * height;
	am.memory_type = mt_local;
	am.global = false;

	result = ioctl( ai->fd, RADEON_ALLOC_MEM, &am );
	if( result != B_OK )
		goto err;

	node->mem_handle = am.handle;
	node->mem_offset = am.offset;
	buffer->buffer = si->local_mem + am.offset;
	buffer->buffer_dma = (void *) ((unsigned long) si->framebuffer_pci + am.offset);

	// add to list of overlays
	node->next = vc->overlay_buffers;
	node->prev = NULL;
	if( node->next )
		node->next->prev = node;

	vc->overlay_buffers = node;

	RELEASE_BEN( si->engine.lock );

	SHOW_FLOW( 0, "success: mem_handle=%x, offset=%x, CPU-address=%x, phys-address=%x", 
		node->mem_handle, node->mem_offset, buffer->buffer, buffer->buffer_dma );

	return buffer;

err:
	free(node);
	RELEASE_BEN( si->engine.lock );
	return NULL;
}


// public function: discard overlay buffer
status_t RELEASE_OVERLAY_BUFFER( const overlay_buffer *ob )
{
	virtual_card *vc = ai->vc;
	shared_info *si = ai->si;
	overlay_buffer_node *node;
	radeon_free_mem fm;
	status_t result;

	SHOW_FLOW0( 3, "" );

	node = (overlay_buffer_node *)((char *)ob - offsetof( overlay_buffer_node, buffer ));

	if( si->active_overlay.on == node || si->active_overlay.prev_on )
		Radeon_HideOverlay( ai );

	// free memory
	fm.magic = RADEON_PRIVATE_DATA_MAGIC;
	fm.handle = node->mem_handle;
	fm.memory_type = mt_local;
	fm.global = false;

	result = ioctl( ai->fd, RADEON_FREE_MEM, &fm );
	if( result != B_OK ) {
		SHOW_FLOW( 3, "ups - couldn't free memory (handle=%x, status=%s)", 
			node->mem_handle, strerror( result ));
	}

	ACQUIRE_BEN( si->engine.lock );

	// remove from list
	if( node->next )
		node->next->prev = node->prev;

	if( node->prev )
		node->prev->next = node->next;
	else
		vc->overlay_buffers = node->next;

	RELEASE_BEN( si->engine.lock );

	SHOW_FLOW0( 3, "success" );

	return B_OK;
}


// public function: get constraints of overlay unit
status_t GET_OVERLAY_CONSTRAINTS( const display_mode *dm, const overlay_buffer *ob, 
	overlay_constraints *oc )
{
	SHOW_FLOW0( 3, "" );

	// probably, this is paranoia as we only get called by app_server 
	// which should know what it's doing
	if( dm == NULL || ob == NULL || oc == NULL )
		return B_BAD_VALUE;

	// scaler input restrictions
	// TBD: check all these values; I reckon that
	//      most of them are too restrictive

	// position
	oc->view.h_alignment = 0;
	oc->view.v_alignment = 0;

	// alignment
	switch (ob->space) {
		case B_RGB15:
			oc->view.width_alignment = 7;
			break;
		case B_RGB16:
			oc->view.width_alignment = 7;
			break;
		case B_RGB32:
			oc->view.width_alignment = 3;
			break;
		case B_YCbCr422:
			oc->view.width_alignment = 7;
			break;
		case B_YUV12:
			oc->view.width_alignment = 7;
		default:
			return B_BAD_VALUE;
	}
	oc->view.height_alignment = 0;

	// size
	oc->view.width.min = 4;		// make 4-tap filter happy
	oc->view.height.min = 4;
	oc->view.width.max = ob->width;
	oc->view.height.max = ob->height;

	// scaler output restrictions
	oc->window.h_alignment = 0;
	oc->window.v_alignment = 0;
	oc->window.width_alignment = 0;
	oc->window.height_alignment = 0;
	oc->window.width.min = 2;
	oc->window.width.max = dm->virtual_width;
	oc->window.height.min = 2;
	oc->window.height.max = dm->virtual_height;

	// TBD: these values need to be checked
	//      (shamelessly copied from Matrix driver)
	oc->h_scale.min = 1.0f / (1 << 4);
	oc->h_scale.max = 1 << 12;
	oc->v_scale.min = 1.0f / (1 << 4);
	oc->v_scale.max = 1 << 12;

	SHOW_FLOW0( 3, "success" );

	return B_OK;
}


// public function: allocate overlay unit
overlay_token ALLOCATE_OVERLAY( void )
{
	shared_info *si = ai->si;
	virtual_card *vc = ai->vc;

	SHOW_FLOW0( 3, "" );

	if( atomic_or( &si->overlay_mgr.inuse, 1 ) != 0 ) {
		SHOW_FLOW0( 3, "already in use" );
		return NULL;
	}

	SHOW_FLOW0( 3, "success" );

	vc->uses_overlay = true;

	return (void *)++si->overlay_mgr.token;
}


// public function: release overlay unit
status_t RELEASE_OVERLAY(overlay_token ot)
{
	virtual_card *vc = ai->vc;
	shared_info *si = ai->si;

	SHOW_FLOW0( 3, "" );

	if( (void *)si->overlay_mgr.token != ot )
		return B_BAD_VALUE;

	if( si->overlay_mgr.inuse == 0 )
		return B_ERROR;

	if( si->active_overlay.on )
		Radeon_HideOverlay( ai );

	si->overlay_mgr.inuse = 0;
	vc->uses_overlay = false;

	SHOW_FLOW0( 3, "released" );

	return B_OK;
}


// public function: show/hide overlay
status_t CONFIGURE_OVERLAY( overlay_token ot, const overlay_buffer *ob,
	const overlay_window *ow, const overlay_view *ov )
{
	shared_info *si = ai->si;
	status_t result;

	SHOW_FLOW0( 4, "" );

	if ( (uintptr_t)ot != si->overlay_mgr.token )
		return B_BAD_VALUE;

	if ( !si->overlay_mgr.inuse )
		return B_BAD_VALUE;

	if ( ow == NULL || ov == NULL ) {
		SHOW_FLOW0( 3, "hide only" );
		Radeon_HideOverlay( ai );
		return B_OK;
	}

	if ( ob == NULL )
		return B_ERROR;

	ACQUIRE_BEN( si->engine.lock );

	// store whished values
	si->pending_overlay.ot = ot;
	si->pending_overlay.ob = *ob;
	si->pending_overlay.ow = *ow;
	si->pending_overlay.ov = *ov;

	si->pending_overlay.on = (overlay_buffer_node *)((char *)ob - offsetof( overlay_buffer_node, buffer ));

	result = Radeon_UpdateOverlay( ai );

	RELEASE_BEN( si->engine.lock );

	return result;
}
