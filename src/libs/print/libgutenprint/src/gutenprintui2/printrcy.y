/*
 * "$Id: printrcy.y,v 1.2 2006/11/15 01:28:49 rlk Exp $"
 *
 *   Test pattern generator for Gutenprint
 *
 *   Copyright 2001 Robert Krawitz <rlk@alum.mit.edu>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

%{

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gutenprint/gutenprint-intl-internal.h>
#include <gutenprintui2/gutenprintui.h>
#include "gutenprintui-internal.h"
#include "printrc.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern int mylineno;

extern int yylex(void);
char *quotestrip(const char *i);
char *endstrip(const char *i);

extern char* yytext;

static int yyerror( const char *s )
{
  fprintf(stderr,"stdin:%d: %s before '%s'\n", mylineno, s, yytext);
  return 0;
}

static stpui_plist_t *current_printer = NULL;

%}

%token <ival> tINT
%token <dval> tDOUBLE
%token <ival> tDIMENSION
%token <sval> tBOOLEAN
%token <sval> tSTRING
%token <sval> tWORD
%token <sval> tGSWORD

%token CURRENT_PRINTER
%token SHOW_ALL_PAPER_SIZES
%token PRINTER
%token DESTINATION
%token SCALING
%token ORIENTATION
%token AUTOSIZE_ROLL_PAPER
%token UNIT
%token DRIVER
%token LEFT
%token TOP
%token CUSTOM_PAGE_WIDTH
%token CUSTOM_PAGE_HEIGHT
%token OUTPUT_TYPE
%token PRINTRC_HDR
%token PARAMETER
%token QUEUE_NAME
%token OUTPUT_FILENAME
%token EXTRA_PRINTER_OPTIONS
%token CUSTOM_COMMAND
%token COMMAND_TYPE
%token GLOBAL_SETTINGS
%token GLOBAL
%token END_GLOBAL_SETTINGS
%token pINT
%token pSTRING_LIST
%token pFILE
%token pDOUBLE
%token pDIMENSION
%token pBOOLEAN
%token pCURVE

%start Thing

%%

Printer: PRINTER tSTRING tSTRING
	{
	  current_printer = stpui_plist_create($2, $3);
	  g_free($2);
	  g_free($3);
	}
;

/*
 * Destination is obsolete; ignore it.
 */
Destination: DESTINATION tSTRING
	{
	  if ($2)
	    g_free($2);
	}
;

Queue_Name: QUEUE_NAME tSTRING
	{
	  if (current_printer && $2)
	    {
	      stpui_plist_set_queue_name(current_printer, $2);
	      g_free($2);
	    }
	}
;

Output_Filename: OUTPUT_FILENAME tSTRING
	{
	  if (current_printer && $2)
	    {
	      stpui_plist_set_output_filename(current_printer, $2);
	      g_free($2);
	    }
	}
;

Extra_Printer_Options: EXTRA_PRINTER_OPTIONS tSTRING
	{
	  if (current_printer && $2)
	    {
	      stpui_plist_set_extra_printer_options(current_printer, $2);
	      g_free($2);
	    }
	}
;

Custom_Command: CUSTOM_COMMAND tSTRING
	{
	  if (current_printer && $2)
	    {
	      stpui_plist_set_custom_command(current_printer, $2);
	      g_free($2);
	    }
	}
;

Command_Type: COMMAND_TYPE tINT
	{
	  if (current_printer)
	    stpui_plist_set_command_type(current_printer, $2);
	}
;

Scaling: SCALING tDOUBLE
	{
	  if (current_printer)
	    current_printer->scaling = $2; 
	}
;

Orientation: ORIENTATION tINT
	{
	  if (current_printer)
	    current_printer->orientation = $2;
	}
;

Autosize_Roll_Paper: AUTOSIZE_ROLL_PAPER tINT
	{
	  if (current_printer)
	    current_printer->auto_size_roll_feed_paper = $2;
	}
;

Unit: UNIT tINT
	{
	  if (current_printer)
	    current_printer->unit = $2; 
	}
;

Left: LEFT tINT
	{
	  if (current_printer)
	    stp_set_left(current_printer->v, $2);
	}
;

Top: TOP tINT
	{
	  if (current_printer)
	    stp_set_top(current_printer->v, $2);
	}
;

Output_Type: OUTPUT_TYPE tINT
	{
	  if (current_printer)
	    {
	      switch ($2)
		{
		case 0:
		  stp_set_string_parameter
		    (current_printer->v, "PrintingMode", "BW");
		  break;
		case 1:
		case 2:
		default:
		  stp_set_string_parameter
		    (current_printer->v, "PrintingMode", "Color");
		  break;
		}
	    }
	}
;

Custom_Page_Width: CUSTOM_PAGE_WIDTH tINT
	{
	  if (current_printer)
	    stp_set_page_width(current_printer->v, $2);
	}
;

Custom_Page_Height: CUSTOM_PAGE_HEIGHT tINT
	{
	  if (current_printer)
	    stp_set_page_height(current_printer->v, $2);
	}
;

Empty:
;

Int_Param: tWORD pINT tBOOLEAN tINT
	{
	  if (current_printer)
	    {
	      stp_set_int_parameter(current_printer->v, $1, $4);
	      if (strcmp($3, "False") == 0)
		stp_set_int_parameter_active(current_printer->v, $1,
					     STP_PARAMETER_INACTIVE);
	      else
		stp_set_int_parameter_active(current_printer->v, $1,
					     STP_PARAMETER_ACTIVE);
	    }
	  g_free($1);
	  g_free($3);
	}
