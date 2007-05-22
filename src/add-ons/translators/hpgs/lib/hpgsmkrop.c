/***********************************************************************
 *                                                                     *
 * $Id: hpgsmkrop.c 270 2006-01-29 21:12:23Z softadm $
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
 * A small programm, which generates the raster operations hpgsrop.c   *
 *                                                                     *
 ***********************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#ifdef __GNUC__
__attribute__((format(printf,4,5)))
#endif
static int apprintf (char *str, size_t str_sz, size_t *str_len,
                     const char *fmt, ...)
{
  int n;
  va_list ap;

  va_start(ap, fmt);
 
  n = vsnprintf(str+*str_len,str_sz-*str_len,fmt,ap);

  va_end(ap);
        
  if (n<0) { perror("snprintf"); return -1; }
  *str_len+=n; 
      
  return 0;
}

static void mk_operand (char *operand, size_t operand_sz, const char *stack, int i)
{
  switch (stack[i])
    {
    case 'D':
      strcpy(operand,"*D");
      break;
    case 'S':
    case 'T':
      operand[0] = stack[i];
      operand[1] = '\0';
      break;
    default:
      snprintf(operand,operand_sz,"stk%d",i+1);
    }
}

static void mk_xoperand (char *operand, size_t operand_sz, const char *stack, int i)
{
  switch (stack[i])
    {
    case 'D':
      strcpy(operand,"D");
      break;
    case 'S':
    case 'T':
      operand[0] = stack[i];
      operand[1] = '\0';
      break;
    default:
      snprintf(operand,operand_sz,"stk%d",i+1);
    }
}

/*
  This function converts a ROP3 description cf. to

    PCL 5 Comparison Guide, Edition 2, 6/2003, Hewlett Packard
    (May be downloaded as bpl13206.pdf from http://www.hp.com)

  into 4 C-programs for all transparency modes supported by
  HPGL/2 and PCL.

  Additional information may be found under

    http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wceshellui5/html/wce50grfTernaryRasterOperations.asp

  (especially typos in HP's documentation are corrected from
   this source of information).

*/ 
static int write_rop3_func(FILE *out, int irop, const char *ropstr)
{
  int stacksz = 0;
  int max_stacksz = 0;
  char stack[32];
  char operand[32];
  char operand2[32];
  const char *c;

  int D_used = 0;
  int S_used = 0;
  int T_used = 0;

  char defn[2048],op;
  size_t defn_len=0;
  char xdefn[2048];
  size_t xdefn_len=0;
  int i;

  defn[0] = '\0';
  xdefn[0] = '\0';

  for (c=ropstr;*c;++c)
    switch (*c)
      {
      case '0':
      case '1':
        i = (*c == '0') ? 0 : 255;
        max_stacksz = stacksz = 1;

        if (c!=ropstr || *(c+1))
          {
            fprintf (stderr,"Misplaced %c in rop string %s.\n",
                     *c,ropstr);
            return -1;
          }
        
        if (apprintf(defn,sizeof(defn),&defn_len,
                     "  stk1 = %d;\n",i ))
         return -1;
        
        if (apprintf(xdefn,sizeof(xdefn),&xdefn_len,
                     "  stk1 = %s;\n", (*c == '0') ? "0x0000" : "0xffff"))
          return -1;
        
        break;
        
      case 'S':
      case 'T':
      case 'D':
        if (stacksz >= sizeof(stack)-1)
          {
            fprintf (stderr,"Stack overflow in rop string %s.\n",ropstr);
            return -1;
          }
          
        stack[stacksz] = *c;
        ++stacksz;

        if (*c == 'D')
          D_used = 1;

        if (*c == 'T')
          T_used = 1;

        if (*c == 'S')
          S_used = 1;
        break;
        
      case 'o':
      case 'a':
      case 'x':
        if (stacksz<1)
          {
            fprintf (stderr,"Stack underflow in rop string %s.\n",ropstr);
            return -1;
          }
        
        mk_operand(operand,sizeof(operand),stack,stacksz-2);
        mk_operand(operand2,sizeof(operand),stack,stacksz-1);

        op = (*c == 'o') ? '|' : ((*c == 'a') ? '&' : '^');

        if (apprintf(defn,sizeof(defn),&defn_len,
                     "  stk%d = %s %c %s;\n",
                     stacksz-1,operand,op,operand2 ))
          return -1;

        mk_xoperand(operand,sizeof(operand),stack,stacksz-2);
        mk_xoperand(operand2,sizeof(operand),stack,stacksz-1);

        if (apprintf(xdefn,sizeof(xdefn),&xdefn_len,
                     "  stk%d = %s %c %s;\n",
                     stacksz-1,operand,op,operand2 ))
          return -1;

        --stacksz;
        // account for the max intermediate result on the stack.
        if (stacksz > max_stacksz) max_stacksz = stacksz;

        stack[stacksz-1] = '\0';
        break;
        
      case 'n':
        mk_operand(operand,sizeof(operand),stack,stacksz-1);

        if (apprintf(defn,sizeof(defn),&defn_len,
                     "  stk%d = ~%s;\n",
                     stacksz,operand ))
          return -1;

        mk_xoperand(operand,sizeof(operand),stack,stacksz-1);

        if (apprintf(xdefn,sizeof(xdefn),&xdefn_len,
                     "  stk%d = ~%s;\n",
                     stacksz,operand ))
          return -1;

        // account for the max intermediate result on the stack.
        if (stacksz > max_stacksz) max_stacksz = stacksz;

        stack[stacksz-1] = '\0';
        break;
        
      default:
        fprintf (stderr,"Illegal character %c in rop string %s.\n",
                 *c,ropstr);
        return -1;
      }

  if (stacksz!=1)
    {
      fprintf (stderr,"Unbalanced shift/reduce in rop string %s.\n",ropstr);
      return -1;
    }

  mk_operand(operand,sizeof(operand),stack,0);

  // ******** normal ROP functions
  // case 1: source/pattern opaque.
  fprintf (out,
           "/* %s source/pattern opaque. */\n"
           "static void rop3_%d_0_0 (unsigned char *D, unsigned char S, unsigned char T)\n{\n",
           ropstr,irop);
  
  for (i=1;i<=max_stacksz;++i)
    fprintf(out,"  unsigned char stk%d;\n",i);

  fputs(defn,out);

  // optimize for noop.
  if (stack[0] != 'D')
    fprintf(out,"  *D = %s;\n",operand);

  fprintf(out,"}\n\n");

  // case 2: source opaque/pattern transparent.
  fprintf (out,
           "/* %s source opaque/pattern transparent. */\n"
           "static void rop3_%d_0_1 (unsigned char *D, unsigned char S, unsigned char T)\n{\n",
           ropstr,irop);
  
  for (i=1;i<=max_stacksz;++i)
    fprintf(out,"  unsigned char stk%d;\n",i);

  fputs(defn,out);

  // Image_A = Temporary_ROP3, & Not Src. 
  // Image_B = Temporary_ROP3 & Pattern. 
  // Image_C = Not Pattern & Src & Dest. 
  // Return Image_A | Image_B | Image_C
  fprintf(out,"  *D = (%s & S) | (%s & (~T)) | (T & (~S) & *D);\n",operand,operand);

  fprintf(out,"}\n\n");

  // case 3: source transparent/pattern opaque.
  fprintf (out,
           "/* %s source transparent/pattern opaque. */\n"
           "static void rop3_%d_1_0 (unsigned char *D, unsigned char S, unsigned char T)\n{\n",
           ropstr,irop);
  
  for (i=1;i<=max_stacksz;++i)
    fprintf(out,"  unsigned char stk%d;\n",i);

  fputs(defn,out);

  // Image_A = Temporary_ROP3 & Src.
  // Image_B = Dest & Not Src.
  // Return Image_A | Image_B
  fprintf(out,"  *D = (%s & (~S)) | (*D & S);\n",operand);

  fprintf(out,"}\n\n");

  // case 4: source/pattern transparent.
  fprintf (out,
           "/* %s source/pattern transparent. */\n"
           "static void rop3_%d_1_1 (unsigned char *D, unsigned char S, unsigned char T)\n{\n",
           ropstr,irop);
  
  for (i=1;i<=max_stacksz;++i)
    fprintf(out,"  unsigned char stk%d;\n",i);

  fputs(defn,out);
  // Image_A = Temporary_ROP3 & Src & Pattern.
  // Image_B = Dest & Not Src.
  // Image_C = Dest & Not Pattern.
  // Return Image_A | Image_B | Image_C.
  fprintf(out,"  *D = (%s & (~S) & (~T)) | (*D & S) | (*D & T);\n",operand);

  fprintf(out,"}\n\n");

  // ******** ROP transfer functions
  mk_xoperand(operand,sizeof(operand),stack,0);
  // case 1: source/pattern opaque.
  fprintf (out,
           "/* %s source/pattern opaque. */\n"
           "static unsigned xrop3_%d_0_0 (unsigned char s, unsigned char t)\n{\n",
           ropstr,irop);
  
  if (D_used)
    fprintf (out,"  unsigned D = 0x00ff;\n");
  if (S_used)
    fprintf (out,"  unsigned S = ((unsigned)s << 8) | s;\n");
  if (T_used)
    fprintf (out,"  unsigned T = ((unsigned)t << 8) | t;\n");

  for (i=1;i<=max_stacksz;++i)
    fprintf(out,"  unsigned stk%d;\n",i);

  fputs(xdefn,out);

  fprintf(out,"  return %s;\n",operand);

  fprintf(out,"}\n\n");

  // case 2: source opaque/pattern transparent.
  fprintf (out,
           "/* %s source opaque/pattern transparent. */\n"
           "static unsigned xrop3_%d_0_1 (unsigned char s, unsigned char t)\n{\n",
           ropstr,irop);
  
  fprintf (out,"  unsigned D = 0x00ff;\n");
  fprintf (out,"  unsigned S = ((unsigned)s << 8) | s;\n");
  fprintf (out,"  unsigned T = ((unsigned)t << 8) | t;\n");

  for (i=1;i<=max_stacksz;++i)
    fprintf(out,"  unsigned stk%d;\n",i);

  fputs(xdefn,out);

  // Image_A = Temporary_ROP3, & Not Src. 
  // Image_B = Temporary_ROP3 & Pattern. 
  // Image_C = Not Pattern & Src & Dest. 
  // Return Image_A | Image_B | Image_C
  fprintf(out,"  return (%s & S) | (%s & (~T)) | (T & (~S) & D);\n",operand,operand);

  fprintf(out,"}\n\n");

  // case 3: source transparent/pattern opaque.
  fprintf (out,
           "/* %s source transparent/pattern opaque. */\n"
           "static unsigned xrop3_%d_1_0 (unsigned char s, unsigned char t)\n{\n",
           ropstr,irop);
  
  fprintf (out,"  unsigned D = 0x00ff;\n");
  fprintf (out,"  unsigned S = ((unsigned)s << 8) | s;\n");
  if (T_used)
    fprintf (out,"  unsigned T = ((unsigned)t << 8) | t;\n");

  for (i=1;i<=max_stacksz;++i)
    fprintf(out,"  unsigned stk%d;\n",i);

  fputs(xdefn,out);

  // Image_A = Temporary_ROP3 & Src.
  // Image_B = Dest & Not Src.
  // Return Image_A | Image_B
  fprintf(out,"  return (%s & (~S)) | (D & S);\n",operand);

  fprintf(out,"}\n\n");

  // case 4: source/pattern transparent.
  fprintf (out,
           "/* %s source/pattern transparent. */\n"
           "static unsigned xrop3_%d_1_1 (unsigned char s, unsigned char t)\n{\n",
           ropstr,irop);
  
  fprintf (out,"  unsigned D = 0x00ff;\n");
  fprintf (out,"  unsigned S = ((unsigned)s << 8) | s;\n");
  fprintf (out,"  unsigned T = ((unsigned)t << 8) | t;\n");

  for (i=1;i<=max_stacksz;++i)
    fprintf(out,"  unsigned stk%d;\n",i);

  fputs(xdefn,out);
  // Image_A = Temporary_ROP3 & Src & Pattern.
  // Image_B = Dest & Not Src.
  // Image_C = Dest & Not Pattern.
  // Return Image_A | Image_B | Image_C.
  fprintf(out,"  return (%s & (~S) & (~T)) | (D & S) | (D & T);\n",operand);

  fprintf(out,"}\n\n");

  return 0;
}

