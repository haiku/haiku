#pragma once
//////////////////////////////////////////////////////
// Header file for emport / import gloval symbols.
// Valid for BeOS Releas 3 or later.
// 1998.5.4 by T.Murai
//

//#define _BUILDING_CANNALIB 

#ifdef _BUILDING_CANNALIB 
#define canna_export     __declspec(dllexport) 
#else 
#define canna_export __declspec(dllimport) 
#endif