;

String_List_Param: tWORD pSTRING_LIST tBOOLEAN tSTRING
	{
	  if (current_printer)
	    {
	      stp_set_string_parameter(current_printer->v, $1, $4);
	      if (strcmp($3, "False") == 0)
		stp_set_string_parameter_active(current_printer->v, $1,
						STP_PARAMETER_INACTIVE);
	      else
		stp_set_string_parameter_active(current_printer->v, $1,
						STP_PARAMETER_ACTIVE);
	    }
	  g_free($1);
	  g_free($3);
	  g_free($4);
	}
;

File_Param: tWORD pFILE tBOOLEAN tSTRING
	{
	  if (current_printer)
	    {
	      stp_set_file_parameter(current_printer->v, $1, $4);
	      if (strcmp($3, "False") == 0)
		stp_set_file_parameter_active(current_printer->v, $1,
					      STP_PARAMETER_INACTIVE);
	      else
		stp_set_file_parameter_active(current_printer->v, $1,
					      STP_PARAMETER_ACTIVE);
	    }
	  g_free($1);
	  g_free($3);
	  g_free($4);
	}
;

Double_Param: tWORD pDOUBLE tBOOLEAN tDOUBLE
	{
	  if (current_printer)
	    {
	      stp_set_float_parameter(current_printer->v, $1, $4);
	      if (strcmp($3, "False") == 0)
		stp_set_float_parameter_active(current_printer->v, $1,
					       STP_PARAMETER_INACTIVE);
	      else
		stp_set_float_parameter_active(current_printer->v, $1,
					       STP_PARAMETER_ACTIVE);
	    }
	  g_free($1);
	  g_free($3);
	}
;

Dimension_Param: tWORD pDIMENSION tBOOLEAN tINT
	{
	  if (current_printer)
	    {
	      stp_set_dimension_parameter(current_printer->v, $1, $4);
	      if (strcmp($3, "False") == 0)
		stp_set_dimension_parameter_active(current_printer->v, $1,
						   STP_PARAMETER_INACTIVE);
	      else
		stp_set_dimension_parameter_active(current_printer->v, $1,
						   STP_PARAMETER_ACTIVE);
	    }
	  g_free($1);
	  g_free($3);
	}
;

Boolean_Param: tWORD pBOOLEAN tBOOLEAN tBOOLEAN
	{
	  if (current_printer)
	    {
	      if (strcmp($4, "False") == 0)
		stp_set_boolean_parameter(current_printer->v, $1, 0);
	      else
		stp_set_boolean_parameter(current_printer->v, $1, 1);
	      if (strcmp($3, "False") == 0)
		stp_set_boolean_parameter_active(current_printer->v, $1,
						 STP_PARAMETER_INACTIVE);
	      else
		stp_set_boolean_parameter_active(current_printer->v, $1,
						 STP_PARAMETER_ACTIVE);
	    }
	  g_free($1);
	  g_free($3);
	  g_free($4);
	}
;

Curve_Param: tWORD pCURVE tBOOLEAN tSTRING
	{
	  if (current_printer)
	    {
	      stp_curve_t *curve = stp_curve_create_from_string($4);
	      if (curve)
		{
		  stp_set_curve_parameter(current_printer->v, $1, curve);
		  if (strcmp($3, "False") == 0)
		    stp_set_curve_parameter_active(current_printer->v, $1,
						   STP_PARAMETER_INACTIVE);
		  else
		    stp_set_curve_parameter_active(current_printer->v, $1,
						   STP_PARAMETER_ACTIVE);
		  stp_curve_destroy(curve);
		}
	    }
	  g_free($1);
	  g_free($3);
	  g_free($4);
	}
;

Typed_Param: Int_Param | String_List_Param | File_Param | Double_Param
	| Boolean_Param | Curve_Param | Dimension_Param
;

Parameter: PARAMETER Typed_Param
;

Parameters: Parameters Parameter | Empty
;

Standard_Value:  Destination | Scaling | Orientation | Autosize_Roll_Paper |
	Unit | Left | Top | Custom_Page_Width | Custom_Page_Height |
	Output_Type | Queue_Name | Output_Filename | Extra_Printer_Options |
	Custom_Command | Command_Type
;

Standard_Values: Standard_Values Standard_Value | Empty
;

A_Printer: Printer Standard_Values Parameters
;

Printers: Printers A_Printer | Empty
;

Current_Printer: CURRENT_PRINTER tSTRING
	{ stpui_printrc_current_printer = $2; }
;

Show_All_Paper_Sizes: SHOW_ALL_PAPER_SIZES tBOOLEAN
	{
	  if (strcmp($2, "True") == 0)
	    stpui_show_all_paper_sizes = 1;
	  else
	    stpui_show_all_paper_sizes = 0;
	  g_free($2);
	}
;

Global: Current_Printer | Show_All_Paper_Sizes
;

Old_Globals: Current_Printer Show_All_Paper_Sizes
;

New_Global_Setting: tWORD tSTRING
	{
	  if ($2)
	    {
	      stpui_set_global_parameter($1, $2);
	      g_free($2);
	    }
	  g_free($1);
	}
;

Global_Setting: Global | New_Global_Setting
;

Global_Settings: Global_Settings Global_Setting | Empty
;

Global_Subblock: GLOBAL_SETTINGS Global_Settings END_GLOBAL_SETTINGS
;

Global_Block: Global_Subblock | Old_Globals | Empty
;

Thing: PRINTRC_HDR Global_Block Printers
;

%%
