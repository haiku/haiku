/*
 * Copyright 2004-2009 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors (in chronological order):
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 *      Marcus Overhagen <marcus@overhagen.de>
 */

/*! PS/2 bus manager */


#include <string.h>

#include "ps2_common.h"
#include "ps2_service.h"
#include "ps2_dev.h"


//#define TRACE_PS2_COMMON
#ifdef TRACE_PS2_COMMON
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif


isa_module_info *gIsa = NULL;
bool gActiveMultiplexingEnabled = false;
sem_id gControllerSem;

static int32 sIgnoreInterrupts = 0;


uint8
ps2_read_ctrl(void)
{
	return gIsa->read_io_8(PS2_PORT_CTRL);
}


uint8
ps2_read_data(void)
{
	return gIsa->read_io_8(PS2_PORT_DATA);
}


void
ps2_write_ctrl(uint8 ctrl)
{
	TRACE("ps2: ps2_write_ctrl 0x%02x\n", ctrl);

	gIsa->write_io_8(PS2_PORT_CTRL, ctrl);
}


void
ps2_write_data(uint8 data)
{
	TRACE("ps2: ps2_write_data 0x%02x\n", data);

	gIsa->write_io_8(PS2_PORT_DATA, data);
}


status_t
ps2_wait_read(void)
{
	int i;
	for (i = 0; i < PS2_CTRL_WAIT_TIMEOUT / 50; i++) {
		if (ps2_read_ctrl() & PS2_STATUS_OUTPUT_BUFFER_FULL)
			return B_OK;
		snooze(50);
	}
	return B_ERROR;
}


status_t
ps2_wait_write(void)
{
	int i;
	for (i = 0; i < PS2_CTRL_WAIT_TIMEOUT / 50; i++) {
		if (!(ps2_read_ctrl() & PS2_STATUS_INPUT_BUFFER_FULL))
			return B_OK;
		snooze(50);
	}
	return B_ERROR;
}


//	#pragma mark -


void
ps2_flush(void)
{
	int i;

	acquire_sem(gControllerSem);
	atomic_add(&sIgnoreInterrupts, 1);

	for (i = 0; i < 64; i++) {
		uint8 ctrl;
		uint8 data;
		ctrl = ps2_read_ctrl();
		if (!(ctrl & PS2_STATUS_OUTPUT_BUFFER_FULL))
			break;
		data = ps2_read_data();
		TRACE("ps2: ps2_flush: ctrl 0x%02x, data 0x%02x (%s)\n", ctrl, data, (ctrl & PS2_STATUS_AUX_DATA) ? "aux" : "keyb");
		snooze(100);
	}

	atomic_add(&sIgnoreInterrupts, -1);
	release_sem(gControllerSem);
}


static status_t
ps2_selftest()
{
	status_t res;
	uint8 in;
	res = ps2_command(PS2_CTRL_SELF_TEST, NULL, 0, &in, 1);
	if (res != B_OK || in != 0x55) {
		INFO("ps2: controller self test failed, status 0x%08" B_PRIx32 ", data "
			"0x%02x\n", res, in);
		return B_ERROR;
	}
	return B_OK;
}


static status_t
ps2_setup_command_byte(bool interruptsEnabled)
{
	status_t res;
	uint8 cmdbyte;

	res = ps2_command(PS2_CTRL_READ_CMD, NULL, 0, &cmdbyte, 1);
	TRACE("ps2: get command byte: res 0x%08" B_PRIx32 ", cmdbyte 0x%02x\n",
		res, cmdbyte);
	if (res != B_OK)
		cmdbyte = 0x47;

	cmdbyte |= PS2_BITS_TRANSLATE_SCANCODES;
	cmdbyte &= ~(PS2_BITS_KEYBOARD_DISABLED | PS2_BITS_MOUSE_DISABLED);

	if (interruptsEnabled)
		cmdbyte |= PS2_BITS_KEYBOARD_INTERRUPT | PS2_BITS_AUX_INTERRUPT;
	else
		cmdbyte &= ~(PS2_BITS_KEYBOARD_INTERRUPT | PS2_BITS_AUX_INTERRUPT);

	res = ps2_command(PS2_CTRL_WRITE_CMD, &cmdbyte, 1, NULL, 0);
	TRACE("ps2: set command byte: res 0x%08" B_PRIx32 ", cmdbyte 0x%02x\n",
		res, cmdbyte);

	return res;
}


