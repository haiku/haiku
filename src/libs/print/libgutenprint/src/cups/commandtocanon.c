/*
 *   CANON BJ command filter for the Common UNIX Printing System.
 *
 *
 * Contents:
 *
 *   main() - Main entry and command processing.
 */

/*
 * Include necessary headers...
 */

#include <cups/cups.h>
#include <ctype.h>
#include <stdlib.h>


/*
 * Macros...
 */

#define pwrite(s,n) fwrite((s), 1, (n), stdout)


/*
 * 'main()' - Main entry and processing of driver.
 */

int			/* O - Exit status */
main(int  argc,		/* I - Number of command-line arguments */
     char *argv[])	/* I - Command-line arguments */
{
  FILE	*fp;		/* Command file */
  char	line[1024],	/* Line from file */
	*lineptr;	/* Pointer into line */
  int	feedpage;	/* Feed the page */


 /*
  * Check for valid arguments...
  */

  if (argc < 6 || argc > 7)
  {
   /*
    * We don't have the correct number of arguments; write an error message
    * and return.
    */

    fputs("ERROR: commandtocanon job-id user title copies options [file]\n", stderr);
    return (1);
  }

 /*
  * Open the command file as needed...
  */

  if (argc == 7)
  {
    if ((fp = fopen(argv[6], "r")) == NULL)
    {
      perror("ERROR: Unable to open command file - ");
      return (1);
    }
  }
  else
    fp = stdin;

 /*
  * Reset the printer and initiate BJL mode
  */

  pwrite("\x1b\x5b\x4b\x02\x00\x00\x1f" "BJLSTART\x0a", 16);
  feedpage = 0;

 /*
  * Read the commands from the file and send the appropriate commands...
  */

  while (fgets(line, sizeof(line), fp) != NULL)
  {
   /*
    * Drop trailing newline...
    */

    lineptr = line + strlen(line) - 1;
    if (*lineptr == '\n')
      *lineptr = '\0';

   /*
    * Skip leading whitespace...
    */

    for (lineptr = line; isspace(*lineptr); lineptr ++);

   /*
    * Skip comments and blank lines...
    */

    if (*lineptr == '#' || !*lineptr)
      continue;

   /*
    * Parse the command...
    */

    if (strncasecmp(lineptr, "Clean", 5) == 0)
    {
     /*
      * Clean heads...
      */

      char *what;

      for (what = lineptr + 6; isspace(*what); what ++);

      if (*what == 0)                   pwrite("@Cleaning=1ALL\x0a", 15);
      if (!strncasecmp(what,"all",3))   pwrite("@Cleaning=1ALL\x0a", 15);
      if (!strncasecmp(what,"black",5)) pwrite("@Cleaning=1K\x0a", 13);

    }
    else if (strncasecmp(lineptr, "PrintAlignmentPage", 18) == 0)
    {
     /*
      * Print alignment page...
      */

      int phase;

      phase = atoi(lineptr + 18);

      if (phase==0) pwrite("@TestPrint=Auto\x0a", 16);
      if (phase==1) pwrite("@TestPrint=Manual1\x0a", 19);
      if (phase==2) pwrite("@TestPrint=Manual2\x0a", 19);

      feedpage = 0;
    }
    else if (strncasecmp(lineptr, "PrintSelfTestPage", 17) == 0)
    {
     /*
      * Print version info and nozzle check...
      */

      pwrite("@TestPrint=NozzleCheck\x0a", 23);

      feedpage = 0;
    }
    else if (strncasecmp(lineptr, "ReportLevels", 12) == 0)
    {
     /*
      * Report ink levels...
      */

    }
    else if (strncasecmp(lineptr, "SetAlignment", 12) == 0)
    {
     /*
      * Set head alignment...
      */

    }
    else
      fprintf(stderr, "ERROR: Invalid printer command \"%s\"!\n", lineptr);
  }

 /*
  * Exit remote mode...
  */

  pwrite("BJLEND\x0a", 7);

 /*
  * Eject the page as needed...
  */

  if (feedpage)
  {
    putchar(13);
    putchar(10);
    putchar(12);
  }

 /*
  * Close the command file and return...
  */

  if (fp != stdin)
    fclose(fp);

  return (0);
}


/*
 */
