/******************************************************************************
 * $Id:$
 *
 * BeMailToMBox is a utility program (requested by Frank Zschockelt) that
 * converts BeOS e-mail files into Unix mailbox files (the kind that Pine
 * uses).  All the files in the input directory are concatenated with the
 * appropriate mbox header lines added between them, and trailing blank lines
 * reduced.  The resulting text is written to standard output.  Command line
 * driven.
 *
 * $Log: BeMailToMBox.cpp,v $ (now manually updated)
 * r13960 | agmsmith | 2005-08-13 22:10:49 -0400 (Sat, 13 Aug 2005) | 3 lines
 * More file movement for the BeMail utilities...  Content updates later to
 * avoid confusing svn.
 *
 * r13955 | agmsmith | 2005-08-13 19:43:41 -0400 (Sat, 13 Aug 2005) | 5 lines
 * Half way through adding some more BeMail related utilities - they use
 * libmail.so which isn't backwards compatibile so they need recompiling for
 * Haiku - thus better to include them here.  Also want spam levels of 1E-6 to
 * be visible for genuine messages.
 *
 * Revision 1.1  2002/02/24 18:16:46  agmsmith
 * Initial revision
 */

/* BeOS headers. */

#include <Application.h>
#include <StorageKit.h>
#include <SupportKit.h>

/* Posix headers. */

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>


/******************************************************************************
 * Globals
 */

time_t DateStampTime;
  /* Time value used for stamping each message header.  Incremented by 1 second
  for each message, starts out with the current local time. */


/******************************************************************************
 * Global utility function to display an error message and return.  The message
 * part describes the error, and if ErrorNumber is non-zero, gets the string
 * ", error code $X (standard description)." appended to it.  If the message
 * is NULL then it gets defaulted to "Something went wrong".
 */

static void DisplayErrorMessage (
  const char *MessageString = NULL,
  int ErrorNumber = 0,
  const char *TitleString = NULL)
{
  char ErrorBuffer [B_PATH_NAME_LENGTH + 80 /* error message */ + 80];

  if (TitleString == NULL)
    TitleString = "Error Message:";

  if (MessageString == NULL)
  {
    if (ErrorNumber == 0)
      MessageString = "No error, no message, why bother?";
    else
      MessageString = "Something went wrong";
  }

  if (ErrorNumber != 0)
  {
    sprintf (ErrorBuffer, "%s, error code $%X/%d (%s) has occured.",
      MessageString, ErrorNumber, ErrorNumber, strerror (ErrorNumber));
    MessageString = ErrorBuffer;
  }

  fputs (TitleString, stderr);
  fputc ('\n', stderr);
  fputs (MessageString, stderr);
  fputc ('\n', stderr);
}



/******************************************************************************
 * Determine if a line of text is the start of another message.  Pine mailbox
 * files have messages that start with a line that could say something like
 * "From agmsmith@achilles.net Fri Oct 31 21:19:36 EST 1997" or maybe something
 * like "From POPmail Mon Oct 20 21:12:36 1997" or in a more modern format,
 * "From agmsmith@achilles.net Tue Sep 4 09:04:11 2001 -0400".  I generalise it
 * to "From blah Day MMM NN XX:XX:XX TZONE1 YYYY TZONE2".  Blah is an e-mail
 * address you can ignore (just treat it as a word separated by spaces).  Day
 * is a 3 letter day of the week.  MMM is a 3 letter month name.  NN is the two
 * digit day of the week, has a leading space if the day is less than 10.
 * XX:XX:XX is the time, the X's are digits.  TZONE1 is the old style optional
 * time zone of 3 capital letters.  YYYY is the four digit year.  TZONE2 is the
 * optional modern time zone info, a plus or minus sign and 4 digits.  Returns
 * true if the line of text (ended with a NUL byte, no line feed or carriage
 * returns at the end) is the start of a message.
 */

