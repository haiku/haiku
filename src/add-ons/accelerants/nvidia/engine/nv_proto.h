/*general card functions*/
status_t nv_general_powerup(void);
status_t nv_set_cas_latency(void);
status_t nv_general_output_select(bool cross);
status_t nv_general_dac_select(int);
status_t nv_general_wait_retrace(void);
status_t nv_general_validate_pic_size (display_mode *target, uint32 *bytes_per_row, bool *acc_mode);

/* apsed: logging macros */
#define MSG(args) do { /* if needed or si->settings with si NULL */ \
	nv_log args; \
} while (0)
#define LOG(level_bit, args) do { \
	uint32 mod = (si->settings.logmask &  0xfffffff0) & MODULE_BIT; \
	uint32 lev = (si->settings.logmask & ~0xfffffff0) & level_bit; \
	if (mod && lev) nv_log args; \
} while (0)

/*support functions*/
void delay(bigtime_t i);
void nv_log(char *format, ...);

/*i2c maven functions*/
int i2c_maven_read(unsigned char address);
void i2c_maven_write(unsigned char address, unsigned char data);
status_t i2c_init(void);
status_t i2c_maven_probe(void);

/*card info functions*/
status_t parse_pins(void);
void fake_pins(void);
void dump_pins(void);

/* DAC functions */
status_t nv_dac_mode(int,float);
status_t nv_dac_palette(uint8*,uint8*,uint8*);
status_t nv_dac_pix_pll_find(display_mode target,float * result,uint8 *,uint8 *,uint8 *, uint8);
status_t nv_dac_set_pix_pll(display_mode target);
status_t g400_dac_set_sys_pll(void);

/* DAC2 functions */
status_t nv_dac2_mode(int,float);
status_t nv_dac2_palette(uint8*,uint8*,uint8*);
status_t nv_dac2_pix_pll_find(display_mode target,float * result,uint8 *,uint8 *,uint8 *, uint8);
status_t nv_dac2_set_pix_pll(display_mode target);

/*MAVENTV functions*/
status_t g100_g400max_maventv_vid_pll_find(
	display_mode target, unsigned int * ht_new, unsigned int * ht_last_line,
	uint8 * m_result, uint8 * n_result, uint8 * p_result);
int maventv_init(display_mode target);

/* CRTC1 functions */
status_t nv_crtc_validate_timing(
	uint16 *hd_e,uint16 *hs_s,uint16 *hs_e,uint16 *ht,
	uint16 *vd_e,uint16 *vs_s,uint16 *vs_e,uint16 *vt
);
status_t nv_crtc_set_timing(display_mode target);
status_t nv_crtc_depth(int mode);
status_t nv_crtc_set_display_start(uint32 startadd,uint8 bpp); 
status_t nv_crtc_set_display_pitch(void);

status_t nv_crtc_dpms(bool, bool, bool);
status_t nv_crtc_dpms_fetch(bool*, bool*, bool*);
status_t nv_crtc_mem_priority(uint8);

status_t nv_crtc_cursor_init(void); /*Yes, cursor follows CRTC1 - not the DAC!*/
status_t nv_crtc_cursor_define(uint8*,uint8*);
status_t nv_crtc_cursor_position(uint16 x ,uint16 y);
status_t nv_crtc_cursor_show(void);
status_t nv_crtc_cursor_hide(void);

/* CRTC2 functions */
status_t nv_crtc2_validate_timing(
	uint16 *hd_e,uint16 *hs_s,uint16 *hs_e,uint16 *ht,
	uint16 *vd_e,uint16 *vs_s,uint16 *vs_e,uint16 *vt
);
status_t nv_crtc2_set_timing(display_mode target);
status_t nv_crtc2_depth(int mode);
status_t nv_crtc2_set_display_start(uint32 startadd,uint8 bpp); 
status_t nv_crtc2_set_display_pitch(void);

status_t nv_crtc2_dpms(bool, bool, bool);
status_t nv_crtc2_dpms_fetch(bool*, bool*, bool*);
status_t nv_crtc2_mem_priority(uint8);

status_t nv_crtc2_cursor_init(void); /*Yes, cursor follows CRTC1 - not the DAC!*/
status_t nv_crtc2_cursor_define(uint8*,uint8*);
status_t nv_crtc2_cursor_position(uint16 x ,uint16 y);
status_t nv_crtc2_cursor_show(void);
status_t nv_crtc2_cursor_hide(void);

/*acceleration functions*/
status_t check_acc_capability(uint32 feature);
status_t nv_acc_init(void);
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

/*backend scaler functions*/
status_t check_overlay_capability(uint32 feature);
status_t nv_bes_to_crtc(uint8 crtc);
status_t nv_bes_init(void);
status_t nv_configure_bes
	(const overlay_buffer *ob, const overlay_window *ow,const overlay_view *ov, int offset);
status_t nv_release_bes(void);

/* I2C functions */
status_t i2c_sec_tv_adapter(void);

/*driver structures and enums*/
enum{BPP8=0,BPP15=1,BPP16=2,BPP24=3,BPP32=4};
enum{DS_CRTC1DAC_CRTC2MAVEN, DS_CRTC1MAVEN_CRTC2DAC, DS_CRTC1CON1_CRTC2CON2, DS_CRTC1CON2_CRTC2CON1};
