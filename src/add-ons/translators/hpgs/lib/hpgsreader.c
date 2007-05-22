/***********************************************************************
 *                                                                     *
 * $Id: hpgsreader.c 382 2007-02-20 11:26:16Z softadm $
 *                                                                     *
 * hpgs - HPGl Script, a hpgl/2 interpreter, which uses a Postscript   *
 *        API for rendering a scene and thus renders to a variety of   *
 *        devices and fileformats.                                     *
 *                                                                     *
 * (C) 2004-2006 ev-i Informationstechnologie GmbH  http://www.ev-i.at *
 *                                                                     *
 * Author: Wolfgang Glas                                               *
 *                                                                     *
 *  hpgs is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU Lesser General Public          *
 * License as published by the Free Software Foundation; either        *
 * version 2.1 of the License, or (at your option) any later version.  *
 *                                                                     *
 * hpgs is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   *
 * Lesser General Public License for more details.                     *
 *                                                                     *
 * You should have received a copy of the GNU Lesser General Public    *
 * License along with this library; if not, write to the               *
 * Free Software  Foundation, Inc., 59 Temple Place, Suite 330,        *
 * Boston, MA  02111-1307  USA                                         *
 *                                                                     *
 ***********************************************************************
 *                                                                     *
 * The implementation of the HPGL reader.                              *
 *                                                                     *
 ***********************************************************************/

#include <hpgsreader.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/*!
   Contructs a HPGL reader on the heap using the given input device
   \c in and the given output vector drawing device \c dev.

   The argument \c multipage specifies, whether the reader processes more than
   one page.
   
   The argument \c v specifies the verbosity of the HPGL reader.
   A value of 0 forces the reader to not report anything to \c hpgs_log().
   Value of 1 or 2 give diagnostics output to \c hpgs_log() with increasing
   volume.
   
   You do not have to call neither \c hpgs_close on \c in nor
   \c hpgs_destroy on \c dev.

   In the case of an error, a null pointer is returned. This only happens,
   if the system is out of memory.
*/
hpgs_reader *hpgs_new_reader(hpgs_istream *in, hpgs_device *dev,
                             hpgs_bool multipage, int v)
{
  int i;
  hpgs_reader *ret = (hpgs_reader *)malloc(sizeof(hpgs_reader));

  if (!ret)
    {
      hpgs_istream_close(in);
      if (dev)
	hpgs_device_destroy(dev);
      return 0;
    }

  ret->in        = in;
  ret->device    = dev;
  ret->verbosity = v;
  ret->lw_factor = 1.0; 

  ret->plotsize_device = 0;
  ret->current_page = multipage ? 1 : -1;

  hpgs_matrix_set_identity(&ret->page_matrix);
  ret->page_scale = 1.0;

  ret->page_bbox.llx = 0.0;
  ret->page_bbox.lly = 0.0;

  ret->page_bbox.urx = 33600.0 * HP_TO_PT;
  ret->page_bbox.ury = 47520.0 * HP_TO_PT;

  ret->content_bbox = ret->page_bbox;
  ret->page_asset_ctxt = 0;
  ret->page_asset_func = 0;
  ret->frame_asset_ctxt = 0;
  ret->frame_asset_func = 0;

  ret->page_mode=0;

  ret->page_width  = 0.0;
  ret->page_height = 0.0;
  ret->page_angle  = 0.0;
  ret->page_border = 0.0;

  ret->pen_widths = NULL;
  ret->pen_colors = NULL;
  ret->poly_buffer = NULL;

  for (i=0;i<8;++i)
    ret->pcl_raster_data[i] = 0;

  ret->pcl_image=0;
  ret->png_dump_filename=0;
  ret->png_dump_count=0;
  ret->pcl_i_palette=-1;
  
  ret->interrupted = HPGS_FALSE;

  hpgs_reader_set_defaults (ret);

  return ret;
}

/*!
   Interrupt a currently running call to \c hpgs_reader_read from another
   thread. There's no guarantee when \c hpgs_read will terminate after
   calling \c hpgs_reader_interrupt. If the reader is currently performing
   a time-consuming task like rendering a large imge to the device,
   \c hpgs_read may continue for several seconds after this call.
*/
void hpgs_reader_interrupt(hpgs_reader *reader)
{
  reader->interrupted = HPGS_TRUE;
}

