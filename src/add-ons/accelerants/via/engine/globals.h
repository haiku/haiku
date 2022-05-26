extern int fd;
extern shared_info *si;
extern area_id shared_info_area;
extern area_id regs_area;
extern vuint32 *regs;
extern display_mode *my_mode_list;
extern area_id my_mode_list_area;
extern int accelerantIsClone;

extern eng_get_set_pci eng_pci_access;
extern eng_in_out_isa eng_isa_access;


typedef status_t (*crtc_validate_timing)(uint16*, uint16*, uint16*, uint16*, uint16*, uint16*, uint16*, uint16*);
typedef status_t (*crtc_set_timing)(display_mode);
typedef status_t (*crtc_depth)(int);
typedef status_t (*crtc_dpms)(bool, bool, bool);
typedef status_t (*crtc_dpms_fetch)(bool*, bool*, bool*);
typedef status_t (*crtc_set_display_pitch)(void);
typedef status_t (*crtc_set_display_start)(uint32, uint8);
typedef status_t (*crtc_cursor_init)(void);
typedef status_t (*crtc_cursor_show)(void);
typedef status_t (*crtc_cursor_hide)(void);
typedef status_t (*crtc_cursor_define)(uint8*, uint8*);
typedef status_t (*crtc_cursor_position)(uint16, uint16);

typedef status_t (*dac_mode)(int, float);
typedef status_t (*dac_palette)(uint8[256], uint8[256], uint8[256]);
typedef status_t (*dac_set_pix_pll)(display_mode);
typedef status_t (*dac_pix_pll_find)(display_mode, float*, uint8*, uint8*, uint8*, uint8);

extern crtc_validate_timing 	head1_validate_timing;
extern crtc_set_timing 		head1_set_timing;
extern crtc_depth				head1_depth;
extern crtc_dpms				head1_dpms;
extern crtc_dpms_fetch			head1_dpms_fetch;
extern crtc_set_display_pitch	head1_set_display_pitch;
extern crtc_set_display_start	head1_set_display_start;
extern crtc_cursor_init		head1_cursor_init;
extern crtc_cursor_show		head1_cursor_show;
extern crtc_cursor_hide		head1_cursor_hide;
extern crtc_cursor_define		head1_cursor_define;
extern crtc_cursor_position	head1_cursor_position;

extern crtc_validate_timing	head2_validate_timing;
extern crtc_set_timing			head2_set_timing;
extern crtc_depth				head2_depth;
extern crtc_dpms				head2_dpms;
extern crtc_dpms_fetch			head2_dpms_fetch;
extern crtc_set_display_pitch	head2_set_display_pitch;
extern crtc_set_display_start	head2_set_display_start;
extern crtc_cursor_init		head2_cursor_init;
extern crtc_cursor_show		head2_cursor_show;
extern crtc_cursor_hide		head2_cursor_hide;
extern crtc_cursor_define		head2_cursor_define;
extern crtc_cursor_position	head2_cursor_position;

extern dac_mode				head1_mode;
extern dac_palette				head1_palette;
extern dac_set_pix_pll			head1_set_pix_pll;
extern dac_pix_pll_find		head1_pix_pll_find;

extern dac_mode				head2_mode;
extern dac_palette				head2_palette;
extern dac_set_pix_pll			head2_set_pix_pll;
extern dac_pix_pll_find		head2_pix_pll_find;
