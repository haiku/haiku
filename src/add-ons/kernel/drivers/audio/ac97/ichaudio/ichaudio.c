#include "lala/lala.h"
#include "ichaudio.h"

status_t ichaudio_attach(drv_t *drv, void *cookie);
status_t ichaudio_powerctl(drv_t *drv, void *cookie);
status_t ichaudio_detach(drv_t *drv, void *cookie);

id_table_t ichaudio_id_table[] = {
	{ 0x8086, 0x7195, -1, -1, -1, -1, -1, "Intel 82443MX AC97 audio" },
	{ 0x8086, 0x2415, -1, -1, -1, -1, -1, "Intel 82801AA (ICH) AC97 audio" },
	{ 0x8086, 0x2425, -1, -1, -1, -1, -1, "Intel 82801AB (ICH0) AC97 audio" },
	{ 0x8086, 0x2445, -1, -1, -1, -1, -1, "Intel 82801BA (ICH2) AC97 audio" },
	{ 0x8086, 0x2485, -1, -1, -1, -1, -1, "Intel 82801CA (ICH3) AC97 audio" },
	{ 0x8086, 0x24C5, -1, -1, -1, -1, -1, "Intel 82801DB (ICH4) AC97 audio", TYPE_ICH4 },
	{ 0x8086, 0x24D5, -1, -1, -1, -1, -1, "Intel 82801EB (ICH5)  AC97 audio", TYPE_ICH4 },
	{ 0x1039, 0x7012, -1, -1, -1, -1, -1, "SiS SI7012 AC97 audio", TYPE_SIS7012 },
	{ 0x10DE, 0x01B1, -1, -1, -1, -1, -1, "NVIDIA nForce (MCP)  AC97 audio" },
	{ 0x10DE, 0x006A, -1, -1, -1, -1, -1, "NVIDIA nForce 2 (MCP2)  AC97 audio" },
	{ 0x10DE, 0x00DA, -1, -1, -1, -1, -1, "NVIDIA nForce 3 (MCP3)  AC97 audio" },
	{ 0x1022, 0x764D, -1, -1, -1, -1, -1, "AMD AMD8111 AC97 audio" },
	{ 0x1022, 0x7445, -1, -1, -1, -1, -1, "AMD AMD768 AC97 audio" },
	{ 0x1103, 0x0004, -1, -1, -1, -1, -1, "Highpoint HPT372 RAID" },
	{ 0x1095, 0x3112, -1, -1, -1, -1, -1, "Silicon Image SATA Si3112" },
	{ 0x8086, 0x244b, -1, -1, -1, -1, -1, "Intel IDE Controller" },
	{ 0x8979, 0x6456, -1, -1, -1, -1, -1, "does not exist" },
	{ /* empty = end */}
};


driver_info_t driver_info = {
	ichaudio_id_table,
	"audio/lala/ichaudio",
	sizeof(ichaudio_cookie),
	ichaudio_attach,
	ichaudio_detach,
	ichaudio_powerctl,
};


status_t
ichaudio_attach(drv_t *drv, void *_cookie)
{
	ichaudio_cookie *cookie = (ichaudio_cookie *)_cookie;
	
	dprintf("ichaudio_attach\n");

	return B_OK;
}


status_t
ichaudio_detach(drv_t *drv, void *_cookie)
{
	ichaudio_cookie *cookie = (ichaudio_cookie *)_cookie;

	dprintf("ichaudio_detach\n");

	return B_OK;
}


status_t
ichaudio_powerctl(drv_t *drv, void *_cookie)
{
	ichaudio_cookie *cookie = (ichaudio_cookie *)_cookie;

	return B_OK;
}

