/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Heavily modified by Rob Judd <judd@ob-wan.com> with
** acknowledgements to Hale Landis <hlandis@ibm.net>
** who wrote the reference implementation.
** Distributed under the terms of the NewOS License.
*/

#include "ide_private.h"
#include <arch/cpu.h>
#include <OS.h>
#include <debug.h>
#include "ide_raw.h"
#include "partition.h"
#include <string.h>
#include <types.h>
#include <ktypes.h>
#include <errors.h>
#include <thread.h>

// ATA register bits

// command block
#define CB_DATA           0   // data reg              in/out pio_base_addr1+0
#define CB_ERR            1   // error reg             in     pio_base_addr1+1
#define CB_FR             1   // feature reg              out pio_base_addr1+1
#define CB_SC             2   // sector count reg      in/out pio_base_addr1+2
#define CB_SN             3   // sector number reg     in/out pio_base_addr1+3
                              // or block address 0-7
#define CB_CL             4   // cylinder low reg      in/out pio_base_addr1+4
                              // or block address 8-15
#define CB_CH             5   // cylinder high reg     in/out pio_base_addr1+5
                              // or block address 16-23
#define CB_DH             6   // drive/head reg        in/out pio_base_addr1+6
#define CB_STAT           7   // primary status reg    in     pio_base_addr1+7
#define CB_CMD            7   // command reg              out pio_base_addr1+7

// control block
#define CB_ASTAT          8   // alternate status reg  in     pio_base_addr2+6
#define CB_DC             8   // device control reg       out pio_base_addr2+6
#define CB_DA             9   // device address reg    in     pio_base_addr2+7

// error register
#define CB_ER_NDAM     0x01   // ATA address mark not found
#define CB_ER_NTK0     0x02   // ATA track 0 not found
#define CB_ER_ABRT     0x04   // ATA command aborted
#define CB_ER_MCR      0x08   // ATA media change request
#define CB_ER_IDNF     0x10   // ATA id not found
#define CB_ER_MC       0x20   // ATA media change
#define CB_ER_UNC      0x40   // ATA uncorrected error
#define CB_ER_BBK      0x80   // ATA bad block
#define CB_ER_ICRC     0x80   // ATA Ultra DMA bad CRC

// drive/head register bits 7-4
#define CB_DH_LBA      0x40   // LBA bit mask
#define CB_DH_DEV0     0xa0   // select device 0
#define CB_DH_DEV1     0xb0   // select device 1

#define	DRIVE_SUPPORT_LBA     0x20 // test mask for LBA support

// status register bits
#define CB_STAT_ERR    0x01   // error (ATA)
#define CB_STAT_CHK    0x01   // check (ATAPI)
#define CB_STAT_IDX    0x02   // index
#define CB_STAT_CORR   0x04   // corrected
#define CB_STAT_DRQ    0x08   // data request
#define CB_STAT_SKC    0x10   // seek complete
#define CB_STAT_SERV   0x10   // service
#define CB_STAT_DF     0x20   // device fault
#define CB_STAT_WFT    0x20   // write fault (old name)
#define CB_STAT_RDY    0x40   // ready
#define CB_STAT_BSY    0x80   // busy

// device control register bits
#define CB_DC_NIEN     0x02   // disable interrupts
#define CB_DC_SRST     0x04   // soft reset
#define CB_DC_HD15     0x08   // bit should always be set to one


// ATAPI commands

#define CB_ER_P_ILI    0x01   // ATAPI illegal length indication
#define CB_ER_P_EOM    0x02   // ATAPI end of media
#define CB_ER_P_ABRT   0x04   // ATAPI command abort
#define CB_ER_P_MCR    0x08   // ATAPI media change request
#define CB_ER_P_SNSKEY 0xf0   // ATAPI sense key mask

// ATAPI interrupt reason bits in the sector count register
#define CB_SC_P_CD     0x01   // ATAPI C/D
#define CB_SC_P_IO     0x02   // ATAPI I/O
#define CB_SC_P_REL    0x04   // ATAPI release
#define CB_SC_P_TAG    0xf8   // ATAPI tag (mask)

//**************************************************************

// The ATA command set

