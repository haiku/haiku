#if !defined(GLOBALDATA_H)
#define GLOBALDATA_H

#include "DriverInterface.h"
#include "regmach64.h"

extern int fd;
extern shared_info *si;
extern area_id shared_info_area;
extern vuint32 *regs;
extern area_id regs_area;
extern display_mode *atimach64_mode_list;
extern area_id atimach64_mode_list_area;
extern int accelerantIsClone;

#endif
