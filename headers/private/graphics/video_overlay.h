#if !defined(_VIDEO_OVERLAY_H_)
#define _VIDEO_OVERLAY_H_

/*
	Copyright Be Incorporated.
	This file will eventually be merged into Accelerant.h once the API is finalized.
*/

#include <Accelerant.h>
#include <GraphicsDefs.h>

#if defined(__cplusplus)
extern "C" {
#endif

enum {
	B_SUPPORTS_OVERLAYS = 1 << 31	// part of display_mode.flags
};

enum {
	B_OVERLAY_COUNT = 0x08000000,
	B_OVERLAY_SUPPORTED_SPACES,
	B_OVERLAY_SUPPORTED_FEATURES,
	B_ALLOCATE_OVERLAY_BUFFER,
	B_RELEASE_OVERLAY_BUFFER,
	B_GET_OVERLAY_CONSTRAINTS,
	B_ALLOCATE_OVERLAY,
	B_RELEASE_OVERLAY,
	B_CONFIGURE_OVERLAY	
};

typedef struct {
	uint32	space;	/* color_space of buffer */
	uint16	width;	/* width in pixels */
	uint16	height;	/* height in lines */
	uint32	bytes_per_row;		/* number of bytes in one line */
	void	*buffer;		/* pointer to first byte of overlay buffer in virtual memory */
	void	*buffer_dma;	/* pointer to first byte of overlay buffer in physical memory for DMA */
} overlay_buffer;

typedef struct {
	uint16	h_start;
	uint16	v_start;
	uint16	width;
	uint16	height;
} overlay_view;

enum {
	B_OVERLAY_COLOR_KEY = 1 << 0,
	B_OVERLAY_CHROMA_KEY = 1 << 1,
	B_OVERLAY_HORIZONTAL_FILTERING = 1 << 2,
	B_OVERLAY_VERTICAL_FILTERING = 1 << 3,
	B_OVERLAY_HORIZONTAL_MIRRORING = 1 << 4,
	B_OVERLAY_KEYING_USES_ALPHA = 1 << 5
};

typedef struct {
	uint8	value;	/* if DAC color component of graphic pixel & mask == value, */
	uint8	mask;	/*  then show overlay pixel, else show graphic pixel */
} overlay_key_color;

typedef struct {
	int16	h_start;	/* Top left un-clipped corner of the window in the virtual display */
	int16	v_start;	/* Note that these are _signed_ values */
	uint16	width;		/* un-clipped width of the overlay window */
	uint16	height;		/* un-clipped height of the overlay window */

	uint16	offset_left;	/* that portion of the overlay_window which is actually displayed */
	uint16	offset_top;		/* ie, the first line displayed is v_start + offset_top           */
	uint16	offset_right;	/*     and the first pixel displayed is h_start + offset_left     */
	uint16	offset_bottom;

	overlay_key_color	red;	/* when using color keying, all components must match */
	overlay_key_color	green;
	overlay_key_color	blue;
	overlay_key_color	alpha;
	uint32	flags;	/* which features should be enabled.  See enum above. */
} overlay_window;

typedef struct {
	uint16	min;
	uint16	max;
} overlay_uint16_minmax;

typedef struct {
	float	min;
	float	max;
} overlay_float_minmax;

typedef struct {
	uint16	h_alignment;	/* alignments: a 1 bit set in every bit which must be zero */
	uint16	v_alignment;
	uint16	width_alignment;
	uint16	height_alignment;
	overlay_uint16_minmax	width;	/* min and max sizes in each axis */
	overlay_uint16_minmax	height;
} overlay_limits;

typedef struct {
	overlay_limits	view;
	overlay_limits	window;
	overlay_float_minmax	h_scale;
	overlay_float_minmax	v_scale;
} overlay_constraints;

typedef void * overlay_token;

typedef uint32 (*overlay_count)(const display_mode *dm);
typedef const uint32 *(*overlay_supported_spaces)(const display_mode *dm);
typedef uint32 (*overlay_supported_features)(uint32 a_color_space);
typedef const overlay_buffer *(*allocate_overlay_buffer)(color_space cs, uint16 width, uint16 height);
typedef status_t (*release_overlay_buffer)(const overlay_buffer *ob);
typedef status_t (*get_overlay_constraints)(const display_mode *dm, const overlay_buffer *ob, overlay_constraints *oc);
typedef overlay_token (*allocate_overlay)(void);
typedef status_t (*release_overlay)(overlay_token ot);
typedef status_t (*configure_overlay)(overlay_token ot, const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov);

#if defined(__cplusplus)
}
#endif

#endif