// Mandatory commands
#define CMD_EXECUTE_DRIVE_DIAGNOSTIC     0x90
#define CMD_FORMAT_TRACK                 0x50
#define CMD_INITIALIZE_DRIVE_PARAMETERS  0x91
#define CMD_READ_LONG                    0x22 // One sector inc. ECC, with retry
#define CMD_READ_LONG_ONCE               0x23 // One sector inc. ECC, sans retry
#define CMD_READ_SECTORS                 0x20
#define CMD_READ_SECTORS_ONCE            0x21
#define CMD_READ_VERIFY_SECTORS          0x40
#define CMD_READ_VERIFY_SECTORS_ONCE     0x41
#define CMD_RECALIBRATE                  0x10 // Actually 0x10 to 0x1F
#define CMD_SEEK                         0x70 // Actually 0x70 to 0x7F
#define CMD_WRITE_LONG                   0x32
#define CMD_WRITE_LONG_ONCE              0x33
#define CMD_WRITE_SECTORS                0x30
#define CMD_WRITE_SECTORS_ONCE           0x31

// Error codes for CMD_EXECUTE_DRIVE_DIAGNOSTICS
#define DIAG_NO_ERROR                    0x01
#define DIAG_FORMATTER                   0x02
#define DIAG_DATA_BUFFER                 0x03
#define DIAG_ECC_CIRCUITRY               0x04
#define DIAG_MICROPROCESSOR              0x05
#define DIAG_SLAVE_DRIVE_MASK            0x80

// Accoustic Management commands
#define CMD_SET_ACCOUSTIC_LEVEL			0x2A
#define CMD_GET_ACCOUSTIC_LEVEL			0xAB

// Command codes for CMD_FORMAT_TRACK
#define FMT_GOOD_SECTOR                  0x00
#define FMT_SUSPEND_REALLOC              0x20
#define FMT_REALLOC_SECTOR               0x40
#define FMT_MARK_SECTOR_DEFECTIVE        0x80

// Optional commands
#define CMD_ACK_MEDIA_CHANGE             0xDB
#define CMD_BOOT_POSTBOOT                0xDC
#define CMD_BOOT_PREBOOT                 0xDD
#define CMD_CFA_ERASE_SECTORS            0xC0
#define CMD_CFA_REQUEST_EXT_ERR_CODE     0x03
#define CMD_CFA_TRANSLATE_SECTOR         0x87
#define CMD_CFA_WRITE_MULTIPLE_WO_ERASE  0xCD
#define CMD_CFA_WRITE_SECTORS_WO_ERASE   0x38
#define CMD_CHECK_POWER_MODE             0x98
#define CMD_DEVICE_RESET                 0x08
#define CMD_DOOR_LOCK                    0xDE
#define CMD_DOOR_UNLOCK                  0xDF
#define CMD_FLUSH_CACHE                  0xE7 // CMD_REST
#define CMD_IDENTIFY_DEVICE              0xEC
#define CMD_IDENTIFY_DEVICE_PACKET       0xA1
#define CMD_IDLE                         0x97
#define CMD_IDLE_IMMEDIATE               0x95
#define CMD_NOP                          0x00
#define CMD_PACKET                       0xA0
#define CMD_READ_BUFFER                  0xE4
#define CMD_READ_DMA                     0xC8
#define CMD_READ_DMA_QUEUED              0xC7
#define CMD_READ_MULTIPLE                0xC4
#define CMD_RESTORE_DRIVE_STATE          0xEA
#define CMD_SET_FEATURES                 0xEF
#define CMD_SET_MULTIPLE_MODE            0xC6
#define CMD_SLEEP                        0x99
#define CMD_STANDBY                      0x96
#define CMD_STANDBY_IMMEDIATE            0x94
#define CMD_WRITE_BUFFER                 0xE8
#define CMD_WRITE_DMA                    0xCA
#define CMD_WRITE_DMA_ONCE               0xCB
#define CMD_WRITE_DMA_QUEUED             0xCC
#define CMD_WRITE_MULTIPLE               0xC5
#define CMD_WRITE_SAME                   0xE9
#define CMD_WRITE_VERIFY                 0x3C

