/* general card functions */
status_t nv_general_powerup(void);
status_t nv_set_cas_latency(void);
void setup_virtualized_heads(bool);
void set_crtc_owner(bool);
status_t nv_general_output_select(bool);
status_t nv_general_head_select(bool);
status_t nv_general_validate_pic_size (display_mode *target, uint32 *bytes_per_row, bool *acc_mode);

/* AGP functions */
status_t nv_agp_setup(bool enable_agp);

/* apsed: logging macros */
#define MSG(args) do { /* if needed or si->settings with si NULL */ \
	nv_log args; \
} while (0)
#define LOG(level_bit, args) do { \
	uint32 mod = (si->settings.logmask &  0xfffffff0) & MODULE_BIT; \
	uint32 lev = (si->settings.logmask & ~0xfffffff0) & level_bit; \
	if (mod && lev) nv_log args; \
} while (0)

/* support functions */
void delay(bigtime_t i);
void nv_log(char *format, ...);

/* i2c functions */
status_t i2c_sec_tv_adapter(void);
char i2c_flag_error (char ErrNo);
void i2c_bstart (uint8 BusNR);
void i2c_bstop (uint8 BusNR);
uint8 i2c_readbyte(uint8 BusNR, bool Ack);
bool i2c_writebyte (uint8 BusNR, uint8 byte);
void i2c_readbuffer (uint8 BusNR, uint8* buf, uint8 size);
void i2c_writebuffer (uint8 BusNR, uint8* buf, uint8 size);
status_t i2c_init(void);
void i2c_TestEDID(void);
void i2c_DetectScreens(void);

/* card info functions */
status_t parse_pins(void);
void set_pll(uint32 reg, uint32 clk);
void get_panel_modes(display_mode *p1, display_mode *p2, bool *pan1, bool *pan2);
void fake_panel_start(void);
status_t get_crtc1_screen_native_mode(display_mode *mode);
status_t get_crtc2_screen_native_mode(display_mode *mode);
void set_specs(void);
void dump_pins(void);

/* DAC functions */
bool nv_dac_crt_connected(void);
status_t nv_dac_mode(int,float);
status_t nv_dac_dither(bool dither);
status_t nv_dac_palette(uint8*,uint8*,uint8*);
status_t nv_dac_pix_pll_find(display_mode target,float * result,uint8 *,uint8 *,uint8 *, uint8);
status_t nv_dac_set_pix_pll(display_mode target);
status_t nv_dac_sys_pll_find(float, float*, uint8*, uint8*, uint8*, uint8);

/* DAC2 functions */
bool nv_dac2_crt_connected(void);
status_t nv_dac2_mode(int,float);
status_t nv_dac2_palette(uint8*,uint8*,uint8*);
status_t nv_dac2_pix_pll_find(display_mode target,float * result,uint8 *,uint8 *,uint8 *, uint8);
status_t nv_dac2_set_pix_pll(display_mode target);

/* Brooktree TV functions */
bool BT_probe(void);
uint8 BT_dpms(bool display);
uint8 BT_check_tvmode(display_mode target);
status_t BT_stop_tvout(void);
status_t BT_setmode(display_mode target);

/* CRTC1 functions */
status_t nv_crtc_interrupt_enable(bool);
status_t nv_crtc_update_fifo(void);
status_t nv_crtc_validate_timing(
	uint16 *hd_e,uint16 *hs_s,uint16 *hs_e,uint16 *ht,
	uint16 *vd_e,uint16 *vs_s,uint16 *vs_e,uint16 *vt);
status_t nv_crtc_set_timing(display_mode target);
status_t nv_crtc_depth(int mode);
status_t nv_crtc_set_display_start(uint32 startadd,uint8 bpp); 
status_t nv_crtc_set_display_pitch(void);
status_t nv_crtc_dpms(bool, bool, bool, bool);
status_t nv_crtc_mem_priority(uint8);
status_t nv_crtc_cursor_init(void);
status_t nv_crtc_cursor_define(uint8*,uint8*);
status_t nv_crtc_cursor_position(uint16 x ,uint16 y);
status_t nv_crtc_cursor_show(void);
status_t nv_crtc_cursor_hide(void);
status_t nv_crtc_stop_tvout(void);
status_t nv_crtc_start_tvout(void);

/* CRTC2 functions */
status_t nv_crtc2_interrupt_enable(bool);
status_t nv_crtc2_update_fifo(void);
status_t nv_crtc2_validate_timing(
	uint16 *hd_e,uint16 *hs_s,uint16 *hs_e,uint16 *ht,
	uint16 *vd_e,uint16 *vs_s,uint16 *vs_e,uint16 *vt);
