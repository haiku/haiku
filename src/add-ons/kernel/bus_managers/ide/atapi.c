/*
 * Copyright 2004-2007, Haiku, Inc. All RightsReserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Open IDE bus manager

	ATAPI command protocol
*/

#include "ide_internal.h"

#include <scsi_cmds.h>

#include "ide_cmds.h"
#include "ide_sim.h"

#include <string.h>


// used for MODE SENSE/SELECT 6 emulation; maximum size is 255 + header,
// so this is a safe bet
#define IDE_ATAPI_BUFFER_SIZE 512


/*!
	Set sense according to error reported by device
	return: true - device reported error
*/
static bool
check_packet_error(ide_device_info *device, ide_qrequest *qrequest)
{
	ide_bus_info *bus = device->bus;
	int status;

	status = bus->controller->get_altstatus(bus->channel_cookie);

	if ((status & (ide_status_err | ide_status_df)) != 0) {
		int error;

		SHOW_FLOW(3, "packet error, status=%02x", status);

		if (bus->controller->read_command_block_regs(bus->channel_cookie,
				&device->tf, ide_mask_error) != B_OK) {
			device->subsys_status = SCSI_HBA_ERR;
			return true;
		}

		// the upper 4 bits contain sense key
		// we don't want to clutter syslog with "not ready" and UA messages,
		// so use FLOW messages for them
		error = device->tf.read.error;
		if ((error >> 4) == SCSIS_KEY_NOT_READY
			|| (error >> 4) == SCSIS_KEY_UNIT_ATTENTION)
			SHOW_FLOW(3, "error=%x", error);
		else
			SHOW_ERROR(3, "error=%x", error);

		// ATAPI says that:
		// "ABRT shall be set to one if the requested command has been command 
		//  aborted because the command code or a command parameter is invalid. 
		//  ABRT may be set to one if the device is not able to complete the 
		//  action requested by the command."
		// Effectively, it can be set if "something goes wrong", including
		// if the medium got changed. Therefore, we currently ignore the bit
		// and rely on auto-sense information
		/*
		if ((error & ide_error_abrt) != 0) {
			// if command got aborted, there's no point in reading sense
			set_sense(device, SCSIS_KEY_ABORTED_COMMAND, SCSIS_ASC_NO_SENSE);
			return false;
		}
		*/

		// tell SCSI layer that sense must be requested
		// (we don't take care of auto-sense ourselve)
		device->subsys_status = SCSI_REQ_CMP_ERR;
		qrequest->request->device_status = SCSI_STATUS_CHECK_CONDITION;
		// reset pending emulated sense - its overwritten by a real one
		device->combined_sense = 0;
		return true;
	}

	return false;
}