// Connor Peripherals' variations
#define CONNOR_CHECK_POWER_MODE          0xE5
#define CONNOR_IDLE                      0xE3
#define CONNOR_IDLE_IMMEDIATE            0xE1
#define CONNOR_SLEEP                     0xE6
#define CONNOR_STANDBY                   0xE2
#define CONNOR_STANDBY_IMMEDIATE         0xE0

//**************************************************************


// Waste some time by reading the alternate status a few times.
// This gives the drive time to set BUSY in the status register on
// really fast systems.  If we don't do this, a slow drive on a fast
// system may not set BUSY fast enough and we would think it had
// completed the command when it really had not even started yet.
#define DELAY400NS  { pio_inbyte(CB_ASTAT); pio_inbyte(CB_ASTAT); \
                      pio_inbyte(CB_ASTAT); pio_inbyte(CB_ASTAT); }

// Standard ide base addresses. For pc-card (pcmcia) drives, use
// unused contiguous address block { 100H < (base1=base2) < 3F0H }
unsigned int pio_base0_addr1 = 0x1f0; // Command block, ide bus 0
unsigned int pio_base0_addr2 = 0x3f0; // Control block, ide bus 0
unsigned int pio_base1_addr1 = 0x170; // Command block, ide bus 1
unsigned int pio_base1_addr2 = 0x370; // Control block, ide bus 1

unsigned int pio_memory_seg = 0;
unsigned int pio_reg_addrs[10];
unsigned char pio_last_read[10];
unsigned char pio_last_write[10];


static uint8 pio_inbyte(uint16 port)
{
  return in8(pio_reg_addrs[port]);
}

static uint16 pio_inword(uint16 port)
{
  return in16(pio_reg_addrs[port]);
}

static void	pio_outbyte(uint16 port, uint8 data)
{
	out8(data, pio_reg_addrs[port]);
}

static void	pio_rep_inword(uint16 port, uint16 *addr, unsigned long count)
{
  __asm__ __volatile__ 
    (
     "rep ; insw" : "=D" (addr), "=c" (count) : "d" (pio_reg_addrs[port]),
     "0" (addr), "1" (count)
     );
}

static void	pio_rep_outword(uint16 port, uint16 *addr, unsigned long count)
{
  __asm__ __volatile__ 
    (
     "rep ; outsw" : "=S" (addr), "=c" (count) : "d" (pio_reg_addrs[port]),
     "0" (addr), "1" (count)
     );
}

static void	ide_reg_poll()
{
	while(1)
	{
	  if ((pio_inbyte(CB_ASTAT) & CB_STAT_BSY) == 0)  // If not busy
        break;
	}
}

static bool ide_wait_busy()
{
  int i;

  for(i=0; i<10000; i++)
	{
      if ((pio_inbyte(CB_ASTAT) & CB_STAT_BSY) == 0)
        return true;
	}
  return false;
}

static int ide_select_device(int bus, int device)
{
	uint8		status;
	int			i;
	ide_device	ide = devices[(bus*2) + device];

	// Test for a known, valid device
	if(ide.device_type == (NO_DEVICE | UNKNOWN_DEVICE))
      return NO_ERR;
	// See if we can get its attention
	if(ide_wait_busy() == false)
	  return ERR_TIMEOUT;
	// Select required device
	pio_outbyte(CB_DH, device ? CB_DH_DEV1 : CB_DH_DEV0);
	DELAY400NS;
	for(i=0; i<10000; i++)
      {
		// Read the device status
		status = pio_inbyte(CB_STAT);
		if (ide.device_type == ATA_DEVICE)
		  {
		    if ((status & (CB_STAT_BSY | CB_STAT_RDY | CB_STAT_SKC))
                    == (CB_STAT_RDY | CB_STAT_SKC))
			  return NO_ERR;
		  }
		else
		  {
			if ((status & CB_STAT_BSY) == 0)
			  return NO_ERR;
		  }
      }
  return ERR_TIMEOUT;
}

static void ide_delay(int bus, int device)
{
	ide_device	ide = devices[(bus*2) + device];

	if(ide.device_type == ATAPI_DEVICE)
	  thread_snooze(1000000);

	return;
}

