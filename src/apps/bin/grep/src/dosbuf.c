/* Messy DOS-specific code for correctly treating binary, Unix text
   and DOS text files.

   This has several aspects:

     * Guessing the file type (unless the user tells us);
     * Stripping CR characters from DOS text files (otherwise regex
       functions won't work correctly);
     * Reporting correct byte count with -b for any kind of file.

*/

typedef enum {
  UNKNOWN, DOS_BINARY, DOS_TEXT, UNIX_TEXT
} File_type;

struct dos_map {
  off_t pos;	/* position in buffer passed to matcher */
  off_t add;	/* how much to add when reporting char position */
};

static int       dos_report_unix_offset = 0;

static File_type dos_file_type     = UNKNOWN;
static File_type dos_use_file_type = UNKNOWN;
static off_t     dos_stripped_crs  = 0;
static struct dos_map *dos_pos_map;
static int       dos_pos_map_size  = 0;
static int       dos_pos_map_used  = 0;
static int       inp_map_idx = 0, out_map_idx = 1;

/* Guess DOS file type by looking at its contents.  */
static inline File_type
guess_type (char *buf, register size_t buflen)
{
  int crlf_seen = 0;
  register char *bp = buf;

  while (buflen--)
    {
      /* Treat a file as binary if it has a NUL character.  */
      if (!*bp)
        return DOS_BINARY;

      /* CR before LF means DOS text file (unless we later see
         binary characters).  */
      else if (*bp == '\r' && buflen && bp[1] == '\n')
        crlf_seen = 1;

      bp++;
    }

  return crlf_seen ? DOS_TEXT : UNIX_TEXT;
}

/* Convert external DOS file representation to internal.
   Return the count of characters left in the buffer.
   Build table to map character positions when reporting byte counts.  */
static inline int
undossify_input (register char *buf, size_t buflen)
{
  int chars_left = 0;

  if (totalcc == 0)
    {
      /* New file: forget everything we knew about character
         position mapping table and file type.  */
      inp_map_idx = 0;
      out_map_idx = 1;
      dos_pos_map_used = 0;
      dos_stripped_crs = 0;
      dos_file_type = dos_use_file_type;
    }

  /* Guess if this file is binary, unless we already know that.  */
  if (dos_file_type == UNKNOWN)
    dos_file_type = guess_type(buf, buflen);

  /* If this file is to be treated as DOS Text, strip the CR characters
     and maybe build the table for character position mapping on output.  */
  if (dos_file_type == DOS_TEXT)
    {
      char   *destp   = buf;

      while (buflen--)
        {
          if (*buf != '\r')
            {
              *destp++ = *buf++;
              chars_left++;
            }
          else
            {
              buf++;
              if (out_byte && !dos_report_unix_offset)
                {
                  dos_stripped_crs++;
                  while (buflen && *buf == '\r')
                    {
                      dos_stripped_crs++;
                      buflen--;
                      buf++;
                    }
                  if (inp_map_idx >= dos_pos_map_size - 1)
                    {
                      dos_pos_map_size = inp_map_idx ? inp_map_idx * 2 : 1000;
                      dos_pos_map =
                        (struct dos_map *)xrealloc((char *)dos_pos_map,
						   dos_pos_map_size *
						   sizeof(struct dos_map));
                    }

                  if (!inp_map_idx)
                    {
                      /* Add sentinel entry.  */
                      dos_pos_map[inp_map_idx].pos = 0;
                      dos_pos_map[inp_map_idx++].add = 0;

                      /* Initialize first real entry.  */
                      dos_pos_map[inp_map_idx].add = 0;
                    }

                  /* Put the new entry.  If the stripped CR characters
                     precede a Newline (the usual case), pretend that
                     they were found *after* the Newline.  This makes
                     displayed byte offsets more reasonable in some
                     cases, and fits better the intuitive notion that
                     the line ends *before* the CR, not *after* it.  */
                  inp_map_idx++;
                  dos_pos_map[inp_map_idx-1].pos =
                    (*buf == '\n' ? destp + 1 : destp ) - bufbeg + totalcc;
                  dos_pos_map[inp_map_idx].add = dos_stripped_crs;
                  dos_pos_map_used = inp_map_idx;

                  /* The following will be updated on the next pass.  */
                  dos_pos_map[inp_map_idx].pos = destp - bufbeg + totalcc + 1;
                }
            }
        }

      return chars_left;
    }

  return buflen;
}

/* Convert internal byte count into external.  */
static inline off_t
dossified_pos (off_t byteno)
{
  off_t pos_lo;
  off_t pos_hi;

  if (dos_file_type != DOS_TEXT || dos_report_unix_offset)
    return byteno;

  /* Optimization: usually the file will be scanned sequentially.
     So in most cases, this byte position will be found in the
     table near the previous one, as recorded in `out_map_idx'.  */
  pos_lo = dos_pos_map[out_map_idx-1].pos;
  pos_hi = dos_pos_map[out_map_idx].pos;

  /* If the initial guess failed, search up or down, as
     appropriate, beginning with the previous place.  */
  if (byteno >= pos_hi)
    {
      out_map_idx++;
      while (out_map_idx < dos_pos_map_used &&
             byteno >= dos_pos_map[out_map_idx].pos)
        out_map_idx++;
    }

  else if (byteno < pos_lo)
    {
      out_map_idx--;
      while (out_map_idx > 1 && byteno < dos_pos_map[out_map_idx-1].pos)
        out_map_idx--;
    }

  return byteno + dos_pos_map[out_map_idx].add;
}