status_t nv_crtc2_set_timing(display_mode target);
status_t nv_crtc2_depth(int mode);
status_t nv_crtc2_set_display_start(uint32 startadd,uint8 bpp); 
status_t nv_crtc2_set_display_pitch(void);
status_t nv_crtc2_dpms(bool, bool, bool, bool);
status_t nv_crtc2_mem_priority(uint8);
status_t nv_crtc2_cursor_init(void);
status_t nv_crtc2_cursor_define(uint8*,uint8*);
status_t nv_crtc2_cursor_position(uint16 x ,uint16 y);
status_t nv_crtc2_cursor_show(void);
status_t nv_crtc2_cursor_hide(void);
status_t nv_crtc2_stop_tvout(void);
status_t nv_crtc2_start_tvout(void);

/* acceleration functions */
//offscreen_buffer_config, cliprect and clipped_scaled_blit_params are here temporary..
typedef struct {
	void*	buffer;						/* pointer to first byte of frame */
										/* buffer in virtual memory */

	void*	buffer_dma;					/* pointer to first byte of frame */
										/* buffer in physical memory for DMA */

	uint32	bytes_per_row;				/* number of bytes in one */
										/* virtual_width line */
										/* not neccesarily the same as */
										/* virtual_width * byte_per_pixel */

	uint32	space;						/* pixel configuration */
} offscreen_buffer_config;

typedef struct {
	uint16	left;						/* offset */
	uint16	top;
	uint16	width;						/* 0 to N, where zero means one */
										/* pixel, one means two pixels, */
										/* etc. */
	uint16	height;						/* 0 to M, where zero means one */
										/* line, one means two lines, etc. */
} cliprect;

typedef struct {
	uint16		src_left;				/* offset within source rectangle */
	uint16		src_top;
	uint16		src_width;				/* 0 to N, where zero means one */
										/* pixel, one means two pixels, */
										/* etc. */
	uint16		src_height;				/* 0 to M, where zero means one */
										/* line, one means two lines, etc. */
	uint16		dest_left;				/* output reference, only the cliplist is displayed */
	uint16		dest_top;
	uint16		dest_width;				/* 0 to N, where zero means one */
										/* pixel, one means two pixels, etc. */
	uint16		dest_height;			/* 0 to M, where zero means one */
										/* line, one means two lines, etc. */

	cliprect*	dest_cliplist;			/* rectangle(s) to draw in destination */
										/* guaranteed constrained to */
										/* virtual width and height */
	uint16		dest_clipcount;			/* number of rectangles to draw */
} clipped_scaled_blit_params;

status_t check_acc_capability(uint32 feature);
status_t nv_acc_init(void);
void nv_acc_assert_fifo(void);
status_t nv_acc_setup_blit(void);
status_t nv_acc_blit(uint16,uint16,uint16, uint16,uint16,uint16 );
status_t nv_acc_setup_rectangle(uint32 color);
status_t nv_acc_rectangle(uint32 xs,uint32 xe,uint32 ys,uint32 yl);
status_t nv_acc_setup_rect_invert(void);
status_t nv_acc_rectangle_invert(uint32 xs,uint32 xe,uint32 ys,uint32 yl);
status_t nv_acc_transparent_blit(uint16,uint16,uint16, uint16,uint16,uint16, uint32);
status_t nv_acc_video_blit(uint16 xs,uint16 ys,uint16 ws, uint16 hs,
	uint16 xd,uint16 yd,uint16 wd,uint16 hd);
status_t nv_acc_wait_idle(void);
/* DMA versions */
status_t nv_acc_wait_idle_dma(void);
status_t nv_acc_init_dma(void);
void nv_acc_assert_fifo_dma(void);
void SCREEN_TO_SCREEN_BLIT_DMA(engine_token *et, blit_params *list, uint32 count);
void SCREEN_TO_SCREEN_TRANSPARENT_BLIT_DMA(engine_token *et, uint32 transparent_colour, blit_params *list, uint32 count);
void SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT_DMA(engine_token *et, scaled_blit_params *list, uint32 count);
void OFFSCREEN_TO_SCREEN_SCALED_FILTERED_BLIT_DMA(
	engine_token *et, offscreen_buffer_config *config, clipped_scaled_blit_params *list, uint32 count);
void FILL_RECTANGLE_DMA(engine_token *et, uint32 color, fill_rect_params *list, uint32 count);
void INVERT_RECTANGLE_DMA(engine_token *et, fill_rect_params *list, uint32 count);
void FILL_SPAN_DMA(engine_token *et, uint32 color, uint16 *list, uint32 count);

/* backend scaler functions */
status_t check_overlay_capability(uint32 feature);
void nv_bes_move_overlay(void);
status_t nv_bes_to_crtc(bool crtc);
status_t nv_bes_init(void);
status_t nv_configure_bes
	(const overlay_buffer *ob, const overlay_window *ow,const overlay_view *ov, int offset);
status_t nv_release_bes(void);

/* driver structures and enums */
enum{BPP8 = 0, BPP15 = 1, BPP16 = 2, BPP24 = 3, BPP32 = 4};