static int reg_pio_data_in(int bus, int dev, int cmd, int fr, int sc,
					unsigned int cyl, int head, int sect, uint8 *output,
					unsigned int numSect, unsigned int multiCnt)
{
    unsigned char devHead;
    unsigned char devCtrl;
    unsigned char cylLow;
    unsigned char cylHigh;
    unsigned char status;
    uint16        *buffer = (uint16*)output;

//  dprintf("reg_pio_data_in: bus %d dev %d cmd %d fr %d sc %d cyl %d head %d sect %d numSect %d multiCnt %d\n",
//  	bus, dev, cmd, fr, sc, cyl, head, sect, numSect, multiCnt);

    devCtrl = CB_DC_HD15 | CB_DC_NIEN;
    devHead = dev ? CB_DH_DEV1 : CB_DH_DEV0;
    devHead = devHead | (head & 0x4f);
    cylLow = cyl & 0x00ff;
    cylHigh = (cyl & 0xff00) >> 8;
    // these commands transfer only 1 sector
    if(cmd == (CMD_IDENTIFY_DEVICE | CMD_IDENTIFY_DEVICE_PACKET | CMD_READ_BUFFER))
      numSect = 1;
    // multiCnt = 1 unless CMD_READ_MULTIPLE true
    if(cmd != CMD_READ_MULTIPLE || !multiCnt)
      multiCnt = 1;
    // select the drive
    if(ide_select_device(bus, dev) == ERR_TIMEOUT)
      return ERR_TIMEOUT;
    // set up the registers
    pio_outbyte(CB_DC, devCtrl);
    pio_outbyte(CB_FR, fr);
    pio_outbyte(CB_SC, sc);
    pio_outbyte(CB_SN, sect);
    pio_outbyte(CB_CL, cylLow);
    pio_outbyte(CB_CH, cylHigh);
    pio_outbyte(CB_DH, devHead);
    // Start the command. The drive should immediately set BUSY status.
    pio_outbyte(CB_CMD, cmd);
    DELAY400NS;
    while(1)
    {
      ide_delay(bus, dev);
      // ensure drive isn't still busy
      ide_reg_poll();
      // check status once per read
      status = pio_inbyte(CB_STAT);
      if((numSect < 1) && (status & CB_STAT_DRQ))
        return ERR_BUFFER_NOT_EMPTY;
      if (numSect < 1)
        break;
      if((status & (CB_STAT_BSY | CB_STAT_DRQ)) == CB_STAT_DRQ)
      {
        unsigned int wordCnt = multiCnt > numSect ? numSect : multiCnt;
        wordCnt = wordCnt * 256;
        pio_rep_inword(CB_DATA, buffer, wordCnt);
        DELAY400NS;    
        numSect = numSect - multiCnt;
        buffer += wordCnt;
      }
      // catch all possible fault conditions
      if(status & CB_STAT_BSY)
        return ERR_DISK_BUSY;
      if(status & CB_STAT_DF)
        return ERR_DEVICE_FAULT;
      if(status & CB_STAT_ERR)
        return ERR_HARDWARE_ERROR;
      if((status & CB_STAT_DRQ) == 0)
        return ERR_DRQ_NOT_SET;
    }
    return NO_ERR;
}

