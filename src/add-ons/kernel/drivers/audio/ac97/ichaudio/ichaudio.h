#include <SupportDefs.h>
#include "hardware.h"
#include "lala/lala.h"

typedef struct
{
	uint32				regbase;
	ich_bd *			bd[ICH_BD_COUNT];
	void *				buffer[ICH_BD_COUNT];

	area_id				bd_area;
	area_id				buffer_area;

	volatile int		lastindex;
	volatile int64		processed_samples;

} ichaudio_stream;


typedef struct
{
	uint32	irq;
	uint32	nambar;
	uint32	nabmbar;
	area_id area_mmbar; // ich4
	area_id area_mbbar; // ich4
	void *	mmbar; // ich4
	void *	mbbar; // ich4

	uint32 codecoffset;
	uint32 input_rate;
	uint32 output_rate;
	
	ichaudio_stream stream[2];

	pci_module_info *	pci;
	uint32				flags;

} ichaudio_cookie;



#define TYPE_ICH4			0x01
#define TYPE_SIS7012		0x02

/* The SIS7012 chipset has SR and PICB registers swapped when compared to Intel */
#define	GET_REG_X_PICB(cookie)		(((cookie)->flags & TYPE_SIS7012) ? _ICH_REG_X_SR : _ICH_REG_X_PICB)
#define	GET_REG_X_SR(cookie)		(((cookie)->flags & TYPE_SIS7012) ? _ICH_REG_X_PICB : _ICH_REG_X_SR)
/* Each 16 bit sample is counted as 1 in SIS7012 chipsets, 2 in all others */
#define GET_HW_SAMPLE_SIZE(cookie)	(((cookie)->flags & TYPE_SIS7012) ? 1 : 2)

