/*
 *	USB audio spec utils
 */

#include <OS.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <byteorder.h>

#include <USB.h>
#include <USB_spec.h>

#include "USB_audio_spec.h"
#include "USB_audio_utils.h"

extern void 	set_triplet(int8* ptr, uint32 param);

uint32
get_triplet(int8* ptr) {
        uint32 lsb = ptr[0] & 0xff;
        uint32 hsb = ptr[1] & 0xff;
        uint32 msb = ptr[2] & 0xff;
        uint32 res;
        res = (msb << 16) | (hsb << 8) | (lsb);
        return res;
}


/* 
 *	 dump a whole configuration
 */

void dump_usb_configuration_info(const usb_configuration_info *conf)
{
	usb_interface_info* inf;
	int i,j,k;
	bool isactive;

	dprintf("Dumping usb_configuration_info... %d interfaces\n", conf->interface_count);
	for (i=0; i<conf->interface_count; i++) {
		dprintf("INTERFACE %d, %d alternatitives:\n", i, conf->interface[i].alt_count);
		for (j=0; j<conf->interface[i].alt_count; j++) {
			isactive = &conf->interface[i].alt[j] == conf->interface[i].active;
			dprintf("  altconf #%d (%s):\n", j, isactive?"*":"-");
			if (isactive) {
				inf = conf->interface[i].active;
				dump_interface(inf);
			} else {
				inf = &conf->interface[i].alt[j];
				dump_interface(inf);
//				dprintf("    #endp: %d\n",conf->interface[i].alt[j].endpoint_count);
			}
		}
	}
}

void dump_interface_descriptor(usb_interface_descriptor* intf_des) 
{
	dprintf("  interface descriptor: %d bytes\n",intf_des->length);
	dprintf("    interface_number:  %d\n",intf_des->interface_number);
	dprintf("    alternate_setting: %d\n",intf_des->alternate_setting);
	dprintf("    num_endpoints:     %d\n",intf_des->num_endpoints);
	dprintf("    class/sub/prot:    %d/%d/%d\n",intf_des->interface_class, intf_des->interface_subclass, intf_des->interface_protocol);
	dprintf("    interface str:     %d\n",intf_des->interface);
}

void dump_interface(usb_interface_info* inf)
{
	int i;
	usb_interface_descriptor* intf_des = inf->descr;
	
	dump_interface_descriptor(intf_des);
	if (inf->descr->interface_class == 1) {
		if (inf->descr->interface_subclass == 1) {
			dump_usb_class_110(inf->endpoint, inf->endpoint_count, 
				(int8**)inf->generic, inf->generic_count);
		}
		if (inf->descr->interface_subclass == 2) {
			dump_usb_class_120(inf->endpoint, inf->endpoint_count, 
				(int8**)inf->generic, inf->generic_count);
		}
	}
	//if (inf->descr->interface_class == 3) {
	//	if (inf->descr->interface_subclass == 0) {
	//		dump_usb_class_300(inf->endpoint, inf->endpoint_count, 
	//			(int8**)inf->generic, inf->generic_count);
//	//		dprintf("    <HID stuff here>\n");
	//	}
	//}
}

char* attr(int8 attrib)
{
	switch (attrib) {
		case 0x00: return "Control";
		case 0x01: return "Isochronous";
		case 0x02: return "Bulk";
		case 0x03: return "Interrupt";
	}
	return "?";
}

void dump_endpoint_info(int i, usb_endpoint_info* ep_inf)
{
	dprintf("    endpoint %d:\n",i);
	dprintf("      address:         %d, %s\n",ep_inf->descr->endpoint_address & 0x07, (ep_inf->descr->endpoint_address & 0x80)?"IN":"OUT");
	dprintf("      transfer type:   %s\n",attr(ep_inf->descr->attributes));
	dprintf("      max_packet_size: %d\n",ep_inf->descr->max_packet_size);
	dprintf("      interval:        %d\n",ep_inf->descr->interval);
	dprintf("     usb_pipe handle:  %p\n",ep_inf->handle);
}

/* 
 *	dump class 1/1/0
 */

