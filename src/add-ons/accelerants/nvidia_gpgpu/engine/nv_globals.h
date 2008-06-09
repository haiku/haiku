extern int fd;
extern shared_info *si;
extern area_id shared_info_area;
extern area_id dma_cmd_buf_area;
extern area_id regs_area;
extern vuint32 *regs;
extern display_mode *my_mode_list;
extern area_id my_mode_list_area;
extern int accelerantIsClone;

extern nv_get_set_pci nv_pci_access;
extern nv_in_out_isa nv_isa_access;

typedef status_t (*crtc_interrupt_enable)(bool);
typedef status_t (*crtc_update_fifo)(void);
typedef status_t (*crtc_validate_timing)(uint16*, uint16*, uint16*, uint16*, uint16*, uint16*, uint16*, uint16*);
typedef status_t (*crtc_set_timing)(display_mode);
typedef status_t (*crtc_depth)(int);
typedef status_t (*crtc_dpms)(bool, bool, bool, bool);
typedef status_t (*crtc_set_display_pitch)(void);
typedef status_t (*crtc_set_display_start)(uint32, uint8);
typedef status_t (*crtc_cursor_init)(void);
typedef status_t (*crtc_cursor_show)(void);
typedef status_t (*crtc_cursor_hide)(void);
typedef status_t (*crtc_cursor_define)(uint8*, uint8*);
typedef status_t (*crtc_cursor_position)(uint16, uint16);
typedef status_t (*crtc_stop_tvout)(void);
typedef status_t (*crtc_start_tvout)(void);

typedef status_t (*dac_mode)(int, float);
typedef status_t (*dac_palette)(uint8[256], uint8[256], uint8[256]);
typedef status_t (*dac_set_pix_pll)(display_mode);
typedef status_t (*dac_pix_pll_find)(display_mode, float*, uint8*, uint8*, uint8*, uint8);

crtc_interrupt_enable	head1_interrupt_enable;
crtc_update_fifo		head1_update_fifo;
crtc_validate_timing 	head1_validate_timing;
crtc_set_timing 		head1_set_timing;
crtc_depth				head1_depth;
crtc_dpms				head1_dpms;
crtc_set_display_pitch	head1_set_display_pitch;
crtc_set_display_start	head1_set_display_start;
crtc_cursor_init		head1_cursor_init;
crtc_cursor_show		head1_cursor_show;
crtc_cursor_hide		head1_cursor_hide;
crtc_cursor_define		head1_cursor_define;
crtc_cursor_position	head1_cursor_position;
crtc_stop_tvout			head1_stop_tvout;
crtc_start_tvout		head1_start_tvout;

crtc_interrupt_enable	head2_interrupt_enable;
crtc_update_fifo		head2_update_fifo;
crtc_validate_timing	head2_validate_timing;
crtc_set_timing			head2_set_timing;
crtc_depth				head2_depth;
crtc_dpms				head2_dpms;
crtc_set_display_pitch	head2_set_display_pitch;
crtc_set_display_start	head2_set_display_start;
crtc_cursor_init		head2_cursor_init;
crtc_cursor_show		head2_cursor_show;
crtc_cursor_hide		head2_cursor_hide;
crtc_cursor_define		head2_cursor_define;
crtc_cursor_position	head2_cursor_position;
crtc_stop_tvout			head2_stop_tvout;
crtc_start_tvout		head2_start_tvout;

dac_mode				head1_mode;
dac_palette				head1_palette;
dac_set_pix_pll			head1_set_pix_pll;
dac_pix_pll_find		head1_pix_pll_find;

dac_mode				head2_mode;
dac_palette				head2_palette;
dac_set_pix_pll			head2_set_pix_pll;
dac_pix_pll_find		head2_pix_pll_find;