static int reg_pio_data_out( int bus, int dev, int cmd, int fr, int sc,
			     unsigned int cyl, int head, int sect, const uint8 *output,
			     unsigned int numSect, unsigned int multiCnt )
{
    unsigned char devHead;
    unsigned char devCtrl;
    unsigned char cylLow;
    unsigned char cylHigh;
    unsigned char status;
    uint16		  *buffer = (uint16*)output;

    devCtrl = CB_DC_HD15 | CB_DC_NIEN;
    devHead = dev ? CB_DH_DEV1 : CB_DH_DEV0;
    devHead = devHead | (head & 0x4f);
    cylLow  = cyl & 0x00ff;
    cylHigh = (cyl & 0xff00) >> 8;
    if (cmd == CMD_WRITE_BUFFER)
      numSect = 1;
    // only Write Multiple and CFA Write Multiple W/O Erase uses multCnt
    if ((cmd != CMD_WRITE_MULTIPLE) && (cmd != CMD_CFA_WRITE_MULTIPLE_WO_ERASE))
      multiCnt = 1;
    // select the drive
    if (ide_select_device(bus, dev) != NO_ERR)
      return ERR_TIMEOUT;
    // set up the registers
    pio_outbyte(CB_DC, devCtrl);
    pio_outbyte(CB_FR, fr);
    pio_outbyte(CB_SC, sc);
    pio_outbyte(CB_SN, sect);
    pio_outbyte(CB_CL, cylLow);
    pio_outbyte(CB_CH, cylHigh);
    pio_outbyte(CB_DH, devHead);
    // Start the command. The drive should immediately set BUSY status.
    pio_outbyte(CB_CMD, cmd);
    DELAY400NS;
    if (ide_wait_busy() == false)
      return ERR_TIMEOUT;
    status = pio_inbyte(CB_STAT);
    while (1)
    {
      if ((status & (CB_STAT_BSY | CB_STAT_DRQ)) == CB_STAT_DRQ)
      {
        unsigned int wordCnt = multiCnt > numSect ? numSect : multiCnt;
        wordCnt = wordCnt * 256;
        pio_rep_outword(CB_DATA, buffer, wordCnt);
        DELAY400NS;
        numSect = numSect - multiCnt;
        buffer += wordCnt;
      }
      // check all possible fault conditions
      if(status & CB_STAT_BSY)
        return ERR_DISK_BUSY;
      if(status & CB_STAT_DF)
        return ERR_DEVICE_FAULT;
      if(status & CB_STAT_ERR)
        return ERR_HARDWARE_ERROR;
      if ((status & CB_STAT_DRQ) == 0)
        return ERR_DRQ_NOT_SET;
      ide_delay(bus, dev);
      // ensure drive isn't still busy
      ide_reg_poll();
      if(numSect < 1 && status & (CB_STAT_BSY | CB_STAT_DF | CB_STAT_ERR))
        {
          dprintf("status = 0x%x\n", status);
          return ERR_BUFFER_NOT_EMPTY;
        }
    }
    return NO_ERR;
}

static void ide_btochs(uint32 block, ide_device *dev, int *cylinder, int *head, int *sect)
{
  *sect = (block % dev->hardware_device.sectors) + 1;
  block /= dev->hardware_device.sectors;
  *head = (block % dev->hardware_device.heads) | (dev->device ? 1 : 0);
  block /= dev->hardware_device.heads;
  *cylinder = block & 0xFFFF;
//  dprintf("ide_btochs: block %d -> cyl %d head %d sect %d\n", block, *cylinder, *head, *sect);
}

static void ide_btolba(uint32 block, ide_device *dev, int *cylinder, int *head, int *sect)
{
  *sect = block & 0xFF;
  *cylinder = (block >> 8) & 0xFFFF;
  *head = ((block >> 24) & 0xF) | (dev->device ? 1: 0) | CB_DH_LBA;
//  dprintf("ide_btolba: block %d -> cyl %d head %d sect %d\n", block, *cylinder, *head, *sect);
}

int	ide_read_block(ide_device *device, char *data, uint32 block, uint8 numSectors)
{
  int cyl, head, sect;

  if(device->lba_supported)
    ide_btolba(block, device, &cyl, &head, &sect);
  else
    ide_btochs(block, device, &cyl, &head, &sect);
    
  return reg_pio_data_in(device->bus, device->device, CMD_READ_SECTORS,
			          0, numSectors, cyl, head, sect, data, numSectors, 2);
}

int	ide_write_block(ide_device *device, const char *data, uint32 block, uint8 numSectors)
{
  int cyl, head, sect;

  if(device->lba_supported)
    ide_btolba(block, device, &cyl, &head, &sect);
  else
    ide_btochs(block, device, &cyl, &head, &sect);
    
  return reg_pio_data_out(device->bus, device->device, CMD_WRITE_SECTORS,
			          0, numSectors, cyl, head, sect, data, numSectors, 2);
}

void ide_string_conv (char *str, int len)
{
  unsigned int i;
  int	       j;

  for (i=0; i<len/sizeof(unsigned short); i++)
  {
    char c     = str[i*2+1];
    str[i*2+1] = str[i*2];
    str[i*2]   = c;
  }

  str[len - 1] = 0;
  for (j=len-1; j>=0 && str[j]==' '; j--)
    str[j] = 0;
}

