/*general card functions*/
status_t mn_general_powerup();
status_t nm_set_cas_latency();
status_t nm_general_output_select();
uint8 nm_general_output_read();
status_t mn_general_dac_select(int);
status_t mn_general_wait_retrace();
status_t mn_general_validate_pic_size (display_mode *target, uint32 *bytes_per_row);
//status_t mn_general_bios_to_powergraphics();

/* apsed: logging macros */
#define MSG(args) do { /* if needed or si->settings with si NULL */ \
	nm_log args; \
} while (0)
#define LOG(level_bit, args) do { \
	uint32 mod = (si->settings.logmask &  0xfffffff0) & MODULE_BIT; \
	uint32 lev = (si->settings.logmask & ~0xfffffff0) & level_bit; \
	if (mod && lev) nm_log args; \
} while (0)

/*support functions*/
void delay(bigtime_t i);
void nm_log(char *format, ...);

/*card info functions*/
void set_specs(void);
void dump_specs(void);

/*DAC functions*/
status_t mn_dac_mode(int,float);
status_t mn_dac_palette(uint8*,uint8*,uint8*, uint16);
status_t mn_dac_pix_pll_find(display_mode target,float * result,uint8 *,uint8 *,uint8 *);
status_t mn_dac_set_pix_pll(display_mode target);

/*CRTC1 functions*/
status_t mn_crtc_validate_timing(
	uint16 *hd_e,uint16 *hs_s,uint16 *hs_e,uint16 *ht,
	uint16 *vd_e,uint16 *vs_s,uint16 *vs_e,uint16 *vt
);
status_t mn_crtc_set_timing(display_mode target, bool crt_only);
status_t mn_crtc_depth(int mode);
status_t mn_crtc_set_display_start(uint32 startadd,uint8 bpp); 
status_t mn_crtc_set_display_pitch();
status_t mn_crtc_center(display_mode target);

status_t mn_crtc_dpms(bool, bool, bool);
status_t mn_crtc_dpms_fetch(bool*, bool*, bool*);
status_t mn_crtc_mem_priority(uint8);

status_t mn_crtc_cursor_init(); /*Yes, cursor follows CRTC1 - not the DAC!*/
status_t mn_crtc_cursor_define(uint8*,uint8*);
status_t mn_crtc_cursor_position(uint16 x ,uint16 y);
status_t mn_crtc_cursor_show();
status_t mn_crtc_cursor_hide();

/*acceleration functions*/
status_t check_acc_capability(uint32 feature);
status_t mn_acc_init();
status_t mn_acc_rectangle(uint32 xs,uint32 xe,uint32 ys,uint32 yl,uint32 col);
status_t mn_acc_rectangle_invert(uint32 xs,uint32 xe,uint32 ys,uint32 yl,uint32 col);
status_t mn_acc_blit(uint16,uint16,uint16, uint16,uint16,uint16 );
status_t mn_acc_transparent_blit(uint16,uint16,uint16, uint16,uint16,uint16, uint32);
status_t mn_acc_video_blit(uint16 xs,uint16 ys,uint16 ws, uint16 hs,
	uint16 xd,uint16 yd,uint16 wd,uint16 hd);
status_t mn_acc_wait_idle();

/*backend scaler functions*/
status_t check_overlay_capability(uint32 feature);
status_t mn_configure_bes
	(const overlay_buffer *ob, const overlay_window *ow,const overlay_view *ov, int offset);
status_t mn_release_bes();

/*driver structures and enums*/
enum{BPP8=0,BPP15=1,BPP16=2,BPP24=3};
enum{DS_CRTC1DAC_CRTC2MAVEN, DS_CRTC1MAVEN_CRTC2DAC, DS_CRTC1CON1_CRTC2CON2, DS_CRTC1CON2_CRTC2CON1};
