//****************************************************************************************
//
//	File:		Common.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef COMMON_H
#define COMMON_H

enum {
	NORMAL_WINDOW_MODE = 0,
	MINI_WINDOW_MODE,
	DESKBAR_MODE
};

enum {
	PULSEVIEW_WIDTH = 263,
	PULSEVIEW_MIN_HEIGHT = 82,
	PROGRESS_MLEFT = 101,
	PROGRESS_MTOP = 18,
	PROGRESS_MBOTTOM = 10,
	CPUBUTTON_MLEFT = 79,
	CPUBUTTON_MTOP = 20,
	CPUBUTTON_WIDTH = 16,
	CPUBUTTON_HEIGHT = 16,
	ITEM_OFFSET = 27
};

#define APP_SIGNATURE				"application/x-vnd.Haiku-Pulse"

#define PV_NORMAL_MODE				'pvnm'
#define PV_MINI_MODE				'pvmm'
#define PV_DESKBAR_MODE				'pvdm'
#define PV_PREFERENCES				'pvpr'
#define PV_ABOUT					'pvab'
#define PV_QUIT						'pvqt'
#define PV_CPU_MENU_ITEM			'pvcm'
#define PV_REPLICANT_PULSE			'pvrp'

#define PRV_NORMAL_FADE_COLORS		'prnf'
#define PRV_NORMAL_CHANGE_COLOR		'prnc'
#define PRV_MINI_ACTIVE				'prma'
#define PRV_MINI_IDLE				'prmi'
#define PRV_MINI_FRAME				'prmf'
#define PRV_MINI_CHANGE_COLOR		'prmc'
#define PRV_DESKBAR_ACTIVE			'prda'
#define PRV_DESKBAR_IDLE			'prdi'
#define PRV_DESKBAR_FRAME			'prdf'
#define PRV_DESKBAR_ICON_WIDTH		'prdw'
#define PRV_DESKBAR_CHANGE_COLOR	'prdc'
#define PRV_BOTTOM_OK				'prbo'
#define PRV_BOTTOM_DEFAULTS			'prbd'
#define PRV_QUIT					'prvq'

#define DEFAULT_NORMAL_BAR_COLOR		0x00f00000
#define DEFAULT_MINI_ACTIVE_COLOR		0x20c02000
#define DEFAULT_MINI_IDLE_COLOR			0x20402000
#define DEFAULT_MINI_FRAME_COLOR		0x20202000
#define DEFAULT_DESKBAR_ACTIVE_COLOR	0x20c02000
#define DEFAULT_DESKBAR_IDLE_COLOR		0x20402000
#define DEFAULT_DESKBAR_FRAME_COLOR		0x20202000
#define DEFAULT_NORMAL_FADE_COLORS		true
#define DEFAULT_DESKBAR_ICON_WIDTH		9

#endif