static status_t
ps2_setup_active_multiplexing(bool *enabled)
{
	status_t res;
	uint8 in, out;

	out = 0xf0;
	res = ps2_command(0xd3, &out, 1, &in, 1);
	if (res)
		goto fail;
	// Step 1, if controller is good, in does match out.
	// This test failes with MS Virtual PC.
	if (in != out)
		goto no_support;

	out = 0x56;
	res = ps2_command(0xd3, &out, 1, &in, 1);
	if (res)
		goto fail;
	// Step 2, if controller is good, in does match out.
	if (in != out)
		goto no_support;

	out = 0xa4;
	res = ps2_command(0xd3, &out, 1, &in, 1);
	if (res)
		goto fail;
	// Step 3, if the controller doesn't support active multiplexing,
	// then in data does match out data (0xa4), else it's version number.
	if (in == out)
		goto no_support;

	// With some broken USB legacy emulation, it's 0xac, and with
	// MS Virtual PC, it's 0xa6. Since current active multiplexing
	// specification version is 1.1 (0x11), we validate the data.
	if (in > 0x9f) {
		TRACE("ps2: active multiplexing v%d.%d detected, but ignored!\n", (in >> 4), in & 0xf);
		goto no_support;
	}

	INFO("ps2: active multiplexing v%d.%d enabled\n", (in >> 4), in & 0xf);
	*enabled = true;
	goto done;

no_support:
	TRACE("ps2: active multiplexing not supported\n");
	*enabled = false;

done:
	// Some controllers get upset by the d3 command and will continue data
	// loopback, thus we need to send a harmless command (enable keyboard
	// interface) next.
	// This fixes bug report #1175
	res = ps2_command(0xae, NULL, 0, NULL, 0);
	if (res != B_OK) {
		INFO("ps2: active multiplexing d3 workaround failed, status 0x%08"
			B_PRIx32 "\n", res);
	}
	return B_OK;

fail:
	TRACE("ps2: testing for active multiplexing failed\n");
	*enabled = false;
	// this should revert the controller into legacy mode,
	// just in case it has switched to multiplexed mode
	return ps2_selftest();
}


status_t
ps2_command(uint8 cmd, const uint8 *out, int outCount, uint8 *in, int inCount)
{
	status_t res;
	int i;

	acquire_sem(gControllerSem);
	atomic_add(&sIgnoreInterrupts, 1);

#ifdef TRACE_PS2
	TRACE("ps2: ps2_command cmd 0x%02x, out %d, in %d\n", cmd, outCount, inCount);
	for (i = 0; i < outCount; i++)
		TRACE("ps2: ps2_command out 0x%02x\n", out[i]);
#endif

	res = ps2_wait_write();
	if (res == B_OK)
		ps2_write_ctrl(cmd);

	for (i = 0; res == B_OK && i < outCount; i++) {
		res = ps2_wait_write();
		if (res == B_OK)
			ps2_write_data(out[i]);
		else
			TRACE("ps2: ps2_command out byte %d failed\n", i);
	}

	for (i = 0; res == B_OK && i < inCount; i++) {
		res = ps2_wait_read();
		if (res == B_OK)
			in[i] = ps2_read_data();
		else
			TRACE("ps2: ps2_command in byte %d failed\n", i);
	}

#ifdef TRACE_PS2
	for (i = 0; i < inCount; i++)
		TRACE("ps2: ps2_command in 0x%02x\n", in[i]);
	TRACE("ps2: ps2_command result 0x%08" B_PRIx32 "\n", res);
#endif

	atomic_add(&sIgnoreInterrupts, -1);
	release_sem(gControllerSem);

	return res;
}


//	#pragma mark -