/*! IRQ handler of packet transfer (executed as DPC) */
void
packet_dpc(ide_qrequest *qrequest)
{
	ide_device_info *device = qrequest->device;
	ide_bus_info *bus = device->bus;
	int status;
	uint32 timeout = qrequest->request->timeout > 0 ? 
		qrequest->request->timeout : IDE_STD_TIMEOUT;

	SHOW_FLOW0(3, "");

	bus->controller->read_command_block_regs(bus->channel_cookie,
		&device->tf, ide_mask_error | ide_mask_ireason);

	status = bus->controller->get_altstatus(bus->channel_cookie);

	if (qrequest->packet_irq) {
		// device requests packet
		qrequest->packet_irq = false;

		if (!device->tf.packet_res.cmd_or_data
			|| device->tf.packet_res.input_or_output
			|| (status & ide_status_drq) == 0) {
			device->subsys_status = SCSI_SEQUENCE_FAIL;
			goto err;
		}

		start_waiting_nolock(device->bus, timeout, ide_state_async_waiting);

		// send packet			
		if (bus->controller->write_pio(bus->channel_cookie,
				(uint16 *)device->packet, sizeof(device->packet) / sizeof(uint16), 
				true) != B_OK) {
			SHOW_ERROR0( 1, "Error sending command packet" );

			device->subsys_status = SCSI_HBA_ERR;
			goto err_cancel_timer;
		}

		return;
	}

	if (qrequest->uses_dma) {
		// DMA transmission finished
		bool dma_err, dev_err;

		// don't check drq - if there is some data left, we cannot handle
		// it anyway
		// XXX does the device throw remaining data away on DMA overflow?
		SHOW_FLOW0(3, "DMA done");

		dma_err = !finish_dma(device);
		dev_err = check_packet_error(device, qrequest);

		// what to do if both the DMA controller and the device reports an error?
		// let's assume that the DMA controller got problems because there was a
		// device error, so we ignore the dma error and use the device error instead
		if (dev_err) {
			finish_checksense(qrequest);
			return;
		}

		// device is happy, let's see what the controller says
		if (!dma_err) {
			// if DMA works, reset error counter so we don't disable
			// DMA only because it didn't work once in a while
			device->DMA_failures = 0;
			// this is a lie, but there is no way to find out
			// how much has been transmitted
			qrequest->request->data_resid = 0;
			finish_checksense(qrequest);
		} else {
			// DMA transmission went wrong
			set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_LUN_COM_FAILURE);

			if (++device->DMA_failures >= MAX_DMA_FAILURES) {
				SHOW_ERROR0(1, "Disabling DMA because of too many errors");

				device->DMA_enabled = false;
			}

			finish_checksense(qrequest);
		}

		return;
	}

	// PIO mode	
	if ((status & ide_status_drq) != 0) {
		// device wants to transmit data
		int length;
		status_t err;

		SHOW_FLOW0(3, "data transmission");

		if (device->tf.packet_res.cmd_or_data) {
			device->subsys_status = SCSI_SEQUENCE_FAIL;
			goto err;
		}

		// check whether transmission direction matches
		if ((device->tf.packet_res.input_or_output ^ qrequest->is_write) == 0) {
			SHOW_ERROR0(2, "data transmission in wrong way!?");

			// TODO: hm, either the device is broken or the caller has specified
			// the wrong direction - what is the proper handling?
			set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_LUN_COM_FAILURE);

			// reset device to make it alive
			// TODO: the device will abort next command with a reset condition
			// 		perhaps we should hide that by reading sense?
			SHOW_FLOW0(3, "Reset");
			reset_device(device, qrequest);
			
			finish_checksense(qrequest);
			return;
		}

		// ask device how much data it wants to transmit
		bus->controller->read_command_block_regs(bus->channel_cookie,
			&device->tf, ide_mask_byte_count);

		length = device->tf.packet_res.byte_count_0_7
			| ((int)device->tf.packet_res.byte_count_8_15 << 8);

		SHOW_FLOW(3, "device transmittes %d bytes", length);

		// start waiting before starting transmission, else we 
		// could start waiting too late;
		// don't mind getting overtaken by IRQ handler - as it will
		// issue a DPC for the thread context we are in, we are save
		start_waiting_nolock(device->bus, timeout, ide_state_async_waiting);

		if (device->tf.packet_res.input_or_output)
			err = read_PIO_block(qrequest, length);
		else
			err = write_PIO_block(qrequest, length);

		// only report "real" errors;
		// discarding data (ERR_TOO_BIG) can happen but is OK
		if (err == B_ERROR) {
			SHOW_ERROR0(2, "Error during PIO transmission");

			device->subsys_status = SCSI_HBA_ERR;
			goto err_cancel_timer;
		}

		SHOW_FLOW0(3, "7");
		return;
	} else {
		// device has done job and doesn't want to transmit data anymore
		// -> finish request
		SHOW_FLOW0(3, "no data");

		check_packet_error(device, qrequest);

		SHOW_FLOW(3, "finished: %d of %d left", 
			(int)qrequest->request->data_resid,
			(int)qrequest->request->data_length);

		finish_checksense(qrequest);
		return;
	}

	return;

err_cancel_timer:
	cancel_irq_timeout(device->bus);
err:
	finish_checksense(qrequest);
}


