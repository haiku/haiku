/*general card functions*/
status_t gx00_general_powerup(void);
status_t mga_set_cas_latency(void);
status_t gx50_general_output_select(void);
status_t gx00_general_dac_select(int);
status_t gx00_general_wait_retrace(void);
status_t gx00_general_validate_pic_size (display_mode *target, uint32 *bytes_per_row, bool *acc_mode);

/* apsed: logging macros */
#define MSG(args) do { /* if needed or si->settings with si NULL */ \
	mga_log args; \
} while (0)
#define LOG(level_bit, args) do { \
	uint32 mod = (si->settings.logmask &  0xfffffff0) & MODULE_BIT; \
	uint32 lev = (si->settings.logmask & ~0xfffffff0) & level_bit; \
	if (mod && lev) mga_log args; \
} while (0)

/*support functions*/
void delay(bigtime_t i);
void mga_log(char *format, ...);

/*i2c maven functions*/
int i2c_maven_read(unsigned char address);
void i2c_maven_write(unsigned char address, unsigned char data);
status_t i2c_init(void);
status_t i2c_maven_probe(void);


/*card info functions*/
status_t parse_pins(void);
status_t pins1_read(uint8 *pins, uint8 length);
status_t pins2_read(uint8 *pins, uint8 length);
status_t pins3_read(uint8 *pins, uint8 length);
status_t pins4_read(uint8 *pins, uint8 length);
status_t pins5_read(uint8 *pins, uint8 length);
void fake_pins(void);
void pinsmil1_fake(void);
void pinsmil2_fake(void);
void pinsg100_fake(void);
void pinsg200_fake(void);
void pinsg400_fake(void);
void pinsg400max_fake(void);
void pinsg450_fake(void);
void pinsg550_fake(void);
void dump_pins(void);

/*DAC functions*/
status_t gx00_dac_mode(int,float);
status_t gx00_dac_palette(uint8*,uint8*,uint8*);

status_t gx00_dac_pix_pll_find(display_mode target,float * result,uint8 *,uint8 *,uint8 *, uint8);
status_t gx00_dac_set_pix_pll(display_mode target);

status_t g450_dac_set_sys_pll(void);
status_t g400_dac_set_sys_pll(void);
status_t g200_dac_set_sys_pll(void);
status_t g100_dac_set_sys_pll(void);

status_t mil2_dac_init(void);
status_t mil2_dac_set_pix_pll(float f_vco,int bpp);

/*MAVEN functions*/
status_t gx00_maven_clrline(void);
status_t gx00_maven_dpms(bool display, bool h, bool v);
status_t gx00_maven_set_timing(display_mode target);
status_t gx00_maven_mode(int,float);

status_t g100_g400max_maven_vid_pll_find(display_mode target,float * calc_pclk,
				uint8 * m_result,uint8 * n_result,uint8 * p_result);
status_t g450_g550_maven_vid_pll_find(display_mode target,float * calc_pclk,
				uint8 * m_result,uint8 * n_result,uint8 * p_result, uint8 test);
void gx00_maven_shutoff(void);
status_t gx00_maven_set_vid_pll(display_mode target);

/*MAVENTV functions*/
status_t g100_g400max_maventv_vid_pll_find(
	display_mode target, unsigned int * ht_new, unsigned int * ht_last_line,
	uint8 * m_result, uint8 * n_result, uint8 * p_result);
int maventv_init(display_mode target);

/*CRTC1 functions*/
status_t gx00_crtc_validate_timing(
	uint16 *hd_e,uint16 *hs_s,uint16 *hs_e,uint16 *ht,
	uint16 *vd_e,uint16 *vs_s,uint16 *vs_e,uint16 *vt
);
status_t gx00_crtc_set_timing(display_mode target);
status_t gx00_crtc_depth(int mode);
status_t gx00_crtc_set_display_start(uint32 startadd,uint8 bpp); 
status_t gx00_crtc_set_display_pitch(void);
status_t gx00_crtc_dpms(bool display,bool h,bool v);
status_t gx00_crtc_mem_priority(uint8);
status_t gx00_crtc_cursor_init(void); /*Yes, cursor follows CRTC1 - not the DAC!*/
status_t gx00_crtc_cursor_define(uint8*,uint8*);
status_t gx00_crtc_cursor_position(uint16 x ,uint16 y);
status_t gx00_crtc_cursor_show(void);
status_t gx00_crtc_cursor_hide(void);

/*CRTC2 functions*/
/*XXX - validate_timing*/
status_t g400_crtc2_set_timing(display_mode target);
status_t g400_crtc2_depth(int mode);
status_t g400_crtc2_set_display_pitch(void); 
status_t g400_crtc2_set_display_start(uint32 startadd,uint8 bpp); 
status_t g400_crtc2_dpms(bool display, bool h, bool v);

/*acceleration functions*/
status_t check_acc_capability(uint32 feature);
status_t gx00_acc_wait_idle(void);
status_t gx00_acc_init(void);
void SCREEN_TO_SCREEN_BLIT(engine_token *et, blit_params *list, uint32 count);
void SCREEN_TO_SCREEN_TRANSPARENT_BLIT(engine_token *et, uint32 transparent_colour, blit_params *list, uint32 count);
void SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT(engine_token *et, scaled_blit_params *list, uint32 count);
void FILL_RECTANGLE(engine_token *et, uint32 color, fill_rect_params *list, uint32 count);
void FILL_SPAN(engine_token *et, uint32 color, uint16 *list, uint32 count);
void INVERT_RECTANGLE(engine_token *et, fill_rect_params *list, uint32 count);

/*backend scaler functions*/
status_t check_overlay_capability(uint32 feature);
void gx00_bes_move_overlay(void);
status_t gx00_configure_bes
	(const overlay_buffer *ob, const overlay_window *ow,const overlay_view *ov, int offset);
status_t gx00_release_bes(void);

/* I2C functions */
status_t i2c_sec_tv_adapter(void);

/*driver structures and enums*/
enum
{
	BPP8 = 0,
	BPP15 = 1,
	BPP16 = 2,
	BPP24 = 3,
	BPP32DIR = 4,
	BPP32 = 7
};

enum
{
	DS_CRTC1DAC,
	DS_CRTC1MAVEN,
	DS_CRTC1DAC_CRTC2MAVEN,
	DS_CRTC1MAVEN_CRTC2DAC,
	DS_CRTC1CON1_CRTC2CON2,
	DS_CRTC1CON2_CRTC2CON1
};
