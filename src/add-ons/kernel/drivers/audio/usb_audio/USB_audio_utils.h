/*
 *	USB audio spec utils
 */

#ifndef __USB_AUDIO_SPEC_UTILS_H__
#define __USB_AUDIO_SPEC_UTILS_H__

#include <OS.h>

#ifdef __cplusplus
extern "C" {
#endif

// A whole config
void dump_usb_configuration_info(const usb_configuration_info *conf);
void dump_interface_descriptor(usb_interface_descriptor* intf_des);
void dump_interface(usb_interface_info* inf);
void dump_endpoint_info(int i, usb_endpoint_info* ep_inf);

// Class 1/1/0 (Audio Control)
void dump_usb_class_110(usb_endpoint_info* ep_inf, size_t endpoint_count,  int8** ptr, size_t count);
void dump_usb_audiocontrol_header_descriptor(int8* data);
void dump_usb_input_terminal_descriptor(int8* data);
void dump_usb_output_terminal_descriptor(int8* data);
void dump_usb_selector_unit_descriptor(int8* data);
void dump_usb_feature_unit_descriptor(int8* data);

// Class 1/2/0 (Audio Streaming)
void dump_usb_class_120(usb_endpoint_info* ep_inf, size_t endpoint_count, int8** ptr, size_t count);

void dump_usb_as_interface_descriptor(int8* data);
void dump_usb_type_I_format_descriptor(int8* data);
void dump_usb_as_cs_endpoint_descriptor(int8* data);

// utils
void	dump_descr(int8* data);
void	dump_data(int8* data, int length);

#ifdef __cplusplus
}
#endif

#endif