/*!
   Set the factor \c lw_factor for scaling HPGL
   linewidths to output linewidths. Usually you set this factor to 1.0,
   although for rendering to pixel device with a low resolution
   a factor of 0.5 may be useful to enforce the creation of thin
   lines.
*/
void hpgs_reader_set_lw_factor(hpgs_reader *reader, double lw_factor)
{
  reader->lw_factor=lw_factor;
}

/*!
   Sets a vector drawing device to a HPGL reader an resets the input
   stream.

   The function return 0 in any case.
*/
int  hpgs_reader_imbue(hpgs_reader *reader, hpgs_device *dev)
{
  if (reader->device)
    {
      // overtake a plotsize device for transferring individual plot sizes
      // for each page.
      if (hpgs_device_capabilities(reader->device) & HPGS_DEVICE_CAP_PLOTSIZE)
        {
          if (reader->plotsize_device)
            hpgs_device_destroy(reader->plotsize_device);

          reader->plotsize_device = reader->device;
        }
      else
        hpgs_device_destroy(reader->device);
    }

  reader->device = dev;

  if (reader->current_page <= 0)
    reader->current_page = -1;
  else
    reader->current_page = 1;

  hpgs_reader_set_defaults (reader);

  return hpgs_istream_seek(reader->in,0);
}

/*!
   Attach a new input stream to a HPGL reader. This is useful
   in multipage mode in order to collate multiple input files.

   The function return 0 in any case.
*/
int hpgs_reader_attach(hpgs_reader *reader, hpgs_istream *in)
{
  if (reader->in)
    hpgs_istream_close(reader->in);

  reader->in = in;
  return 0;
}

static int skip_ESC_block (hpgs_reader *reader)
{
  int arg = 0;
  int arg_sign = 0;
  int r;

  while (1)
    {
      switch (reader->bytes_ignored)
	{
	case 1:
	  if (reader->last_byte != HPGS_ESC)
	    reader->bytes_ignored = 0;
	  break;
	case 2:
	  if (reader->last_byte == '.')
            {
              reader->last_byte = hpgs_getc(reader->in);
              if (reader->last_byte == EOF)
                return 1;

              // <ESC>.( or <ESC>.Y interpretation.
              if (reader->last_byte == '(' ||
                  reader->last_byte == 'Y'   )
                return 0;

              reader->bytes_ignored = 0;
              break;
            }

	  if (reader->last_byte != '%')
	    reader->bytes_ignored = 0;
	  break;
	case 3:
	  switch (hpgs_reader_read_pcl_int(reader,&arg,&arg_sign))
	    {
            case -1:
              return 1;
            case 1:
              break;
            default:
              reader->bytes_ignored = 0;
	    }

	  break;
	case 4:
          reader->bytes_ignored = 0;
          switch (reader->last_byte)
            {
            case 'B':
              return 0;
            case 'A':
              r = hpgs_reader_do_PCL(reader,arg);
              if (r <= 0) return r;
              if (r==2) return 1;
            case 'X':
              r=1;
              while (r)
                switch (hpgs_reader_do_PJL(reader))
                  {
                  case 0: // ESC found.
                    r = 0;
                    break;
                  case 1: // Enter HPGL mode.
                    return 0;
                  case 2: // plotsize found. skip rest of file.
                    return 1;
                  case 3: // Enter PCL mode.
                    r = hpgs_reader_do_PCL(reader,arg);
                    if (r <= 0) return r;
                    if (r==2) return 1;
                    break;
                  default: /* EOF, error */
                    return 1;
                  }
            }
	  break;
	default:
	  reader->bytes_ignored = 0;	  
	}

      if (reader->bytes_ignored != 2)
        {
          reader->last_byte = hpgs_getc(reader->in);
          if (reader->last_byte == EOF)
            return 1;
        }
     
      ++reader->bytes_ignored;
    }
}