bool IsStartOfMailMessage (char *LineString)
{
  char        *StringPntr;

  /* It starts with "From " */

  if (memcmp ("From ", LineString, 5) != 0)
    return false;
  StringPntr = LineString + 4;
  while (*StringPntr == ' ')
    StringPntr++;

  /* Skip over the e-mail address (or stop at the end of string). */

  while (*StringPntr != ' ' && *StringPntr != 0)
    StringPntr++;
  while (*StringPntr == ' ')
    StringPntr++;

  /* Look for the 3 letter day of the week. */

  if (memcmp (StringPntr, "Mon", 3) != 0 &&
  memcmp (StringPntr, "Tue", 3) != 0 &&
  memcmp (StringPntr, "Wed", 3) != 0 &&
  memcmp (StringPntr, "Thu", 3) != 0 &&
  memcmp (StringPntr, "Fri", 3) != 0 &&
  memcmp (StringPntr, "Sat", 3) != 0 &&
  memcmp (StringPntr, "Sun", 3) != 0)
  {
    fprintf (stderr, "False alarm, not a valid day of the week in \"%s\".\n",
      LineString);
    return false;
  }
  StringPntr += 3;
  while (*StringPntr == ' ')
    StringPntr++;

  /* Look for the 3 letter month code. */

  if (memcmp (StringPntr, "Jan", 3) != 0 &&
  memcmp (StringPntr, "Feb", 3) != 0 &&
  memcmp (StringPntr, "Mar", 3) != 0 &&
  memcmp (StringPntr, "Apr", 3) != 0 &&
  memcmp (StringPntr, "May", 3) != 0 &&
  memcmp (StringPntr, "Jun", 3) != 0 &&
  memcmp (StringPntr, "Jul", 3) != 0 &&
  memcmp (StringPntr, "Aug", 3) != 0 &&
  memcmp (StringPntr, "Sep", 3) != 0 &&
  memcmp (StringPntr, "Oct", 3) != 0 &&
  memcmp (StringPntr, "Nov", 3) != 0 &&
  memcmp (StringPntr, "Dec", 3) != 0)
  {
    fprintf (stderr, "False alarm, not a valid month name in \"%s\".\n",
      LineString);
    return false;
  }
  StringPntr += 3;
  while (*StringPntr == ' ')
    StringPntr++;

  /* Skip the day of the month.  Require at least one digit. */

  if (*StringPntr < '0' || *StringPntr > '9')
  {
    fprintf (stderr, "False alarm, not a valid day of the "
      "month number in \"%s\".\n", LineString);
    return false;
  }
  while (*StringPntr >= '0' && *StringPntr <= '9')
    StringPntr++;
  while (*StringPntr == ' ')
    StringPntr++;

  /* Check the time.  Look for the sequence
  digit-digit-colon-digit-digit-colon-digit-digit. */

  if (StringPntr[0] < '0' || StringPntr[0] > '9' ||
  StringPntr[1] < '0' || StringPntr[1] > '9' ||
  StringPntr[2] != ':' ||
  StringPntr[3] < '0' || StringPntr[3] > '9' ||
  StringPntr[4] < '0' || StringPntr[4] > '9' ||
  StringPntr[5] != ':' ||
  StringPntr[6] < '0' || StringPntr[6] > '9' ||
  StringPntr[7] < '0' || StringPntr[7] > '9')
  {
    fprintf (stderr, "False alarm, not a valid time value in \"%s\".\n",
      LineString);
    return false;
  }
  StringPntr += 8;
  while (*StringPntr == ' ')
    StringPntr++;

  /* Look for the optional antique 3 capital letter time zone and skip it. */

  if (StringPntr[0] >= 'A' && StringPntr[0] <= 'Z' &&
  StringPntr[1] >= 'A' && StringPntr[1] <= 'Z' &&
  StringPntr[2] >= 'A' && StringPntr[2] <= 'Z')
  {
    StringPntr += 3;
    while (*StringPntr == ' ')
      StringPntr++;
  }

  /* Look for the 4 digit year. */

  if (StringPntr[0] < '0' || StringPntr[0] > '9' ||
  StringPntr[1] < '0' || StringPntr[1] > '9' ||
  StringPntr[2] < '0' || StringPntr[2] > '9' ||
  StringPntr[3] < '0' || StringPntr[3] > '9')
  {
    fprintf (stderr, "False alarm, not a valid 4 digit year in \"%s\".\n",
      LineString);
    return false;
  }
  StringPntr += 4;
  while (*StringPntr == ' ')
    StringPntr++;

  /* Look for the optional modern time zone and skip over it if present. */

  if ((StringPntr[0] == '+' || StringPntr[0] == '-') &&
  StringPntr[1] >= '0' && StringPntr[1] <= '9' &&
  StringPntr[2] >= '0' && StringPntr[2] <= '9' &&
  StringPntr[3] >= '0' && StringPntr[3] <= '9' &&
  StringPntr[4] >= '0' && StringPntr[4] <= '9')
  {
    StringPntr += 5;
    while (*StringPntr == ' ')
      StringPntr++;
  }

  /* Look for end of string. */

  if (*StringPntr != 0)
  {
    fprintf (stderr, "False alarm, extra stuff after the "
      "year/time zone in \"%s\".\n", LineString);
    return false;
  }

  return true;
}



/******************************************************************************
 * Read the input file, convert it to mbox format, and write it to standard
 * output.  Returns zero if successful, a negative error code if an error
 * occured.
 */