// ide_reset() - execute a software reset
bool ide_reset (int bus, int device)
{
  unsigned char devCtrl = CB_DC_HD15 | CB_DC_NIEN;
	
  // Set and then reset the soft reset bit in the Device
  // Control register.  This causes device 0 be selected
  pio_outbyte(CB_DC, devCtrl | CB_DC_SRST);
  DELAY400NS;
  pio_outbyte(CB_DC, devCtrl);
  DELAY400NS;

  return (ide_wait_busy() ? true : false);
}

bool ide_identify_device(int bus, int device)
{
	ide_device* ide = &devices[(bus*2) + device];
	uint8* buffer;
	int rc;

	// Store specs for device		
	ide->bus         = bus;
	ide->device      = device;

	// Do an IDENTIFY DEVICE command
	buffer = (uint8*)&ide->hardware_device;
	rc = reg_pio_data_in(bus, device, CMD_IDENTIFY_DEVICE,
						1, 0, 0, 0, 0, buffer, 1, 0);

	if (rc == NO_ERR) {	
		// If command was ok, lets assume ATA device
		ide->device_type = ATA_DEVICE;
		
		// Convert the model string to ASCIIZ
		ide_string_conv(ide->hardware_device.model, 40);
		
		// Get copy over interesting data
		ide->sector_count = ide->hardware_device.cyls * ide->hardware_device.heads
		* ide->hardware_device.sectors;
		ide->bytes_per_sector = 512;
		ide->lba_supported = ide->hardware_device.capabilities & DRIVE_SUPPORT_LBA;
		ide->start_block = 0;
		ide->end_block = ide->sector_count + ide->start_block;
		
		// Give some debugging output to show what was found
		dprintf ("ide: disk at bus %d, device %d %s\n", bus, device, ide->hardware_device.model);
		dprintf ("ide/%d/%d: %dMB; %d cyl, %d head, %d sec, %d bytes/sec  (LBA=%d)\n",
		bus, device, ide->sector_count * ide->bytes_per_sector / (1024*1024),
		ide->hardware_device.cyls, ide->hardware_device.heads,
		ide->hardware_device.sectors, ide->bytes_per_sector, ide->lba_supported );
	} else {
		// Something went wrong, let's forget about this device
		ide->device_type = NO_DEVICE;
	}

	return (rc == NO_ERROR) ? true : false;
}

// Set the pio base addresses
void ide_raw_init(int base1, int base2)
{
  unsigned int pio_base_addr1 = base1;
  unsigned int pio_base_addr2 = base2;

  pio_reg_addrs[CB_DATA] = pio_base_addr1 + 0;  // 0
  pio_reg_addrs[CB_FR  ] = pio_base_addr1 + 1;  // 1
  pio_reg_addrs[CB_SC  ] = pio_base_addr1 + 2;  // 2
  pio_reg_addrs[CB_SN  ] = pio_base_addr1 + 3;  // 3
  pio_reg_addrs[CB_CL  ] = pio_base_addr1 + 4;  // 4
  pio_reg_addrs[CB_CH  ] = pio_base_addr1 + 5;  // 5
  pio_reg_addrs[CB_DH  ] = pio_base_addr1 + 6;  // 6
  pio_reg_addrs[CB_CMD ] = pio_base_addr1 + 7;  // 7
  pio_reg_addrs[CB_DC  ] = pio_base_addr2 + 6;  // 8
  pio_reg_addrs[CB_DA  ] = pio_base_addr2 + 7;  // 9
}

static char getHexChar(uint8 value)
{
  if(value < 10)
    return value + '0';
  return 'A' + (value - 10);
}

static void dumpHexLine(uint8 *buffer, int numberPerLine)
{
  uint8	*copy = buffer;
  int	i;

  for(i=0; i<numberPerLine; i++)
    {
      uint8	value1 = getHexChar(((*copy) >> 4));
      uint8	value2 = getHexChar(((*copy) & 0xF));
      dprintf("%c%c ", value1, value2);
      copy++;
    }
  copy = buffer;
  for(i=0; i<numberPerLine; i++)
    {
      if(*copy >= ' ' && *copy <= 'Z')
		dprintf("%c", *copy);
      else
		dprintf(".");
      copy++;
    }
  dprintf("\n");
}

