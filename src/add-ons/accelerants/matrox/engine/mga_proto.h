/*general card functions*/
status_t gx00_general_powerup();
status_t mga_set_cas_latency();
status_t gx50_general_output_select();
status_t gx00_general_dac_select(int);
status_t gx00_general_wait_retrace();
//status_t gx00_general_bios_to_powergraphics();

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

status_t g450_dac_set_sys_pll();
status_t g400_dac_set_sys_pll();
status_t g200_dac_set_sys_pll();
status_t g100_dac_set_sys_pll();

status_t mil2_dac_init(void);
status_t mil2_dac_mode(int,float, int hsync_pos,int vsync_pos, int sync_green);
status_t mil2_dac_palette(uint8*,uint8*,uint8*);
status_t mil2_dac_pix_pll_find(float f_vco,float * result,uint8 *,uint8 *,uint8 *);
status_t mil2_dac_set_pix_pll(float f_vco,int bpp);

/*MAVEN functions*/
status_t gx00_maven_dpms(uint8,uint8,uint8);
status_t gx00_maven_set_timing(
	uint32 hdisp_e,uint32 hsync_s,uint32 hsync_e,uint32 htotal,
	uint32 vdisp_e,uint32 vsync_s,uint32 vsync_e,uint32 vtotal,
	uint8 hsync_pos,uint8 vsync_pos
	);
status_t gx00_maven_mode(int,float);

status_t gx00_maven_pix_pll_find(float f_vco,float * result,uint8 *,uint8 *,uint8 *);
status_t gx00_maven_set_pix_pll(float f_vco);

/*CRTC1 functions*/
status_t gx00_crtc_validate_timing(
	uint16 *hd_e,uint16 *hs_s,uint16 *hs_e,uint16 *ht,
	uint16 *vd_e,uint16 *vs_s,uint16 *vs_e,uint16 *vt
);
status_t gx00_crtc_set_timing(
	uint16 hd_e,uint16 hs_s,uint16 hs_e,uint16 ht,
	uint16 vd_e,uint16 vs_s,uint16 vs_e,uint16 vt,
	uint8 hsync_pos,uint8 vsync_pos
);
status_t gx00_crtc_depth(int mode);
status_t gx00_crtc_set_display_start(uint32 startadd,uint8 bpp); 
status_t gx00_crtc_set_display_pitch(uint32 pitch,uint8 bpp);

status_t gx00_crtc_dpms(uint8,uint8,uint8);
status_t gx00_crtc_dpms_fetch(uint8*,uint8*,uint8*);
status_t gx00_crtc_mem_priority(uint8);

status_t gx00_crtc_cursor_init(); /*Yes, cursor follows CRTC1 - not the DAC!*/
status_t gx00_crtc_cursor_define(uint8*,uint8*);
status_t gx00_crtc_cursor_position(uint16 x ,uint16 y);
status_t gx00_crtc_cursor_show();
status_t gx00_crtc_cursor_hide();

/*CRTC2 functions*/
/*XXX - validate_timing*/
status_t g400_crtc2_set_timing(
	uint32 hdisp_e,uint32 hsync_s,uint32 hsync_e,uint32 htotal,
	uint32 vdisp_e,uint32 vsync_s,uint32 vsync_e,uint32 vtotal,
	uint8 hsync_pos,uint8 vsync_pos
	);
status_t g400_crtc2_depth(int mode);
status_t g400_crtc2_set_display_pitch(uint32 pitch,uint8 bpp); 
status_t g400_crtc2_set_display_start(uint32 startadd,uint8 bpp); 

status_t g400_crtc2_dpms(uint8 display,uint8 h,uint8 v);
status_t g400_crtc2_dpms_fetch(uint8 * display,uint8 * h,uint8 * v);

/*acceleration functions*/
status_t gx00_acc_init();
status_t gx00_acc_rectangle(uint32 xs,uint32 xe,uint32 ys,uint32 yl,uint32 col);
status_t gx00_acc_rectangle_invert(uint32 xs,uint32 xe,uint32 ys,uint32 yl,uint32 col);
status_t gx00_acc_blit(uint16,uint16,uint16, uint16,uint16,uint16 );
status_t gx00_acc_transparent_blit(uint16,uint16,uint16, uint16,uint16,uint16, uint32);
status_t gx00_acc_wait_idle();

/*backend scaler functions*/
status_t check_overlay_capability(uint32 feature);
status_t gx00_configure_bes
	(const overlay_buffer *ob, const overlay_window *ow,const overlay_view *ov, int offset);
status_t gx00_release_bes();

/* I2C functions */
status_t i2c_sec_tv_adapter();

/*driver structures and enums*/
enum{BPP8=0,BPP15=1,BPP16=2,BPP24=3,BPP32DIR=4,BPP32=7};
enum{DS_CRTC1DAC_CRTC2MAVEN, DS_CRTC1MAVEN_CRTC2DAC, DS_CRTC1CON1_CRTC2CON2, DS_CRTC1CON2_CRTC2CON1};