// skip until we reach 'ENDMF;'
static int skip_MF_block (hpgs_reader *reader, const char *stop_seq)
{
  while (1)
    {
      if (reader->bytes_ignored > 0)
	{
	  if (reader->last_byte != stop_seq[reader->bytes_ignored-1])
	    reader->bytes_ignored = 0;	  
	}

      if (stop_seq[reader->bytes_ignored] == '\0')
	{
	  reader->bytes_ignored = 0;
	  return 0;
	}

      reader->last_byte = hpgs_getc(reader->in);
      if (reader->last_byte == EOF)
	return 1;
     
      ++reader->bytes_ignored;
    }
}

// a function which recovers from a malformed command.
static int hpgl_recover(hpgs_reader *reader)
{
  size_t pos;

  if (hpgs_istream_tell(reader->in,&pos))
    return -1;

  reader->bytes_ignored = 0;
  
  while (reader->bytes_ignored < 256 &&
         (reader->last_byte = hpgs_getc(reader->in)) != ';')
    {
      if (reader->last_byte == EOF)
        return hpgs_set_error(hpgs_i18n("EOF during recovery from malformed HPGL command at file position %lu."),(unsigned long)pos);

      ++reader->bytes_ignored;
    }

  if (reader->bytes_ignored >= 256)
    return hpgs_set_error(hpgs_i18n("No semicolon in the next 256 bytes during recovery from malformed HPGL command at file position %lu."),(unsigned long)pos);

  reader->bytes_ignored = 0;
  reader->eoc = 1;

  return 0;
}

// this is a sorted table of hpgl commands and corersponding funtions.
typedef struct hpgs_reader_hpglcmd_rec_st hpgs_reader_hpglcmd_rec;

struct hpgs_reader_hpglcmd_rec_st
{
  int cmd;
  hpgs_reader_hpglcmd_func_t func;
};

static hpgs_reader_hpglcmd_rec hpglcmds[] =
  {
    { AA_CMD, hpgs_reader_do_AA },
    { AC_CMD, hpgs_reader_do_AC },
    { AD_CMD, hpgs_reader_do_AD },
    { AR_CMD, hpgs_reader_do_AR },
    { AT_CMD, hpgs_reader_do_AT },
    { BP_CMD, hpgs_reader_do_BP },
    { BR_CMD, hpgs_reader_do_BR },
    { BZ_CMD, hpgs_reader_do_BZ },
    { CI_CMD, hpgs_reader_do_CI },
    { CO_CMD, hpgs_reader_do_CO },
    { CP_CMD, hpgs_reader_do_CP },
    { CR_CMD, hpgs_reader_do_CR },
    { DI_CMD, hpgs_reader_do_DI },
    { DR_CMD, hpgs_reader_do_DR },
    { DT_CMD, hpgs_reader_do_DT },
    { DV_CMD, hpgs_reader_do_DV },
    { EA_CMD, hpgs_reader_do_EA },
    { EP_CMD, hpgs_reader_do_EP },
    { ER_CMD, hpgs_reader_do_ER },
    { ES_CMD, hpgs_reader_do_ES },
    { EW_CMD, hpgs_reader_do_EW },
    { FP_CMD, hpgs_reader_do_FP },
    { FR_CMD, hpgs_reader_do_FR },
    { FT_CMD, hpgs_reader_do_FT },
    { IN_CMD, hpgs_reader_do_IN },
    { IP_CMD, hpgs_reader_do_IP },
    { IR_CMD, hpgs_reader_do_IR },
    { IW_CMD, hpgs_reader_do_IW },
    { LA_CMD, hpgs_reader_do_LA },
    { LB_CMD, hpgs_reader_do_LB },
    { LO_CMD, hpgs_reader_do_LO },
    { LT_CMD, hpgs_reader_do_LT },
    { MC_CMD, hpgs_reader_do_MC },
    { MG_CMD, hpgs_reader_do_MG },
    { NP_CMD, hpgs_reader_do_NP },
    { PA_CMD, hpgs_reader_do_PA },
    { PC_CMD, hpgs_reader_do_PC },
    { PD_CMD, hpgs_reader_do_PD },
    { PE_CMD, hpgs_reader_do_PE },
    { PG_CMD, hpgs_reader_do_PG },
    { PM_CMD, hpgs_reader_do_PM },
    { PP_CMD, hpgs_reader_do_PP },
    { PR_CMD, hpgs_reader_do_PR },
    { PS_CMD, hpgs_reader_do_PS },
    { PU_CMD, hpgs_reader_do_PU },
    { PW_CMD, hpgs_reader_do_PW },
    { RA_CMD, hpgs_reader_do_RA },
    { RO_CMD, hpgs_reader_do_RO },
    { RR_CMD, hpgs_reader_do_RR },
    { RT_CMD, hpgs_reader_do_RT },
    { SA_CMD, hpgs_reader_do_SA },
    { SC_CMD, hpgs_reader_do_SC },
    { SD_CMD, hpgs_reader_do_SD },
    { SI_CMD, hpgs_reader_do_SI },
    { SL_CMD, hpgs_reader_do_SL },
    { SM_CMD, hpgs_reader_do_SM },
    { SP_CMD, hpgs_reader_do_SP },
    { SR_CMD, hpgs_reader_do_SR },
    { SS_CMD, hpgs_reader_do_SS },
    { TR_CMD, hpgs_reader_do_TR },
    { UL_CMD, hpgs_reader_do_UL },
    { WG_CMD, hpgs_reader_do_WG },
    { WU_CMD, hpgs_reader_do_WU }
  };

