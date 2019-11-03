#ifndef KEYBOARD_H
#define KEYBOARD_H


#include <SupportDefs.h>


union key {
	uint32 d0;
	struct {
		uint8 modifiers; // not always present!
		uint8 bios;	// scan code
		uint8 dummy;
		uint8 ascii;
	} code;
};

#define BIOS_KEY_UP 		0x48
#define BIOS_KEY_DOWN		0x50
#define BIOS_KEY_LEFT		0x4b
#define BIOS_KEY_RIGHT		0x4d
// XXX: FIXME
#define BIOS_KEY_HOME		0x47
#define BIOS_KEY_END		0x4f
#define BIOS_KEY_PAGE_UP	0x49
#define BIOS_KEY_PAGE_DOWN	0x51

#ifdef __cplusplus
extern "C" {
#endif

extern void clear_key_buffer(void);
extern union key wait_for_key(void);
extern uint32 check_for_boot_keys(void);

#ifdef __cplusplus
}
#endif

#endif	/* KEYBOARD_H */