static void dumpHexBuffer(uint8 *buffer, int size)
{
  int	numberPerLine = 8;
  int	numberOfLines = size / numberPerLine;
  int	i;

  for(i=0; i<numberOfLines; i++)
    {
      dprintf("%d ", i*numberPerLine);
      dumpHexLine(buffer, numberPerLine);
      buffer += numberPerLine;
    }
}

static bool ide_get_partition_info(ide_device *device, tPartition *partition, uint32 position)
{
	char buffer[512];
	uint8* partitionBuffer = buffer;		
	
	// Try to read partition table
	if (ide_read_block(device, buffer, position, 1) != 0) {
		dprintf("unable to read partition table\n");
		return false;
	}
	
	// Check partition table signature
	if (partitionBuffer[PART_MAGIC_OFFSET] != PARTITION_MAGIC1 ||
		partitionBuffer[PART_MAGIC_OFFSET+1] != PARTITION_MAGIC2) {
		dprintf("partition table magic is incorrect\n");
		return false;
	}
	
	memcpy(partition, partitionBuffer + PARTITION_OFFSET, sizeof(tPartition) * NUM_PARTITIONS);
	
	return true;
}

bool ide_get_partitions(ide_device *device)
{
  int		i;

  memset(&device->partitions, 0, sizeof(tPartition) * 2 * NUM_PARTITIONS);
  if(ide_get_partition_info(device, device->partitions, 0) == false)
    return false;
  
  dprintf("Primary Partition Table\n");
  for (i = 0; i < NUM_PARTITIONS; i++)
    {
      dprintf("  %d: flags:%x type:%x start:%d:%d:%d end:%d:%d:%d stblk:%d count:%d\n", 
	      i, 
	  device->partitions[i].boot_flags,
	  device->partitions[i].partition_type,
	  device->partitions[i].starting_head,
	  device->partitions[i].starting_sector,
	  device->partitions[i].starting_cylinder,
	  device->partitions[i].ending_head,
	  device->partitions[i].ending_sector,
	  device->partitions[i].ending_cylinder,
	  device->partitions[i].starting_block,
	  device->partitions[i].sector_count);
    }
  if(device->partitions[1].partition_type == PTDosExtended)
    {
      int extOffset = device->partitions[1].starting_block;
      if(ide_get_partition_info(device, &device->partitions[4], extOffset) == false)
        return false;
      dprintf("Extended Partition Table\n");
      for (i=4; i<4+NUM_PARTITIONS; i++)
        {
          device->partitions[i].starting_block += extOffset;
          dprintf("  %d: flags:%x type:%x start:%d:%d:%d end:%d:%d:%d stblk:%d count:%d\n",
		  i, 
		  device->partitions[i].boot_flags,
		  device->partitions[i].partition_type,
		  device->partitions[i].starting_head,
		  device->partitions[i].starting_sector,
		  device->partitions[i].starting_cylinder,
		  device->partitions[i].ending_head,
		  device->partitions[i].ending_sector,
		  device->partitions[i].ending_cylinder,
		  device->partitions[i].starting_block,
		  device->partitions[i].sector_count);
        }
    }
  return true;
}


// These two functions are called from ide_ioctl in ide.c. They are far from
// tested, and mostly just a C conversion of a DEBUG script presented on
// http://www.satwerk.de/ufd552.htm
// This should be tweaked some more before actual release!

int ide_get_accoustic(ide_device *device, int8* level_ptr)
{
	return ERR_UNIMPLEMENTED;
}

int ide_set_accoustic(ide_device *device, int8 level)
{
	pio_outbyte(CB_DH, (device->device == 1) ? CB_DH_DEV1 : CB_DH_DEV0);
	pio_outbyte(CB_CMD, CMD_SET_ACCOUSTIC_LEVEL);	
	pio_outbyte(CB_SC, level);
	pio_outbyte(CB_STAT, 0xEF);

	return NO_ERROR;
}