/*!	Create taskfile for ATAPI packet */
static bool
create_packet_taskfile(ide_device_info *device, ide_qrequest *qrequest,
	bool write)
{
	scsi_ccb *request = qrequest->request;

	SHOW_FLOW(3, "DMA enabled=%d, uses_dma=%d, scsi_cmd=%x", 
		device->DMA_enabled, qrequest->uses_dma, device->packet[0]);

	device->tf_param_mask = ide_mask_features | ide_mask_byte_count;

	device->tf.packet.dma = qrequest->uses_dma;
	device->tf.packet.ovl = 0;
	device->tf.packet.byte_count_0_7 = request->data_length & 0xff;
	device->tf.packet.byte_count_8_15 = request->data_length >> 8;
	device->tf.packet.command = IDE_CMD_PACKET;

	return true;
}


/*! Send ATAPI packet */
void
send_packet(ide_device_info *device, ide_qrequest *qrequest, bool write)
{
	ide_bus_info *bus = device->bus;
	bool packet_irq = device->atapi.packet_irq;
	uint8 scsi_cmd = device->packet[0];

	SHOW_FLOW( 3, "qrequest=%p, command=%x", qrequest, scsi_cmd );

	/*{
		unsigned int i;
		
		for( i = 0; i < sizeof( device->packet ); ++i ) 
			dprintf( "%x ", device->packet[i] );
	}*/

	SHOW_FLOW(3, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x (len=%d)",
		device->packet[0], device->packet[1], device->packet[2], 
		device->packet[3], device->packet[4], device->packet[5], 
		device->packet[6], device->packet[7], device->packet[8], 
		device->packet[9], device->packet[10], device->packet[11],
		qrequest->request->cdb_length);

	//snooze( 1000000 );

	qrequest->is_write = write;
	// if needed, mark first IRQ as being packet request IRQ
	qrequest->packet_irq = packet_irq;

	// only READ/WRITE commands can use DMA
	// (the device may support it always, but IDE controllers don't
	// report how much data is transmitted, and this information is
	// crucial for the SCSI protocol)
	// special offer: let READ_CD commands use DMA too
	qrequest->uses_dma = device->DMA_enabled
		&& (scsi_cmd == SCSI_OP_READ_6 || scsi_cmd == SCSI_OP_WRITE_6
		|| scsi_cmd == SCSI_OP_READ_10 || scsi_cmd == SCSI_OP_WRITE_10
		|| scsi_cmd == SCSI_OP_READ_12 || scsi_cmd == SCSI_OP_WRITE_12
		|| scsi_cmd == SCSI_OP_READ_CD);

	// try preparing DMA, if that fails, fall back to PIO	
	if (qrequest->uses_dma) {
		SHOW_FLOW0(3, "0");
		if (!prepare_dma( device, qrequest))
			qrequest->uses_dma = false;

		SHOW_FLOW(3, "0->%d", qrequest->uses_dma);
	}

	SHOW_FLOW0(3, "1");

	if (!qrequest->uses_dma)
		prep_PIO_transfer(device, qrequest);

	SHOW_FLOW0(3, "2");

	if (!create_packet_taskfile(device, qrequest, write))
		goto err_setup;

	SHOW_FLOW0(3, "3");

	if (!send_command(device, qrequest, false, 
			device->atapi.packet_irq_timeout, 
			device->atapi.packet_irq ? ide_state_async_waiting : ide_state_accessing))
		goto err_setup;

	SHOW_FLOW0(3, "4");

	if (packet_irq) {
		// device asks for packet via IRQ;
		// timeout and stuff is already set by send_command
		return;
	}
	
	SHOW_FLOW0(3, "5");

	// wait for device to get ready for packet transmission
	if (!ide_wait(device, ide_status_drq, ide_status_bsy, false, 100000))
		goto err_setup;

	SHOW_FLOW0(3, "6");

	// make sure device really asks for command packet		
	bus->controller->read_command_block_regs(bus->channel_cookie, &device->tf,
		ide_mask_ireason);

	if (!device->tf.packet_res.cmd_or_data
		|| device->tf.packet_res.input_or_output) {
		device->subsys_status = SCSI_SEQUENCE_FAIL;
		goto err_setup;
	}

	SHOW_FLOW0(3, "7");

	// some old drives need a delay before submitting the packet
	spin(10);

	// locking is evil here: as soon as the packet is transmitted, the device
	// may raise an IRQ (which actually happens if the device reports an Check 
	// Condition error). Thus, we have to lock out the IRQ handler _before_ we
	// start packet transmission, which forbids all kind of interrupts for some
	// time; to reduce this period, blocking is done just before last dword is
	// sent (avoid sending 16 bits as controller may transmit 32 bit chunks)

	// write packet
	if (bus->controller->write_pio(bus->channel_cookie, 
			(uint16 *)device->packet, sizeof(device->packet) / sizeof(uint16) - 2, 
			true) != B_OK) {
		goto err_packet;
	}

	IDE_LOCK(bus);

	if (bus->controller->write_pio(bus->channel_cookie, 
			(uint16 *)device->packet + sizeof(device->packet) / sizeof(uint16) - 2, 
			2, true) != B_OK) {
		goto err_packet2;
	}

	if (qrequest->uses_dma) {
		SHOW_FLOW0( 3, "ready for DMA" );

		// S/G table must already be setup - we hold the bus lock, so
		// we really have to hurry up
		start_dma_wait(device, qrequest);		
	} else {
		uint32 timeout = qrequest->request->timeout > 0 ? 
			qrequest->request->timeout : IDE_STD_TIMEOUT;

		start_waiting(bus, timeout, ide_state_async_waiting);
	}

	SHOW_FLOW0(3, "8");
	return;

err_packet2:
	IDE_UNLOCK(bus);

err_packet:
	device->subsys_status = SCSI_HBA_ERR;
	
err_setup:
	if (qrequest->uses_dma)
		abort_dma(device, qrequest);

	finish_checksense(qrequest);
}