/*!
   Interprets the input stream associated with \c reader and
   passes the result to the associated vector device.

   The argument \c finish specifies, whether this is the last
   file written to the underlying \c hpgs_device.
   If \c finish is \c HPGS_TRUE, \c hpgs_device_finish is called.
   Otherwise, another input stream may be attached using
   \c hpgs_reader_attach and \c hpgs_read may be called again.
   This way, multiple input files may collated together in a single
   output file.

   The function return 0 upon success.
   A value of -1 is returned, when an error is encountered.

   You have to call \c hpgs_reader_get_error in order to
   retrieve the error message in the latter case.
*/
int hpgs_read(hpgs_reader *reader, hpgs_bool finish)
{
  int command=0;
  int status = 0;
  int i0,i1;
  int ncmds = sizeof(hpglcmds)/sizeof(hpgs_reader_hpglcmd_rec);
  hpgs_bool oce_special=HPGS_FALSE; /* Did we find *OceJobBegin ? */

  hpgs_clear_error();

  // restart for single page devices.
  if (reader->current_page <= 0)
    reader->current_page = -1;
  else
    {
      // try to set the individual page size in multipage mode.
      if (reader->plotsize_device &&
          (hpgs_device_capabilities(reader->device) & HPGS_DEVICE_CAP_MULTISIZE))
        {
          hpgs_bbox page_bb;

          if (hpgs_getplotsize(reader->plotsize_device,reader->current_page,&page_bb) < 0)
            goto syntax_error;
          
          if (reader->verbosity)
            hpgs_log("Bounding Box of Page %d: %g %g %g %g.\n",
                     reader->current_page,
                     page_bb.llx,page_bb.lly,
                     page_bb.urx,page_bb.ury );

          // change the page layout.
          hpgs_reader_set_page_matrix(reader,&page_bb);
          hpgs_reader_set_default_transformation(reader);

          if (hpgs_setplotsize(reader->device,&reader->page_bbox) < 0)
            goto syntax_error;
        }
    }

  hpgs_reader_set_defaults (reader);

  if (hpgs_istream_seek(reader->in,0))
    return hpgs_set_error("I/O Error rewinding input stream.");

  /* Check for a BEGMF stanza. */
  if (hpgs_getc(reader->in) == 'B' &&
      hpgs_getc(reader->in) == 'E' &&
      hpgs_getc(reader->in) == 'G' &&
      hpgs_getc(reader->in) == 'M' &&
      hpgs_getc(reader->in) == 'F'   )
    {
      // just skip until we reach ENDMF...
      if (skip_MF_block (reader,"ENDMF;"))
	return hpgs_set_error(hpgs_i18n("Unexpected EOF after BEGMF;."));
    }
  else
    {
      if (hpgs_istream_seek(reader->in,0))
        return hpgs_set_error(hpgs_i18n("I/O Error rewinding input stream."));
 
      if (hpgs_getc(reader->in) == '*' &&
          hpgs_getc(reader->in) == 'O' &&
          hpgs_getc(reader->in) == 'c' &&
          hpgs_getc(reader->in) == 'e' &&
          hpgs_getc(reader->in) == 'J' &&
          hpgs_getc(reader->in) == 'o' &&
          hpgs_getc(reader->in) == 'b' &&
          hpgs_getc(reader->in) == 'B' &&
          hpgs_getc(reader->in) == 'e' &&
          hpgs_getc(reader->in) == 'g' &&
          hpgs_getc(reader->in) == 'i' &&
          hpgs_getc(reader->in) == 'n'   )
        {
          // just skip until we reach OceJobData...
          if (skip_MF_block (reader,"*OceJobData"))
            return hpgs_set_error(hpgs_i18n("Unexpected EOF after *OceJobBegin."));

          oce_special = HPGS_TRUE;
        }
      else
        {
          if (hpgs_istream_seek(reader->in,0))
            return hpgs_set_error(hpgs_i18n("I/O Error rewinding input stream."));
        }
    }

  while (!hpgs_istream_iseof(reader->in))
    {
    start_of_command:
      if (reader->interrupted) goto syntax_error;

      // build command
      if (reader->bytes_ignored == 0)
	{
	  do
	    {
	      reader->last_byte = hpgs_getc(reader->in);
	      if (reader->last_byte == EOF)
		goto end_of_file;
	    }
	  while (!isalpha(reader->last_byte) && reader->last_byte!=HPGS_ESC &&
                 (!oce_special || reader->last_byte != '*'));
	  reader->bytes_ignored = 1;
	}

      if (reader->bytes_ignored == 1)
	{
          if (oce_special && reader->last_byte == '*')
            {
              if (hpgs_getc(reader->in) == 'O' &&
                  hpgs_getc(reader->in) == 'c' &&
                  hpgs_getc(reader->in) == 'e' &&
                  hpgs_getc(reader->in) == 'J' &&
                  hpgs_getc(reader->in) == 'o' &&
                  hpgs_getc(reader->in) == 'b' &&
                  hpgs_getc(reader->in) == 'E' &&
                  hpgs_getc(reader->in) == 'n' &&
                  hpgs_getc(reader->in) == 'd'   )
                {
                  if (reader->verbosity)
                    hpgs_log("Found *OceJobEnd, skipping rest of file.\n");
                  
                  goto end_of_file;
                }

              return hpgs_set_error(hpgs_i18n("Invalid *-command after *OceJobData."));
            }

	  while (reader->last_byte == HPGS_ESC)
	    {
              command = 0;
              switch (skip_ESC_block(reader))
                {
                case 1:
                  goto end_of_file;
                case -1:
                  goto syntax_error;
                }

	      reader->last_byte = hpgs_getc(reader->in);
	      if (reader->last_byte == EOF)
		goto end_of_file;
	    }

          reader->bytes_ignored = reader->last_byte == ';' ? 0 : 1;

          command = toupper(reader->last_byte);
	}

      while (reader->bytes_ignored < 2)
	{
	  reader->last_byte=hpgs_getc(reader->in);
	  if (reader->last_byte==EOF)
	    return hpgs_set_error(hpgs_i18n("Unexpected EOF between commands."));

	  if (reader->last_byte == ';')
	    {
	      reader->bytes_ignored = 0;
	      continue;
	    }

	  if (!isalpha(reader->last_byte))
            {
              if (hpgl_recover(reader))
                return -1;
              else
                goto start_of_command;
            }

	  command = ((command & 0xff) << 8) | toupper(reader->last_byte);
	  ++reader->bytes_ignored;
	}

      // check for empty command.
      reader->bytes_ignored = 0;

      if (command != LB_CMD && command != DT_CMD && command != PE_CMD)
	{
	  do
	    {
	      reader->last_byte = hpgs_getc(reader->in);
	    }
	  while (isspace(reader->last_byte));

	  if (reader->last_byte==EOF || reader->last_byte== ';')
	    reader->eoc = 1;
	  else if (isalpha(reader->last_byte) || reader->last_byte == HPGS_ESC)
	    {
	      reader->bytes_ignored = 1;
	      reader->eoc = 1;
	    }
	  else
	    {
	      hpgs_ungetc(reader->last_byte,reader->in);
	      reader->eoc = 0;
	    }
	}
      else
	reader->eoc = 0;

      if (reader->interrupted) goto syntax_error;

      if (reader->verbosity >= 2)
	hpgs_log("Command %c%c found.\n",
                 (char)(command >> 8),(char)(command&0xff));

      // do a binary search in the command table.
      i0 = 0;
      i1 = ncmds;

      while (i1>i0)
        {
          int i = i0+(i1-i0)/2;

          if (hpglcmds[i].cmd  < command)
            i0 = i+1;
          else
            i1 = i;
        }

      if (i0 < ncmds && hpglcmds[i0].cmd == command)
        // command found -> exec.
        status = hpglcmds[i0].func(reader);
      else
        {
          // interpret unknown command.
	  if (reader->verbosity)
            {
              size_t pos=0;
              hpgs_istream_tell(reader->in,&pos);

              hpgs_log(hpgs_i18n("Unknown command %c%c found at file position %lu.\n"),
                       (char)(command >> 8),(char)(command&0xff),(unsigned long)pos);
            }

	  while (!reader->eoc)
	    {
	      double x;
	      if (hpgs_reader_read_double(reader,&x))
		{
                  if (hpgl_recover(reader))
                    return -1;
                  else
                    goto start_of_command;
		}
	    }
	}

      if (status == 2) goto end_of_file;
      if (status) goto syntax_error;

      if (!reader->eoc)
        {
          size_t pos=0;
          hpgs_istream_tell(reader->in,&pos);

          return hpgs_set_error(hpgs_i18n("Trailing garbage in command %c%c at file position %lu."),
                                (char)(command >> 8),(char)(command&0xff),(unsigned long)pos);
        }
    }
      
 end_of_file:

  while (reader->clipsave_depth > 0)
    {
      hpgs_cliprestore(reader->device);
      --reader->clipsave_depth;
    }

  // enforce a showpage
  if ((reader->current_page == -1 || reader->current_page == 1) &&
      hpgs_reader_showpage(reader,0) < 0)
    {
      size_t pos=0;
      hpgs_istream_tell(reader->in,&pos);

      return hpgs_error_ctxt("Error showing a single page at file position " HPGS_SIZE_T_FMT,
                             pos);
    }

  if (finish && hpgs_device_finish(reader->device))
    {
      size_t pos=0;
      hpgs_istream_tell(reader->in,&pos);

      return hpgs_error_ctxt("Error finishing plot at file position " HPGS_SIZE_T_FMT,
                             pos);
    }

  return 0;

 syntax_error:
  {
    char cmd_str[20];
    size_t pos=0;
    hpgs_istream_tell(reader->in,&pos);

    if (command)
      snprintf(cmd_str,sizeof(cmd_str),hpgs_i18n("command %c%c"),
               (char)(command >> 8),(char)(command&0xff));
    else
      strcpy(cmd_str,hpgs_i18n("PCL command"));

    if (reader->interrupted)
      return hpgs_set_error(hpgs_i18n("The HPGL interpreter has been interrupted in %s at file position %lu."),
                            cmd_str,(unsigned long)pos);
    else if (hpgs_istream_iserror(reader->in))
      return hpgs_set_error(hpgs_i18n("Read error in %s at file position %lu."),
                            cmd_str,(unsigned long)pos);
    else if (hpgs_have_error())
      return hpgs_error_ctxt(hpgs_i18n("Error in %s at file position %lu."),
                             cmd_str,(unsigned long)pos);
    else if (hpgs_istream_iseof(reader->in))
      return hpgs_set_error(hpgs_i18n("Unexpected EOF in %s at file position %lu."),
                            cmd_str,(unsigned long)pos);
    else
      return hpgs_set_error(hpgs_i18n("Syntax error in %s at position %lu."),
                            cmd_str,(unsigned long)pos);
  }
}