void dump_usb_class_110(usb_endpoint_info* ep_inf, size_t endpoint_count,  int8** ptr, size_t count) 
{
	int i, length;
	int8* data;
	int descr;
	
	dprintf("  Class 1/1/0 - Audio Control\n");
	dprintf("    With %d endpoints:\n", endpoint_count);
	for (i=0; i<endpoint_count; i++) {
		dump_endpoint_info(i, &ep_inf[i]);
	}

	dprintf("    And %d other descriptors:\n",count);
	for (i=0; i<count; i++) {
		data = ptr[i];
		length = data[0];
		if (data[1] != UAS_CS_INTERFACE) { // 0x24
			dump_data(&data[0], length);
		} else {
			descr = data[2];
			switch (descr) {
				default:
				case UAS_AC_DESCRIPTOR_UNDEFINED:
					dump_data(&data[0], length); 
					break;
				case UAS_HEADER:
					dump_usb_audiocontrol_header_descriptor(data); 
					break;
				case UAS_INPUT_TERMINAL: 
					dump_usb_input_terminal_descriptor(data);
					break;
				case UAS_OUTPUT_TERMINAL: 
					dump_usb_output_terminal_descriptor(data);
					break;
				case UAS_MIXER_UNIT:
					dump_data(&data[0], length); 
					break;
				case UAS_SELECTOR_UNIT:
					dump_usb_selector_unit_descriptor(data);
					break;
				case UAS_FEATURE_UNIT:
					dump_usb_feature_unit_descriptor(data);
					break;
				case UAS_PROCESSING_UNIT:
				case UAS_EXTENSION_UNIT:
					dump_data(&data[0], length); 
					break;
			}
		}
	}
}

void dump_usb_audiocontrol_header_descriptor(int8* data)
{	
	int i;
	usb_audiocontrol_header_descriptor* h = (usb_audiocontrol_header_descriptor*)data;

	dump_descr(data);
	dprintf("  HEADER\n");
	dprintf("    bcd_release_no: 0x%x\n", h->bcd_release_no);
	dprintf("    total_length:   %d bytes\n", h->total_length);
	dprintf("    in_collection:  %d\n", h->in_collection);
	for (i=0; i<h->in_collection; i++) {
		dprintf("    %d\n", h->interface_numbers[i]);
	}
}

void dump_usb_input_terminal_descriptor(int8* data)
{
	usb_input_terminal_descriptor* it = (usb_input_terminal_descriptor*)data;

	dump_descr(data);
	dprintf("  INPUT_TERMINAL\n");
	dprintf("    terminal id:    %d\n", it->terminal_id);
	dprintf("    terminal_type:  0x%x\n", it->terminal_type);
	dprintf("    assoc_terminal: %d\n", it->assoc_terminal);
	dprintf("    num_channels:   %d\n", it->num_channels);
	dprintf("    channel_config: %d\n", it->channel_config);
	dprintf("    channel_names:  %d\n", it->channel_names);
	dprintf("    terminal str:   %d\n", it->terminal);
}

void dump_usb_output_terminal_descriptor(int8* data)
{
	usb_output_terminal_descriptor* ot = (usb_output_terminal_descriptor*)data;

	dump_descr(data);
	dprintf("  OUTPUT_TERMINAL\n");
	dprintf("    terminal id:    %d\n", ot->terminal_id);
	dprintf("    terminal_type:  0x%x\n", ot->terminal_type);
	dprintf("    assoc_terminal: %d\n", ot->assoc_terminal);
	dprintf("    source_id:      %d\n", ot->source_id);
	dprintf("    terminal str:   %d\n", ot->terminal);
}

void dump_usb_selector_unit_descriptor(int8* data)
{
	int i;
	usb_selector_unit_descriptor* su = (usb_selector_unit_descriptor*)data;

	dump_descr(data);
	dprintf("  SELECTOR_UNIT\n");
	dprintf("    unit_id:        %d\n", su->unit_id);
	dprintf("    num_input_pins: %d\n", su->num_input_pins);
	for (i=0; i < su->num_input_pins; i++) {
		dprintf("    %d\n", su->input_pins[i]);
	}
	dprintf("    selector str:   %d\n", su->input_pins[i]); // or selector_string
}