/*! Execute SCSI I/O for atapi devices */
void
atapi_exec_io(ide_device_info *device, ide_qrequest *qrequest)
{
	scsi_ccb *request = qrequest->request;

	SHOW_FLOW(3, "command=%x", qrequest->request->cdb[0]);

	// ATAPI command packets are 12 bytes long; 
	// if the command is shorter, remaining bytes must be padded with zeros
	memset(device->packet, 0, sizeof(device->packet));
	memcpy(device->packet, request->cdb, request->cdb_length);

	if (request->cdb[0] == SCSI_OP_REQUEST_SENSE && device->combined_sense) {
		// we have a pending emulated sense - return it on REQUEST SENSE
		ide_request_sense(device, qrequest);
		finish_checksense(qrequest);
	} else {
		// reset all error codes for new request
		start_request(device, qrequest);

		// now we have an IDE packet
		send_packet(device, qrequest, 
			(request->flags & SCSI_DIR_MASK) == SCSI_DIR_OUT);
	}
}


/*!	Prepare device info for ATAPI device */
bool
prep_atapi(ide_device_info *device)
{
	ide_device_infoblock *infoblock = &device->infoblock;

	SHOW_FLOW0(3, "");

	device->is_atapi = true;
	device->exec_io = atapi_exec_io;

	if (infoblock->_0.atapi.ATAPI != 2)
		return false;

	switch(infoblock->_0.atapi.drq_speed) {
		case 0:
		case 2:
			device->atapi.packet_irq = false;
			break;
		case 1:
			device->atapi.packet_irq = true;
			device->atapi.packet_irq_timeout = IDE_STD_TIMEOUT;
			break;
		default:
			return false;
	}

	SHOW_FLOW(3, "drq speed: %d", infoblock->_0.atapi.drq_speed);

	/*if( infoblock->_0.atapi.packet_size != 0 )
		return false;*/

	device->device_type = infoblock->_0.atapi.type;
	device->last_lun = infoblock->last_lun;

	SHOW_FLOW(3, "device_type=%d, last_lun=%d", 
		device->device_type, device->last_lun);

	// don't use task file to select LUN but command packet
	// (SCSI bus manager sets LUN there automatically)
	device->tf.packet.lun = 0;

	if (!initialize_qreq_array(device, 1)
		|| !configure_dma(device))
		return false;

	// currently, we don't support queuing, but I haven't found any
	// ATAPI device that supports queuing anyway, so this is no loss
	device->CQ_enabled = device->CQ_supported = false;

	return true;
}