/*!
   Force the placement of the content on a fixed page.
   Call this subroutine after you determined the plot size using
   a plotsize device.

   \c page_width and \c page_height specifiy the dimension of the
   page in point (1/72 inch). \c border specifies the border from the
   page to the HPGL content.
   
   \c bbox holds the bounding box of the HPGL content of the first
   page on input and on output, the bounding box of the whole page
   is returned. The returned bounding box should be used to set up
   your device, which will be passed to \c hpgs_reader_imbue after
   issuing this call.
*/
void hpgs_reader_set_fixed_page(hpgs_reader *reader,
                                hpgs_bbox *bbox,
                                double page_width,
                                double page_height,
                                double border,
                                double angle       )
{
  reader->page_mode   = 1;
  reader->page_width  = page_width;
  reader->page_height = page_height;
  reader->page_border = border;
  reader->page_angle  = angle;

  // change the page layout.
  hpgs_reader_set_page_matrix(reader,bbox);
  hpgs_reader_set_default_transformation(reader);
  *bbox = reader->page_bbox;
}

/*!
   Force the placement of the content on a dynamically sized page.
   Call this subroutine after you determined the plot size using
   a plotsize device.

   \c max_page_width and \c max_page_height specifiy the maximal
   dimension of the page, which will be adjusted to the size of
   the HPGL content of each individual page.
   \c border specifies the border from the page to the HPGL content.
   
   \c bbox holds the bounding box of the HPGL content of the first
   page on input and on output, the bounding box of the whole page
   is returned. The returned bounding box should be used to set up
   your device, which will be passed to \c hpgs_reader_imbue after
   issuing this call.
*/
void hpgs_reader_set_dynamic_page(hpgs_reader *reader,
                                  hpgs_bbox *bbox,
                                  double max_page_width,
                                  double max_page_height,
                                  double border,
                                  double angle       )
{
  reader->page_mode   = 2;
  reader->page_width  = max_page_width;
  reader->page_height = max_page_height;
  reader->page_border = border;
  reader->page_angle  = angle;

  // change the page layout.
  hpgs_reader_set_page_matrix(reader,bbox);
  hpgs_reader_set_default_transformation(reader);
  *bbox = reader->page_bbox;
}