void bma_dec(int16 bitmap) {
	if (bitmap & 0x0001) dprintf("Mute ");
	if (bitmap & 0x0002) dprintf("Volume ");
	if (bitmap & 0x0004) dprintf("Bass ");
	if (bitmap & 0x0008) dprintf("Mid ");
	if (bitmap & 0x0010) dprintf("Treble ");
	if (bitmap & 0x0020) dprintf("Graphic EQ ");
	if (bitmap & 0x0040) dprintf("Automatic Gain ");
	if (bitmap & 0x0080) dprintf("Delay ");
	if (bitmap & 0x0100) dprintf("Bass Boost ");
	if (bitmap & 0x0200) dprintf("Loudness ");
	dprintf("\n");
}

void dump_usb_feature_unit_descriptor(int8* data)
{
	// warning: either doc is out of date, or this & 1325 implementation is wrong on control_size vs bma_controls[] length 
	int i, length;
	usb_feature_unit_descriptor* fu = (usb_feature_unit_descriptor*)data;

	dump_descr(data);
	length = data[0];
	dprintf("  FEATURE_UNIT\n");
	dprintf("    unit_id:        %d\n", fu->unit_id);
	dprintf("    source_id:      %d\n", fu->source_id);
	dprintf("    control_size:   %d\n", fu->control_size);
	if (fu->control_size == 2) {
		dprintf("    master bma:     "); 
		bma_dec(fu->master_bma_control);
		for (i=0; i < fu->control_size; i++) {
	 		dprintf("    ch %d bma:       ", i);
	 		bma_dec(fu->bma_controls[i]);
		}
		dprintf("    featurestr: %d\n", fu->feature_string);
	} else {
		dprintf("    err: unsupported control_size\n");
	}
}

/* 
 *	dump class 1/2/0
 */

void dump_usb_class_120(usb_endpoint_info* ep_inf, size_t endpoint_count, int8** ptr, size_t count) 
{
	int i, length;
	int8* data;
	dprintf("   Class 1/2/0 - Audio Streaming\n");

	dprintf("    With %d endpoints:\n", endpoint_count);
	for (i=0; i<endpoint_count; i++) {
		dump_endpoint_info(i, &ep_inf[i]);
	}

	dprintf("    And %d other descriptors:\n",count);
	for (i=0; i<count; i++) {
		data = ptr[i];
		length = data[0];
		switch (data[1]) {
			case UAS_CS_INTERFACE: // 0x24
				switch (data[2]) {
					case UAS_AS_GENERAL:
						dump_usb_as_interface_descriptor(data); 
						break;
					case UAS_FORMAT_TYPE:
						dump_usb_type_I_format_descriptor(data); 
						break;
					default:
						dump_data(&data[0], length); 
						break;
				}
				break;
			case UAS_CS_ENDPOINT:
				switch (data[2]) {
					case UAS_EP_GENERAL:
						dump_usb_as_cs_endpoint_descriptor(data);
						break;
					default:
						dump_data(&data[0], length);
						break;
				}
				break;
			default:
				dump_data(&data[0], length);
				break;
		}
	}
}

void dump_usb_as_interface_descriptor(int8* data)
{
	usb_as_interface_descriptor* as = (usb_as_interface_descriptor*)data;

}

void dump_usb_type_I_format_descriptor(int8* data)
{
	int i;
	usb_type_I_format_descriptor* fmt = (usb_type_I_format_descriptor*)data;
}

void dump_usb_as_cs_endpoint_descriptor(int8* data)
{
	usb_as_cs_endpoint_descriptor* ep = (usb_as_cs_endpoint_descriptor*)data;

	dump_descr(data);
	dprintf("    Audio Streaming Endpoint Descriptor\n");
	dprintf("      attributes:         %d\n", ep->attributes);
	dprintf("      lock_delay_units:   %d\n", ep->lock_delay_units);
	dprintf("      lock_delay:         %d\n", ep->lock_delay);
}


/*
 * Last resort...
 */

void dump_descr(int8* data)
{
	dump_data(data + 1, *data);
}


void dump_data(int8* data, int length)
{
	int i;
	for (i=0; i<length; i++) {
		dprintf("%02x ",data[i]);
	}
	dprintf("\n");
}
