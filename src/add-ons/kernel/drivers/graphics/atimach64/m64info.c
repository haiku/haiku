/* All this code belongs to Linux Xfree86 Project with lots of modifications */
/* 
   Rene MacKinney <rene_@freenet.co.uk>

   7.May.99
*/

#include <KernelExport.h>
#include "DriverInterface.h"
#include "regmach64.h"
#include "string.h"

#if DEBUG > 0
#define ddprintf(a)	dprintf a
#else
#define	ddprintf(a)
#endif

status_t
GetATIInfo(si)
    shared_info *si;
{
   char                       signature[]    = " 761295520";
   char                       *bios_data = rommem(0x00);
   char                       bios_signature[10];
   unsigned short             *sbios_data = (unsigned short *)bios_data;
   int                        tmp,i,j;
   int                        ROM_Table_Offset;
   int                        Freq_Table_Ptr;
   int                        CDepth_Table_Ptr;
   int                        CTable_Size;

         
   if (si->rom == NULL)
      return B_ERROR;

   memcpy(bios_signature, rommem(0x30), 10);

   if (strncmp( signature, bios_signature, 10 )) {
         ddprintf(("Mach64 probe failed on BIOS signature\n"));
         return B_ERROR;
   }

   si->board_identifier[0]    = bios_data[ 0x40 ];
   si->board_identifier[1]    = bios_data[ 0x41 ];
   si->equipment_flags[0]     = bios_data[ 0x42 ];
   si->equipment_flags[1]     = bios_data[ 0x44 ];
   si->asic_identifier        = bios_data[ 0x43 ];
   si->bios_major             = bios_data[ 0x4c ];
   si->bios_minor             = bios_data[ 0x4d ];
   strncpy( si->bios_date, bios_data + 0x50, 20 );
   
   si->VGA_Wonder_Present     = bios_data[ 0x44 ] & 0x40;

   si->Mach64_Present = 1;     /* Test for Mach64 product */

   tmp = inw(SCRATCH_REG0);


   outw(SCRATCH_REG0, 0x55555555);
   if (inw(SCRATCH_REG0) != 0x55555555) {
      si->Mach64_Present = 0;
      ddprintf(("Mach64 probe failed on read 1 of SCRATCH_REG0 %x\n",
             SCRATCH_REG0));
   } else {
      outw(SCRATCH_REG0, 0xaaaaaaaa);
      if (inw(SCRATCH_REG0) != 0xaaaaaaaa) {
         si->Mach64_Present = 0;
          ddprintf(("Mach64 probe failed on read 2 of SCRATCH_REG0 %x\n",
                 SCRATCH_REG0));
      }
   }
   
   outw(SCRATCH_REG0, tmp);

   if (!si->Mach64_Present)
         return B_ERROR;

   tmp = inw(CONFIG_CHIP_ID);
   if (si->device_id != (tmp & CFG_CHIP_TYPE)) {
        ddprintf(("%x %x: Mach64 chipset mismatch", 
                si->device_id, tmp & CFG_CHIP_TYPE));
        return(B_ERROR);
   }

   ddprintf(("CONFIG_CHIP_ID reports: %x rev %d\n",
          si->device_id, si->revision));

   tmp = inw(CONFIG_STAT0);
   if (si->device_id == MACH64_GX_ID || si->device_id == MACH64_CX_ID) {
        si->Bus_Type = tmp & CFG_BUS_TYPE;
        si->Mem_Type = (tmp & CFG_MEM_TYPE) >> 3;
        si->DAC_Type = (tmp & CFG_INIT_DAC_TYPE) >> 9;
        si->DAC_SubType = (inb(SCRATCH_REG1+1) & 0xf0) | si->DAC_Type;
   } else {
        si->Mem_Type = tmp & CFG_MEM_TYPE_xT;
        si->DAC_Type = DAC_INTERNAL;
        si->DAC_SubType = DAC_INTERNAL;

        ddprintf(("CONFIG_STAT0 reports mem type %d\n", si->Mem_Type));
        ddprintf(("CONFIG_STAT0 is 0x%08x\n", tmp));
   }

  /* The copy of the registers starts at 0x7ffc00 or 0x3ffc00 depending on the
     memory on the card */

   tmp = inw(MEM_CNTL);
   if (si->device_id == MACH64_GX_ID ||
       si->device_id == MACH64_CX_ID ||
       si->device_id == MACH64_CT_ID ||
       si->device_id == MACH64_ET_ID ||
       ((si->device_id == MACH64_VT_ID || si->device_id == MACH64_GT_ID) &&
        !(si->revision & 0x07))) {
     switch (tmp & MEM_SIZE_ALIAS) {
     case MEM_SIZE_512K:
       si->mem_size = 512 * 1024;
       break;
     case MEM_SIZE_1M:
       si->mem_size = 1024 * 1024;
       break;
     case MEM_SIZE_2M:
       si->mem_size = 2*1024 * 1024;
       break;
     case MEM_SIZE_4M:
       si->mem_size = 4*1024 * 1024;
      break;
     case MEM_SIZE_6M:
       si->mem_size = 6*1024 * 1024;
       break;
     case MEM_SIZE_8M:
       si->mem_size = 8*1024 * 1024;
       break;
     }
     si->Mem_Size = tmp & MEM_SIZE_ALIAS;
   } else {
     switch (tmp & MEM_SIZE_ALIAS_GTB) {
     case MEM_SIZE_512K:
       si->mem_size = 512 * 1024;
       break;
     case MEM_SIZE_1M:
       si->mem_size = 1024 * 1024;
       break;
     case MEM_SIZE_2M_GTB:
       si->mem_size = 2*1024 * 1024;
       break;
     case MEM_SIZE_4M_GTB:
       si->mem_size = 4*1024 * 1024;
       break;
     case MEM_SIZE_6M_GTB:
       si->mem_size = 6*1024 * 1024;
       break;
     case MEM_SIZE_8M_GTB:
       si->mem_size = 8*1024 * 1024;
       break;
     case MEM_SIZE_16M_GTB:
       si->mem_size = 16*1024 * 1024;
       break;
     }
     si->Mem_Size = tmp & MEM_SIZE_ALIAS_GTB;
   }

   ddprintf(("Card Reports 0x%08x Memory\n", si->mem_size));

   ROM_Table_Offset = sbios_data[0x48 >> 1];
   Freq_Table_Ptr = sbios_data[(ROM_Table_Offset >> 1) + 8];
   si->Clock_Type = bios_data[Freq_Table_Ptr];

   si->MinFreq = sbios_data[(Freq_Table_Ptr >> 1) + 1];
   si->MaxFreq = sbios_data[(Freq_Table_Ptr >> 1) + 2];
   si->RefFreq = sbios_data[(Freq_Table_Ptr >> 1) + 4];
   si->RefDivider = sbios_data[(Freq_Table_Ptr >> 1) + 5];
   si->NAdj = sbios_data[(Freq_Table_Ptr >> 1) + 6];
   si->DRAMMemClk = sbios_data[(Freq_Table_Ptr >> 1) + 8];
   si->VRAMMemClk = sbios_data[(Freq_Table_Ptr >> 1) + 9];
   si->MemClk = bios_data[Freq_Table_Ptr + 22];
   si->CXClk = bios_data[Freq_Table_Ptr + 6];

   CDepth_Table_Ptr = sbios_data[(Freq_Table_Ptr >> 1) - 3];
   Freq_Table_Ptr = sbios_data[(Freq_Table_Ptr >> 1) - 1];

   for (i = 0; i < MACH64_NUM_CLOCKS; i++)
      si->Clocks[i] = sbios_data[(Freq_Table_Ptr >> 1) + i];

   si->MemCycle = bios_data[ROM_Table_Offset + 0];

   CTable_Size = bios_data[CDepth_Table_Ptr - 1];
   for (i = 0, j = 0;
        bios_data[CDepth_Table_Ptr + i] != 0;
        i += CTable_Size, j++) {
     si->Freq_Table[j].h_disp        = bios_data[CDepth_Table_Ptr + i];
     si->Freq_Table[j].dacmask       = bios_data[CDepth_Table_Ptr + i + 1];
     si->Freq_Table[j].ram_req       = bios_data[CDepth_Table_Ptr + i + 2];
     si->Freq_Table[j].max_dot_clock = bios_data[CDepth_Table_Ptr + i + 3];
     si->Freq_Table[j].color_depth   = bios_data[CDepth_Table_Ptr + i + 4];
     switch(si->Freq_Table[j].color_depth) {
     case 2: /* 8 Bit */
       si->pix_clk_max8 = max(si->pix_clk_max8, si->Freq_Table[j].max_dot_clock);
       break;
     case 4: /* 16 Bit */
       si->pix_clk_max16 = max(si->pix_clk_max16, si->Freq_Table[j].max_dot_clock);
       break;
     case 6: /* 8 Bit */
       si->pix_clk_max32 = max(si->pix_clk_max32, si->Freq_Table2[j].max_dot_clock);
       break;
     }
   }
   si->Freq_Table[j].h_disp = 0;

   if (bios_data[CDepth_Table_Ptr + i + 1] != 0) {
       CDepth_Table_Ptr += i + 2;
       CTable_Size = bios_data[CDepth_Table_Ptr - 1];
       for (i = 0, j = 0;
            bios_data[CDepth_Table_Ptr + i] != 0;
            i += CTable_Size, j++) {
           si->Freq_Table2[j].h_disp        = bios_data[CDepth_Table_Ptr + i];
           si->Freq_Table2[j].dacmask       = bios_data[CDepth_Table_Ptr + i + 
1];
           si->Freq_Table2[j].ram_req       = bios_data[CDepth_Table_Ptr + i + 
2];
           si->Freq_Table2[j].max_dot_clock = bios_data[CDepth_Table_Ptr + i + 
3];
           si->Freq_Table2[j].color_depth   = bios_data[CDepth_Table_Ptr + i + 
4];
           switch(si->Freq_Table[j].color_depth) {
	     case 2: /* 8 Bit */
	       si->pix_clk_max8 = max(si->pix_clk_max8, si->Freq_Table2[j].max_dot_clock);
	       break;
	     case 4: /* 16 Bit */
	       si->pix_clk_max16 = max(si->pix_clk_max16, si->Freq_Table2[j].max_dot_clock);
	       break;
	     case 6: /* 8 Bit */
	       si->pix_clk_max32 = max(si->pix_clk_max32, si->Freq_Table2[j].max_dot_clock);
	       break;
	   }
       }
       si->Freq_Table2[j].h_disp = 0;
   } else
       si->Freq_Table2[0].h_disp = 0;

   /* Hardwired   120000 for 8bpp and 80000 for the rest */

   ddprintf(("Clocks %ld %ld %ld\n",si->pix_clk_max32, 
             si->pix_clk_max16, si->pix_clk_max8));


   si->pix_clk_max32 = 80000;
   si->pix_clk_max16 = 80000;
   si->pix_clk_max8 = 120000;


   ddprintf(("Clocks %ld %ld %ld\n",si->pix_clk_max32, 
             si->pix_clk_max16, si->pix_clk_max8));


   return B_OK;
}