/*!
  This call is deprecated, use \c hpgs_device_stamp instead.
*/
int  hpgs_reader_stamp(hpgs_reader *reader,
		       const hpgs_bbox *bb,
		       const char *stamp, const char *encoding,
                       double stamp_size)
{
  return hpgs_device_stamp(reader->device,bb,stamp,encoding,stamp_size);
}

/*!
  This is a highlevel interface to stamp the device with a
  text prior to printing any data. This is used in order to print a
  'draft' or 'obsoleted' mark to the paper.

  If encodeing is a null pointer
*/
int  hpgs_device_stamp(hpgs_device *dev,
		       const hpgs_bbox *bb,
		       const char *stamp, const char *encoding,
                       double stamp_size)
{
  int l = strlen(stamp);
  int nc;

  if (encoding && (strcmp(encoding,"utf8")  == 0 ||
                   strcmp(encoding,"UTF8")  == 0 ||
                   strcmp(encoding,"utf-8") == 0 ||
                   strcmp(encoding,"UTF-8") == 0   )
      )
    nc = hpgs_utf8_strlen(stamp,-1);
  else
    nc = l;

  int i,j;

  // calculate the number of partitions.
  int nx = (int)ceil((bb->urx-bb->llx)/stamp_size);
  int ny = (int)ceil((sqrt(3.0)*(bb->ury-bb->lly))/stamp_size);

  int n =  nx < ny ? nx : ny;

  // cell sizes
  double a = nx < ny ? (bb->urx-bb->llx) : (bb->urx-bb->llx)*(double)ny/(double)nx;
  double b = ny < nx ? (bb->ury-bb->lly) : (bb->ury-bb->lly)*(double)nx/(double)ny;

  double ratio = (n * nc + (n-1) - 0.1) * 0.8; 

  double ah = (a - ratio*b)/(1.0 - ratio*ratio);
  double bh = (b - ratio*a)/(1.0 - ratio*ratio);

  hpgs_point left_vec,up_vec,space_vec;
  hpgs_color rgb;

  if (nc<=0) return 0;

  space_vec.x = (a-ah) / (n * nc + (n-1));
  space_vec.y = (b-bh) / (n * nc + (n-1));

  left_vec.x = space_vec.x * 0.9;
  left_vec.y = space_vec.y * 0.9;

  up_vec.x = -ah;
  up_vec.y =  bh;

  rgb.r = rgb.g = rgb.b = 0.8;
  if (hpgs_setrgbcolor(dev,&rgb))
    return -1;

  a = space_vec.x * (nc+1);
  b = space_vec.y * (nc+1);

  for (i = 0; i < nx; ++i)
    {
      hpgs_point pos;

      for (j = 0; j < ny; ++j)
	{  
          pos.x = bb->llx + ah + i * a;
	  pos.y = bb->lly + j * b;
	  
	  if (hpgs_device_label(dev,&pos,
                                stamp, l,
                                0,
                                encoding ? encoding : "ISO-8859-1",
                                0,
                                5,
                                &left_vec,
                                &up_vec,
                                &space_vec))
	    return -1;
	}
    }

  rgb.r = rgb.g = rgb.b = 0.0;
  if (hpgs_setrgbcolor(dev,&rgb))
    return -1;

  return 0;
}

