/*
 * Copyright 2005-2009, Haiku Inc.
 * This file may be used under the terms of the MIT License.
 *
 * Originally public domain written by Alexander G. M. Smith.
 */


/*!	MboxToBeMail is a utility program that converts Unix mailbox (mbox) files
	(the kind that Pine uses) into e-mail files for use with BeOS.  It also
	handles news files from rn and trn, which have messages very similar to mail
	messages but with a different separator line.  The input files store
	multiple mail messages in text format separated by "From ..." lines or
	"Article ..." lines.  The output is a bunch of separate files, each one with
	one message plus BeOS BFS attributes describing that message.  For
	convenience, all the messages that were from one file are put in a specified
	directory.
*/

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <Application.h>
#include <E-mail.h>
#include <StorageKit.h>
#include <SupportKit.h>

#include <MailMessage.h>
#include <mail_util.h>


extern const char* __progname;
static const char* kProgramName = __progname;

char       InputPathName [B_PATH_NAME_LENGTH];
FILE      *InputFile;
BDirectory OutputDir;


typedef enum StandardHeaderEnum
{
  STD_HDR_DATE = 0, /* The Date: field.  First one since it is numeric. */
  STD_HDR_FROM, /* The whole From: field, including quotes and address. */
  STD_HDR_TO, /* All the stuff in the To: field. */
  STD_HDR_CC, /* All the CC: field (originally means carbon copy). */
  STD_HDR_REPLY, /* Things in the reply-to: field. */
  STD_HDR_SUBJECT, /* The Subject: field. */
  STD_HDR_PRIORITY, /* The Priority: and related fields, usually "Normal". */
  STD_HDR_STATUS, /* The BeOS mail Read / New status text attribute. */
  STD_HDR_THREAD, /* The subject simplified. */
  STD_HDR_NAME, /* The From address simplified into a plain name. */
  STD_HDR_MAX
} StandardHeaderCodes;

