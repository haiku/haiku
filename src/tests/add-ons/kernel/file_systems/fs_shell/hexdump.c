/*
  This file contains a simple hex dump routine.
  
  THIS CODE COPYRIGHT DOMINIC GIAMPAOLO.  NO WARRANTY IS EXPRESSED 
  OR IMPLIED.  YOU MAY USE THIS CODE AND FREELY DISTRIBUTE IT FOR
  NON-COMMERCIAL USE AS LONG AS THIS NOTICE REMAINS ATTACHED.

  FOR COMMERCIAL USE, CONTACT DOMINIC GIAMPAOLO (dbg@be.com).

  Dominic Giampaolo
  dbg@be.com
*/
#include <stdio.h>
#include <ctype.h>

#include "compat.h"

/*
 * This routine is a simple memory dumper.  It's nice and simple, and works
 * well.
 *
 * The bad things about it are that it assumes roughly an 80 column output
 * device and that output is fixed at BYTES_PER_LINE/2 columns (separated
 * every two bytes). 
 *
 * Obviously the bad things are fixable, but I don't need the extra
 * flexibility at the moment, so I don't feel like doing it.
 *
 *   Dominic Giampaolo
 *   (dbg@be.com)
 */

#define BYTES_PER_LINE 16  /* a reasonable power of two */


void hexdump(void *address, int size)
{
  int i;
  int offset, num_spaces;
  unsigned char *mem, *tmp;


  offset = 0;
  mem = (unsigned char *)address;

  /*
   * Each line contains BYTES_PER_LINE bytes of data (presently 16).  I
   * chose 16 because it is a nice power of two and makes reading the
   * hex offset column much easier.  I used to use a value of 18 to fit
   * more info on the screen, and 20 is also doable but much too crowded.
   * Ideally it should be an argument or settable parameter...
   *
   * The data is formatted into BYTES_PER_LINE/2 (8) columns of 2 bytes
   * each (printed in hex).
   *
   * The offset is formatted as a 4 byte hex number (i.e. 8 characters).
   */
  while(offset < size)
    {
      printf("%.8x: ", offset);

      for(i=0,tmp=mem; i < BYTES_PER_LINE && (offset+i) < size; i++,tmp++)
       {
     printf("%.2x", *tmp);
     if (((i+1) % 4) == 0)
       printf(" ");
       }
    
      /*
       * This formula for the number of spaces to print is as follows:
       *   10 is the number of characters printed at the beginning of
       *      the line (8 hex digits, the colon and a space).
       *   i*2 is the number of characters of data we dumped in hex.
       *   i/2 is the number of blanks we printed between columns.
       *   i is the number of bytes we will print in ascii.
       *
       * Then we subtract all that from 74 (the width of the output
       * device) to decide how many spaces we need to push the ascii
       * column as far to the right as possible.
       *
       * The number 58 is the column where we start we start printing
       * the ascii dump.  We subtract how many characters we've already
       * printed and that gets us to where we need to be to start the
       * ascii portion of the dump.
       *
       */
      num_spaces = 58 - (12 + i*2 + i/4);
      for(i=0; i < num_spaces; i++)
    printf(" ");

      for(i=0,tmp=mem; i < BYTES_PER_LINE && (offset+i) < size; i++, tmp++)
    if (isprint(*tmp))
      printf("%c", *tmp);
    else
      printf(".");

      printf("\n");

      offset += BYTES_PER_LINE;
      mem = tmp;
    }
}



#ifdef TEST


char buff[] = "!blah, blah blah blah blah asldfj lkasjdf lka lkjasdflasdlj"
              "asdj lasdfj lasdjf lasdjf lkasjdfl kjasdlf jasldfj lasdfj l"
              "asdjflkasjdflk;ja sdfljasdfjk asjkfl;kasjfl;asjdfl;azzzzzzz";

main(int argc, char **argv)
{
  int i,j,k;
  FILE *fp;

  hexdump(buff, 171);

  printf("---------\n");

  hexdump(main, 57);
}

#endif /* TEST */