/*!
   Sets a callback function, which allows to render additional
   page assets before the reader calls \c hpgs_showpage.
   
   The call back function is passed the given \c ctxt pointer,
   the device on which the reader operates, the bounding box
   of the HPGL content and the transformation matrix from
   HPGL coordinates to page coordinates.
 */
void hpgs_reader_set_page_asset_func(hpgs_reader *reader,
                                     void *ctxt,
                                     hpgs_reader_asset_func_t func)
{
  reader->page_asset_ctxt = ctxt;
  reader->page_asset_func = func;
}

/*!
   Sets a callback function, which allows to render additional
   frame assets before a HPGL FR command becomes effective or
   the reader calls \c hpgs_showpage.
   
   The call back function is passed the given \c ctxt pointer,
   the device on which the reader operates, the bounding box
   of the HPGL content and the transformation matrix from
   HPGL coordinates to page coordinates.
 */
void hpgs_reader_set_frame_asset_func(hpgs_reader *reader,
                                      void *ctxt,
                                      hpgs_reader_asset_func_t func)
{
  reader->frame_asset_ctxt = ctxt;
  reader->frame_asset_func = func;
}

/*!
    Sets a filename for dumping inline PCL images in
    PNG format. The actual filenames written are:"
       filename0001.png, filename0002.png, .....
    If \c filename contains an extension .png the counter
    is put between the basename and the .png extension.
*/
int hpgs_reader_set_png_dump(hpgs_reader *reader, const char *filename)
{
  int l;

  if (reader->png_dump_filename)
    free (reader->png_dump_filename);

  if (!filename)
    {
      reader->png_dump_filename = 0;
      return 0;
    }

  // strip png extension.
  l = strlen(filename);
  
  if (l>4 && strcmp(filename+l-4,".png")==0)
    l-=4;

  reader->png_dump_filename = (char*) malloc(l+1);

  if (!reader->png_dump_filename)
    return hpgs_set_error(hpgs_i18n("Error allocating memory for png dump filename: %s"),
                          strerror(errno));

  strncpy(reader->png_dump_filename,filename,l);
  reader->png_dump_filename[l] = '\0';
  reader->png_dump_count=0;
 
  return 0;
}