const char *g_StandardAttributeNames [STD_HDR_MAX] =
{
  B_MAIL_ATTR_WHEN,
  B_MAIL_ATTR_FROM,
  B_MAIL_ATTR_TO,
  B_MAIL_ATTR_CC,
  B_MAIL_ATTR_REPLY,
  B_MAIL_ATTR_SUBJECT,
  B_MAIL_ATTR_PRIORITY,
  B_MAIL_ATTR_STATUS,
  B_MAIL_ATTR_THREAD,
  B_MAIL_ATTR_NAME
};



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
    printf ("False alarm, not a valid day of the week in \"%s\".\n",
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
    printf ("False alarm, not a valid month name in \"%s\".\n",
      LineString);
    return false;
  }
  StringPntr += 3;
  while (*StringPntr == ' ')
    StringPntr++;

  /* Skip the day of the month.  Require at least one digit. */

  if (*StringPntr < '0' || *StringPntr > '9')
  {
    printf ("False alarm, not a valid day of the month number in \"%s\".\n",
      LineString);
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
    printf ("False alarm, not a valid time value in \"%s\".\n",
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
    printf ("False alarm, not a valid 4 digit year in \"%s\".\n",
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
    printf ("False alarm, extra stuff after the year/time zone in \"%s\".\n",
      LineString);
    return false;
  }

  return true;
}



/******************************************************************************
 * Determine if a line of text is the start of a news article.  TRN and RN news
 * article save files have messages that start with a line that looks like
 * "Article 11721 of rec.games.video.3do:".  Returns true if the line of text
 * (ended with a NUL byte, no line feed or carriage returns at the end) is the
 * start of an article.
 */

bool IsStartOfUsenetArticle (char *LineString)
{
  char        *StringPntr;

  /* It starts with "Article " */

  if (memcmp ("Article ", LineString, 8) != 0)
    return false;
  StringPntr = LineString + 7;
  while (*StringPntr == ' ')
    StringPntr++;

  /* Skip the article number.  Require at least one digit. */

  if (*StringPntr < '0' || *StringPntr > '9')
  {
    printf ("False alarm, not a valid article number in \"%s\".\n",
      LineString);
    return false;
  }
  while (*StringPntr >= '0' && *StringPntr <= '9')
    StringPntr++;
  while (*StringPntr == ' ')
    StringPntr++;

  /* Now it should have "of " */

  if (memcmp ("of ", StringPntr, 3) != 0)
  {
    printf ("False alarm, article line \"of\" misssing in \"%s\".\n",
      LineString);
    return false;
  }
  StringPntr += 2;
  while (*StringPntr == ' ')
    StringPntr++;

  /* Skip over the newsgroup name (no spaces) to the colon. */

  while (*StringPntr != ':' && *StringPntr != ' ' && *StringPntr != 0)
    StringPntr++;

  if (StringPntr[0] != ':' || StringPntr[1] != 0)
  {
    printf ("False alarm, article doesn't end with a colon in \"%s\".\n",
      LineString);
    return false;
  }

  return true;
}



/******************************************************************************
 * Saves the message text to a file in the output directory.  The file name is
 * derived from the message headers.  Returns zero if successful, a negative
 * error code if an error occured.
 */

status_t SaveMessage (BString &MessageText)
{
  time_t                   DateInSeconds;
  status_t                 ErrorCode;
  BString                  FileName;
  BString                  HeaderValues [STD_HDR_MAX];
  int                      i;
  int                      Length;
  BEmailMessage            MailMessage;
  BFile                    OutputFile;
  char                     TempString [80];
  struct tm                TimeFields;
  BString                  UniqueFileName;
  int32                    Uniquer;

  /* Remove blank lines from the end of the message (a pet peeve of mine), but
  end the message with a single new line to avoid annoying text editors that
  like to have it. */

  i = MessageText.Length ();
  while (i > 0 && (MessageText[i-1] == '\n' || MessageText[i-1] == '\r'))
    i--;
  MessageText.Truncate (i);
  MessageText.Append ("\r\n");

  /* Make a pretend file to hold the message, so the MDR library can use it. */

  BMemoryIO FakeFile (MessageText.String (), MessageText.Length ());

  /* Hand the message text off to the MDR library, which will parse it, extract
  the subject, sender's name, and other attributes, taking into account the
  character set headers. */

  ErrorCode = MailMessage.SetToRFC822 (&FakeFile,
    MessageText.Length (), false /* parse_now - decodes message body */);
  if (ErrorCode != B_OK)
  {
    DisplayErrorMessage ("Mail library was unable to process a mail "
      "message for some reason", ErrorCode);
    return ErrorCode;
  }

  /* Get the values for the standard attributes.  NULL if missing. */

  HeaderValues [STD_HDR_TO] = MailMessage.To ();
  HeaderValues [STD_HDR_FROM] = MailMessage.From ();
  HeaderValues [STD_HDR_CC] = MailMessage.CC ();
  HeaderValues [STD_HDR_DATE] = MailMessage.Date ();
  HeaderValues [STD_HDR_REPLY] = MailMessage.ReplyTo ();
  HeaderValues [STD_HDR_SUBJECT] = MailMessage.Subject ();
  HeaderValues [STD_HDR_STATUS] = "Read";
  if (MailMessage.Priority () != 3 /* Normal */)
  {
    sprintf (TempString, "%d", MailMessage.Priority ());
    HeaderValues [STD_HDR_PRIORITY] = TempString;
  }

  HeaderValues[STD_HDR_THREAD] = HeaderValues[STD_HDR_SUBJECT];
  SubjectToThread (HeaderValues[STD_HDR_THREAD]);
  if (HeaderValues[STD_HDR_THREAD].Length() <= 0)
    HeaderValues[STD_HDR_THREAD] = "No Subject";

  HeaderValues[STD_HDR_NAME] = HeaderValues[STD_HDR_FROM];
  extract_address_name (HeaderValues[STD_HDR_NAME]);

  // Generate a file name for the incoming message.

  FileName = HeaderValues [STD_HDR_THREAD];
  if (FileName[0] == '.')
    FileName.Prepend ("_"); // Avoid hidden files, starting with a dot.

  // Convert the date into a year-month-day fixed digit width format, so that
  // sorting by file name will give all the messages with the same subject in
  // order of date.

  DateInSeconds =
    ParseDateWithTimeZone (HeaderValues[STD_HDR_DATE].String());
  if (DateInSeconds == -1)
    DateInSeconds = 0; /* Set it to the earliest time if date isn't known. */

  localtime_r (&DateInSeconds, &TimeFields);
  sprintf (TempString, "%04d%02d%02d%02d%02d%02d",
    TimeFields.tm_year + 1900,
    TimeFields.tm_mon + 1,
    TimeFields.tm_mday,
    TimeFields.tm_hour,
    TimeFields.tm_min,
    TimeFields.tm_sec);
  FileName << " " << TempString << " " << HeaderValues[STD_HDR_NAME];
  FileName.Truncate (240);  // reserve space for the uniquer

  // Get rid of annoying characters which are hard to use in the shell.
  FileName.ReplaceAll('/','_');
  FileName.ReplaceAll('\'','_');
  FileName.ReplaceAll('"','_');
  FileName.ReplaceAll('!','_');
  FileName.ReplaceAll('<','_');
  FileName.ReplaceAll('>','_');
  while (FileName.FindFirst("  ") >= 0) // Remove multiple spaces.
    FileName.Replace("  " /* Old */, " " /* New */, 1024 /* Count */);

  Uniquer = 0;
  UniqueFileName = FileName;
  while (true)
  {
    ErrorCode = OutputFile.SetTo (&OutputDir,
      const_cast<const char *> (UniqueFileName.String ()),
      B_READ_WRITE | B_CREATE_FILE | B_FAIL_IF_EXISTS);
    if (ErrorCode == B_OK)
      break;
    if (ErrorCode != B_FILE_EXISTS)
    {
      UniqueFileName.Prepend ("Unable to create file \"");
      UniqueFileName.Append ("\" for writing a message to");
      DisplayErrorMessage (UniqueFileName.String (), ErrorCode);
      return ErrorCode;
    }
    Uniquer++;
    UniqueFileName = FileName;
    UniqueFileName << " " << Uniquer;
  }

  /* Write the message contents to the file, use the unchanged original one. */

  ErrorCode = OutputFile.Write (MessageText.String (), MessageText.Length ());
  if (ErrorCode < 0)
  {
      UniqueFileName.Prepend ("Error while writing file \"");
      UniqueFileName.Append ("\"");
      DisplayErrorMessage (UniqueFileName.String (), ErrorCode);
      return ErrorCode;
  }

  /* Attach the attributes to the file.  Save the MIME type first, otherwise
  the live queries don't pick up the new file.  Theoretically it would be
  better to do it last so that other programs don't start reading the message
  before the other attributes are set. */

  OutputFile.WriteAttr ("BEOS:TYPE", B_MIME_STRING_TYPE, 0,
    "text/x-email", 13);

  OutputFile.WriteAttr (g_StandardAttributeNames[STD_HDR_DATE],
    B_TIME_TYPE, 0, &DateInSeconds, sizeof (DateInSeconds));

  /* Write out all the string based attributes. */

  for (i = 1 /* The date was zero */; i < STD_HDR_MAX; i++)
  {
    if ((Length = HeaderValues[i].Length()) > 0)
      OutputFile.WriteAttr (g_StandardAttributeNames[i], B_STRING_TYPE, 0,
      HeaderValues[i].String(), Length + 1);
  }

  return 0;
}


int main (int argc, char** argv)
{
  char         ErrorMessage [B_PATH_NAME_LENGTH + 80];
  bool         HaveOldMessage = false;
  int          MessagesDoneCount = 0;
  BString      MessageText;
  BApplication MyApp ("application/x-vnd.Haiku-mbox2mail");
  int          NextArgIndex;
  char         OutputDirectoryPathName [B_PATH_NAME_LENGTH];
  status_t     ReturnCode = -1;
  bool         SaveSeparatorLine = false;
  char        *StringPntr;
  char         TempString [102400];

  if (argc <= 1)
  {
    printf ("%s is a utility for converting Pine e-mail\n",
      argv[0]);
    printf ("files (mbox files) to Mail e-mail files with attributes.  It\n");
    printf ("could well work with other Unix style mailbox files, and\n");
    printf ("saved Usenet article files.  Each message in the input\n");
    printf ("mailbox is converted into a separate file.  You can\n");
    printf ("optionally specify a directory (will be created if needed) to\n");
    printf ("put all the output files in, otherwise it scatters them into\n");
    printf ("the current directory.  The -s option makes it leave in the\n");
    printf ("separator text line at the top of each message, the default\n");
    printf ("is to lose it.\n\n");
    printf ("Usage:\n\n");
    printf ("%s [-s] InputFile [OutputDirectory]\n\n", kProgramName);
    printf ("Public domain, by Alexander G. M. Smith.\n");
    return -10;
  }

  NextArgIndex = 1;
  if (strcmp (argv[NextArgIndex], "-s") == 0)
  {
    SaveSeparatorLine = true;
    NextArgIndex++;
  }

  /* Try to open the input file. */

  if (NextArgIndex >= argc)
  {
    ReturnCode = -20;
    DisplayErrorMessage ("Missing the input file (mbox file) name argument.");
    goto ErrorExit;
  }
  strncpy (InputPathName, argv[NextArgIndex], sizeof (InputPathName) - 1);
  NextArgIndex++;
  InputFile = fopen (InputPathName, "rb");
  if (InputFile == NULL)
  {
    ReturnCode = errno;
    sprintf (ErrorMessage, "Unable to open file \"%s\" for reading",
      InputPathName);
    DisplayErrorMessage (ErrorMessage, ReturnCode);
    goto ErrorExit;
  }

  /* Try to make the output directory.  First get its name. */

  if (NextArgIndex < argc)
  {
    strncpy (OutputDirectoryPathName, argv[NextArgIndex],
      sizeof (OutputDirectoryPathName) - 2
      /* Leave space for adding trailing slash and NUL byte */);
    NextArgIndex++;
  }
  else
    strcpy (OutputDirectoryPathName, ".");

  /* Remove trailing '/' characters from the output directory path. */

  StringPntr =
    OutputDirectoryPathName + (strlen (OutputDirectoryPathName) - 1);
  while (StringPntr >= OutputDirectoryPathName)
  {
    if (*StringPntr != '/')
      break;
    StringPntr--;
  }
  *(++StringPntr) = 0;

  if (StringPntr - OutputDirectoryPathName > 0 &&
  strcmp (OutputDirectoryPathName, ".") != 0)
  {
    if (mkdir (OutputDirectoryPathName, 0777))
    {
      ReturnCode = errno;
      if (ReturnCode != B_FILE_EXISTS)
      {
        sprintf (ErrorMessage, "Unable to make output directory \"%s\"",
          OutputDirectoryPathName);
        DisplayErrorMessage (ErrorMessage, ReturnCode);
        goto ErrorExit;
      }
    }
  }

  /* Set the output BDirectory. */

  ReturnCode = OutputDir.SetTo (OutputDirectoryPathName);
  if (ReturnCode != B_OK)
  {
    sprintf (ErrorMessage, "Unable to set output BDirectory to \"%s\"",
      OutputDirectoryPathName);
    DisplayErrorMessage (ErrorMessage, ReturnCode);
    goto ErrorExit;
  }

  printf ("Input file: \"%s\", Output directory: \"%s\", ",
    InputPathName, OutputDirectoryPathName);
  printf ("%ssaving separator text line at the top of each message.  Working",
    SaveSeparatorLine ? "" : "not ");

  /* Extract a text message from the mail file.  It starts with a line that
  says "From blah Day MM NN XX:XX:XX YYYY TZONE".  Blah is an e-mail address
  you can ignore (just treat it as a word separated by spaces).  Day is a 3
  letter day of the week.  MM is a 3 letter month name.  NN is the two digit
  day of the week, has a leading space if the day is less than 10.  XX:XX:XX is
  the time, the X's are digits.  YYYY is the four digit year.  TZONE is the
  optional time zone info, a plus or minus sign and 4 digits. */

  while (!feof (InputFile))
  {
    /* First read in one line of text. */

    if (!fgets (TempString, sizeof (TempString), InputFile))
    {
      ReturnCode = errno;
      if (ferror (InputFile))
      {
        sprintf (ErrorMessage,
          "Error while reading from \"%s\"", InputPathName);
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

    /* See if this is the start of a new message. */

    if (IsStartOfUsenetArticle (TempString) ||
    IsStartOfMailMessage (TempString))
    {
      if (HaveOldMessage)
      {
        if ((ReturnCode = SaveMessage (MessageText)) != 0)
          goto ErrorExit;
        putchar ('.');
        fflush (stdout);
        MessagesDoneCount++;
      }
      HaveOldMessage = true;
      MessageText.SetTo (SaveSeparatorLine ? TempString : "");
    }
    else
    {
      /* Append the line to the current message text. */

      if (MessageText.Length () > 0)
        MessageText.Append ("\r\n"); /* Yes, BeMail expects CR/LF line ends. */
      MessageText.Append (TempString);
    }
  }

  /* Flush out the last message. */

  if (HaveOldMessage)
  {
    if ((ReturnCode = SaveMessage (MessageText)) != 0)
      goto ErrorExit;
    putchar ('.');
    MessagesDoneCount++;
  }
  printf ("  Did %d messages.\n", MessagesDoneCount);

  ReturnCode = 0;

ErrorExit:
  if (InputFile != NULL)
    fclose (InputFile);
  OutputDir.Unset ();
  return ReturnCode;
}