int main (int argc, const char *argv[])
{
  FILE *in=0;
  FILE *out=0;
  int irop=0,i;
  char ropstr[32];
  int ret = 0;
  time_t now;

  if (argc != 3)
    {
      fprintf(stderr,"usage: %s <in> <out>.\n",argv[0]);
      return 1;
    }

  in = fopen(argv[1],"rb");

  if (!in)
    {
      fprintf(stderr,"%s: Error opening file <%s>: %s.\n",
              argv[0],argv[1],strerror(errno));
      ret = 1;
      goto cleanup;
    }

  out = fopen(argv[2],"wb");

  if (!out)
    {
      fprintf(stderr,"%s: Error opening file <%s>: %s.\n",
              argv[0],argv[2],strerror(errno));
      ret = 1;
      goto cleanup;
    }

  now = time(0);
  fprintf(out,"/* Generated automatically by %s at %.24s.\n",argv[0],ctime(&now));
  fprintf(out,"   Do not edit!\n");
  fprintf(out," */\n");
  fprintf(out,"#include <hpgs.h>\n\n");


  // go through all ROP descritions in hpgsrop.dat
  while (fscanf(in,"%d %31s",&i,ropstr) == 2)
    {
      if (i!=irop)
        {
          fprintf(stderr,"%s: Illegal count %d in stanza %d.\n",
                  argv[0],i,irop);
          ret = 1;
          goto cleanup;
        }
       
      if (write_rop3_func(out,irop,ropstr))
        goto cleanup;

      ++irop;
    }

  // Collect all rop function in one big lookup table...
  fprintf(out,
          "static hpgs_rop3_func_t rop3_table[][2][2] = {\n");

  for (i=0;i<irop;++i)
    fprintf(out,
            "  {{rop3_%d_0_0,rop3_%d_0_1},{rop3_%d_1_0,rop3_%d_1_1}}%s\n",
            i,i,i,i,i<irop-1 ? "," : "");
    
  fprintf(out,
          "};\n\n");

  // generate our public interface hpgs_rop3_func.
  fprintf(out,
          "hpgs_rop3_func_t hpgs_rop3_func(int rop3,\n"
          "                                hpgs_bool src_transparency,\n"
          "                                hpgs_bool pattern_transparency)\n"
          "{\n"
          "  if (rop3 < 0 || rop3 >= %d) return 0;\n"
          "  return rop3_table[rop3][src_transparency!=0][pattern_transparency!=0];\n"
          "}\n",
          irop);

  // Collect all rop xfer function in one big lookup table...
  fprintf(out,
          "static hpgs_xrop3_func_t xrop3_table[][2][2] = {\n");

  for (i=0;i<irop;++i)
    fprintf(out,
            "  {{xrop3_%d_0_0,xrop3_%d_0_1},{xrop3_%d_1_0,xrop3_%d_1_1}}%s\n",
            i,i,i,i,i<irop-1 ? "," : "");
    
  fprintf(out,
          "};\n\n");

  // generate our public interface hpgs_xrop3_func.
  fprintf(out,
          "hpgs_xrop3_func_t hpgs_xrop3_func(int rop3,\n"
          "                                  hpgs_bool src_transparency,\n"
          "                                  hpgs_bool pattern_transparency)\n"
          "{\n"
          "  if (rop3 < 0 || rop3 >= %d) return 0;\n"
          "  return xrop3_table[rop3][src_transparency!=0][pattern_transparency!=0];\n"
          "}\n",
          irop);

 cleanup:
  if (in) fclose(in);
  if (out) fclose(out);
  return ret;
}
