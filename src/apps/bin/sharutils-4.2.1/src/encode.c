/* Handle so called `shell archives'.
   Copyright (C) 1994, 1995 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "system.h"

/* Basic one-character encoding function to make a char printing.  */
#define ENCODE_BYTE(Byte) ((((Byte) + 63) & 63) + ' ' + 1)

/* Buffer size for one line of output.  */
#define LINE_BUFFER_SIZE 45

/*------------------------------------------.
| Output one GROUP of three bytes on FILE.  |
`------------------------------------------*/

static void
write_encoded_bytes (group, file)
     char *group;
     FILE *file;
{
  int c1, c2, c3, c4;

  c1 = group[0] >> 2;
  c2 = ((group[0] << 4) & (3 << 4)) | ((group[1] >> 4) & 15);
  c3 = ((group[1] << 2) & (15 << 2)) | ((group[2] >> 6) & 3);
  c4 = group[2] & 63;
  putc (ENCODE_BYTE (c1), file);
  putc (ENCODE_BYTE (c2), file);
  putc (ENCODE_BYTE (c3), file);
  putc (ENCODE_BYTE (c4), file);
}

/*--------------------------------------------------------------------.
| From FILE, refill BUFFER up to BUFFER_SIZE raw bytes, returning the |
| number of bytes read.						      |
`--------------------------------------------------------------------*/

static int
read_raw_bytes (file, buffer, buffer_size)
     FILE *file;
     char *buffer;
     int buffer_size;
{
  int character;
  int counter;

  for (counter = 0; counter < buffer_size; counter++)
    {
      character = getc (file);
      if (character == EOF)
	return counter;
      buffer[counter] = character;
    }
  return buffer_size;
}

/*----------------------------------------------------.
| Copy INPUT file to OUTPUT file, while encoding it.  |
`----------------------------------------------------*/

void
copy_file_encoded (input, output)
     FILE *input;
     FILE *output;
{
  char buffer[LINE_BUFFER_SIZE];
  int counter;
  int number_of_bytes;

  while (1)
    {
      number_of_bytes = read_raw_bytes (input, buffer, LINE_BUFFER_SIZE);
      putc (ENCODE_BYTE (number_of_bytes), output);

      for (counter = 0; counter < number_of_bytes; counter += 3)
	write_encoded_bytes (&buffer[counter], output);
      putc ('\n', output);

      if (number_of_bytes == 0)
	break;
    }
}