status_t ProcessMessageFile (char *FileName)
{
  char        ErrorMessage [B_PATH_NAME_LENGTH + 80];
  int         i;
  int         InputFileDescriptor = -1;
  FILE       *InputFile = NULL;
  int         LineNumber;
  BString     MessageText;
  status_t    ReturnCode = -1;
  char       *StringPntr;
  char        TempString [102400];
  time_t      TimeValue;

  fprintf (stderr, "Now processing: \"%s\"\n", FileName);

  /* Open the file.  Try to open it in exclusive mode, so that it doesn't
  accidentally open the output file, or any other already open file. */

  InputFileDescriptor = open (FileName, O_RDONLY);
  if (InputFileDescriptor < 0)
  {
    ReturnCode = errno;
    DisplayErrorMessage ("Unable to open file", ReturnCode);
    return ReturnCode;
  }
  InputFile = fdopen (InputFileDescriptor, "rb");
  if (InputFile == NULL)
  {
    ReturnCode = errno;
    close (InputFileDescriptor);
    DisplayErrorMessage ("fdopen failed to convert file handle to a "
      "buffered file", ReturnCode);
    return ReturnCode;
  }

  /* Extract a text message from the BeMail file. */

  LineNumber = 0;
  while (!feof (InputFile))
  {
    /* First read in one line of text. */

    if (!fgets (TempString, sizeof (TempString), InputFile))
    {
      ReturnCode = errno;
      if (ferror (InputFile))
      {
        sprintf (ErrorMessage,
          "Error while reading from \"%s\"", FileName);
        DisplayErrorMessage (ErrorMessage, ReturnCode);
        goto ErrorExit;
      }
      break; /* No error, just end of file. */
    }

    /* Remove any trailing control characters (line feed usually, or CRLF).
    Might also nuke trailing tabs too.  Doesn't usually matter.  The main thing
    is to allow input files with both LF and CRLF endings (and even CR endings
    if you come from the Macintosh world). */

    StringPntr = TempString + strlen (TempString) - 1;
    while (StringPntr >= TempString && *StringPntr < 32)
      StringPntr--;
    *(++StringPntr) = 0;

    if (LineNumber == 0 && TempString[0] == 0)
      continue;  /* Skip leading blank lines. */
    LineNumber++;

    /* Prepend the new mbox message header, if the first line of the message
    doesn't already have one. */

    if (LineNumber == 1 && !IsStartOfMailMessage (TempString))
    {
      TimeValue = DateStampTime++;
      MessageText.Append ("From baron@be.com ");
      MessageText.Append (ctime (&TimeValue));
    }

    /* Append the line to the current message text. */

    MessageText.Append (TempString);
    MessageText.Append ("\n");
  }

  /* Remove blank lines from the end of the message (a pet peeve of mine), but
  end the message with two new lines to separate it from the next message. */

  i = MessageText.Length ();
  while (i > 0 && (MessageText[i-1] == '\n' || MessageText[i-1] == '\r'))
    i--;
  MessageText.Truncate (i);
  MessageText.Append ("\n\n");

  /* Write the message out. */

  if (puts (MessageText.String()) < 0)
  {
    ReturnCode = errno;
    DisplayErrorMessage ("Error while writing the message", ReturnCode);
    goto ErrorExit;
  }

  ReturnCode = 0;

ErrorExit:
  fclose (InputFile);
  return ReturnCode;
}



/******************************************************************************
 * Finally, the main program which drives it all.
 */

int main (int argc, char** argv)
{
  dirent_t    *DirEntPntr;
  DIR         *DirHandle;
  status_t     ErrorCode;
  char         InputPathName [B_PATH_NAME_LENGTH];
  int          MessagesDoneCount = 0;
  BApplication MyApp ("application/x-vnd.agmsmith.bemailtombox");
  const char  *StringPntr;
  char         TempString [1024 + 256];

  if (argc <= 1 || argc >= 3)
  {
    printf ("%s is a utility for converting BeMail e-mail\n", argv[0]);
    printf ("files to Unix Pine style e-mail files.  It could well\n");
    printf ("work with other Unix style mailbox files.  Each message in\n");
    printf ("the input directory is converted and sent to the standard\n");
    printf ("output.  Usage:\n\n");
    printf ("bemailtombox InputDirectory >OutputFile\n\n");
    printf ("Public domain, by Alexander G. M. Smith.\n");
    printf ("$Id:$\n");
    printf ("$HeadURL$\n");
    return -10;
  }

  /* Set the date stamp to the current time. */

  DateStampTime = time (NULL);

  /* Try to open the input directory. */

  memset (InputPathName, 0, sizeof (InputPathName));
  strncpy (InputPathName, argv[1], sizeof (InputPathName) - 2);
  DirHandle = opendir (InputPathName);
  if (DirHandle == NULL)
  {
    ErrorCode = errno;
    sprintf (TempString, "Problems opening directory named \"%s\".",
      InputPathName);
    DisplayErrorMessage (TempString, ErrorCode);
    return ErrorCode;
  }

  /* Append a trailing slash to the directory name, if it needs one. */

  if (InputPathName [strlen (InputPathName) - 1] != '/')
    strcat (InputPathName, "/");

  ErrorCode = 0;
  while (ErrorCode == 0 && (DirEntPntr = readdir (DirHandle)) != NULL)
  {
    for (StringPntr = DirEntPntr->d_name; *StringPntr != 0; StringPntr++)
      if (*StringPntr != '.')
        break;
    if (*StringPntr == 0)
      continue; /* Skip all period names, like . or .. or ... etc. */

    strcpy (TempString, InputPathName);
    strcat (TempString, DirEntPntr->d_name);
    ErrorCode = ProcessMessageFile (TempString);
    MessagesDoneCount++;
  }
  closedir (DirHandle);
  if (ErrorCode != 0)
  {
    DisplayErrorMessage ("Stopping early because an error occured", ErrorCode);
    return ErrorCode;
  }

  fprintf (stderr, "Did %d messages successfully.\n", MessagesDoneCount);
  return 0;
}