/*!
   Returns the curent HPGL pen of the given \c reader structure.
   This information might be used by page or frame asset functions.
*/
int hpgs_reader_get_current_pen(hpgs_reader *reader)
{
  return reader->current_pen;
}

/*!
    Destroys the given \c reader structure and frees all allocated
    resources of the HPGL reader.
*/
void hpgs_destroy_reader(hpgs_reader *reader)
{
  int i;

  if (reader->pen_widths)
    free(reader->pen_widths);

  if (reader->pen_colors)
    free(reader->pen_colors);

  if (reader->in)
    hpgs_istream_close(reader->in);

  if (reader->device)
    hpgs_device_destroy(reader->device);

  if (reader->plotsize_device)
    hpgs_device_destroy(reader->plotsize_device);

  if (reader->poly_buffer)
    free(reader->poly_buffer);

  for (i=0;i<=reader->pcl_i_palette;++i)
    if (reader->pcl_palettes[i])
      free(reader->pcl_palettes[i]);

  for (i=0;i<8;++i)
    if (reader->pcl_raster_data[i])
      free(reader->pcl_raster_data[i]);

  if (reader->pcl_image)
    hpgs_image_destroy(reader->pcl_image);

  if (reader->png_dump_filename)
    free(reader->png_dump_filename);

  free(reader);
}
