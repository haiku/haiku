/*
	Copyright (c) 2003, Thomas Kurschel


	Part of DDC driver
		
	Dumps EDID content
*/

#include "edid.h"
#include <KernelExport.h>
#include "ddc_int.h"
#include <stdio.h>

void edid_dump( edid1_info *edid )
{
	int i, j;
	char buffer[256];
	
	SHOW_INFO( 0, "Vendor: %s", edid->vendor.manufacturer );
	SHOW_INFO( 0, "Product ID: %d", (int)edid->vendor.prod_id );
	SHOW_INFO( 0, "Serial #: %d", (int)edid->vendor.serial );
	SHOW_INFO( 0, "Produced in week/year: %d/%d", edid->vendor.week, edid->vendor.year );
	
	SHOW_INFO( 0, "EDID version: %d.%d", edid->version.version, edid->version.revision );
	
	SHOW_INFO( 0, "Type: %s", edid->display.input_type ? "Digital" : "Analog" );
	SHOW_INFO( 0, "Size: %d cm x %d cm", edid->display.h_size, edid->display.v_size );
	SHOW_INFO( 0, "Gamma=%.3f", (edid->display.gamma + 100) / 100.0 );
	SHOW_INFO( 0, "White (X,Y)=(%.3f,%.3f)", edid->display.white_x / 1024.0, edid->display.white_y / 1024.0 );
	
	SHOW_INFO0( 0, "Supported Future Video Modes:" );
	for( i = 0; i < EDID1_NUM_STD_TIMING; ++i ) {
		if( edid->std_timing[i].h_size <= 256 )
			continue;
			
		SHOW_INFO( 0, "%dx%d@%dHz (id=%d)", 
			edid->std_timing[i].h_size, edid->std_timing[i].v_size,
			edid->std_timing[i].refresh, edid->std_timing[i].id );
	}
	
	SHOW_INFO0( 0, "Supported VESA Video Modes:" );
	if( edid->established_timing.res_720x400x70 )
		SHOW_INFO0( 0, "720x400@70" );
	if( edid->established_timing.res_720x400x88 )
		SHOW_INFO0( 0, "720x400@88" );
	if( edid->established_timing.res_640x480x60 )
		SHOW_INFO0( 0, "640x480@60" );
	if( edid->established_timing.res_640x480x67 )
		SHOW_INFO0( 0, "640x480x67" );
	if( edid->established_timing.res_640x480x72 )
		SHOW_INFO0( 0, "640x480x72" );
	if( edid->established_timing.res_640x480x75 )
		SHOW_INFO0( 0, "640x480x75" );
	if( edid->established_timing.res_800x600x56 )
		SHOW_INFO0( 0, "800x600@56" );
	if( edid->established_timing.res_800x600x60 )
		SHOW_INFO0( 0, "800x600@60" );
		
	if( edid->established_timing.res_800x600x72 )
		SHOW_INFO0( 0, "800x600@72" );
	if( edid->established_timing.res_800x600x75 )
		SHOW_INFO0( 0, "800x600@75" );
	if( edid->established_timing.res_832x624x75 )
		SHOW_INFO0( 0, "832x624@75" );
	if( edid->established_timing.res_1024x768x87i )
		SHOW_INFO0( 0, "1024x768@87 interlaced" );
	if( edid->established_timing.res_1024x768x60 )
		SHOW_INFO0( 0, "1024x768@60" );
	if( edid->established_timing.res_1024x768x70 )
		SHOW_INFO0( 0, "1024x768@70" );
	if( edid->established_timing.res_1024x768x75 )
		SHOW_INFO0( 0, "1024x768@75" );
	if( edid->established_timing.res_1280x1024x75 )
		SHOW_INFO0( 0, "1280x1024@75" );
		
	if( edid->established_timing.res_1152x870x75 )
		SHOW_INFO0( 0, "1152x870@75" );
		
	for( i = 0; i < EDID1_NUM_DETAILED_MONITOR_DESC; ++i ) {
		edid1_detailed_monitor *monitor = &edid->detailed_monitor[i];
		
		switch( monitor->monitor_desc_type ) {
		case edid1_serial_number:
			SHOW_INFO( 0, "Serial Number: %s", monitor->data.serial_number );
			break;
		case edid1_ascii_data:
			SHOW_INFO( 0, " %s", monitor->data.serial_number );
			break;
		case edid1_monitor_ranges: {
			edid1_monitor_range monitor_range = monitor->data.monitor_range;
			
			SHOW_INFO( 0, "Horizontal frequency range = %d..%d kHz",
				monitor_range.min_h, monitor_range.max_h );
			SHOW_INFO( 0, "Vertical frequency range = %d..%d Hz",
				monitor_range.min_v, monitor_range.max_v );
			SHOW_INFO( 0, "Maximum pixel clock = %d MHz", (uint16)monitor_range.max_clock * 10 );
			break; }
		case edid1_monitor_name:
			SHOW_INFO( 0, "Monitor Name: %s", monitor->data.serial_number );
			break;
		case edid1_add_colour_pointer: {
			for( j = 0; j < EDID1_NUM_EXTRA_WHITEPOINTS; ++j ) {
				edid1_whitepoint *whitepoint = &monitor->data.whitepoint[j];
				
				if( whitepoint->index == 0 )
					continue;

				sprintf( buffer, "Additional whitepoint: (X,Y)=(%f,%f) gamma=%f index=%i",
					whitepoint->white_x / 1024.0, 
					whitepoint->white_y / 1024.0, 
					(whitepoint->gamma + 100) / 100.0, 
					whitepoint->index );
				SHOW_INFO( 0, "%s", buffer );
			}
			break; }
		case edid1_add_std_timing: {		
			for( j = 0; j < EDID1_NUM_EXTRA_STD_TIMING; ++j ) {
				edid1_std_timing *timing = &monitor->data.std_timing[j];
				
				if( timing->h_size <= 256 )
					continue;
					
				SHOW_INFO( 0, "%dx%d@%dHz (id=%d)", 
					timing->h_size, timing->v_size,
					timing->refresh, timing->id );
			}
			break; }
		case edid1_is_detailed_timing: {
			edid1_detailed_timing *timing = &monitor->data.detailed_timing;
			
			SHOW_INFO0( 0, "Additional Video Mode:" );
			sprintf( buffer, "clock=%f MHz", timing->pixel_clock / 100.0 );
			SHOW_INFO( 0, "%s", buffer );
			SHOW_INFO( 0, "h: (%d, %d, %d, %d)", 
				timing->h_active, timing->h_active + timing->h_sync_off,
				timing->h_active + timing->h_sync_off + timing->h_sync_width,
				timing->h_active + timing->h_blank );
			SHOW_INFO( 0, "v: (%d, %d, %d, %d)", 
				timing->v_active, timing->v_active + timing->v_sync_off,
				timing->v_active + timing->v_sync_off + timing->v_sync_width,
				timing->v_active + timing->v_blank );
			sprintf( buffer, "size: %.1f cm x %.1f cm", 
				timing->h_size / 10.0, timing->v_size / 10.0 );
			SHOW_INFO( 0, "%s", buffer );
			sprintf( buffer, "border: %.1f cm x %.1f cm",
				timing->h_border / 10.0, timing->v_border / 10.0 );
			SHOW_INFO( 0, "%s", buffer );
			break; }
		}
	}
}