static int32
ps2_interrupt(void* cookie)
{
	uint8 ctrl;
	uint8 data;
	bool error;
	ps2_dev *dev;

	ctrl = ps2_read_ctrl();
	if (!(ctrl & PS2_STATUS_OUTPUT_BUFFER_FULL)) {
		TRACE("ps2: ps2_interrupt unhandled, OBF bit unset, ctrl 0x%02x (%s)\n",
				ctrl, (ctrl & PS2_STATUS_AUX_DATA) ? "aux" : "keyb");
		return B_UNHANDLED_INTERRUPT;
	}

	if (atomic_get(&sIgnoreInterrupts)) {
		TRACE("ps2: ps2_interrupt ignoring, ctrl 0x%02x (%s)\n", ctrl,
			(ctrl & PS2_STATUS_AUX_DATA) ? "aux" : "keyb");
		return B_HANDLED_INTERRUPT;
	}

	data = ps2_read_data();

	if ((ctrl & PS2_STATUS_AUX_DATA) != 0) {
		uint8 idx;
		if (gActiveMultiplexingEnabled) {
			idx = ctrl >> 6;
			error = (ctrl & 0x04) != 0;
			TRACE("ps2: ps2_interrupt ctrl 0x%02x, data 0x%02x (mouse %d)\n",
				ctrl, data, idx);
		} else {
			idx = 0;
			error = (ctrl & 0xC0) != 0;
			TRACE("ps2: ps2_interrupt ctrl 0x%02x, data 0x%02x (aux)\n", ctrl,
				data);
		}
		dev = &ps2_device[PS2_DEVICE_MOUSE + idx];
	} else {
		TRACE("ps2: ps2_interrupt ctrl 0x%02x, data 0x%02x (keyb)\n", ctrl,
			data);

		dev = &ps2_device[PS2_DEVICE_KEYB];
		error = (ctrl & 0xC0) != 0;
	}

	dev->history[1] = dev->history[0];
	dev->history[0].time = system_time();
	dev->history[0].data = data;
	dev->history[0].error = error;

	return ps2_dev_handle_int(dev);
}


//	#pragma mark - driver interface


status_t
ps2_init(void)
{
	status_t status;

	TRACE("ps2: init\n");

	status = get_module(B_ISA_MODULE_NAME, (module_info **)&gIsa);
	if (status < B_OK)
		return status;

	gControllerSem = create_sem(1, "ps/2 keyb ctrl");

	ps2_flush();

	status = ps2_dev_init();
	if (status < B_OK)
		goto err1;

	status = ps2_service_init();
	if (status < B_OK)
		goto err2;

	status = install_io_interrupt_handler(INT_PS2_KEYBOARD, &ps2_interrupt,
		NULL, 0);
	if (status)
		goto err3;

	status = install_io_interrupt_handler(INT_PS2_MOUSE, &ps2_interrupt, NULL,
		0);
	if (status)
		goto err4;

	// While this might have fixed bug #1185, we can't do this unconditionally
	// as it obviously messes up many controllers which couldn't reboot anymore
	// after that
	//ps2_selftest();

	// Setup the command byte with disabled keyboard and AUX interrupts
	// to prevent interrupts storm on some KBCs during active multiplexing
	// activation procedure. Fixes #7635.
	status = ps2_setup_command_byte(false);
	if (status) {
		INFO("ps2: initial setup of command byte failed\n");
		goto err5;
	}

	ps2_flush();
	status = ps2_setup_active_multiplexing(&gActiveMultiplexingEnabled);
	if (status) {
		INFO("ps2: setting up active multiplexing failed\n");
		goto err5;
	}

	status = ps2_setup_command_byte(true);
	if (status) {
		INFO("ps2: setting up command byte with enabled interrupts failed\n");
		goto err5;
	}

	if (gActiveMultiplexingEnabled) {
		if (ps2_dev_command_timeout(&ps2_device[PS2_DEVICE_MOUSE],
				PS2_CMD_MOUSE_SET_SCALE11, NULL, 0, NULL, 0, 100000)
			== B_TIMED_OUT) {
			INFO("ps2: accessing multiplexed mouse port 0 timed out, ignoring it!\n");
		} else {
			ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_MOUSE]);
		}
		ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_MOUSE + 1]);
		ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_MOUSE + 2]);
		ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_MOUSE + 3]);
		ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_KEYB]);
	} else {
		ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_MOUSE]);
		ps2_service_notify_device_added(&ps2_device[PS2_DEVICE_KEYB]);
	}

	TRACE("ps2: init done!\n");
	return B_OK;

err5:
	remove_io_interrupt_handler(INT_PS2_MOUSE, &ps2_interrupt, NULL);
err4:
	remove_io_interrupt_handler(INT_PS2_KEYBOARD, &ps2_interrupt, NULL);
err3:
	ps2_service_exit();
err2:
	ps2_dev_exit();
err1:
	delete_sem(gControllerSem);
	put_module(B_ISA_MODULE_NAME);
	TRACE("ps2: init failed!\n");
	return B_ERROR;
}


void
ps2_uninit(void)
{
	TRACE("ps2: uninit\n");
	remove_io_interrupt_handler(INT_PS2_MOUSE,    &ps2_interrupt, NULL);
	remove_io_interrupt_handler(INT_PS2_KEYBOARD, &ps2_interrupt, NULL);
	ps2_service_exit();
	ps2_dev_exit();
	delete_sem(gControllerSem);
	put_module(B_ISA_MODULE_NAME);
}
