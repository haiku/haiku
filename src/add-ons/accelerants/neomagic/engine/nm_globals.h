extern int fd;
extern shared_info *si;
extern area_id shared_info_area;
extern area_id regs_area, regs2_area;
extern vuint32 *regs, *regs2;
extern display_mode *my_mode_list;
extern area_id my_mode_list_area;
extern int accelerantIsClone;

extern nm_get_set_pci nm_pci_access;
extern nm_in_out_isa nm_isa_access;
extern nm_bes_data nm_bes_access;
